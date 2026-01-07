#ifndef fasernux_PixelSD_hh
#define fasernux_PixelSD_hh

#include "PixelHit.hh"
#include "G4VSensitiveDetector.hh"
#include "G4SystemOfUnits.hh"
#include <map>
#include <vector>

class G4Step;
class G4HCofThisEvent;

class PixelHitAccumulator
{
  public:
    PixelHitAccumulator();
    ~PixelHitAccumulator();

    G4bool AddHit(G4Step* step);
    void FillHitCollection(PixelHitsCollection* hitCollection) const;
    void Init();
    void Clear();

  private:
    std::vector<G4float> fEdep;
    std::vector<G4int> fRowID;
    std::vector<G4int> fColID;
    std::vector<G4int> fLayerID;
    // std::vector<G4int> fPixelID;
    std::vector<G4int> fPDGID;
    std::vector<G4int> fTrackID;
    std::vector<G4int> fParentID;
    std::vector<G4bool> fIsPrimary;

    std::map<G4int, G4int> fUID_VectIdx_Map;
    
    G4int fNPixelsX{0};
    G4int fNPixelsY{0};
    G4int fTotalPixelsPerLayer{0};
    
    G4int currentIndex{0};

    // Minimum energy deposit to register a hit
    // Based roughly on the fact that ~3.6 eV is needed to create an electron-hole pair in silicon
    // Roughly 100 electron-hole pairs to be accepted as a hit
    G4double fEdepThreshold = 360 * eV;

    // Number of hits to reserve memory for, 100,000 seems like a good starting point
    G4int fNReservedHits{100000};
};


class PixelSD : public G4VSensitiveDetector
{
public:
  PixelSD(const G4String& name, const G4String& hitsCollectionName);
  ~PixelSD() override = default;

  void Initialize(G4HCofThisEvent* hitCollection) override;
  G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
  void EndOfEvent(G4HCofThisEvent* hitCollection) override;

private:
  PixelHitsCollection* fHitsCollection = nullptr;

  PixelHitAccumulator fHitAccumulator;

  G4long fCurrentHitId = 0;
};


#endif