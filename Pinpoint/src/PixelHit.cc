#include "PixelHit.hh"
#include "G4UnitsTable.hh"
#include "G4VVisManager.hh"
#include "G4Circle.hh"
#include "G4Colour.hh"
#include "G4VisAttributes.hh"

#include <iomanip>

G4ThreadLocal G4Allocator<PixelHit>* PixelHitAllocator = nullptr;

G4bool PixelHit::operator==(const PixelHit& right) const
{
  return ( this == &right ) ? true : false;
}

void PixelHit::Draw()
{
  G4VVisManager* pVVisManager = G4VVisManager::GetConcreteInstance();
  if(pVVisManager)
  {
    G4Circle circle({0.0,0.0,0.0});
    circle.SetScreenSize(4.);
    circle.SetFillStyle(G4Circle::filled);
    G4Colour colour(1.,0.,0.);
    G4VisAttributes attribs(colour);
    circle.SetVisAttributes(attribs);
    pVVisManager->Draw(circle);
  }
}

void PixelHit::Print()
{
  G4cout
    //  << "  trackID: " << fTrackID
    //  << "  PDG: " << fPDGCode
     << "  layer: " << fLayerID
     << "  pixel(" << fRowID << "," << fColID << ")"
    //  << "  Position: " << std::setw(7) << G4BestUnit( fPos,"Length")
     << "  Charge: " << std::setw(7) << G4BestUnit(fEnergyDeposit,"Energy")
     << G4endl;
}
