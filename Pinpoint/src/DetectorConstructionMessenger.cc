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
    
    fortuneTungstenThicknessCmd = new G4UIcmdWithADoubleAndUnit("/det/setTungstenThickness", this);
    fortuneTungstenThicknessCmd->SetUnitCategory("Length");
    fortuneTungstenThicknessCmd->SetDefaultUnit("mm");
    fortuneTungstenThicknessCmd->SetParameterName("TungstenThickness", false);
    fortuneTungstenThicknessCmd->SetRange("TungstenThickness>0.");

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

    pixelDetectorWidthCmd = new G4UIcmdWithADoubleAndUnit("/det/setDetectorWidth", this);
    pixelDetectorWidthCmd->SetParameterName("DetectorWidth", false);
    pixelDetectorWidthCmd->SetDefaultUnit("cm");
    pixelDetectorWidthCmd->SetRange("DetectorWidth>0.");
    pixelDetectorWidthCmd->SetDefaultValue(26.6);

    pixelDetectorHeightCmd = new G4UIcmdWithADoubleAndUnit("/det/setDetectorHeight", this);
    pixelDetectorHeightCmd->SetParameterName("DetectorHeight", false);
    pixelDetectorHeightCmd->SetDefaultUnit("cm");
    pixelDetectorHeightCmd->SetRange("DetectorHeight>0.");
    pixelDetectorHeightCmd->SetDefaultValue(19.6);

    detGdmlCmd = new G4UIcmdWithAString("/det/setGDMLFile", this);
    detGdmlCmd->SetParameterName("GDMLFile", false);
    detGdmlCmd->SetDefaultValue("pinpoint.gdml");

    // --- numScintPanelsPerLayer : 0=no scint, 1=single panel, 2=double panel ---
    numScintPanelsPerLayerCmd = new G4UIcmdWithAnInteger("/det/setNumScintPanelsPerLayer", this);
    numScintPanelsPerLayerCmd->SetGuidance("Set number of scintillator panels per scint layer.");
    numScintPanelsPerLayerCmd->SetGuidance("  0: no scintillator");
    numScintPanelsPerLayerCmd->SetGuidance("  1: one scintillator panel");
    numScintPanelsPerLayerCmd->SetGuidance("  2: two scintillator panels");
    numScintPanelsPerLayerCmd->SetParameterName("NumScintPanelsPerLayer", false);
    numScintPanelsPerLayerCmd->SetRange("NumScintPanelsPerLayer>=0 && NumScintPanelsPerLayer<=2");
    numScintPanelsPerLayerCmd->SetDefaultValue(1);

    // --- scint_bar_flag : true = bar geometry, false = block geometry ---
    scintBarFlagCmd = new G4UIcmdWithABool("/det/setScintBarFlag", this);
    scintBarFlagCmd->SetGuidance("Use scintillator bar geometry (true) or solid block (false).");
    scintBarFlagCmd->SetParameterName("ScintBarFlag", false);
    scintBarFlagCmd->SetDefaultValue(false);

    // --- scintHeight : height of scintillator panels (may differ from detector height) ---
    scintDetectorHeightCmd = new G4UIcmdWithADoubleAndUnit("/det/setScintHeight", this);
    scintDetectorHeightCmd->SetGuidance("Set the height of the scintillator panels.");
    scintDetectorHeightCmd->SetGuidance("May differ from detector height; panels are bottom-aligned.");
    scintDetectorHeightCmd->SetParameterName("ScintHeight", false);
    scintDetectorHeightCmd->SetDefaultUnit("cm");
    scintDetectorHeightCmd->SetRange("ScintHeight>0.");
    scintDetectorHeightCmd->SetDefaultValue(60.0);

    scintDetectorWidthCmd = new G4UIcmdWithADoubleAndUnit("/det/setScintWidth", this);
    scintDetectorWidthCmd->SetGuidance("Set the width of the scintillator panels.");
    scintDetectorWidthCmd->SetParameterName("ScintWidth", false);
    scintDetectorWidthCmd->SetDefaultUnit("cm");
    scintDetectorWidthCmd->SetRange("ScintWidth>0.");
    scintDetectorWidthCmd->SetDefaultValue(27.0);

    // --- pinpointThickness : thickness of the initial pixel-only section ---
    pinpointThicknessCmd = new G4UIcmdWithADoubleAndUnit("/det/setPinpointThickness", this);
    pinpointThicknessCmd->SetGuidance("Thickness of the initial pinpoint section (alternating pixels + scintillators).");
    pinpointThicknessCmd->SetGuidance("Set to 0 to disable.");
    pinpointThicknessCmd->SetParameterName("PinpointThickness", false);
    pinpointThicknessCmd->SetDefaultUnit("cm");
    pinpointThicknessCmd->SetUnitCategory("Length");
    pinpointThicknessCmd->SetRange("PinpointThickness>=0.");
    pinpointThicknessCmd->SetDefaultValue(10.4);

    pinpointTungstenThicknessCmd = new G4UIcmdWithADoubleAndUnit("/det/setPinpointTungstenThickness", this);
    pinpointTungstenThicknessCmd->SetGuidance("Tungsten thickness for the Pinpoint sub-detector layers.");
    pinpointTungstenThicknessCmd->SetParameterName("PinpointTungstenThickness", false);
    pinpointTungstenThicknessCmd->SetDefaultUnit("mm");
    pinpointTungstenThicknessCmd->SetUnitCategory("Length");
    pinpointTungstenThicknessCmd->SetRange("PinpointTungstenThickness>0.");
    pinpointTungstenThicknessCmd->SetDefaultValue(8.0);

    // --- maxDetectorThickness : maximum total detector thickness ---
    maxDetectorThicknessCmd = new G4UIcmdWithADoubleAndUnit("/det/setMaxDetectorThickness", this);
    maxDetectorThicknessCmd->SetGuidance("Maximum total detector thickness; number of layers is capped to fit within this.");
    maxDetectorThicknessCmd->SetParameterName("MaxDetectorThickness", false);
    maxDetectorThicknessCmd->SetDefaultUnit("cm");
    maxDetectorThicknessCmd->SetUnitCategory("Length");
    maxDetectorThicknessCmd->SetRange("MaxDetectorThickness>0.");
    maxDetectorThicknessCmd->SetDefaultValue(150.);


    scintBarWidthCmd = new G4UIcmdWithADoubleAndUnit("/det/setScintBarWidth", this);
    scintBarWidthCmd->SetGuidance("Width of a single scintillator bar (segmentation in X).");
    scintBarWidthCmd->SetParameterName("ScintBarWidth", false);
    scintBarWidthCmd->SetDefaultUnit("mm");
    scintBarWidthCmd->SetRange("ScintBarWidth>0.");
    scintBarWidthCmd->SetDefaultValue(10.0);

    scintBarHeightCmd = new G4UIcmdWithADoubleAndUnit("/det/setScintBarHeight", this);
    scintBarHeightCmd->SetGuidance("Height of a single scintillator bar (segmentation in Y).");
    scintBarHeightCmd->SetParameterName("ScintBarHeight", false);
    scintBarHeightCmd->SetDefaultUnit("mm");
    scintBarHeightCmd->SetRange("ScintBarHeight>0.");
    scintBarHeightCmd->SetDefaultValue(10.0);

    scintThicknessCmd = new G4UIcmdWithADoubleAndUnit("/det/setScintThickness", this);
    scintThicknessCmd->SetGuidance("Thickness of a single scintillator panel.");
    scintThicknessCmd->SetParameterName("ScintThickness", false);
    scintThicknessCmd->SetDefaultUnit("mm");
    scintThicknessCmd->SetRange("ScintThickness>0.");
    scintThicknessCmd->SetDefaultValue(5.0);

    // --- numScintLayers : scintillator groups per detector layer ---
    numScintLayersCmd = new G4UIcmdWithAnInteger("/det/setNumScintLayers", this);
    numScintLayersCmd->SetGuidance("Number of scintillator groups per detector layer.");
    numScintLayersCmd->SetGuidance("0: no scintillators (T+P only)");
    numScintLayersCmd->SetGuidance("N: N*(T+S) or N*(T+S+S) groups appended to each T+P layer.");
    numScintLayersCmd->SetParameterName("NumScintLayers", false);
    numScintLayersCmd->SetRange("NumScintLayers>=0");
    numScintLayersCmd->SetDefaultValue(0);

    enableFaserSpectrometerCmd = new G4UIcmdWithABool("/det/enableFaserSpectrometer", this);
    enableFaserSpectrometerCmd->SetGuidance("Enable FASER spectrometer magnets, tracking stations and magnetic field (default: true).");
    enableFaserSpectrometerCmd->SetDefaultValue(true);
    enableFaserSpectrometerCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    numIPTLayersCmd = new G4UIcmdWithAnInteger("/det/setNumIPTLayers", this);
    numIPTLayersCmd->SetGuidance("Number of Interface Pixel Tracker (IPT) layers at the end of the detector.");
    numIPTLayersCmd->SetParameterName("NumIPTLayers", false);
    numIPTLayersCmd->SetRange("NumIPTLayers>=0");
    numIPTLayersCmd->SetDefaultValue(3);

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
  delete fortuneTungstenThicknessCmd;
  delete siliconThicknessCmd;
  delete boxThicknessCmd;
  delete pixelHeightCmd;
  delete pixelWidthCmd;
  delete pixelDetectorWidthCmd;
  delete pixelDetectorHeightCmd;
  delete detGdmlCmd;
  delete numScintPanelsPerLayerCmd;
  delete scintBarFlagCmd;
  delete scintDetectorHeightCmd;
  delete scintDetectorWidthCmd;
  delete scintBarWidthCmd;
  delete scintBarHeightCmd;
  delete scintThicknessCmd;
  delete numScintLayersCmd;
  delete maxDetectorThicknessCmd;
  delete pinpointThicknessCmd;
  delete pinpointTungstenThicknessCmd;
  delete enableFaserSpectrometerCmd;
  delete numIPTLayersCmd;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstructionMessenger::SetNewValue(G4UIcommand* command, G4String newValues) {
  
  if (command == fortuneTungstenThicknessCmd) {
    G4double thickness = fortuneTungstenThicknessCmd->ConvertToDimensionedDouble(newValues);
    det->SetFortuneTungstenThickness(thickness);
  }
  if (command == siliconThicknessCmd) {
    G4double thickness = siliconThicknessCmd->ConvertToDimensionedDouble(newValues);
    det->SetSiliconThickness(thickness);
  }
  if (command == boxThicknessCmd) {
    G4double thickness = boxThicknessCmd->ConvertToDimensionedDouble(newValues);
    det->SetBoxThickness(thickness);
  }
  if (command == pixelHeightCmd) {
    G4double height = pixelHeightCmd->GetNewDoubleValue(newValues);
    det->SetPixelHeight(height);
  }
  if (command == pixelWidthCmd) {
    G4double width = pixelWidthCmd->GetNewDoubleValue(newValues);
    det->SetPixelWidth(width);
  }
  if (command == pixelDetectorWidthCmd) {
    G4double width = pixelDetectorWidthCmd->GetNewDoubleValue(newValues);
    det->SetPixelDetectorWidth(width);
  }
  if (command == pixelDetectorHeightCmd) {
    G4double height = pixelDetectorHeightCmd->GetNewDoubleValue(newValues);
    det->SetPixelDetectorHeight(height);
  }
  if (command == detGdmlCmd) {
    // G4String filename = detGdmlCmd->GetNewStringValue(newValues);
    det->SetGDMLFile(newValues);
  }
  if (command == numScintPanelsPerLayerCmd) {
    det->SetNumScintPanelsPerLayer(numScintPanelsPerLayerCmd->GetNewIntValue(newValues));
  }
  if (command == scintBarFlagCmd) {
      det->SetScintBarFlag(scintBarFlagCmd->GetNewBoolValue(newValues));
  }
  if (command == scintDetectorHeightCmd) {
    det->SetScintDetectorHeight(scintDetectorHeightCmd->GetNewDoubleValue(newValues));
  }
  if (command == scintDetectorWidthCmd) {
    det->SetScintDetectorWidth(scintDetectorWidthCmd->GetNewDoubleValue(newValues));
  }
  if (command == scintBarWidthCmd) {
    det->SetScintBarWidth(scintBarWidthCmd->GetNewDoubleValue(newValues));
  }
  if (command == scintBarHeightCmd) {
    det->SetScintBarHeight(scintBarHeightCmd->GetNewDoubleValue(newValues));
  }
  if (command == scintThicknessCmd) {
    det->SetScintThickness(scintThicknessCmd->GetNewDoubleValue(newValues));
  }
  if (command == numScintLayersCmd) {
    det->SetNumScintLayers(numScintLayersCmd->GetNewIntValue(newValues));
  }
  if (command == maxDetectorThicknessCmd) {
    det->SetMaxDetectorThickness(maxDetectorThicknessCmd->GetNewDoubleValue(newValues));
  }
  if (command == pinpointThicknessCmd) {
    det->SetPinpointThickness(pinpointThicknessCmd->GetNewDoubleValue(newValues));
  }
  if (command == pinpointTungstenThicknessCmd) {
    det->SetPinpointTungstenThickness(pinpointTungstenThicknessCmd->GetNewDoubleValue(newValues));
  }
  if (command == enableFaserSpectrometerCmd) {
    det->SetEnableFaserSpectrometer(enableFaserSpectrometerCmd->GetNewBoolValue(newValues));
  }
  if (command == numIPTLayersCmd) {
    det->SetNIPTLayers(numIPTLayersCmd->GetNewIntValue(newValues));
  }


//   if (command == detGdmlCmd) det->SaveGDML(detGdmlCmd->GetNewBoolValue(newValues));
    // if (command == magnetFieldCmd) { 
    //   G4cout << "Changing magnetic field!!!" << G4endl;
    //   GeometricalParameters->SetSpectrometerMagnetField(magnetFieldCmd->ConvertToDimensionedDouble(newValues)); 
    //   G4RunManager::GetRunManager()->ReinitializeGeometry();
    //   }; 
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......