#ifndef GFaserGeneratorMessenger_h
#define GFaserGeneratorMessenger_h

#include "G4UImessenger.hh"
#include "globals.hh"

class GFaserGenerator;
class G4UIdirectory;
class G4UIcmdWithAString;
class G4UIcmdWithAnInteger;
class G4UIcmdWithABool;


class GFaserGeneratorMessenger: public G4UImessenger
{
  public:
    GFaserGeneratorMessenger(GFaserGenerator*);
    ~GFaserGeneratorMessenger();
    void SetNewValue(G4UIcommand*, G4String);
    
  private:
    GFaserGenerator* fGFaserAction;

    G4UIdirectory* fGFaserGeneratorDir;
    G4UIcmdWithAString* fInputFileCmd;
    // G4UIcmdWithAnInteger* fFirstEventCmd;
    G4UIcmdWithABool* fUseFixedZPositionCmd;
};

#endif
