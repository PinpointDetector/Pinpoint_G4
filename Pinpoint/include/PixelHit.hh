#ifndef fasernux_PixelHit_hh
#define fasernux_PixelHit_hh

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "G4Threading.hh"
#include "G4LorentzVector.hh"

class PixelHit : public G4VHit
{
public:
  PixelHit() = default;
  PixelHit(const PixelHit&) = default;
  ~PixelHit() override = default;

  PixelHit& operator=(const PixelHit&) = default;
  G4bool operator==(const PixelHit&) const;

  inline void* operator new(size_t);
  inline void operator delete(void*);

  void Draw() override;
  void Print() override;

  // void SetTrackID(G4int track) { fTrackID = track; }
  // void SetPDGCode(G4int pdg) { fPDGCode = pdg; }
  void SetRowID(G4int row) { fRowID = row; }
  void SetPDGCode(G4int pdg) { fPDGCode = pdg; }
  void SetColID(G4int column) { fColID = column; }
  void SetLayerID(G4int layer) { fLayerID = layer; }
  void SetTrackID(G4int trackID) { fTrackID = trackID; }
  void SetParentID(G4int parentID) { fParentID = parentID; }
  void SetPixelX(G4int pixelX) { fPixelX = pixelX; }
  void SetPixelY(G4int pixelY) { fPixelY = pixelY; }
  // void SetPos(G4ThreeVector xyz) { fPos = xyz; }
  // void SetIsFromPrimary(G4bool fromPrimary) { fIsFromPrimary = fromPrimary; }
  void SetEnergyDeposit(G4double edep) { fEnergyDeposit = edep; }
  void SetFromMuon(G4bool fromMuon) { fFromMuon = fromMuon; }
  // void AddEnergyDeposit(G4double edep) { fEnergyDeposit += edep; }
  void SetP4(const G4LorentzVector& p4) { fP4 = p4; }
  void SetCharge(G4int charge) { fCharge = charge; }

  G4int GetPDGCode() const { return fPDGCode; }
  G4int GetRowID() const { return fRowID; }
  G4int GetColID() const { return fColID; }
  G4int GetLayerID() const { return fLayerID; }
  G4int GetTrackID() const { return fTrackID; }
  G4int GetParentID() const { return fParentID; }
  G4int GetPixelX() const { return fPixelX; }
  G4int GetPixelY() const { return fPixelY; }
  G4int GetPixelZ() const { return fPixelZ; }
  G4LorentzVector GetP4() const { return fP4; }
  G4int GetCharge() const { return fCharge; }
  // G4ThreeVector GetPos() const { return fPos; }
  // G4bool GetIsFromPrimary() const { return fIsFromPrimary; }
  G4double GetEnergyDeposit() const { return fEnergyDeposit; }
  G4bool GetFromMuon() const { return fFromMuon; }
  G4double GetPx() const { return fP4.px(); }
  G4double GetPy() const { return fP4.py(); }
  G4double GetPz() const { return fP4.pz(); }
  G4double GetEnergy() const { return fP4.e(); }

private:
  G4int fTrackID = -1;
  G4int fParentID = -1;
  G4int fPDGCode = 0;
  G4LorentzVector fP4 = G4LorentzVector();
  G4int fRowID = -1;
  G4int fColID = -1;
  G4int fLayerID = -1;
  G4int fPixelX = -1;
  G4int fPixelY = -1;
  G4int fPixelZ = -1;
  G4int fCharge = 0;

  // G4ThreeVector fPos;
  // G4bool fIsFromPrimary = false;
  G4double fEnergyDeposit = 0.0;
  G4bool fFromMuon = false;
};


using PixelHitsCollection = G4THitsCollection<PixelHit>;

extern G4ThreadLocal G4Allocator<PixelHit>* PixelHitAllocator;


inline void* PixelHit::operator new(size_t)
{
  if (!PixelHitAllocator) PixelHitAllocator = new G4Allocator<PixelHit>;
  return (void*)PixelHitAllocator->MallocSingle();
}


inline void PixelHit::operator delete(void* hit)
{
  PixelHitAllocator->FreeSingle((PixelHit*)hit);
}


#endif
