#ifndef DETECTORCONSTRUCTIONMESSENGER_HH
#define DETECTORCONSTRUCTIONMESSENGER_HH

#include "G4UImessenger.hh"
#include "G4UIcommand.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWith3VectorAndUnit.hh"
#include "globals.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class DetectorConstruction;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class DetectorConstructionMessenger: public G4UImessenger {
  public:
    DetectorConstructionMessenger(DetectorConstruction*);
    ~DetectorConstructionMessenger();

    void SetNewValue(G4UIcommand*, G4String);

  private:
    DetectorConstruction* det;
    G4UIdirectory* detDir;

    DetectorConstruction* fDetector; 
    
    G4UIcmdWithADoubleAndUnit* fortuneTungstenThicknessCmd;
    G4UIcmdWithADoubleAndUnit* siliconThicknessCmd;
    G4UIcmdWithADoubleAndUnit* boxThicknessCmd;
    G4UIcmdWithADoubleAndUnit* pixelHeightCmd;
    G4UIcmdWithADoubleAndUnit* pixelWidthCmd;
    G4UIcmdWithADoubleAndUnit* pixelDetectorWidthCmd;
    G4UIcmdWithADoubleAndUnit* pixelDetectorHeightCmd;
    G4UIcmdWithAString* detGdmlCmd;
    G4UIcmdWithAnInteger* numScintPanelsPerLayerCmd;
    G4UIcmdWithABool* scintBarFlagCmd;
    G4UIcmdWithADoubleAndUnit* scintDetectorHeightCmd;
    G4UIcmdWithADoubleAndUnit* scintDetectorWidthCmd;
    G4UIcmdWithADoubleAndUnit* scintBarWidthCmd;
    G4UIcmdWithADoubleAndUnit* scintBarHeightCmd;
    G4UIcmdWithADoubleAndUnit* scintThicknessCmd;
    G4UIcmdWithAnInteger* numScintLayersCmd;
    G4UIcmdWithADoubleAndUnit* maxDetectorThicknessCmd;
    G4UIcmdWithADoubleAndUnit* pinpointThicknessCmd;
    G4UIcmdWithADoubleAndUnit* pinpointTungstenThicknessCmd;
    G4UIcmdWithABool* enableFaserSpectrometerCmd;
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif