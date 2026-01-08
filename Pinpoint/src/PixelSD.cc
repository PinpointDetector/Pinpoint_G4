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
  fUIDToHitIndex.clear();
  fPixelHits.clear();

}

void PixelHitAccumulator::Init()
{
  G4cout << "Initializing PixelHitAccumulator" << G4endl;
  fPixelHits.clear();
  fPixelHits.reserve(fNReservedHits);
  fUIDToHitIndex.reserve(fNReservedHits);

}


PixelHitAccumulator::PixelHitAccumulator()
{
  const auto* det =
    static_cast<const DetectorConstruction*>(
        G4RunManager::GetRunManager()->GetUserDetectorConstruction()
    );

  fNPixelsX = det->GetNPixelsX();
  fNPixelsY = det->GetNPixelsY();
  fNLayers = det->GetNlayers();
  fTotalPixelsPerLayer = fNPixelsX * fNPixelsY;
  Init();
}


PixelHitAccumulator::~PixelHitAccumulator()
{
}

G4bool PixelHitAccumulator::AddHit(G4Step* step)
{

  const auto* track = step->GetTrack();
  const auto* preStepPoint = step->GetPreStepPoint();
  const auto& touchable = preStepPoint->GetTouchableHandle();

  G4double edep = step->GetTotalEnergyDeposit();
  if (edep <= fEdepThreshold / 100.) { // Ignore deposits less than 1/100th of threshold
    return false;
  }

  G4int charge = track->GetDefinition()->GetPDGCharge();
  if (charge == 0) { // Only charged particles hit
    return false;
  }


  static const G4int rowIDVolume = 0, colIDVolume = 1, layerVolume = 3;
  G4int rowID = touchable->GetCopyNumber(rowIDVolume);
  G4int colID = touchable->GetCopyNumber(colIDVolume);
  G4int layerID = touchable->GetCopyNumber(layerVolume);
  G4int trackID = track->GetTrackID();
  G4int parentID = track->GetParentID();
  G4int pdgid = track->GetParticleDefinition()->GetPDGEncoding();
  const auto* info =static_cast<const TrackInformation*>(track->GetUserInformation());
  const G4bool fromPrimaryLepton = info && info->IsTrackFromPrimaryLepton();

  assert(rowID < fNPixelsY);
  assert(colID < fNPixelsX);
  
  using PixelUID = std::uint64_t;

  PixelUID uniqueID =
    static_cast<PixelUID>(layerID) * fTotalPixelsPerLayer +
    static_cast<PixelUID>(rowID)   * fNPixelsX +
    static_cast<PixelUID>(colID);


  auto [it, inserted] =
      fUIDToHitIndex.try_emplace(uniqueID, fPixelHits.size());

  if (!inserted) {
    fPixelHits[it->second]->AddEnergyDeposit(edep);
  } else {
    fPixelHits.push_back(
      new PixelHit(edep, rowID, colID, layerID,
                  trackID, parentID, pdgid, fromPrimaryLepton)
    );
  }

  return true;
}

void PixelHitAccumulator::FillHitCollection(PixelHitsCollection* hitCollection) const
{
  for (size_t i = 0; i < fPixelHits.size(); ++i) {
  
    if (fPixelHits[i]->GetEnergyDeposit() <= fEdepThreshold) 
    {
      delete fPixelHits[i];
      continue;
    }

    hitCollection->insert(fPixelHits[i]);
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