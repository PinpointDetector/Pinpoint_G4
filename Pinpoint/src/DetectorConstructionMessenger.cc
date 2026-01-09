#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcommand.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWith3VectorAndUnit.hh"
#include "G4ThreeVector.hh"
#include "G4GeometryManager.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"
#include "G4RunManager.hh"

#include "DetectorConstructionMessenger.hh"
#include "DetectorConstruction.hh"
#include "DetectorConstruction.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstructionMessenger::DetectorConstructionMessenger(DetectorConstruction* detector) 
  : det(detector) {
    detDir = new G4UIdirectory("/det/");
    detDir->SetGuidance("detector control");
 
    // GENERAL OPTIONS  
    // detGdmlCmd = new G4UIcmdWithABool("/det/saveGdml", this);
    // detGdmlCmd->SetParameterName("saveGdml", true);
    // detGdmlCmd->SetDefaultValue(false);
    
    tungstenThicknessCmd = new G4UIcmdWithADoubleAndUnit("/det/setTungstenThickness", this);
    tungstenThicknessCmd->SetUnitCategory("Length");
    tungstenThicknessCmd->SetDefaultUnit("mm");
    tungstenThicknessCmd->SetParameterName("TungstenThickness", false);
    tungstenThicknessCmd->SetRange("TungstenThickness>0.");

    siliconThicknessCmd = new G4UIcmdWithADoubleAndUnit("/det/setSiliconThickness", this);
    siliconThicknessCmd->SetUnitCategory("Length");
    siliconThicknessCmd->SetDefaultUnit("um");
    siliconThicknessCmd->SetParameterName("SiliconThickness", false);
    siliconThicknessCmd->SetRange("SiliconThickness>0.");

    boxThicknessCmd = new G4UIcmdWithADoubleAndUnit("/det/setBoxThickness", this);
    boxThicknessCmd->SetUnitCategory("Length");
    boxThicknessCmd->SetDefaultUnit("mm");
    boxThicknessCmd->SetParameterName("BoxThickness", false);
    boxThicknessCmd->SetRange("BoxThickness>0.");

    nLayersCmd = new G4UIcmdWithAnInteger("/det/setNLayers", this);
    nLayersCmd->SetParameterName("NLayers", false);
    nLayersCmd->SetRange("NLayers>0");
    nLayersCmd->SetDefaultValue(100);

    pixelHeightCmd = new G4UIcmdWithADoubleAndUnit("/det/setPixelHeight", this);
    pixelHeightCmd->SetParameterName("PixelHeight", false);
    pixelHeightCmd->SetDefaultUnit("um");
    pixelHeightCmd->SetRange("PixelHeight>0.");
    pixelHeightCmd->SetDefaultValue(22.8);

    pixelWidthCmd = new G4UIcmdWithADoubleAndUnit("/det/setPixelWidth", this);
    pixelWidthCmd->SetParameterName("PixelWidth", false);
    pixelWidthCmd->SetDefaultUnit("um");
    pixelWidthCmd->SetRange("PixelWidth>0.");
    pixelWidthCmd->SetDefaultValue(20.8);

    detectorWidthCmd = new G4UIcmdWithADoubleAndUnit("/det/setDetectorWidth", this);
    detectorWidthCmd->SetParameterName("DetectorWidth", false);
    detectorWidthCmd->SetDefaultUnit("cm");
    detectorWidthCmd->SetRange("DetectorWidth>0.");
    detectorWidthCmd->SetDefaultValue(26.6);

    detectorHeightCmd = new G4UIcmdWithADoubleAndUnit("/det/setDetectorHeight", this);
    detectorHeightCmd->SetParameterName("DetectorHeight", false);
    detectorHeightCmd->SetDefaultUnit("cm");
    detectorHeightCmd->SetRange("DetectorHeight>0.");
    detectorHeightCmd->SetDefaultValue(19.6);

    detGdmlCmd = new G4UIcmdWithAString("/det/setGDMLFile", this);
    detGdmlCmd->SetParameterName("GDMLFile", false);
    detGdmlCmd->SetDefaultValue("pinpoint.gdml");

    // --- sim_flag : -1 = no scint, 0 = single layer, 1 = double layer ---
    simFlagCmd = new G4UIcmdWithAnInteger("/det/setSimFlag", this);
    simFlagCmd->SetGuidance("Set simulation scintillator configuration.");
    simFlagCmd->SetGuidance(" -1: no scintillator");
    simFlagCmd->SetGuidance("  0: one scintillator layer");
    simFlagCmd->SetGuidance("  1: two scintillator layers");
    simFlagCmd->SetParameterName("SimFlag", false);
    simFlagCmd->SetRange("SimFlag>=-1 && SimFlag<=1");
    simFlagCmd->SetDefaultValue(0);

    // --- scint_bar_flag : true = bar geometry, false = block geometry ---
    scintBarFlagCmd = new G4UIcmdWithABool("/det/setScintBarFlag", this);
    scintBarFlagCmd->SetGuidance("Use scintillator bar geometry (true) or solid block (false).");
    scintBarFlagCmd->SetParameterName("ScintBarFlag", false);
    scintBarFlagCmd->SetDefaultValue(false);

    // magnetFieldCmd = new G4UIcmdWithADoubleAndUnit("/det/magnetField", this);
    // magnetFieldCmd->SetUnitCategory("Magnetic flux density");
    // magnetFieldCmd->SetDefaultUnit("tesla");
    // magnetFieldCmd->SetUnitCandidates("tesla kG G");
    // magnetFieldCmd->SetDefaultValue(1.0);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstructionMessenger::~DetectorConstructionMessenger() {
//   delete detGdmlCmd;
  // delete magnetFieldCmd;
  delete detDir;
  delete tungstenThicknessCmd;
  delete siliconThicknessCmd;
  delete boxThicknessCmd;
  delete nLayersCmd;
  delete pixelHeightCmd;
  delete pixelWidthCmd;
  delete detectorWidthCmd;
  delete detectorHeightCmd;
  delete detGdmlCmd;
  delete simFlagCmd;
  delete scintBarFlagCmd;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstructionMessenger::SetNewValue(G4UIcommand* command, G4String newValues) {
  
  if (command == tungstenThicknessCmd) {
    G4double thickness = tungstenThicknessCmd->ConvertToDimensionedDouble(newValues);
    det->SetTungstenThickness(thickness);
  }
  if (command == siliconThicknessCmd) {
    G4double thickness = siliconThicknessCmd->ConvertToDimensionedDouble(newValues);
    det->SetSiliconThickness(thickness);
  }
  if (command == boxThicknessCmd) {
    G4double thickness = boxThicknessCmd->ConvertToDimensionedDouble(newValues);
    det->SetBoxThickness(thickness);
  }
  if (command == nLayersCmd) {
    G4int nLayers = nLayersCmd->GetNewIntValue(newValues);
    det->SetNLayers(nLayers);
  }
  if (command == pixelHeightCmd) {
    G4double height = pixelHeightCmd->GetNewDoubleValue(newValues);
    det->SetPixelHeight(height);
  }
  if (command == pixelWidthCmd) {
    G4double width = pixelWidthCmd->GetNewDoubleValue(newValues);
    det->SetPixelWidth(width);
  }
  if (command == detectorWidthCmd) {
    G4double width = detectorWidthCmd->GetNewDoubleValue(newValues);
    det->SetDetectorWidth(width);
  }
  if (command == detectorHeightCmd) {
    G4double height = detectorHeightCmd->GetNewDoubleValue(newValues);
    det->SetDetectorHeight(height);
  }
  if (command == detGdmlCmd) {
    // G4String filename = detGdmlCmd->GetNewStringValue(newValues);
    det->SetGDMLFile(newValues);
  }
  if (command == simFlagCmd) {
    det->SetSimFlag(simFlagCmd->GetNewIntValue(newValues));
  }
  if (command == scintBarFlagCmd) {
      det->SetScintBarFlag(scintBarFlagCmd->GetNewBoolValue(newValues));
  }


//   if (command == detGdmlCmd) det->SaveGDML(detGdmlCmd->GetNewBoolValue(newValues));
    // if (command == magnetFieldCmd) { 
    //   G4cout << "Changing magnetic field!!!" << G4endl;
    //   GeometricalParameters->SetSpectrometerMagnetField(magnetFieldCmd->ConvertToDimensionedDouble(newValues)); 
    //   G4RunManager::GetRunManager()->ReinitializeGeometry();
    //   }; 
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......