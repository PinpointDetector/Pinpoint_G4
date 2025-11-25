#ifndef fasernux_ScintillatorSD_hh
#define fasernux_ScintillatorSD_hh

#include "ScintHit.hh"
#include "G4VSensitiveDetector.hh"
#include <set>
#include <vector>

class G4Step;
class G4HCofThisEvent;

class ScintillatorSD : public G4VSensitiveDetector
{
public:
    ScintillatorSD(const G4String& name, const G4String& hitsCollectionName);
    ~ScintillatorSD() override = default;

    void Initialize(G4HCofThisEvent* hitCollection) override;
    G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
    void EndOfEvent(G4HCofThisEvent* hitCollection) override;

    // ---- Muon ancestry tracking (same as PixelSD) ----
    static void RecordMuonDescendant(G4int trackID, G4bool fromMuon);
    static G4bool IsFromMuon(G4int trackID);
    static void ClearMuonHistory();

private:
    ScintHitsCollection* fHitsCollection = nullptr;

    // Static set to track all descendants of the primary lepton (trackId 1)
    static std::set<G4int> sScintPrimaryDescendants;
    // Static set to track particles that have already hit each layer: (trackID, layerID)
    static std::set<std::pair<G4int, G4int>> sScintHitParticles;
    // Static set to track which particles come from muons
     static std::set<G4int> sScintMuonDescendants;

    G4long fScintCurrentHitId = 0;
};

#endif
