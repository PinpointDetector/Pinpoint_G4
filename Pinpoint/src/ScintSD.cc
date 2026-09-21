#include "ScintSD.hh"
#include "ScintHit.hh"
#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VTouchable.hh"
#include "G4TouchableHistory.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "G4SDManager.hh"
#include "G4LorentzVector.hh"
#include "TrackInformation.hh"
#include "G4ios.hh"
#include "G4Box.hh"
#include "G4PhysicalConstants.hh"
#include "Randomize.hh"
#include <map>
#include <set>
#include <limits>

std::set<G4int> ScintillatorSD::sScintMuonDescendants;
std::set<std::pair<G4int,G4int>> ScintillatorSD::sScintHitParticles;

// Key for bar-level grouping: one entry per (layer, isHorizontal, bar, track)
struct ScintBarHitID {
    G4int layerID;
    G4int panelID;      
    G4int barID;        // colID for vertical panels, rowID for horizontal panels
    G4bool isHorizontal;
    G4int trackID;
    G4int pdgCode;
    G4int parentID;
    G4bool fromPrimaryLepton;
    G4bool fromPrimaryEMShower;
    G4bool fromTau;

    bool operator<(const ScintBarHitID& other) const {
        if(layerID != other.layerID) return layerID < other.layerID;
        if (panelID != other.panelID) return panelID < other.panelID;
        if((int)isHorizontal != (int)other.isHorizontal) return (int)isHorizontal < (int)other.isHorizontal;
        if(barID != other.barID) return barID < other.barID;
        return trackID < other.trackID;
    }
};

// Key for pixel-level grouping: one entry per (layer, isHorizontal, col, row, track)
struct ScintPixelHitID {
    G4int layerID;
    G4int panelID;
    G4int colID;
    G4int rowID;
    G4bool isHorizontal;
    G4int trackID;
    G4int pdgCode;
    G4int parentID;
    G4bool fromPrimaryLepton;
    G4bool fromPrimaryEMShower;
    G4bool fromTau;

    bool operator<(const ScintPixelHitID& other) const {
        if(layerID != other.layerID) return layerID < other.layerID;
        if (panelID != other.panelID) return panelID < other.panelID;
        if((int)isHorizontal != (int)other.isHorizontal) return (int)isHorizontal < (int)other.isHorizontal;
        if(colID != other.colID) return colID < other.colID;
        if(rowID != other.rowID) return rowID < other.rowID;
        return trackID < other.trackID;
    }
};

// Bar-level energy accumulation (sum over all pixels in a row/column per track)
static std::map<ScintBarHitID, G4double> barEnergyMap;
static std::map<ScintBarHitID, G4bool> barFromMuonMap;
// Pixel-level energy accumulation
static std::map<ScintPixelHitID, G4double> pixelEnergyMap;
static std::map<ScintPixelHitID, G4bool> pixelFromMuonMap;

namespace {
  // BC408 (Saint-Gobain) fast light/PE readout model. Photons are never tracked directly --
  // these are analytic, energy-deposit-driven parametrizations of light production, bulk
  // attenuation, trapping/transport and PDE, standing in for a full G4OpticalPhysics run
  // (which would be far too slow for production-scale statistics). See project notes for
  // the calibration discussion that produced these numbers.
  constexpr G4double kLightYield        = 10000. / MeV;  // BC408 datasheet: photons/MeV
  constexpr G4double kAttenuationLength = 380. * cm;      // BC408 bulk attenuation length
  constexpr G4double kTransportEff      = 0.10;           // trapping + coupling/wrapping losses -- TODO: calibrate against a one-off full-optical run
  constexpr G4double kQuantumEff        = 0.40;           // SiPM PDE at BC408 emission peak (~425nm) -- TODO: use real device PDE curve
  constexpr G4double kGroupVelocity     = c_light / 1.58; // BC408 refractive index n~1.58
  constexpr G4double kDecayConstant     = 2.1 * ns;       // BC408 scintillation decay time
}

// Per-bar-per-track accumulator for the fast PE/timing model, parallel to barEnergyMap
// (kept separate rather than folded in, so the existing edep accumulation is untouched).
struct BarSignalAccum {
    G4double attenuatedYield = 0.;                               // sum_i edep_i * lightYield * exp(-d_i/attLen)
    G4double earliestArrivalTime = std::numeric_limits<G4double>::max(); // min_i (t_i + d_i/v_group)
};
static std::map<ScintBarHitID, BarSignalAccum> barSignalMap;

ScintillatorSD::ScintillatorSD(const G4String& name, const G4String& hitsCollectionName,
                               const G4String& pixelHitsCollectionName)
    : G4VSensitiveDetector(name)
{
    collectionName.insert(hitsCollectionName);
    collectionName.insert(pixelHitsCollectionName);
}

void ScintillatorSD::Initialize(G4HCofThisEvent* hce)
{
    fHitsCollection = new ScintHitsCollection(SensitiveDetectorName, collectionName[0]);
    G4int hcID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
    hce->AddHitsCollection(hcID, fHitsCollection);

    fPixelHitsCollection = new ScintHitsCollection(SensitiveDetectorName, collectionName[1]);
    G4int pixelHcID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[1]);
    hce->AddHitsCollection(pixelHcID, fPixelHitsCollection);

    fScintCurrentHitId = 0;
    barEnergyMap.clear();
    barFromMuonMap.clear();
    pixelEnergyMap.clear();
    pixelFromMuonMap.clear();
    barSignalMap.clear();
    sScintHitParticles.clear();
}

G4bool ScintillatorSD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    G4Track* track = step->GetTrack();
    if(track->GetDefinition()->GetPDGCharge() == 0) return false;

    G4double edep = step->GetTotalEnergyDeposit();
    if(edep <= 0.) return false;

    G4StepPoint* preStep = step->GetPreStepPoint();
    G4TouchableHandle touchable = preStep->GetTouchableHandle();

    G4int panelID    = -1;
    G4int layerID    = -1;
    G4int scintRowID = -1;
    G4int scintColID = -1;
    G4int scintPixelID = -1;   // Reserved for future spacepoint reconstruction (intersection of a
                               // vertical and horizontal bar within a layer); no dedicated sub-bar
                               // volume exists in the current geometry, so this never gets set yet.
    G4bool isHorizontal = false;

    // Current geometry hierarchy (innermost -> outermost) for a scintillator hit:
    //   VertBar/HorizBar -> VertSlot/HorizSlot (replica, bar index)
    //   -> Scintillator (Pinpoint block's single panel) or Scintillator_Vertical/Scintillator_Horizontal
    //      (Fortune block's per-group panels, group index)
    //   -> PinpointBlock or FortuneBlock (block index) -> Detector -> World
    G4int barCopy   = -1;  // VertSlot/HorizSlot copy number: bar index within its panel
    G4int groupCopy = -1;  // Scintillator_Vertical/Horizontal copy number: scint-group index within a
                           // Fortune block (0 for a Pinpoint block's single "Scintillator" panel)
    G4int blockCopy = -1;  // PinpointBlock or FortuneBlock copy number
    G4bool isFortuneBlock = false;

    const G4int depth = touchable->GetHistoryDepth();

    for(G4int i = 0; i < depth; ++i) {
        const G4String& volName = touchable->GetVolume(i)->GetName();
        const G4int copyNum = touchable->GetCopyNumber(i);

        if(volName == "VertSlot")               { barCopy = copyNum; }
        if(volName == "HorizSlot")              { barCopy = copyNum; isHorizontal = true; }
        if(volName == "Scintillator")             groupCopy = 0; // Pinpoint block's single scintillator panel
        if(volName == "Scintillator_Vertical")    groupCopy = copyNum;
        if(volName == "Scintillator_Horizontal")  groupCopy = copyNum;
        if(volName == "PinpointBlock")          { blockCopy = copyNum; isFortuneBlock = false; }
        if(volName == "FortuneBlock")           { blockCopy = copyNum; isFortuneBlock = true; }
        if(volName == "ScintPixel")               scintPixelID = copyNum; // not present yet; see note above
    }

    if(barCopy < 0 || blockCopy < 0 || groupCopy < 0) {
        G4Exception("ScintillatorSD", "Hit001", JustWarning,
                    "Could not find scintillator bar/panel/block volume in touchable hierarchy!");
        return false;
    }

    if(isHorizontal) scintRowID = barCopy;
    else             scintColID = barCopy;

    // layerID/panelID come from lookup tables built by DetectorConstruction (see SetLayerIndexing()),
    // indexed directly by the PinpointBlock/FortuneBlock copy number just walked above -- NOT
    // re-derived here. blockCopy alone does not distinguish an initial Pinpoint block from an
    // intermediate one (both use the "PinpointBlock" name, but intermediate blocks' copy numbers
    // continue on past the Fortune blocks' own copy-number range and would otherwise collide with
    // them), so the lookup tables are what correctly separate the two.
    if(!isFortuneBlock) {
        if(blockCopy < 0 || blockCopy >= static_cast<G4int>(fPinpointPanelID.size())) {
            G4Exception("ScintillatorSD", "Hit002", JustWarning,
                        "PinpointBlock copy number out of range for scint layer indexing!");
            return false;
        }
        layerID = fPinpointLayerID[blockCopy];
        panelID = fPinpointPanelID[blockCopy];   // single scintillator panel per Pinpoint block
    } else {
        if(blockCopy < 0 || blockCopy >= static_cast<G4int>(fFortunePanelIDBase.size())) {
            G4Exception("ScintillatorSD", "Hit002", JustWarning,
                        "FortuneBlock copy number out of range for scint layer indexing!");
            return false;
        }
        layerID = fFortuneLayerID[blockCopy];
        panelID = fFortunePanelIDBase[blockCopy]
                + groupCopy * 2
                + (isHorizontal ? 1 : 0);
    }

    // Would refine col/row to a finer sub-bar pixel position, once a ScintPixel-equivalent volume
    // (or a spacepoint reconstruction step combining vertical/horizontal bar intersections) exists.
    if(scintPixelID >= 0) {
        if(isHorizontal) {
            scintColID = scintPixelID;
        } else {
            scintRowID = scintPixelID;
        }
    }


    G4int trackID  = track->GetTrackID();
    G4int parentID = track->GetParentID();
    G4int pdgCode  = track->GetParticleDefinition()->GetPDGEncoding();

    const auto* info = static_cast<const TrackInformation*>(track->GetUserInformation());
    G4bool fromPrimaryLepton = (info && info->IsTrackFromPrimaryLepton()) || parentID == 0;
    fromPrimaryLepton = fromPrimaryLepton &&
                        (std::abs(pdgCode) == 11 || std::abs(pdgCode) == 13 || std::abs(pdgCode) == 15);
    G4bool fromPrimaryEMShower = info && info->IsTrackFromPrimaryEMShower();
    G4bool fromTau = info && info->IsTrackFromTau();

    // Bar-level: accumulate energy per (layer, isHorizontal, bar, track)
    G4int barID = isHorizontal ? scintRowID : scintColID;
    ScintBarHitID barID_key {layerID, panelID, barID, isHorizontal, trackID, pdgCode, parentID, fromPrimaryLepton, fromPrimaryEMShower, fromTau};
    barEnergyMap[barID_key] += edep;
    if(IsFromMuon(trackID)) barFromMuonMap[barID_key] = true;

    // Fast BC408 light/PE model: accumulate this step's contribution to the bar's SiPM signal.
    if (const G4Box* barBox = dynamic_cast<const G4Box*>(touchable->GetVolume()->GetLogicalVolume()->GetSolid())) {
        const G4double halfLength = isHorizontal ? barBox->GetXHalfLength() : barBox->GetYHalfLength();
        const G4ThreeVector localPos = touchable->GetHistory()->GetTopTransform().TransformPoint(preStep->GetPosition());
        // SiPM at local +Y for vertical bars, local -X for horizontal bars.
        const G4double distToSiPM = isHorizontal ? (localPos.x() + halfLength) : (halfLength - localPos.y());

        const G4double attenuated = edep * kLightYield * std::exp(-distToSiPM / kAttenuationLength);
        const G4double arrivalTime = preStep->GetGlobalTime() + distToSiPM / kGroupVelocity;

        auto& sig = barSignalMap[barID_key];
        sig.attenuatedYield += attenuated;
        sig.earliestArrivalTime = std::min(sig.earliestArrivalTime, arrivalTime);
    }

    // Pixel-level: accumulate energy per (layer, isHorizontal, col, row, track)
    ScintPixelHitID pixelID_key {layerID, panelID, scintColID, scintRowID, isHorizontal, trackID, pdgCode, parentID, fromPrimaryLepton, fromPrimaryEMShower, fromTau};
    pixelEnergyMap[pixelID_key] += edep;
    if(IsFromMuon(trackID)) pixelFromMuonMap[pixelID_key] = true;

    fScintCurrentHitId++;
    return true;
}

void ScintillatorSD::EndOfEvent(G4HCofThisEvent*)
{
    // Bar-level hits: sum of all pixel energy in the same row/column per track
    for(const auto& [hitID, edep] : barEnergyMap)
    {
        if(edep <= 0.) continue;
        auto hit = new ScintHit();
        hit->SetLayerID(hitID.layerID);
        hit->SetPanelID(hitID.panelID);
        hit->SetColID(hitID.isHorizontal ? -1 : hitID.barID);
        hit->SetRowID(hitID.isHorizontal ? hitID.barID : -1);
        hit->SetIsHorizontal(hitID.isHorizontal);
        hit->SetTrackID(hitID.trackID);
        hit->SetParentID(hitID.parentID);
        hit->SetPDGCode(hitID.pdgCode);
        hit->SetEnergyDeposit(edep);
        hit->SetFromMuon(barFromMuonMap.count(hitID) ? barFromMuonMap[hitID] : false);
        hit->SetFromPrimaryLepton(hitID.fromPrimaryLepton);
        hit->SetFromPrimaryEMShower(hitID.fromPrimaryEMShower);
        hit->SetFromTau(hitID.fromTau);

        const auto& sig = barSignalMap[hitID];
        const G4double nPEMean = sig.attenuatedYield * kTransportEff * kQuantumEff;
        hit->SetPhotoelectrons(static_cast<G4int>(CLHEP::RandPoisson::shoot(nPEMean)));
        hit->SetHitTime(sig.earliestArrivalTime < std::numeric_limits<G4double>::max()
                        ? sig.earliestArrivalTime + CLHEP::RandExponential::shoot(kDecayConstant)
                        : -1.);

        fHitsCollection->insert(hit);
    }

    // Pixel-level hits: individual scintillator pixel energy per track
    for(const auto& [hitID, edep] : pixelEnergyMap)
    {
        if(edep <= 0.) continue;
        auto hit = new ScintHit();
        hit->SetLayerID(hitID.layerID);
        hit->SetPanelID(hitID.panelID);
        hit->SetColID(hitID.colID);
        hit->SetRowID(hitID.rowID);
        hit->SetIsHorizontal(hitID.isHorizontal);
        hit->SetTrackID(hitID.trackID);
        hit->SetParentID(hitID.parentID);
        hit->SetPDGCode(hitID.pdgCode);
        hit->SetEnergyDeposit(edep);
        hit->SetFromMuon(pixelFromMuonMap.count(hitID) ? pixelFromMuonMap[hitID] : false);
        hit->SetFromPrimaryLepton(hitID.fromPrimaryLepton);
        hit->SetFromPrimaryEMShower(hitID.fromPrimaryEMShower);
        hit->SetFromTau(hitID.fromTau);
        fPixelHitsCollection->insert(hit);
    }

    barEnergyMap.clear();
    barFromMuonMap.clear();
    pixelEnergyMap.clear();
    pixelFromMuonMap.clear();
    barSignalMap.clear();

    if(verboseLevel > 1) {
        std::size_t nofHits = fHitsCollection->entries();
        G4cout << G4endl << "-------->Hits Collection: in this event there are " << nofHits
               << " hits in scintillator layers: " << G4endl;
        for(std::size_t i=0; i<nofHits; ++i)
            (*fHitsCollection)[i]->Print();
    }
}

void ScintillatorSD::RecordMuonDescendant(G4int trackID, G4bool fromMuon)
{
    if(fromMuon) sScintMuonDescendants.insert(trackID);
}

G4bool ScintillatorSD::IsFromMuon(G4int trackID)
{
    return sScintMuonDescendants.find(trackID) != sScintMuonDescendants.end();
}

void ScintillatorSD::ClearMuonHistory()
{
    sScintMuonDescendants.clear();
}
