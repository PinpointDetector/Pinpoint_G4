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
#include <map>
#include <set>

std::set<G4int> ScintillatorSD::sScintMuonDescendants;
std::set<std::pair<G4int,G4int>> ScintillatorSD::sScintHitParticles;

// Key for bar-level grouping: one entry per (layer, isHorizontal, bar, track)
struct ScintBarHitID {
    G4int layerID;
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
        if((int)isHorizontal != (int)other.isHorizontal) return (int)isHorizontal < (int)other.isHorizontal;
        if(barID != other.barID) return barID < other.barID;
        return trackID < other.trackID;
    }
};

// Key for pixel-level grouping: one entry per (layer, isHorizontal, col, row, track)
struct ScintPixelHitID {
    G4int layerID;
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

    G4int layerID    = -1;
    G4int scintRowID = -1;
    G4int scintColID = -1;
    G4int scintPixelID = -1;   // copy number of the ScintPixel volume
    G4bool isHorizontal = false;

    const G4int depth = touchable->GetHistoryDepth();

    for(G4int i = 0; i < depth; ++i) {
        const G4String& volName = touchable->GetVolume(i)->GetName();
        const G4int copyNum = touchable->GetCopyNumber(i);

        if(volName == "ScintLayer")  layerID      = copyNum;
        if(volName == "ScintRow")  { scintRowID    = copyNum; isHorizontal = true; }
        if(volName == "ScintColumn") scintColID    = copyNum;
        if(volName == "ScintPixel")  scintPixelID  = copyNum;
    }

    // ScintPixel is inside ScintColumn (vertical panel) or ScintRow (horizontal panel).
    // Its copy number encodes rowID within a column, or colID within a row.
    if(scintPixelID >= 0) {
        if(isHorizontal) {
            scintColID = scintPixelID; // pixel copy = colID within the row
        } else {
            scintRowID = scintPixelID; // pixel copy = rowID within the column
        }
    }

    if(layerID < 0) {
        G4Exception("ScintillatorSD", "Hit001", JustWarning,
                    "Could not find ScintLayer volume in touchable hierarchy!");
        return false;
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
    ScintBarHitID barID_key {layerID, barID, isHorizontal, trackID, pdgCode, parentID, fromPrimaryLepton, fromPrimaryEMShower, fromTau};
    barEnergyMap[barID_key] += edep;
    if(IsFromMuon(trackID)) barFromMuonMap[barID_key] = true;

    // Pixel-level: accumulate energy per (layer, isHorizontal, col, row, track)
    ScintPixelHitID pixelID_key {layerID, scintColID, scintRowID, isHorizontal, trackID, pdgCode, parentID, fromPrimaryLepton, fromPrimaryEMShower, fromTau};
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
        fHitsCollection->insert(hit);
    }

    // Pixel-level hits: individual scintillator pixel energy per track
    for(const auto& [hitID, edep] : pixelEnergyMap)
    {
        if(edep <= 0.) continue;
        auto hit = new ScintHit();
        hit->SetLayerID(hitID.layerID);
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
