#include "TrackInformation.hh"

G4ThreadLocal G4Allocator<TrackInformation> *
                                   aTrackInformationAllocator = 0;

TrackInformation::TrackInformation()
  : G4VUserTrackInformation()
{
  fFromPrimaryPizero = 0;
  fFromFSLPizero = 0;
  fFromPrimaryLepton = 0;
  fFromPrimaryEMShower = 0;
  fFromCharmedHadron = 0;
  fFromTau = 0;
  fHitIndices = std::make_shared<std::vector<G4long>>();
}

TrackInformation::TrackInformation(const G4Track* aTrack) 
  : G4VUserTrackInformation()
{
  fFromPrimaryPizero = 0;
  fFromFSLPizero = 0;
  fFromPrimaryLepton = 0;
  fFromPrimaryEMShower = 0;
  fFromTau = 0;
  fHitIndices = std::make_shared<std::vector<G4long>>();
}

TrackInformation::~TrackInformation()
{;}

TrackInformation& TrackInformation
::operator =(const TrackInformation& aTrackInfo)
{
  fFromPrimaryPizero = aTrackInfo.fFromPrimaryPizero;
  fFromFSLPizero = aTrackInfo.fFromFSLPizero;
  fFromPrimaryLepton = aTrackInfo.fFromPrimaryLepton;
  fFromPrimaryEMShower = aTrackInfo.fFromPrimaryEMShower;
  fFromTau = aTrackInfo.fFromTau;
  return *this;
}

void TrackInformation::Print() const
{
    G4cout << "Is from primary pizero " << fFromPrimaryPizero << G4endl;
    G4cout << "Is from final state lepton decay pizero " << fFromFSLPizero << G4endl;
    G4cout << "Is from primary lepton (tau or muon)    " << fFromPrimaryLepton << G4endl;
    G4cout << "Is from primary EM shower    " << fFromPrimaryEMShower << G4endl;
    G4cout << "Is from charmed hadron    " << fFromCharmedHadron << G4endl;
    G4cout << "Is from tau    " << fFromTau << G4endl;
}

