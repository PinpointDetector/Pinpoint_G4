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
  fPixelHits.clear();
  fUIDToHitIndex.clear();
}

void PixelHitAccumulator::Init()
{
  Clear();
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
  fPixelWidth = det->GetPixelWidth();
  fPixelHeight = det->GetPixelHeight();
  fDetWidth = det->GetDetectorWidth();
  fDetHeight = det->GetDetectorHeight();
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
  if (edep < fEdepThreshold) { // Ignore deposits less than threshold for one electron-hole pair
    return false;
  }

  G4int charge = track->GetDefinition()->GetPDGCharge();
  if (charge == 0) { // Only charged particles hit
    return false;
  }

  auto pos = step->GetPreStepPoint()->GetPosition();
  G4int colID = static_cast<G4int>(
  (pos.x() + 0.5*fDetWidth) / fPixelWidth
  );
  G4int rowID = static_cast<G4int>(
    (pos.y() + 0.5*fDetHeight) / fPixelHeight
  );
    
  G4int layerID = touchable->GetCopyNumber(1);
  G4ThreeVector sensorCenterGlobal = touchable->GetTranslation();
  G4double sensorCentreZ = sensorCenterGlobal.z();
    
  G4int trackID = track->GetTrackID();
  G4int parentID = track->GetParentID();
  G4int pdgid = track->GetParticleDefinition()->GetPDGEncoding();
  const auto* info =static_cast<const TrackInformation*>(track->GetUserInformation());
  G4bool fromPrimaryLepton = info && info->IsTrackFromPrimaryLepton() || parentID ==0;
  fromPrimaryLepton = fromPrimaryLepton && (std::abs(pdgid) == 11 || std::abs(pdgid) == 13 || std::abs(pdgid) == 15);
  G4bool fromPrimaryEMShower = info && info->IsTrackFromPrimaryEMShower();
  G4bool fromCharmedHadron = info && info->IsTrackFromCharmedHadron();
  G4bool fromTau = info && info->IsTrackFromTau();

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
                  trackID, parentID, pdgid, fromPrimaryLepton, fromPrimaryEMShower, fromCharmedHadron, fromTau)
    );
  }

  return true;
}

void PixelHitAccumulator::FillHitCollection(PixelHitsCollection* hitCollection) const
{
  for (size_t i = 0; i < fPixelHits.size(); ++i) {
  
    if (fPixelHits[i]->GetEnergyDeposit() < fEdepThreshold * fNElectronsThreshold) 
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