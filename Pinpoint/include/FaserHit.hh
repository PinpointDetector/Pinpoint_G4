#ifndef pinpoint_FaserHit_hh
#define pinpoint_FaserHit_hh

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "G4Threading.hh"
#include "G4LorentzVector.hh"

class FaserHit : public G4VHit
{
public:
  FaserHit() = default;
  FaserHit(G4int trackerID, G4int trackID, G4int parentID, G4int pdgc, 
           const G4ThreeVector& pos, const G4ThreeVector& mom, 
           G4double energy, G4double edep, G4double charge);
  FaserHit(const FaserHit&) = default;
  ~FaserHit() override = default;

  FaserHit& operator=(const FaserHit&) = default;
  G4bool operator==(const FaserHit&) const;

  inline void* operator new(size_t);
  inline void operator delete(void*);

  void Draw() override;
  void Print() override;

  void SetTrackerID(G4int id) { fTrackerID = id; }
  void SetTrackID(G4int trackID) { fTrackID = trackID; }
  void SetParentID(G4int parentID) { fParentID = parentID; }
  void SetPDGCode(G4int pdg) { fPDGCode = pdg; }
  void SetPosition(const G4ThreeVector& pos) { fPosition = pos; }
  void SetMomentum(const G4ThreeVector& mom) { fMomentum = mom; }
  void SetEnergy(G4double energy) { fEnergy = energy; }
  void SetEnergyDeposit(G4double edep) { fEnergyDeposit = edep; }
  void SetCharge(G4double charge) { fCharge = charge; }

  G4int GetTrackerID() const { return fTrackerID; }
  G4int GetTrackID() const { return fTrackID; }
  G4int GetParentID() const { return fParentID; }
  G4int GetPDGCode() const { return fPDGCode; }
  G4ThreeVector GetPosition() const { return fPosition; }
  G4ThreeVector GetMomentum() const { return fMomentum; }
  G4double GetEnergy() const { return fEnergy; }
  G4double GetEnergyDeposit() const { return fEnergyDeposit; }
  G4double GetCharge() const { return fCharge; }
  G4double GetPx() const { return fMomentum.x(); }
  G4double GetPy() const { return fMomentum.y(); }
  G4double GetPz() const { return fMomentum.z(); }
  G4double GetX() const { return fPosition.x(); }
  G4double GetY() const { return fPosition.y(); }
  G4double GetZ() const { return fPosition.z(); }

private:
  G4int fTrackerID = -1;    // FASER tracker layer: 0, 1, or 2
  G4int fTrackID = -1;
  G4int fParentID = -1;
  G4int fPDGCode = 0;
  G4ThreeVector fPosition;
  G4ThreeVector fMomentum;
  G4double fEnergy = 0.0;
  G4double fEnergyDeposit = 0.0;
  G4double fCharge = 0.0;
};

using FaserHitsCollection = G4THitsCollection<FaserHit>;

extern G4ThreadLocal G4Allocator<FaserHit>* FaserHitAllocator;

inline void* FaserHit::operator new(size_t)
{
  if (!FaserHitAllocator) FaserHitAllocator = new G4Allocator<FaserHit>;
  return (void*)FaserHitAllocator->MallocSingle();
}

inline void FaserHit::operator delete(void* hit)
{
  FaserHitAllocator->FreeSingle((FaserHit*)hit);
}

#endif
