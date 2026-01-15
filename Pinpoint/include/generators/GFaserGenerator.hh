#ifndef GFaserGenerator_hh
#define GFaserGenerator_hh

#include "G4String.hh"
#include "G4Types.hh"
#include "generators/GeneratorBase.hh"

#include <vector>

class G4ParticleGun;
class G4Event;
class G4Box;
class TFile;
class TTree;


class GFaserGenerator : public GeneratorBase
{
  public:
    GFaserGenerator();
    ~GFaserGenerator() override;
    void GeneratePrimaries(G4Event*) override;
    void LoadData() override;

    void SetInputFileName(const G4String& filename) { fInputFileName = filename; }
    void SetFirstEvent(G4long event) { fFirstEvent = event; }
    void SetUseFixedZPosition(G4bool useFixedZPosition) { fUseFixedZPosition = useFixedZPosition; }


  private:
    G4String fInputFileName;
    G4long fFirstEvent{0};
    G4bool fUseFixedZPosition;
    G4int fLayerId = 4;

    TFile *fGfaserFile = nullptr;
    TTree *fGfaserTree = nullptr;
    G4long fCurrentEvent{0};
    G4long fTotalEvents{0};
    
    int fN{0};
    double fVx{0.}, fVy{0.}, fVz{0.};
    int fNuPdg{0}, fLeptonPdg{0}, fHitPdg{0};
    double fPxv{0.}, fPyv{0.}, fPzv{0.}, fEv{0.};
    double fQ2{0}, fW{0}, fX{0}, fY{0};
    int fIntType{0}, fScatType{0};
    bool fIsCc{0}, fIsNc{0};
    int fTgtZ{0}, fTgtA{0}, fTgtPdg{0};
    double xsec{0.};

    std::vector<std::string>* fName = nullptr;
    std::vector<int>* fPdgc = nullptr;
    std::vector<int>* fStatus = nullptr;
    std::vector<double>* fPx = nullptr;
    std::vector<double>* fPy = nullptr;
    std::vector<double>* fPz = nullptr;
    std::vector<double>* fE = nullptr;

    G4bool FindParticleDefinition(G4int pdg, G4ParticleDefinition* &particleDefinition) const;
    G4double GenerateRandomZVertex(G4int layerIndex) const;
    G4String EncodeProcessName() const;
};

#endif // GFaserGenerator_hh