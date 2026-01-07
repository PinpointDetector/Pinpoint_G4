#include "PixelSD.hh"
#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4Track.hh"
#include "G4VTouchable.hh"
#include "G4TouchableHistory.hh"
#include "G4ParticleDefinition.hh"
#include "G4ios.hh"
#include "G4LorentzVector.hh"
#include "G4RunManager.hh"
#include "G4Event.hh"
#include "TrackInformation.hh"
#include "DetectorConstruction.hh"

void PixelHitAccumulator::Clear()
{
  fEdep.clear();
  fRowID.clear();
  fColID.clear();
  fLayerID.clear();
  // fPixelID.clear();
  fPDGID.clear();
  fTrackID.clear();
  fParentID.clear();
  fIsPrimary.clear();
  fUID_VectIdx_Map.clear();
}

void PixelHitAccumulator::Init()
{
  Clear();

  // Reserve memory for the hit data
  fEdep.reserve(fNReservedHits);
  fRowID.reserve(fNReservedHits);
  fColID.reserve(fNReservedHits);
  fLayerID.reserve(fNReservedHits);
  // fPixelID.reserve(fNReservedHits);
  fPDGID.reserve(fNReservedHits);
  fTrackID.reserve(fNReservedHits);
  fParentID.reserve(fNReservedHits);
  fIsPrimary.reserve(fNReservedHits);
}

PixelHitAccumulator::PixelHitAccumulator()
{
  const auto* det =
    static_cast<const DetectorConstruction*>(
        G4RunManager::GetRunManager()->GetUserDetectorConstruction()
    );

  fNPixelsX = det->GetNPixelsX();
  fNPixelsY = det->GetNPixelsY();
  fTotalPixelsPerLayer = fNPixelsX * fNPixelsY;
  Init();
}


PixelHitAccumulator::~PixelHitAccumulator()
{
}

G4bool PixelHitAccumulator::AddHit(G4Step* step)
{
  G4Track* track = step->GetTrack();
  G4int charge = track->GetDefinition()->GetPDGCharge();
  if (charge == 0) { // Only charged particles hit
    return false;
  }

  G4StepPoint* preStepPoint = step->GetPreStepPoint();
  G4double edep = step->GetTotalEnergyDeposit();
  if (edep <= fEdepThreshold / 100.) { // Ignore deposits less than 1/100th of threshold
    return false;
  }

  static const G4int rowIDVolume = 0, colIDVolume = 1, layerVolume = 3;
  G4TouchableHandle touchable = preStepPoint->GetTouchableHandle();
  G4int rowID = touchable->GetCopyNumber(rowIDVolume);
  G4int colID = touchable->GetCopyNumber(colIDVolume);
  G4int layerID = touchable->GetCopyNumber(layerVolume);
  G4int trackID = track->GetTrackID();
  G4int parentID = track->GetParentID();
  G4int pdgid = track->GetParticleDefinition()->GetPDGEncoding();
  TrackInformation* trackInfo = dynamic_cast<TrackInformation*>(track->GetUserInformation());
  G4bool fromPrimaryLepton = trackInfo ? (trackInfo->IsTrackFromPrimaryLepton() != 0) : false;

  assert(rowID < fNPixelsY);
  assert(colID < fNPixelsX);
  
  G4int uniqueID = (layerID * fTotalPixelsPerLayer) + (rowID * fNPixelsX) + colID;

  auto it = fUID_VectIdx_Map.find(uniqueID);
  if (it != fUID_VectIdx_Map.end()) { // We've already had a hit on this pixel, accumulate the energy
    G4int index = it->second;
    fEdep[index] += edep;
  } else { // No hit so far; extend vectors and push back data
    fEdep.push_back(std::move(edep));
    fRowID.push_back(std::move(rowID));
    fColID.push_back(std::move(colID));
    fLayerID.push_back(std::move(layerID));
    // fPixelID.push_back(std::move(uniqueID));
    fPDGID.push_back(std::move(pdgid));
    fTrackID.push_back(std::move(trackID));
    fParentID.push_back(std::move(parentID));
    fIsPrimary.push_back(std::move(fromPrimaryLepton));
  }
  return true;
}


void PixelHitAccumulator::FillHitCollection(PixelHitsCollection* hitCollection) const
{
  for (size_t i = 0; i < fEdep.size(); ++i) {
    PixelHit* hit = new PixelHit();

    if (fEdep[i] <= fEdepThreshold) 
    {
      // G4cout << "Warning: Pixel hit energy deposit too low: " << fEdep[i]/eV << " eV. Skipping this hit." << G4endl;
      continue;
    }
    

    // G4int layerID  = fPixelID[i] / fTotalPixelsPerLayer;
    // G4int localID = fPixelID[i] % fTotalPixelsPerLayer;
    // G4int rowID = localID / fNPixelsX;
    // G4int colID = localID % fNPixelsX;

    hit->SetEnergyDeposit(std::move(fEdep[i]));
    hit->SetRowID(std::move(fRowID[i]));
    hit->SetColID(std::move(fColID[i]));
    hit->SetLayerID(std::move(fLayerID[i]));
    hit->SetPDGCode(std::move(fPDGID[i]));
    hit->SetTrackID(std::move(fTrackID[i]));
    hit->SetParentID(std::move(fParentID[i]));
    hit->SetFromPrimaryLepton(std::move(fIsPrimary[i]));

    hitCollection->insert(hit);
  }
}


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
  fCurrentHitId = 0;
  fHitAccumulator.Init();

}


G4bool PixelSD::ProcessHits(G4Step* step, G4TouchableHistory* /*history*/)
{
  return fHitAccumulator.AddHit(step);
}


void PixelSD::EndOfEvent(G4HCofThisEvent* /*hce*/)
{
  fHitAccumulator.FillHitCollection(fHitsCollection);
  fHitAccumulator.Clear();

  if (verboseLevel > 1) {
    std::size_t nofHits = fHitsCollection->entries();
    G4cout << G4endl << "-------->Hits Collection: in this event there are " << nofHits
           << " pixels with charge deposits in the pixel detector: " << G4endl;
    for (std::size_t i = 0; i < nofHits; i++)
      (*fHitsCollection)[i]->Print();
  }
}