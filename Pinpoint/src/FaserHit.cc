#include "FaserHit.hh"
#include "G4UnitsTable.hh"
#include "G4VVisManager.hh"
#include "G4Circle.hh"
#include "G4Colour.hh"
#include "G4VisAttributes.hh"
#include "G4SystemOfUnits.hh"

#include <iomanip>

G4ThreadLocal G4Allocator<FaserHit>* FaserHitAllocator = nullptr;

G4bool FaserHit::operator==(const FaserHit& right) const
{
  return ( this == &right ) ? true : false;
}

void FaserHit::Draw()
{
  G4VVisManager* pVVisManager = G4VVisManager::GetConcreteInstance();
  if(pVVisManager)
  {
    G4Circle circle(fPosition);
    circle.SetScreenSize(4.);
    circle.SetFillStyle(G4Circle::filled);
    G4Colour colour(0.,1.,0.);
    G4VisAttributes attribs(colour);
    circle.SetVisAttributes(attribs);
    pVVisManager->Draw(circle);
  }
}

void FaserHit::Print()
{
  G4cout
     << "  TrackerID: " << fTrackerID
     << "  TrackID: " << fTrackID
     << "  PDG: " << fPDGCode
     << "  Position: (" << fPosition.x()/mm << ", " << fPosition.y()/mm << ", " << fPosition.z()/mm << ") mm"
     << "  Momentum: (" << fMomentum.x()/GeV << ", " << fMomentum.y()/GeV << ", " << fMomentum.z()/GeV << ") GeV"
     << "  Energy: " << fEnergy/GeV << " GeV"
     << "  Edep: " << G4BestUnit(fEnergyDeposit,"Energy")
     << G4endl;
}

FaserHit::FaserHit(G4int trackerID, G4int trackID, G4int parentID, G4int pdgc, 
                   const G4ThreeVector& pos, const G4ThreeVector& mom, 
                   G4double energy, G4double edep, G4double charge)
: fTrackerID{trackerID}, fTrackID{trackID}, fParentID{parentID}, fPDGCode{pdgc}, 
  fPosition{pos}, fMomentum{mom}, fEnergy{energy}, fEnergyDeposit{edep}, fCharge{charge}
{
}
