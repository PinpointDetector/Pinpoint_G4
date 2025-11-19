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

    void SetGFaserFileName(const G4String& filename) { fGfaserFileName = filename; }
    void SetStartEventId(G4int eventId) { fStartEventId = eventId; }

  private:
    G4String fGfaserFileName;
    G4int fStartEventId;

    TFile *fGfaserFile = nullptr;
    TTree *fGfaserTree = nullptr;
    G4long fCurrentEvent = 0;
    G4long fTotalEvents = 0;
    
    int fN;
    double fVx, fVy, fVz;
    std::vector<std::string>* fName = nullptr;
    std::vector<int>* fPdgc = nullptr;
    std::vector<int>* fStatus = nullptr;
    std::vector<double>* fPx = nullptr;
    std::vector<double>* fPy = nullptr;
    std::vector<double>* fPz = nullptr;
    std::vector<double>* fE = nullptr;

    G4bool FindParticleDefinition(G4int pdg, G4ParticleDefinition* &particleDefinition) const;
};

#endif // GFaserGenerator_hh