#include "generators/GFaserGeneratorMessenger.hh"
#include "generators/GFaserGenerator.hh"

#include "G4UIdirectory.hh"
#include "G4UIcommand.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithABool.hh"


GFaserGeneratorMessenger::GFaserGeneratorMessenger(GFaserGenerator* action) 
  : fGFaserAction(action) 
{
  fGFaserGeneratorDir = new G4UIdirectory("/gen/gfaser");
  fGFaserGeneratorDir->SetGuidance("gfaser generator control");

  fGFaserFileCmd = new G4UIcmdWithAString("/gen/gfaser/gfaserInput", this);
  fGFaserFileCmd->SetGuidance("set input filename of the gfaser generator");
  fGFaserFileCmd->AvailableForStates(G4State_PreInit, G4State_Init, G4State_Idle);


  fGFaserStartEventIdCmd = new G4UIcmdWithAnInteger("/gen/gfaser/gfaserStartEventId", this);
  fGFaserStartEventIdCmd->SetGuidance("set the index of the start event in the .ghep file");
  fGFaserStartEventIdCmd->SetDefaultValue((G4int)0);
  fGFaserStartEventIdCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

}


GFaserGeneratorMessenger::~GFaserGeneratorMessenger()
{
  delete fGFaserFileCmd;
  delete fGFaserStartEventIdCmd;
  delete fGFaserGeneratorDir;
}


void GFaserGeneratorMessenger::SetNewValue(G4UIcommand* command, G4String newValues)
{
  if (command == fGFaserFileCmd) fGFaserAction->SetGFaserFileName(newValues);
  else if (command == fGFaserStartEventIdCmd) fGFaserAction->SetStartEventId(fGFaserStartEventIdCmd->GetNewIntValue(newValues));
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
