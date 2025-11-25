#include "ScintHit.hh"
#include "G4UnitsTable.hh"
#include "G4VVisManager.hh"
#include "G4Circle.hh"
#include "G4Colour.hh"
#include "G4VisAttributes.hh"
#include "G4ios.hh"
#include <iomanip>

G4ThreadLocal G4Allocator<ScintHit>* ScintHitAllocator = nullptr;

void ScintHit::Draw()
{
    G4VVisManager* pVVisManager = G4VVisManager::GetConcreteInstance();
    if (pVVisManager)
    {
        G4Circle circle({0.0,0.0,0.0});
        circle.SetScreenSize(6.);
        circle.SetFillStyle(G4Circle::filled);
        G4Colour colour(0.,0.,1.); // Blue for scintillator
        G4VisAttributes attribs(colour);
        circle.SetVisAttributes(attribs);
        pVVisManager->Draw(circle);
    }
}

void ScintHit::Print()
{
    G4cout
         << "  trackID: " << fTrackID
         << "  PDG: " << fPDGCode
         << "  layer: " << fLayerID
         << "  EnergyDeposit: " << std::setw(7) << G4BestUnit(fEnergyDeposit,"Energy")
         << G4endl;
}
