#ifndef pinpoint_FaserSD_hh
#define pinpoint_FaserSD_hh

#include "FaserHit.hh"
#include "G4VSensitiveDetector.hh"
#include "G4SystemOfUnits.hh"

class G4Step;
class G4HCofThisEvent;

class FaserSD : public G4VSensitiveDetector
{
public:
  FaserSD(const G4String& name, const G4String& hitsCollectionName);
  ~FaserSD() override = default;

  void Initialize(G4HCofThisEvent* hitCollection) override;
  G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
  void EndOfEvent(G4HCofThisEvent* hitCollection) override;

private:
  FaserHitsCollection* fHitsCollection = nullptr;
};

#endif
