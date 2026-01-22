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

struct ScintLayerHitID {
    G4int layerID;
    G4int scintColID;
    G4int scintRowID;
    G4int trackID;
    G4int pdgCode;
    G4int parentID;
    G4bool fromPrimaryLepton;

    bool operator<(const ScintLayerHitID& other) const {
        if(layerID != other.layerID) return layerID < other.layerID;
        return trackID < other.trackID;
    }
};

// Energy and muon flags per layer hit
static std::map<ScintLayerHitID, G4double> layerEnergyMap;
static std::map<ScintLayerHitID, G4bool> layerFromMuonMap;

ScintillatorSD::ScintillatorSD(const G4String& name, const G4String& hitsCollectionName)
    : G4VSensitiveDetector(name)
{
    collectionName.insert(hitsCollectionName);
}

void ScintillatorSD::Initialize(G4HCofThisEvent* hce)
{
    fHitsCollection = new ScintHitsCollection(SensitiveDetectorName, collectionName[0]);
    G4int hcID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
    hce->AddHitsCollection(hcID, fHitsCollection);

    fScintCurrentHitId = 0;
    layerEnergyMap.clear();
    layerFromMuonMap.clear();
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

    G4int layerID = -1;        // detector layer (replica number)
    G4int scintRowID = -1;   // 1 or 2
    G4int scintColID = -1;          // scintillator bar index (if segmented)

    const G4int depth = touchable->GetHistoryDepth();

    for(G4int i = 0; i < depth; ++i) {
        const G4String& volName = touchable->GetVolume(i)->GetName();
        const G4int copyNum = touchable->GetCopyNumber(i);
        //G4cout << "Depth [" << i << "]: " << volName << " (copy=" << copyNum << ")" << G4endl;

        if(volName == "Layer") {
            layerID = copyNum;
        }
        if(volName == "ScintRow"){
            scintRowID = copyNum;
        }
        if(volName == "ScintColumn"){
            scintColID = copyNum;
        }
    }
    // G4cout << "Extracted IDs: layerID=" << layerID 
    //        << ", scintRowID=" << scintRowID 
    //        << ", scintColID=" << scintColID << G4endl;
    // G4cout << "========================" << G4endl;

    if(layerID < 0) {
        G4Exception("ScintillatorSD", "Hit001", JustWarning,
                    "Could not find Layer replica in touchable hierarchy!");
        return false;
    }

    // Layer ID is assumed to be copy number 0
    //G4int layerID = touchable->GetCopyNumber(1);
    G4int trackID = track->GetTrackID();
    G4int parentID = track->GetParentID();
    G4int pdgCode = track->GetParticleDefinition()->GetPDGEncoding();
    //G4LorentzVector p4 = track->GetDynamicParticle()->Get4Momentum();
    G4ThreeVector truthPos = preStep->GetPosition();

    const auto* info =static_cast<const TrackInformation*>(track->GetUserInformation());
    G4bool fromPrimaryLepton = info && info->IsTrackFromPrimaryLepton() || parentID ==0;
    fromPrimaryLepton = fromPrimaryLepton && (std::abs(pdgCode) == 11 || std::abs(pdgCode) == 13 || std::abs(pdgCode) == 15);

    ScintLayerHitID hitID {layerID, scintColID, scintRowID, trackID, pdgCode, parentID, fromPrimaryLepton};
    
    layerEnergyMap[hitID] += edep;

    if(IsFromMuon(trackID))
        layerFromMuonMap[hitID] = true;

    fScintCurrentHitId++;
    return true;
}

void ScintillatorSD::EndOfEvent(G4HCofThisEvent*)
{
    for(const auto& [hitID, edep] : layerEnergyMap)
    {
        if(edep <= 0.) continue;

        auto hit = new ScintHit();
        hit->SetLayerID(hitID.layerID);
        hit->SetColID(hitID.scintColID);
        hit->SetRowID(hitID.scintRowID);
        hit->SetTrackID(hitID.trackID);
        hit->SetParentID(hitID.parentID);
        hit->SetPDGCode(hitID.pdgCode);
        hit->SetEnergyDeposit(edep);
        hit->SetFromMuon(layerFromMuonMap[hitID]);
        hit->SetFromPrimaryLepton(hitID.fromPrimaryLepton);

        fHitsCollection->insert(hit);
    }

    layerEnergyMap.clear();
    layerFromMuonMap.clear();

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
