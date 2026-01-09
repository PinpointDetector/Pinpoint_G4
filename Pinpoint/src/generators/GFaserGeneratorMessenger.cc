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

  fInputFileCmd = new G4UIcmdWithAString("/gen/gfaser/inputFile", this);
  fInputFileCmd->SetGuidance("set input filename of the gfaser generator");
  fInputFileCmd->AvailableForStates(G4State_PreInit, G4State_Init, G4State_Idle);

  // fFirstEventCmd = new G4UIcmdWithAnInteger("/gen/gfaser/firstEvent", this);
  // fFirstEventCmd->SetGuidance("set the index of the first event in the *.gfaser.root file");
  // fFirstEventCmd->SetDefaultValue((G4int)0);
  // fFirstEventCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fUseFixedZPositionCmd = new G4UIcmdWithABool("/gen/gfaser/useFixedZPosition", this);
  fUseFixedZPositionCmd->SetGuidance("set whether to use a fixed Z position for the neutrino vertex");
  fUseFixedZPositionCmd->SetDefaultValue((G4bool)1);
  fUseFixedZPositionCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}


GFaserGeneratorMessenger::~GFaserGeneratorMessenger()
{
  delete fInputFileCmd;
  // delete fFirstEventCmd;
  delete fGFaserGeneratorDir;
}


void GFaserGeneratorMessenger::SetNewValue(G4UIcommand* command, G4String newValues)
{
  if (command == fInputFileCmd) fGFaserAction->SetInputFileName(newValues);
  // else if (command == fFirstEventCmd)
  //   fGFaserAction->SetFirstEvent(fFirstEventCmd->GetNewIntValue(newValues));
  else if (command == fUseFixedZPositionCmd)
    fGFaserAction->SetUseFixedZPosition(fUseFixedZPositionCmd->GetNewBoolValue(newValues));
}
