#ifndef fasernux_PixelSD_hh
#define fasernux_PixelSD_hh

#include "PixelHit.hh"
#include "G4VSensitiveDetector.hh"
#include <set>
#include <tuple>
#include <vector>
#include <functional>
#include <map>

class G4Step;
class G4HCofThisEvent;

// Structure to uniquely identify a pixel
struct PixelID {
  G4int layerID;
  G4int rowID;
  G4int colID;
  G4ThreeVector truthPos;
  G4int trackID;
  
  bool operator<(const PixelID& other) const {
    if (layerID != other.layerID) return layerID < other.layerID;
    if (rowID != other.rowID) return rowID < other.rowID;
    if (trackID != other.trackID) return trackID < other.trackID;
    return colID < other.colID;
  }

  bool operator==(const PixelID& other) const {
    return layerID == other.layerID &&
           rowID == other.rowID &&
           colID == other.colID;
  }

};

namespace std {
    template<>
    struct hash<PixelID> {
        std::size_t operator()(const PixelID& id) const noexcept {
            std::size_t h1 = std::hash<int>{}(id.layerID);
            std::size_t h2 = std::hash<int>{}(id.rowID);
            std::size_t h3 = std::hash<int>{}(id.colID);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

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

  // Map to accumate charge on each pixel
  std::unordered_map<PixelID, double> fPixelEdepMap;

  G4long fCurrentHitId = 0;
};

#endif
