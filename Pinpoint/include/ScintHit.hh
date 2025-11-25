#ifndef fasernux_ScintHit_hh
#define fasernux_ScintHit_hh

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "G4Threading.hh"
#include "G4LorentzVector.hh"

class ScintHit : public G4VHit
{
public:
  ScintHit() = default;
  ScintHit(const ScintHit&) = default;
  ~ScintHit() override = default;

  ScintHit& operator=(const ScintHit&) = default;
  G4bool operator==(const ScintHit&) const;

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
  // void SetPos(G4ThreeVector xyz) { fPos = xyz; }
  // void SetIsFromPrimary(G4bool fromPrimary) { fIsFromPrimary = fromPrimary; }
  void SetEnergyDeposit(G4double edep) { fEnergyDeposit = edep; }
  void SetFromMuon(G4bool fromMuon) { fFromMuon = fromMuon; }
  // void AddEnergyDeposit(G4double edep) { fEnergyDeposit += edep; }
  void SetP4(const G4LorentzVector& p4) { fP4 = p4; }
  void SetCharge(G4int charge) { fCharge = charge; }
  void SetFromPrimaryPizero(G4bool fromPrimaryPizero) { fFromPrimaryPizero = fromPrimaryPizero; }
  void SetFromFSLPizero(G4bool fromFSLPizero) { fFromFSLPizero = fromFSLPizero; }
  void SetFromPrimaryLepton(G4bool fromPrimaryLepton) { fFromPrimaryLepton = fromPrimaryLepton; }
  void SetTruthHitPos(G4ThreeVector pos) { fTruthHitPos = pos; }

  G4int GetPDGCode() const { return fPDGCode; }
  G4int GetRowID() const { return fRowID; }
  G4int GetColID() const { return fColID; }
  G4int GetLayerID() const { return fLayerID; }
  G4int GetTrackID() const { return fTrackID; }
  G4int GetParentID() const { return fParentID; }
  G4LorentzVector GetP4() const { return fP4; }
  G4int GetCharge() const { return fCharge; }
  G4ThreeVector GetTruthHitPos() const { return fTruthHitPos; }
  // G4bool GetIsFromPrimary() const { return fIsFromPrimary; }
  G4double GetEnergyDeposit() const { return fEnergyDeposit; }
  G4bool GetFromMuon() const { return fFromMuon; }
  G4double GetPx() const { return fP4.px(); }
  G4double GetPy() const { return fP4.py(); }
  G4double GetPz() const { return fP4.pz(); }
  G4double GetEnergy() const { return fP4.e(); }
  G4bool GetFromPrimaryPizero() const { return fFromPrimaryPizero; }
  G4bool GetFromFSLPizero() const { return fFromFSLPizero; }
  G4bool GetFromPrimaryLepton() const { return fFromPrimaryLepton; }

private:
  G4int fTrackID = -1;
  G4int fParentID = -1;
  G4int fPDGCode = 0;
  G4LorentzVector fP4 = G4LorentzVector();
  G4int fRowID = -1;
  G4int fColID = -1;
  G4int fLayerID = -1;
  G4int fCharge = 0;
  G4bool fFromPrimaryLepton = false;
  G4bool fFromPrimaryPizero = false;
  G4bool fFromFSLPizero = false;
  G4ThreeVector fTruthHitPos;

  G4double fEnergyDeposit = 0.0;
  G4bool fFromMuon = false;
};


using ScintHitsCollection = G4THitsCollection<ScintHit>;

extern G4ThreadLocal G4Allocator<ScintHit>* ScintHitAllocator;


inline void* ScintHit::operator new(size_t)
{
  if (!ScintHitAllocator) ScintHitAllocator = new G4Allocator<ScintHit>;
  return (void*)ScintHitAllocator->MallocSingle();
}


inline void ScintHit::operator delete(void* hit)
{
  ScintHitAllocator->FreeSingle((ScintHit*)hit);
}


#endif
