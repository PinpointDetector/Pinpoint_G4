#include "SteppingAction.hh"
#include "RunAction.hh"

#include <G4Step.hh>
#include <G4Electron.hh>
#include <G4TrackStatus.hh>
#include <G4SystemOfUnits.hh>

#include <TMath.h>

SteppingAction::SteppingAction(RunAction* runAction)
  : fRunAction(runAction)
{
}

void SteppingAction::UserSteppingAction(const G4Step* aStep) {

  //TrackLiveDebugging(aStep);

  G4Track* aTrack = aStep->GetTrack();
  G4ThreeVector post_pos = aStep->GetPostStepPoint()->GetPosition();

  // if the track is out of the active volumes/area, kill this track
  G4VPhysicalVolume* volume = aStep->GetPostStepPoint()->GetTouchable()->GetVolume();

  // In your G4UserSteppingAction (e.g., in the Stepping() method)

  G4Track* track = aStep->GetTrack(); // Get the track from the step
  G4ThreeVector vertexPos = track->GetVertexPosition(); // Get the vertex position
  // Access other info if needed
  // G4double vertexKE = track->GetVertexKineticEnergy();
  // G4ThreeVector vertexMomDir = track->GetVertexMomentumDirection();

  // Use vertexPos (e.g., print it)
  if (track->GetParentID() == 0)
  {
    // G4cout << "Track " << track->GetTrackID() << " vertex at: " << vertexPos << G4endl;
  }
  if (vertexPos.z() < -200.*mm)
  {
   G4cout << "track " << track->GetTrackID() << " born before -400 mm at z = " << vertexPos.z()/mm << " mm" << G4endl; 
  }
  



  // if( volume->GetName() == "expHall_P" ) aTrack->SetTrackStatus(G4TrackStatus::fStopAndKill);
}

void SteppingAction::TrackLiveDebugging(const G4Step* step){

  G4Track* track = step->GetTrack();

  if( track->GetTrackStatus() != fAlive && track->GetTrackStatus() != fStopButAlive) return;

  G4LogicalVolume* volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume();
  G4LogicalVolume* volume_after = step->GetPostStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume();

  G4ThreeVector pre_pos = step->GetPreStepPoint()->GetPosition();
  G4ThreeVector post_pos = step->GetPostStepPoint()->GetPosition();
  G4ThreeVector pre_mom = step->GetPreStepPoint()->GetMomentum();
  G4ThreeVector post_mom = step->GetPostStepPoint()->GetMomentum();
  G4double edep = step->GetTotalEnergyDeposit();

  int PDG = track->GetParticleDefinition()->GetPDGEncoding();
  G4String ParticleName = track->GetDynamicParticle()->GetParticleDefinition()->GetParticleName();
  int TID = track->GetTrackID();
  int SID = step->GetTrack()->GetCurrentStepNumber();

  std::cout << "Track " << TID << " - " << "PDG " << PDG << " " << ParticleName << std::endl;
  std::cout << "stepping... " << SID << " edep" << edep << std::endl;
  std::cout << "(" << pre_pos.x() << "," << pre_pos.y() << "," << pre_pos.z() << ") in " << volume->GetName();
  std::cout << " ---> "  << "(" << post_pos.x() << "," << post_pos.y() << "," << post_pos.z() << ") in " << volume_after->GetName() << std::endl;
  std::cout << "momentum: (" << pre_mom.x() << "," << pre_mom.y() << "," << pre_mom.z() << ") ---> (";
  std::cout << post_mom.x() << "," << post_mom.y() << "," << post_mom.z() << ")" << std::endl;
}
