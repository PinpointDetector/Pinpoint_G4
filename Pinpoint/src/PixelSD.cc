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
#include <map>
#include "G4LorentzVector.hh"
#include "G4RunManager.hh"
#include "G4Event.hh"
#include "TrackInformation.hh"


// std::set<G4int> PixelSD::sPrimaryDescendants;
std::set<std::pair<G4int, G4int>> PixelSD::sHitParticles;
std::set<G4int> PixelSD::sMuonDescendants;

// Structure to uniquely identify a pixel
struct PixelID {
  G4int layerID;
  G4int rowID;
  G4int colID;
  G4LorentzVector p4;
  G4int pdgCode;
  G4int charge;
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

struct PixelKey {
  G4int layerID;
  G4int rowID;
  G4int colID;

  bool operator==(const PixelKey& other) const {
    return layerID == other.layerID &&
           rowID == other.rowID &&
           colID == other.colID;
  }
};

// Hash function for PixelKey
namespace std {
  template <>
  struct hash<PixelKey> {
    std::size_t operator()(const PixelKey& k) const {
      return ((std::hash<G4int>()(k.layerID) ^ (std::hash<G4int>()(k.rowID) << 1)) >> 1)
             ^ (std::hash<G4int>()(k.colID) << 1);
    }
  };
}

static std::unordered_map<PixelKey, PixelID> bestPixels;
static std::unordered_map<PixelKey, double>  totalCharge;


// Map to accumulate charge in each pixel during an event
static std::map<PixelID, G4double> pixelChargeMap;
// Map to track if pixel received energy from muon descendants
static std::map<PixelID, G4bool> pixelFromMuonMap;

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
  
  // Clear the pixel charge map for this event
  pixelChargeMap.clear();
  pixelFromMuonMap.clear();
  bestPixels.clear();
  totalCharge.clear();
  fCurrentHitId = 0;
}


G4bool PixelSD::ProcessHits(G4Step* step, G4TouchableHistory* /*history*/)
{
  G4Track* track = step->GetTrack();
  
  if (track->GetDefinition()->GetPDGCharge() == 0) {
    return false;
  }
  
  G4double edep = step->GetTotalEnergyDeposit();
  if (edep <= 0.) {
    return false;
  }



  G4StepPoint* preStepPoint = step->GetPreStepPoint();
  G4TouchableHandle touchable = preStepPoint->GetTouchableHandle();
  
  G4int rowIDVolume = 0, colIDVolume = 1, layerVolume = 3;
  G4int rowID = touchable->GetCopyNumber(rowIDVolume);
  G4int colID = touchable->GetCopyNumber(colIDVolume);
  G4int layerID = touchable->GetCopyNumber(layerVolume);
  G4int trackID = track->GetTrackID();
  // G4ThreeVector hitPosition = preStepPoint->GetPosition();
  G4int pdgid = track->GetParticleDefinition()->GetPDGEncoding();
  G4LorentzVector p4 = track->GetDynamicParticle()->Get4Momentum();
  G4int charge = track->GetDefinition()->GetPDGCharge();
  
  // TODO: Min hit energy of 360 eV
  if (p4.e() <= 360*1E-6) {
    return false;
  }

  // G4cout << "Processing hit: TrackID=" << trackID 
  //        << " PDG=" << pdgid 
  //        << " Layer=" << layerID 
  //        << " Row=" << rowID 
  //        << " Col=" << colID 
  //        << " Edep=" << edep/keV << " keV" 
  //        << G4endl;

  // Create pixel identifier
  PixelID pixelId = {layerID, rowID, colID, p4, pdgid, charge, trackID};
  pixelChargeMap[pixelId] += edep;

  // Register hit in TrackInformation
  // TrackInformation* trackInfo = dynamic_cast<TrackInformation*>(track->GetUserInformation());
  // if (!trackInfo) {
  //     trackInfo = new TrackInformation(track); // or just new TrackInformation();
  //     track->SetUserInformation(trackInfo);
  // }
  // trackInfo->InsertHit(fCurrentHitId);
  fCurrentHitId++;


  // Track if this pixel received energy from a muon descendant
  if (IsFromMuon(trackID)) {
    pixelFromMuonMap[pixelId] = true;
  }
  
  return true;
}


void PixelSD::EndOfEvent(G4HCofThisEvent* /*hce*/)
{
  // Get detector geometry parameters from DetectorConstruction
  // G4double tungstenThickness = DetectorConstruction::GetTungstenThickness();
  // G4double siliconThickness = DetectorConstruction::GetSiliconThickness();
  // G4double detectorSizeX = DetectorConstruction::GetDetectorSizeX();
  // G4double detectorSizeY = DetectorConstruction::GetDetectorSizeY();
  // G4double pixelSizeX = DetectorConstruction::GetPixelSizeX();
  // G4double pixelSizeY = DetectorConstruction::GetPixelSizeY();
  // G4double layerThickness = DetectorConstruction::GetLayerThickness();

  for (const auto& [pixel, charge] : pixelChargeMap) {
    PixelKey key{pixel.layerID, pixel.rowID, pixel.colID};
  
    auto it = bestPixels.find(key);
    if (it == bestPixels.end()) {
      bestPixels[key] = pixel;
      totalCharge[key] = charge;
    } else {
      const auto& existing = it->second;
      if (pixel.trackID != existing.trackID) {
        if (pixel.p4.e() > existing.p4.e()) {
          bestPixels[key] = pixel;
        }
        totalCharge[key] += charge;
      }
    }
  }
  pixelChargeMap.clear();
  for (const auto& [key, pixel] : bestPixels) {
    pixelChargeMap[pixel] = totalCharge[key];
  }
  

  // Create hits from accumulated charge in each pixel
  for (const auto& pixel : pixelChargeMap) {
    const PixelID& pixelId = pixel.first;
    G4double totalCharge = pixel.second;
    
    // Only create a hit if there's significant charge deposit
    if (totalCharge > 0.0) {
      auto newHit = new PixelHit();
      newHit->SetLayerID(pixelId.layerID);
      newHit->SetRowID(pixelId.rowID);
      newHit->SetColID(pixelId.colID);
      newHit->SetP4(pixelId.p4);
      newHit->SetCharge(pixelId.charge);
      newHit->SetTrackID(pixelId.trackID);
      newHit->SetPDGCode(pixelId.pdgCode);
      newHit->SetEnergyDeposit(totalCharge);
      newHit->SetFromMuon(pixelFromMuonMap[pixelId]);  // Set if any track from muon hit this pixel
      
      // Calculate pixel center position in global coordinates
      // X position: pixel index to world coordinates
      // G4double pixelCenterX = (pixelId.rowID + 0.5) * pixelSizeX - detectorSizeX/2.0;
      
      // // Y position: pixel index to world coordinates  
      // G4double pixelCenterY = (pixelId.colID + 0.5) * pixelSizeY - detectorSizeY/2.0;
      
      // // Z position: layer position + silicon layer offset
      // // Detector starts at z=0, so layer 0 starts at z=0
      // G4double layerCenterZ = (pixelId.layerID + 0.5) * layerThickness;
      // // Add offset to silicon layer center within the layer
      // G4double siliconOffsetZ = tungstenThickness + siliconThickness/2.0;
      // G4double pixelCenterZ = layerCenterZ - layerThickness/2.0 + siliconOffsetZ;
      
      // newHit->SetPos(G4ThreeVector(pixelCenterX, pixelCenterY, pixelCenterZ));
      
      // Set default values for other fields since we're aggregating
      // newHit->SetTrackID(-1); // Multiple tracks may contribute
      // newHit->SetPDGCode(0);   // Multiple particle types may contribute
      // newHit->SetIsFromPrimary(false); // Could be mix of primary/secondary
      
      fHitsCollection->insert(newHit);
    }
  }

  if (verboseLevel > 1) {
    std::size_t nofHits = fHitsCollection->entries();
    G4cout << G4endl << "-------->Hits Collection: in this event there are " << nofHits
           << " pixels with charge deposits in the pixel detector: " << G4endl;
    for (std::size_t i = 0; i < nofHits; i++)
      (*fHitsCollection)[i]->Print();
  }
}


// void PixelSD::ClearTrackHistory()
// {
//   // sPrimaryDescendants.clear();
//   sHitParticles.clear();
//   // Track 1 will be added when RecordTrackParent is called for it
// }

// void PixelSD::RecordTrackParent(G4int trackID, G4int parentID)
// {
//   if (trackID == 1) {
//     sPrimaryDescendants.insert(trackID);
//     return;
//   }

//   // For other tracks, check if parent is from primary
//   // Special case: if parent is 1, then this track is from primary
//   if (parentID == 1) {
//     sPrimaryDescendants.insert(trackID);
//     return;
//   }

//   // If parent is already known to be from primary, add this track too
//   if (sPrimaryDescendants.find(parentID) != sPrimaryDescendants.end()) {
//     sPrimaryDescendants.insert(trackID);
//   }
// }

// G4bool PixelSD::IsFromPrimaryTrack(G4int trackID)
// {
//   // Simple O(1) lookup in the set
//   return sPrimaryDescendants.find(trackID) != sPrimaryDescendants.end();
// }

void PixelSD::RecordMuonDescendant(G4int trackID, G4bool fromMuon)
{
  if (fromMuon) {
    sMuonDescendants.insert(trackID);
  }
}

G4bool PixelSD::IsFromMuon(G4int trackID)
{
  return sMuonDescendants.find(trackID) != sMuonDescendants.end();
}

void PixelSD::ClearMuonHistory()
{
  sMuonDescendants.clear();
}
