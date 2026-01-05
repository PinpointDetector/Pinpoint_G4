#include "PixelSD.hh"
#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4Track.hh"
#include "G4VTouchable.hh"
#include "G4TouchableHistory.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4LorentzVector.hh"
#include "G4RunManager.hh"
#include "G4Event.hh"
#include "TrackInformation.hh"



PixelSD::PixelSD(const G4String& name, const G4String& hitsCollectionName)
  : G4VSensitiveDetector(name)
{
  collectionName.insert(hitsCollectionName);
}


void PixelSD::Initialize(G4HCofThisEvent* hce)
{
  fHitsCollection = new PixelHitsCollection(SensitiveDetectorName, collectionName[0]);

  G4int hcID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
  hce->AddHitsCollection(hcID, fHitsCollection);
  
  fCurrentHitId = 0;
}


G4bool PixelSD::ProcessHits(G4Step* step, G4TouchableHistory* history)
{
  G4Track* track = step->GetTrack();
  
  // Neutral particles don't hot
  if (track->GetDefinition()->GetPDGCharge() == 0) {
    return false;
  }
  
  // Min hit energy of 360 eV
  if (track->GetDynamicParticle()->Get4Momentum().e() <= 360*1E-6) {
    return false;
  }
  
  // If it hasn't left an energy deposit then skip
  G4double edep = step->GetTotalEnergyDeposit();
  if (edep <= 0.) {
    return false;
  }

  // Get the cell ID from the touchable history
  G4StepPoint* preStepPoint = step->GetPreStepPoint();
  G4TouchableHandle touchable = preStepPoint->GetTouchableHandle();

  G4int rowIDVolume = 0, colIDVolume = 1, layerVolume = 3;
  G4int rowID = touchable->GetCopyNumber(rowIDVolume);
  G4int colID = touchable->GetCopyNumber(colIDVolume);
  G4int layerID = touchable->GetCopyNumber(layerVolume);
  G4int trackID = track->GetTrackID();
  G4ThreeVector truthPos = preStepPoint->GetPosition();
  
  auto newHit = new PixelHit();
  newHit->SetLayerID(layerID);
  newHit->SetRowID(rowID);
  newHit->SetColID(colID);
  newHit->SetTrackID(trackID);
  newHit->SetEnergyDeposit(edep);
  newHit->SetTruthHitPos(truthPos);
  
  fHitsCollection->insert(newHit);
  

  fCurrentHitId++;

  return true;
}


void PixelSD::EndOfEvent(G4HCofThisEvent* /*hce*/)
{
  if (verboseLevel > 1) {
    std::size_t nofHits = fHitsCollection->entries();
    G4cout << G4endl << "-------->Hits Collection: in this event there are " << nofHits
           << " pixels with charge deposits in the pixel detector: " << G4endl;
    for (std::size_t i = 0; i < nofHits; i++)
      (*fHitsCollection)[i]->Print();
  }
}

