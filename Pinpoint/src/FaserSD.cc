#include "FaserSD.hh"
#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4Track.hh"
#include "G4VTouchable.hh"
#include "G4TouchableHistory.hh"
#include "G4ParticleDefinition.hh"
#include "G4ios.hh"

FaserSD::FaserSD(const G4String& name, const G4String& hitsCollectionName)
  : G4VSensitiveDetector(name)
{
  collectionName.insert(hitsCollectionName);
}

void FaserSD::Initialize(G4HCofThisEvent* hce)
{
  fHitsCollection = new FaserHitsCollection(SensitiveDetectorName, collectionName[0]);
  
  G4int hcID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
  hce->AddHitsCollection(hcID, fHitsCollection);
}

G4bool FaserSD::ProcessHits(G4Step* step, G4TouchableHistory* /*history*/)
{
  const auto* track = step->GetTrack();
  const auto* preStepPoint = step->GetPreStepPoint();
  const auto& touchable = preStepPoint->GetTouchableHandle();
  
  G4double edep = step->GetTotalEnergyDeposit();
  
  G4double charge = track->GetDefinition()->GetPDGCharge();
  if (charge == 0) {
    return false;
  }
  
  // Get tracker ID from copy number (0, 1, or 2)
  G4int trackerID = touchable->GetCopyNumber();
  
  G4int trackID = track->GetTrackID();
  G4int parentID = track->GetParentID();
  G4int pdgCode = track->GetParticleDefinition()->GetPDGEncoding();
  
  G4ThreeVector position = preStepPoint->GetPosition();
  G4ThreeVector momentum = preStepPoint->GetMomentum();
  G4double energy = preStepPoint->GetTotalEnergy();
  
  auto* hit = new FaserHit(trackerID, trackID, parentID, pdgCode,
                           position, momentum, energy, edep, charge);
  fHitsCollection->insert(hit);
  
  return true;
}

void FaserSD::EndOfEvent(G4HCofThisEvent* /*hce*/)
{
  if (verboseLevel > 1) {
    std::size_t nofHits = fHitsCollection->entries();
    G4cout << G4endl << "-------->Hits Collection: in this event there are " << nofHits
           << " hits in the Faser tracker detector: " << G4endl;
    for (std::size_t i = 0; i < nofHits; i++)
      (*fHitsCollection)[i]->Print();
  }
}
