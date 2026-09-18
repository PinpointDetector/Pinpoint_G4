#include "G4SystemOfUnits.hh"
#include "G4NistManager.hh"
#include "DetectorConstruction.hh"
#include "PixelSD.hh"
#include "ScintSD.hh"
#include "FaserSD.hh"
#include "MagneticField.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Cons.hh"
#include "G4PVReplica.hh"
#include "G4PVParameterised.hh"
#include "G4SDManager.hh"
#include "G4FieldManager.hh"
#include "G4UserLimits.hh"
#include "G4Mag_UsualEqRhs.hh"
#include "G4ClassicalRK4.hh"
#include "G4ChordFinder.hh"
#include <fstream>
#include "G4VisAttributes.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include <algorithm>
#include <cmath>
#include <iomanip>

G4ThreadLocal MagneticField* DetectorConstruction::fMagneticField = nullptr;
G4ThreadLocal G4FieldManager* DetectorConstruction::fFieldMgr = nullptr;

DetectorConstruction::DetectorConstruction()
  : G4VUserDetectorConstruction()
{
  messenger = new DetectorConstructionMessenger(this);
  
  fMagneticField = new MagneticField();
}

DetectorConstruction::~DetectorConstruction()
{
  delete messenger;
}


void ConstructFR4() {
    G4NistManager* nist = G4NistManager::Instance();

    // 1. Fetch required basic elements from the NIST database
    G4Element* elH  = nist->FindOrBuildElement("H");
    G4Element* elC  = nist->FindOrBuildElement("C");
    G4Element* elO  = nist->FindOrBuildElement("O");
    G4Element* elSi = nist->FindOrBuildElement("Si");

    // 2. Define Epoxy Resin (Typical composition: C18 H19 O3)
    G4double densityEpoxy = 1.2 * g/cm3;
    G4Material* Epoxy = new G4Material("EpoxyResin", densityEpoxy, 3);
    Epoxy->AddElement(elC, 18);
    Epoxy->AddElement(elH, 19);
    Epoxy->AddElement(elO, 3);

    // 3. Define Woven Glass (Silicon Dioxide - SiO2)
    G4double densityGlass = 2.2 * g/cm3;
    G4Material* Glass = new G4Material("WovenGlass", densityGlass, 2);
    Glass->AddElement(elSi, 1);
    Glass->AddElement(elO,  2);

    // 4. Combine Epoxy and Glass to form FR-4 (Typical ratio: 60% Glass, 40% Epoxy by weight)
    G4double densityFR4 = 1.85 * g/cm3;
    G4Material* matFR4 = new G4Material("FR4", densityFR4, 2);
    matFR4->AddMaterial(Glass, 0.60);
    matFR4->AddMaterial(Epoxy, 0.40);
}


void DetectorConstruction::DefineMaterial()
{
  //Scintillator Material and Properties
  G4NistManager* nist = G4NistManager::Instance();
  scintillator = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
	scintillator->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);
  ConstructFR4(); 
}


void DetectorConstruction::ConstructWorldLV()
{
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* worldMaterial = nist->FindOrBuildMaterial("G4_AIR");
  G4Box* worldS = new G4Box("World", 0.5 * fWorldSizeX, 0.5 * fWorldSizeY, 0.5 * fWorldSizeZ);
  fWorldLV = new G4LogicalVolume(worldS, worldMaterial, "WorldLV");
  fWorldLV->SetVisAttributes(false);
}

void DetectorConstruction::ConstructTungstenPlateLV()
{
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* tungstenMaterial = nist->FindOrBuildMaterial("G4_W");
  G4Box* tungstenS = new G4Box("TungstenPlate", 0.5 * fTungstenWidth, 0.5 * fTungstenHeight, 0.5 * fTungstenPlateThickness);
  fTungstenPlateLV = new G4LogicalVolume(tungstenS, tungstenMaterial, "TungstenPlateLV");
  fTungstenPlateLV->SetVisAttributes(G4VisAttributes(G4Colour(0.5, 0.5, 0.5))); // Gray color for tungsten
}

void DetectorConstruction::ConstructAlWallLV()
{
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* aluminumMaterial = nist->FindOrBuildMaterial("G4_Al");
  G4Box* alWallS = new G4Box("AlWall", 0.5 * fAlWallWidth, 0.5 * fAlWallHeight, 0.5 * fAlWallThickness);
  fAlWallLV = new G4LogicalVolume(alWallS, aluminumMaterial, "AlWallLV");
  fAlWallLV->SetVisAttributes(G4VisAttributes(G4Colour(0.8, 0.8, 0.8))); // Light gray color for aluminum
}

void DetectorConstruction::ConstructPixelSensorLV()
{
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* siliconMaterial = nist->FindOrBuildMaterial("G4_Si");
  G4Box* pixelSensorS = new G4Box("PixelSensor", 0.5 * fPixelSensorWidth, 0.5 * fPixelSensorHeight, 0.5 * fPixelSensorThickness);
  fPixelSensorLV = new G4LogicalVolume(pixelSensorS, siliconMaterial, "PixelSensorLV");
  fPixelSensorLV->SetVisAttributes(G4VisAttributes(G4Colour(0.0, 0.0, 1.0))); // Blue color for silicon sensor
}

void DetectorConstruction::ConstructAlCoolingPlateLV()
{
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* aluminumMaterial = nist->FindOrBuildMaterial("G4_Al");
  G4Box* alCoolingPlateS = new G4Box("AlCoolingPlate", 0.5 * fPixelSensorWidth, 0.5 * fPixelSensorHeight, 0.5 * fAlCoolingPlateThickness);
  fAlCoolingPlateLV = new G4LogicalVolume(alCoolingPlateS, aluminumMaterial, "AlCoolingPlateLV");
  fAlCoolingPlateLV->SetVisAttributes(G4VisAttributes(G4Colour(0.8, 0.8, 0.8))); // Light gray color for aluminum cooling plate
}

void DetectorConstruction::ConstructScintillatorBarLVs()
{
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* scintillatorMaterial = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");

  G4Box* scintillatorVertical = new G4Box("ScintillatorVert",
      0.5*fScintillatorWidth, 0.5*fScintillatorHeight, 0.5*fScintillatorThickness);
  G4Box* scintillatorHorizontal = new G4Box("ScintillatorHoriz",
      0.5*fScintillatorHeight, 0.5*fScintillatorWidth, 0.5*fScintillatorThickness);

  fScintillatorVertLV = new G4LogicalVolume(scintillatorVertical, scintillatorMaterial, "ScintillatorVertLV");
  fScintillatorVertLV->SetVisAttributes(G4VisAttributes(G4Colour(0.0, 0.0, 1.0)));

  fScintillatorHorizLV = new G4LogicalVolume(scintillatorHorizontal, scintillatorMaterial, "ScintillatorHorizLV");
  fScintillatorHorizLV->SetVisAttributes(G4VisAttributes(G4Colour(0.0, 0.0, 1.0)));
}

void DetectorConstruction::ConstructScintillatorPanelLVs()
{
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* air = nist->FindOrBuildMaterial("G4_AIR");
  G4bool checkOverlaps = true;

  ConstructScintillatorBarLVs();

  G4double pitch = fScintillatorHeight / fNumScintBarsPerPanel; // 42 cm / 40 bars

  // ---- Vertical panel: bars long along Y, arranged side-by-side along X ----
  G4Box* solidVertPanel = new G4Box("VertPanel",
      0.5*fScintillatorHeight, 0.5*fScintillatorHeight, 0.5*fScintillatorThickness);
  fScintillatorVertPanelLV = new G4LogicalVolume(solidVertPanel, air, "VertPanelLV");

  G4Box* solidVertSlot = new G4Box("VertSlot",
      0.5*pitch, 0.5*fScintillatorHeight, 0.5*fScintillatorThickness);
  G4LogicalVolume* logicVertSlot = new G4LogicalVolume(solidVertSlot, air, "VertSlotLV");

  new G4PVReplica("VertSlot", logicVertSlot, fScintillatorVertPanelLV, kXAxis, fNumScintBarsPerPanel, pitch);
  new G4PVPlacement(nullptr, G4ThreeVector(), fScintillatorVertLV, "VertBar",
      logicVertSlot, false, 0, checkOverlaps);

  // ---- Horizontal panel: bars long along X, stacked along Y ----
  G4Box* solidHorizPanel = new G4Box("HorizPanel",
      0.5*fScintillatorHeight, 0.5*fScintillatorHeight, 0.5*fScintillatorThickness);
  fScintillatorHorizPanelLV = new G4LogicalVolume(solidHorizPanel, air, "HorizPanelLV");

  G4Box* solidHorizSlot = new G4Box("HorizSlot",
      0.5*fScintillatorHeight, 0.5*pitch, 0.5*fScintillatorThickness);
  G4LogicalVolume* logicHorizSlot = new G4LogicalVolume(solidHorizSlot, air, "HorizSlotLV");

  new G4PVReplica("HorizSlot", logicHorizSlot, fScintillatorHorizPanelLV, kYAxis, fNumScintBarsPerPanel, pitch);
  new G4PVPlacement(nullptr, G4ThreeVector(), fScintillatorHorizLV, "HorizBar",
      logicHorizSlot, false, 0, checkOverlaps);
}

void DetectorConstruction::ConstructPixelModuleLV()
{
  // Pixel module LV.
  // Consists of Aluminium wall + Scintillator + Tungsten plate + Air Gap + Silicon sensor + Aluminium cooling plate + Tungsten plate + Aluminium wall
  // Make air filled box to hold the module components
  // Tungsten plate and scintillator are NOT centred. They are offset by -85 mm  (fScintDetectorOffsetX) in the x-direction due to trench dimensions. The pixel is centred at (0,0) in the module.
  G4double moduleWidth = fAlWallWidth; // Width of the module is defined by the aluminum wall width
  G4double moduleHeight = fAlWallHeight; // Height of the module is defined by the aluminum wall height
  G4double moduleThickness = fAlWallThickness + fScintillatorThickness + fTungstenPlateThickness + fPixelAirGapThickness + fPixelSensorThickness + fAlCoolingPlateThickness + fTungstenPlateThickness + fAlWallThickness; // Total thickness of the module
  
  fPinpointBlockLength = moduleThickness; // Store the total length of the pixel module for later use
  fPinpointBlockWidth = moduleWidth; // Store the total width of the pixel module for later use
  fPinpointBlockHeight = moduleHeight; // Store the total height of the pixel module for later use

  G4cout << "Pixel module total thickness: " << fPinpointBlockLength/mm << " mm" << G4endl;
  G4cout << "Pixel module width: " << fPinpointBlockWidth/mm << " mm" << G4endl;
  G4cout << "Pixel module height: " << fPinpointBlockHeight/mm << " mm" << G4endl;
  G4cout << "Pixel module components:" << G4endl;
  G4cout << "  - Aluminum wall thickness: " << fAlWallThickness/mm << " mm" << G4endl;
  G4cout << "  - Scintillator thickness: " << fScintillatorThickness/mm << " mm" << G4endl;
  G4cout << "  - Tungsten plate thickness: " << fTungstenPlateThickness/mm << " mm" << G4endl;
  G4cout << "  - Air gap thickness: " << fPixelAirGapThickness/mm << " mm" << G4endl;
  G4cout << "  - Silicon sensor thickness: " << fPixelSensorThickness/mm << " mm" << G4endl;
  G4cout << "  - Aluminum cooling plate thickness: " << fAlCoolingPlateThickness/mm << " mm" << G4endl;


  G4Box* pixelModuleS = new G4Box("PixelModule", 0.5 * moduleWidth, 0.5 * moduleHeight, 0.5 * moduleThickness);
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* airMaterial = nist->FindOrBuildMaterial("G4_AIR");
  G4LogicalVolume* pixelModuleLV = new G4LogicalVolume(pixelModuleS, airMaterial, "PixelModuleLV");
  pixelModuleLV->SetVisAttributes(G4VisAttributes(G4Colour(0.9, 0.9, 0.9, 0.1))); // Light gray with some transparency for the module
  fPixelModuleLV = pixelModuleLV; // Store for later use when placing PinpointBlock copies in Construct() (was previously left uninitialized -> segfault)

  // Ensure that the subcomponents are constructed before placing them
  ConstructAlWallLV();
  ConstructScintillatorBarLVs();
  ConstructTungstenPlateLV();
  ConstructPixelSensorLV();
  fPixelLayerLV = fPixelSensorLV; // Store the sensitive silicon LV for SD attachment and true-position bookkeeping (was previously left uninitialized)
  ConstructAlCoolingPlateLV();
  ConstructScintillatorPanelLVs();
  scintLVs.push_back(fScintillatorVertLV);  // Register the shared scintillator bar LVs for SD attachment in ConstructSDandField()
  scintLVs.push_back(fScintillatorHorizLV); // (was never populated after the switch to shared vert/horiz bar LVs -> no scintillator hits)

  // Place the components inside the pixel module
  G4double zPosition = -0.5 * moduleThickness; // Start from the back of the module
  new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zPosition + 0.5 * fAlWallThickness), fAlWallLV, "AlWall_Back", pixelModuleLV, false, 0, fCheckOverlaps);
  zPosition += fAlWallThickness;
  new G4PVPlacement(nullptr, G4ThreeVector(fScintDetectorOffsetX, 0, zPosition + 0.5 * fScintillatorThickness), fScintillatorVertPanelLV, "Scintillator", pixelModuleLV, false, 0, fCheckOverlaps);
  zPosition += fScintillatorThickness;
  new G4PVPlacement(nullptr, G4ThreeVector(fScintDetectorOffsetX, 0, zPosition + 0.5 * fTungstenPlateThickness), fTungstenPlateLV, "TungstenPlate", pixelModuleLV, false, 0, fCheckOverlaps);
  zPosition += fTungstenPlateThickness + fPixelAirGapThickness;
  new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zPosition + 0.5 * fPixelSensorThickness), fPixelSensorLV, "PixelSensor", pixelModuleLV, false, 0, fCheckOverlaps);
  zPosition += fPixelSensorThickness;
  new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zPosition + 0.5 * fAlCoolingPlateThickness), fAlCoolingPlateLV, "AlCoolingPlate", pixelModuleLV, false, 0, fCheckOverlaps);
  zPosition += fAlCoolingPlateThickness;
  new G4PVPlacement(nullptr, G4ThreeVector(fScintDetectorOffsetX, 0, zPosition + 0.5 * fTungstenPlateThickness), fTungstenPlateLV, "TungstenPlate_Front", pixelModuleLV, false, 0, fCheckOverlaps);
  zPosition += fTungstenPlateThickness;
  new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zPosition + 0.5 * fAlWallThickness), fAlWallLV, "AlWall_Front", pixelModuleLV, false, 0, fCheckOverlaps);
}

void DetectorConstruction::ConstructScintModuleLV()
{
  // This is a module of fNumScintLayersPerModule pairs of horizontal and vertical scintillator bars. Each pair is seperated by a tungsten plate. An air gap is placed between each pair of bars. The entire module is wrapped in an aluminum wall.
  // The tungsten and scintillator bars are NOT centred. They are offset by -85 mm  (fScintDetectorOffsetX) in the x-direction due to trench dimensions. The pixel is centred at (0,0) in the module.
  // Because the scintillator panels and tungsten plates are offset in X, the module must be wide enough to fully contain
  // them -- otherwise they protrude through the aluminum wall. Widen (never shrink) the module cross-section to guarantee
  // this, the same way the pixel module's aluminum wall (60 cm) already comfortably contains its own offset components.
  const G4double transverseMargin = 1.0 * cm; // extra clearance beyond the minimum required, matching the margin already present for the pixel module

  G4double scintHalfWidth = 0.5 * fScintillatorHeight; // scintillator panels are built square, using fScintillatorHeight for both transverse dimensions
  G4double tungstenHalfWidth = 0.5 * fTungstenWidth;
  G4double tungstenHalfHeight = 0.5 * fTungstenHeight;

  G4double requiredHalfWidth = std::max(scintHalfWidth, tungstenHalfWidth) + std::fabs(fScintDetectorOffsetX);
  G4double requiredHalfHeight = std::max(scintHalfWidth, tungstenHalfHeight) + std::fabs(fScintDetectorOffsetY);

  G4double moduleWidth = std::max(fAlWallWidth, 2.0 * requiredHalfWidth + transverseMargin); // Width of the module is defined by the aluminum wall width, widened if needed to contain the offset panels
  G4double moduleHeight = std::max(fAlWallHeight, 2.0 * requiredHalfHeight + transverseMargin); // Height of the module is defined by the aluminum wall height, widened if needed to contain the offset panels
  G4double moduleThickness = fNumScintLayersPerModule * (fScintThickness + fScintAirGapThickness + fScintThickness + fTungstenPlateThickness) + 2 * fAlWallThickness;

  fFortuneModuleLength = moduleThickness; // Store the total length of the scintillator module for later use
  fFortuneModuleWidth = moduleWidth; // Store the total width of the scintillator module for later use
  fFortuneModuleHeight = moduleHeight; // Store the total height of the scintillator module for later use
  G4cout << "Scintillator module total thickness: " << fFortuneModuleLength/mm << " mm" << G4endl;
  G4cout << "Scintillator module width: " << fFortuneModuleWidth/mm << " mm" << G4endl;
  G4cout << "Scintillator module height: " << fFortuneModuleHeight/mm << " mm" << G4endl;
  G4cout << "Scintillator module components:" << G4endl;
  G4cout << "  - Aluminum wall thickness: " << fAlWallThickness/mm << " mm" << G4endl;
  G4cout << "  - Scintillator thickness: " << fScintThickness/mm << " mm" << G4endl;
  G4cout << "  - Tungsten plate thickness: " << fTungstenPlateThickness/mm << " mm" << G4endl;
  G4cout << "  - Air gap thickness: " << fScintAirGapThickness/mm << " mm" << G4endl;
  G4cout << "  - Number of scintillator layers: " << fNumScintLayersPerModule << G4endl;


  G4Box* scintModuleS = new G4Box("ScintModule", 0.5 * moduleWidth, 0.5 * moduleHeight, 0.5 * moduleThickness);
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* airMaterial = nist->FindOrBuildMaterial("G4_AIR");
  scintModuleLV = new G4LogicalVolume(scintModuleS, airMaterial, "ScintModuleLV");
  scintModuleLV->SetVisAttributes(G4VisAttributes(G4Colour(0.9, 0.9, 0.9, 0.1))); // Light gray with some transparency for the module

  // Build a dedicated aluminum wall for this module, sized to its own (possibly widened) cross-section.
  // The pixel module's fAlWallLV (60 x 55 cm) must NOT be reused here: it does not match this module's
  // cross-section and was the cause of the AlWall_Back/AlWall_Front overlaps with ScintModuleLV.
  G4Material* aluminumMaterial = nist->FindOrBuildMaterial("G4_Al");
  G4Box* aluminumWallS = new G4Box("AluminumWall", 0.5 * moduleWidth, 0.5 * moduleHeight, 0.5 * fAlWallThickness);
  G4LogicalVolume* aluminumWallLV = new G4LogicalVolume(aluminumWallS, aluminumMaterial, "AluminumWallLV");
  aluminumWallLV->SetVisAttributes(G4VisAttributes(G4Colour(0.8, 0.8, 0.8))); // Light gray color for aluminum

  // Place the components inside the scintillator module
  G4double zPosition = -0.5 * moduleThickness; // Start from the back of the module
  new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zPosition + 0.5 * fAlWallThickness), aluminumWallLV, "AlWall_Back", scintModuleLV, false, 0, fCheckOverlaps);
  zPosition += fAlWallThickness;
  for (G4int i = 0; i < fNumScintLayersPerModule; ++i) {
    new G4PVPlacement(nullptr, G4ThreeVector(fScintDetectorOffsetX, 0, zPosition + 0.5 * fScintThickness), fScintillatorVertPanelLV, "Scintillator_Vertical", scintModuleLV, false, i, fCheckOverlaps);
    zPosition += fScintThickness;
    new G4PVPlacement(nullptr, G4ThreeVector(fScintDetectorOffsetX, 0, zPosition + 0.5 * fTungstenPlateThickness), fTungstenPlateLV, "TungstenPlate", scintModuleLV, false, i, fCheckOverlaps);
    zPosition += fTungstenPlateThickness + fScintAirGapThickness;
    new G4PVPlacement(nullptr, G4ThreeVector(fScintDetectorOffsetX, 0, zPosition + 0.5 * fScintThickness), fScintillatorHorizPanelLV, "Scintillator_Horizontal", scintModuleLV, false, i, fCheckOverlaps);
    zPosition += fScintThickness;
  }
  new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zPosition + 0.5 * fAlWallThickness), aluminumWallLV, "AlWall_Front", scintModuleLV, false, 0, fCheckOverlaps);
}

void DetectorConstruction::ConstructIPTPixelBlockLV()
{
    // Construct the Interface Pixel Tracker (IPT) pixel block logical volume
    // Note: this is currently a placeholder and can be modified to include more complex structures if needed in the future.
    G4double blockWidth = fPinpointBlockWidth; // Width of the IPT pixel block
    G4double blockHeight = fPinpointBlockHeight; // Height of the IPT pixel block
    G4double blockThickness = fPinpointBlockLength; // Thickness of the IPT pixel block

    fIPTPixelBlockThickness = blockThickness; // Store the thickness of the IPT pixel block for later use

    G4Box* iptPixelBlockS = new G4Box("IPTPixelBlock", 0.5 * blockWidth, 0.5 * blockHeight, 0.5 * blockThickness);
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* airMaterial = nist->FindOrBuildMaterial("G4_AIR");
    fIPTPixelBlockLV = new G4LogicalVolume(iptPixelBlockS, airMaterial, "IPTPixelBlockLV");
    fIPTPixelBlockLV->SetVisAttributes(G4VisAttributes(G4Colour(0.9, 0.9, 0.9, 0.1))); // Light gray with some transparency for the IPT pixel block

    // Place the pixel module inside the IPT pixel block
    G4double zPosition = -0.5 * blockThickness; // Start from the back of the block
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zPosition + 0.5 * fPinpointBlockLength), fPixelSensorLV, "IPTPixelModule", fIPTPixelBlockLV, false, 0, fCheckOverlaps);
}


G4VPhysicalVolume* DetectorConstruction::Construct()
{
  G4bool fCheckOverlaps = true;
  //cleaning scintillator logical volume container
  scintLVs.clear();

  // MOSAIX silcion pixel detectors for ALICE ITS3
  // https://iopscience.iop.org/article/10.1088/1748-0221/20/02/C02015
  fNPixelsX = static_cast<G4int>(fPixelSensorWidth / fPixelWidth);
  fNPixelsY = static_cast<G4int>(fPixelSensorHeight / fPixelHeight);

  // Construct the detector modules (pixel and scintillator)
  ConstructPixelModuleLV();
  ConstructScintModuleLV();
  ConstructIPTPixelBlockLV();

  // Construct the world volume - bounds are deliberately larger than the detector to avoid overlaps. TODO: calculate the world size based on the detector size and add some margin.
  fWorldSizeX = 10 * m;
  fWorldSizeY = 10 * m;
  fWorldSizeZ = 20 * m;
  ConstructWorldLV();
  G4VPhysicalVolume* worldPV = new G4PVPlacement(nullptr, G4ThreeVector(), fWorldLV, "World", nullptr, false, 0, fCheckOverlaps);


  // Create a detector LV to hold all the modules
  G4double detectorLength = (nPinpointLayers * fPinpointBlockLength) + (nFortuneLayers * fFortuneModuleLength) + (nIntermidiatePixelLayers * fPinpointBlockLength);
  G4double detectorWidth = std::max(fPinpointBlockWidth, fFortuneModuleWidth);
  G4double detectorHeight = std::max(fPinpointBlockHeight, fFortuneModuleHeight);
  G4Box* detectorS = new G4Box("Detector", 0.5 * detectorWidth, 0.5 * detectorHeight, 0.5 * detectorLength);
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* worldMaterial = nist->FindOrBuildMaterial("G4_AIR");
  G4LogicalVolume* detectorLV = new G4LogicalVolume(detectorS, worldMaterial, "DetectorLV");
  detectorLV->SetVisAttributes(false);

  // Place detector modules inside the detector LV
  G4double zPosition = -0.5 * detectorLength; // Start from the back of the detector
  for (G4int i = 0; i < nPinpointLayers; ++i) {
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zPosition + 0.5 * fPinpointBlockLength),
                      fPixelModuleLV, "PinpointBlock", detectorLV, false, i, fCheckOverlaps);
    zPosition += fPinpointBlockLength;
  }
  for (G4int i = 0; i < nFortuneLayers; ++i) {
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zPosition + 0.5 * fFortuneModuleLength),
                      scintModuleLV, "FortuneBlock", detectorLV, false, i, fCheckOverlaps);
    zPosition += fFortuneModuleLength;
    
    if (i < nIntermidiatePixelLayers) {
      new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zPosition + 0.5 * fPinpointBlockLength),
                        fPixelModuleLV, "PinpointBlock", detectorLV, false, nPinpointLayers + i, fCheckOverlaps);
      zPosition += fPinpointBlockLength;
    }
  }

  // Place the detector in the world volume
  auto DetectorPV = new G4PVPlacement(0, G4ThreeVector(0, 0, fVetoNuPosition + 0.5 * detectorLength), detectorLV, "Detector", fWorldLV, false, 0, fCheckOverlaps);

  // Interface Pixel Tracker, keep separate from the main detector to allow for different placement and alignment
  zPosition = fVetoNuPosition + detectorLength + 0.5 * fIPTPixelBlockThickness;
  for (G4int i = 0; i < fNIPTLayers; ++i) {
    new G4PVPlacement(0, G4ThreeVector(0, 0, zPosition),
                      fIPTPixelBlockLV, "IPTPixelLayer", fWorldLV, false, i, fCheckOverlaps);
    zPosition += fIPTPixelBlockThickness;
  }
  zPosition += fIPTPixelBlockThickness; // Add extra spacing after the last IPT layer

  if (fEnableFaserSpectrometer) {
    // FASER spectrometer magnets:
    // solid cylinders (0 to outerRadius) and air-filled bore (0 to innerRadius)
    // G4Material* sm2co17 = G4Material::GetMaterial("Sm2Co17");

    // auto longMagnetS  = new G4Tubs("Magnet0",   fInnerRadius, fOuterRadius, 0.5 * fLongMagnetLength,  0., 2*M_PI);
    // auto shortMagnetS = new G4Tubs("Magnet12",  fInnerRadius, fOuterRadius, 0.5 * fShortMagnetLength, 0., 2*M_PI);

    auto longFieldS  = new G4Tubs("FieldRegion0",  0., fInnerRadius, 0.5 * fLongMagnetLength,  0., 2*M_PI);
    auto shortFieldS = new G4Tubs("FieldRegion12", 0., fInnerRadius, 0.5 * fShortMagnetLength, 0., 2*M_PI);

    // Magnet shells placed directly in world (hollow, r = fInnerRadius..fOuterRadius)
    // auto magnet0LV = new G4LogicalVolume(longMagnetS, sm2co17, "Magnet0");
    // new G4PVPlacement(nullptr, G4ThreeVector(0., 0., fMagnet0Position), magnet0LV, "Magnet0", worldLV, false, 0, fCheckOverlaps);
    // magnet0LV->SetVisAttributes(G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 0.5)));

    // auto magnet1LV = new G4LogicalVolume(shortMagnetS, sm2co17, "Magnet1");
    // new G4PVPlacement(nullptr, G4ThreeVector(0., 0., fMagnet1Position), magnet1LV, "Magnet1", worldLV, false, 1, fCheckOverlaps);
    // magnet1LV->SetVisAttributes(G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 0.5)));

    // auto magnet2LV = new G4LogicalVolume(shortMagnetS, sm2co17, "Magnet2");
    // new G4PVPlacement(nullptr, G4ThreeVector(0., 0., fMagnet2Position), magnet2LV, "Magnet2", worldLV, false, 2, fCheckOverlaps);
    // magnet2LV->SetVisAttributes(G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 0.5)));

    // Bore field regions placed directly in world (r = 0..fInnerRadius), siblings of the shells
    // Shift in Y so the bottom edge (-fInnerRadius from centre) aligns with the pixel detector
    // bottom at world Y = -0.5*fPixelSensorHeight.
    G4double fieldYOffset = -0.5 * fPixelSensorHeight + fInnerRadius;
    auto fieldRegion0LV = new G4LogicalVolume(longFieldS, worldMaterial, "FieldRegion0");
    new G4PVPlacement(nullptr, G4ThreeVector(0., fieldYOffset, fMagnet0Position), fieldRegion0LV, "FieldRegion0", fWorldLV, false, 0, fCheckOverlaps);
    fieldRegion0LV->SetVisAttributes(G4VisAttributes::GetInvisible());

    auto fieldRegion1LV = new G4LogicalVolume(shortFieldS, worldMaterial, "FieldRegion1");
    new G4PVPlacement(nullptr, G4ThreeVector(0., fieldYOffset, fMagnet1Position), fieldRegion1LV, "FieldRegion1", fWorldLV, false, 1, fCheckOverlaps);
    fieldRegion1LV->SetVisAttributes(G4VisAttributes::GetInvisible());

    auto fieldRegion2LV = new G4LogicalVolume(shortFieldS, worldMaterial, "FieldRegion2");
    new G4PVPlacement(nullptr, G4ThreeVector(0., fieldYOffset, fMagnet2Position), fieldRegion2LV, "FieldRegion2", fWorldLV, false, 2, fCheckOverlaps);
    fieldRegion2LV->SetVisAttributes(G4VisAttributes::GetInvisible());


    // Set step limits in magnetic field regions for accurate tracking
    auto fieldRegionUserLimits = new G4UserLimits();
    fieldRegionUserLimits->SetMaxAllowedStep(1 * mm);
    // fieldRegionUserLimits->SetUserMinEkine(10.0 * MeV);
    fieldRegion0LV->SetUserLimits(fieldRegionUserLimits);
    fieldRegion1LV->SetUserLimits(fieldRegionUserLimits);
    fieldRegion2LV->SetUserLimits(fieldRegionUserLimits);

    // FASER spectrometer tracking layers (use single layer per station)
    // Y offset: bottom-align tracker with the pixel detector

    G4NistManager* nist = G4NistManager::Instance();
    G4Material* siliconMaterial = nist->FindOrBuildMaterial("G4_Si");

    G4double trackerYOffset = -0.5 * fPixelSensorHeight + 0.5 * fTrackerSize;
    auto trackerS = new G4Box("Tracker", 0.5 * fTrackerSize, 0.5 * fTrackerSize, 0.5 * fPixelSensorThickness);
  
    // Tracker 1
    auto tracker1LV = new G4LogicalVolume(trackerS, siliconMaterial, "Tracker1");
    new G4PVPlacement(nullptr, G4ThreeVector(0., trackerYOffset, fTracker1Position), tracker1LV, "Tracker1", fWorldLV, false, 0, fCheckOverlaps);
    tracker1LV->SetVisAttributes(G4VisAttributes(G4Colour(0.0, 1.0, 0.0, 0.7))); // Green, semi-transparent
  
    // Tracker 2
    auto tracker2LV = new G4LogicalVolume(trackerS, siliconMaterial, "Tracker2");
    new G4PVPlacement(nullptr, G4ThreeVector(0., trackerYOffset, fTracker2Position), tracker2LV, "Tracker2", fWorldLV, false, 1, fCheckOverlaps);
    tracker2LV->SetVisAttributes(G4VisAttributes(G4Colour(0.0, 1.0, 0.0, 0.7))); // Green, semi-transparent
  
    // Tracker 3
    auto tracker3LV = new G4LogicalVolume(trackerS, siliconMaterial, "Tracker3");
    new G4PVPlacement(nullptr, G4ThreeVector(0., trackerYOffset, fTracker3Position), tracker3LV, "Tracker3", fWorldLV, false, 2, fCheckOverlaps);
    tracker3LV->SetVisAttributes(G4VisAttributes(G4Colour(0.0, 1.0, 0.0, 0.7))); // Green, semi-transparent
  }

  // Always overwrite the GDML file
  if (std::ifstream(fWriteFile).good()) {
    std::remove(fWriteFile.c_str());
  }
  fParser.Write(fWriteFile, worldPV);

  return worldPV;
}

void DetectorConstruction::ConstructSDandField()
{
  // Compute lookup tables (silicon/tungsten/scint Z positions, pixel and scint bar centres) first,
  // before constructing any sensitive detector below: SD constructors (e.g. PixelHitAccumulator)
  // read geometry accessors like GetNLayers()/GetSiliconZPositions() that depend on these being
  // filled already, and previously running them afterward left those accessors returning stale
  // (empty/zero) values for anything read at SD-construction time.
  ComputeSiliconZPositions();
  ComputePixelCentersXY();
  ComputeScintCentersXY();

  // Create Scintillator SD
  auto scintSD = new ScintillatorSD("ScintillatorDetector", "ScintHitsCollection", "ScintPixelHitsCollection");
  scintSD->SetLayerIndexing(GetScintPanelIDForPinpointBlock(), GetScintLayerIDForPinpointBlock(),
                            GetScintPanelIDBaseForFortuneBlock(), GetScintLayerIDForFortuneBlock(),
                            fNumScintLayersPerModule);
  G4SDManager::GetSDMpointer()->AddNewDetector(scintSD);

  // Assign SD to all scintillator LVs
  if (scintLVs.empty()) {
    G4cout << "No scintillator layers to assign SD." << G4endl;
  } else {
    for (auto lv : scintLVs) {
      if (lv) lv->SetSensitiveDetector(scintSD);
    }
  }

  // Pixel SD
  if(fPixelLayerLV) {
    G4cout << "Adding pixel SD" << G4endl;
    auto pixelSD = new PixelSD("PixelDetector", "PixelHitsCollection");
    G4SDManager::GetSDMpointer()->AddNewDetector(pixelSD);
    fPixelLayerLV->SetSensitiveDetector(pixelSD);
  }

  if (fEnableFaserSpectrometer) {
    // FASER Tracking spectrometer SD
    G4cout << "Adding tracker SD" << G4endl;
    auto faserSD = new FaserSD("FaserSpectrometer", "FaserHitsCollection");
    G4SDManager::GetSDMpointer()->AddNewDetector(faserSD);
    
    G4LogicalVolume* tracker1LV = G4LogicalVolumeStore::GetInstance()->GetVolume("Tracker1");
    G4LogicalVolume* tracker2LV = G4LogicalVolumeStore::GetInstance()->GetVolume("Tracker2");
    G4LogicalVolume* tracker3LV = G4LogicalVolumeStore::GetInstance()->GetVolume("Tracker3");
    
    if(tracker1LV) tracker1LV->SetSensitiveDetector(faserSD);
    if(tracker2LV) tracker2LV->SetSensitiveDetector(faserSD);
    if(tracker3LV) tracker3LV->SetSensitiveDetector(faserSD);

    // Setup magnetic field manager
    fFieldMgr = new G4FieldManager();
    fFieldMgr->SetDetectorField(fMagneticField);
    fFieldMgr->CreateChordFinder(fMagneticField);

    // Get the air-filled field region logical volumes and assign field manager
    G4LogicalVolume* fieldRegion0LV = G4LogicalVolumeStore::GetInstance()->GetVolume("FieldRegion0");
    G4LogicalVolume* fieldRegion1LV = G4LogicalVolumeStore::GetInstance()->GetVolume("FieldRegion1");
    G4LogicalVolume* fieldRegion2LV = G4LogicalVolumeStore::GetInstance()->GetVolume("FieldRegion2");

    G4bool forceToAllDaughters = true;
    if(fieldRegion0LV) {
        fieldRegion0LV->SetFieldManager(fFieldMgr, forceToAllDaughters);
        G4cout << "Assigned magnetic field to FieldRegion0" << G4endl;
    }
    if(fieldRegion1LV) {
        fieldRegion1LV->SetFieldManager(fFieldMgr, forceToAllDaughters);
        G4cout << "Assigned magnetic field to FieldRegion1" << G4endl;
    }
    if(fieldRegion2LV) {
        fieldRegion2LV->SetFieldManager(fFieldMgr, forceToAllDaughters);
        G4cout << "Assigned magnetic field to FieldRegion2" << G4endl;
    }

    // Also apply field to the magnet shells (r = fInnerRadius..fOuterRadius)
    // G4LogicalVolume* magnet0LV = G4LogicalVolumeStore::GetInstance()->GetVolume("Magnet0");
    // G4LogicalVolume* magnet1LV = G4LogicalVolumeStore::GetInstance()->GetVolume("Magnet1");
    // G4LogicalVolume* magnet2LV = G4LogicalVolumeStore::GetInstance()->GetVolume("Magnet2");
    // if(magnet0LV) { magnet0LV->SetFieldManager(fFieldMgr, forceToAllDaughters); G4cout << "Assigned magnetic field to Magnet0" << G4endl; }
    // if(magnet1LV) { magnet1LV->SetFieldManager(fFieldMgr, forceToAllDaughters); G4cout << "Assigned magnetic field to Magnet1" << G4endl; }
    // if(magnet2LV) { magnet2LV->SetFieldManager(fFieldMgr, forceToAllDaughters); G4cout << "Assigned magnetic field to Magnet2" << G4endl; }

    G4cout << "Configured magnetic field using custom MagneticField class:" << G4endl;
    G4cout << "  Field strength: " << fMagneticField->GetField()/tesla << " T (X-direction)" << G4endl;
    G4cout << "  Applied to full magnet cross-section (r < " << fOuterRadius/mm << " mm)" << G4endl;
  }
}



void DetectorConstruction::ComputeSiliconZPositions()
{
  fSiliconZPositions.clear();
  fTungstenZPositions.clear();
  fTungstenThicknesses.clear();
  fScintZPositions.clear();
  fLayerIsPixel.clear();
  fScintPanelIDForPinpointBlock.clear();
  fScintLayerIDForPinpointBlock.clear();
  fScintPanelIDBaseForFortuneBlock.clear();
  fScintLayerIDForFortuneBlock.clear();

  // Mirror the exact placement sequence built in Construct(): nPinpointLayers Pinpoint
  // blocks, then nFortuneLayers Fortune blocks (each, except the last, followed by one
  // intermediate Pinpoint block), then fNIPTLayers trailing IPT layers.
  // Positions are in WORLD coordinates -- i.e. they include the same
  // fVetoNuPosition + 0.5*detectorLength offset Construct() uses when placing "Detector" --
  // because GFaserGenerator builds primary vertices directly from these vectors.
  const G4double detectorLength = nPinpointLayers * fPinpointBlockLength
                                 + nFortuneLayers * fFortuneModuleLength
                                 + nIntermidiatePixelLayers * fPinpointBlockLength;
  const G4double worldOffset = fVetoNuPosition + 0.5 * detectorLength;

  // Offsets of sub-components from the front face of a Pinpoint (pixel) module,
  // mirroring ConstructPixelModuleLV()'s internal zPosition accumulation.
  const G4double pinScintOffset        = fAlWallThickness + 0.5 * fScintillatorThickness;
  const G4double pinTungstenBackOffset = fAlWallThickness + fScintillatorThickness + 0.5 * fTungstenPlateThickness;
  const G4double pinSiliconOffset      = fAlWallThickness + fScintillatorThickness + fTungstenPlateThickness
                                        + fPixelAirGapThickness + 0.5 * fPixelSensorThickness;
  const G4double pinTungstenFrontOffset = pinSiliconOffset + 0.5 * fPixelSensorThickness
                                         + fAlCoolingPlateThickness + 0.5 * fTungstenPlateThickness;

  G4double cursor = -0.5 * detectorLength; // local z within DetectorLV, same convention as Construct()

  // scintModuleIndex assigns each scintillator-bearing module (initial Pinpoint block, Fortune
  // block, or intermediate Pinpoint block) a single sequential layerID in true physical placement
  // order -- this is the same order ScintSD sees blocks in along the beam, and is what
  // fScintLayerIDForPinpointBlock/fScintLayerIDForFortuneBlock below get filled with.
  G4int scintModuleIndex = 0;

  for (G4int i = 0; i < nPinpointLayers; ++i) {
    const G4double blockFrontZ = worldOffset + cursor;
    // PinpointBlock copy number for this (initial) block is i (see Construct()) -- pushing here,
    // in strictly increasing copy-number order, keeps these lookup vectors indexable directly by
    // copy number, same as fScintZPositions below.
    fScintPanelIDForPinpointBlock.push_back(static_cast<G4int>(fScintZPositions.size()));
    fScintLayerIDForPinpointBlock.push_back(scintModuleIndex++);
    fScintZPositions.push_back(blockFrontZ + pinScintOffset);
    fTungstenZPositions.push_back(blockFrontZ + pinTungstenBackOffset);
    fTungstenThicknesses.push_back(fTungstenPlateThickness);
    fSiliconZPositions.push_back(blockFrontZ + pinSiliconOffset);
    fTungstenZPositions.push_back(blockFrontZ + pinTungstenFrontOffset);
    fTungstenThicknesses.push_back(fTungstenPlateThickness);
    fLayerIsPixel.push_back(true);
    cursor += fPinpointBlockLength;
  }

  for (G4int i = 0; i < nFortuneLayers; ++i) {
    // Fortune (scint) module: mirror ConstructScintModuleLV()'s internal loop.
    // FortuneBlock copy number for this block is i (see Construct()).
    fScintPanelIDBaseForFortuneBlock.push_back(static_cast<G4int>(fScintZPositions.size()));
    fScintLayerIDForFortuneBlock.push_back(scintModuleIndex++);
    const G4double blockFrontZ = worldOffset + cursor;
    G4double local = fAlWallThickness; // past AlWall_Back
    for (G4int j = 0; j < fNumScintLayersPerModule; ++j) {
      fScintZPositions.push_back(blockFrontZ + local + 0.5 * fScintThickness);
      local += fScintThickness;
      fTungstenZPositions.push_back(blockFrontZ + local + 0.5 * fTungstenPlateThickness);
      fTungstenThicknesses.push_back(fTungstenPlateThickness);
      local += fTungstenPlateThickness + fScintAirGapThickness;
      fScintZPositions.push_back(blockFrontZ + local + 0.5 * fScintThickness);
      local += fScintThickness;
    }
    fLayerIsPixel.push_back(false);
    cursor += fFortuneModuleLength;

    if (i < nIntermidiatePixelLayers) {
      const G4double ppBlockFrontZ = worldOffset + cursor;
      // Intermediate PinpointBlock copy number is nPinpointLayers + i (see Construct()), i.e.
      // strictly increasing across successive intermediate blocks -- push_back keeps this vector
      // indexable directly by copy number, same as the initial blocks above.
      fScintPanelIDForPinpointBlock.push_back(static_cast<G4int>(fScintZPositions.size()));
      fScintLayerIDForPinpointBlock.push_back(scintModuleIndex++);
      fScintZPositions.push_back(ppBlockFrontZ + pinScintOffset);
      fTungstenZPositions.push_back(ppBlockFrontZ + pinTungstenBackOffset);
      fTungstenThicknesses.push_back(fTungstenPlateThickness);
      fSiliconZPositions.push_back(ppBlockFrontZ + pinSiliconOffset);
      fTungstenZPositions.push_back(ppBlockFrontZ + pinTungstenFrontOffset);
      fTungstenThicknesses.push_back(fTungstenPlateThickness);
      fLayerIsPixel.push_back(true);
      cursor += fPinpointBlockLength;
    }
  }

  // Trailing IPT layers: placed directly in the world right after the detector volume
  // (see Construct()); each is a single silicon sensor centred in its own
  // fIPTPixelBlockThickness-thick block, so no further sub-offset is needed.
  G4double iptZ = fVetoNuPosition + detectorLength + 0.5 * fIPTPixelBlockThickness;
  for (G4int i = 0; i < fNIPTLayers; ++i) {
    fSiliconZPositions.push_back(iptZ);
    fLayerIsPixel.push_back(true);
    iptZ += fIPTPixelBlockThickness;
  }

  G4cout << "Computed Z positions: "
         << fSiliconZPositions.size()  << " silicon planes, "
         << fTungstenZPositions.size() << " tungsten plates, "
         << fScintZPositions.size()    << " scintillator panels" << G4endl;
}


void DetectorConstruction::ComputePixelCentersXY()
{
  // X centres: symmetric about beam axis (world X = 0)
  fPixelCenterX.clear();
  fPixelCenterX.reserve(fNPixelsX);
  const G4double xMin = -0.5 * fPixelSensorWidth + fPixelDetectorOffsetX;
  for (G4int col = 0; col < fNPixelsX; ++col)
    fPixelCenterX.push_back(xMin + (col + 0.5) * fPixelWidth);

  // Y centres: pixel detector is centred on beam axis (world Y = 0)
  fPixelCenterY.clear();
  fPixelCenterY.reserve(fNPixelsY);
  const G4double yMin = -0.5 * fPixelSensorHeight + fPixelDetectorOffsetY;
  for (G4int row = 0; row < fNPixelsY; ++row)
    fPixelCenterY.push_back(yMin + (row + 0.5) * fPixelHeight);

  G4cout << "Computed pixel centers: "
         << fPixelCenterX.size() << " x " << fPixelCenterY.size() << " centered at ("
         << fPixelCenterX[0] << ", " << fPixelCenterY[0] << ")" << G4endl;
}


void DetectorConstruction::ComputeScintCentersXY()
{
  // The scintillator panels are square (side = fScintillatorHeight) and are always
  // segmented into fNumScintBarsPerPanel bars per axis (see ConstructScintillatorPanelLVs()).
  // fScintDetectorWidth/Height and fScintBarWidth/Height are nominal/metadata values only
  // and are not what's actually built, so we derive the true bar centres from the real
  // panel geometry instead.
  const G4double pitch = fScintillatorHeight / fNumScintBarsPerPanel;

  // Vertical bars are segmented in X (bar runs full panel height)
  fScintBarCenterX.clear();
  fScintBarCenterX.reserve(fNumScintBarsPerPanel);
  const G4double xMin = -0.5 * fScintillatorHeight + fScintDetectorOffsetX;
  for (G4int i = 0; i < fNumScintBarsPerPanel; ++i)
    fScintBarCenterX.push_back(xMin + (i + 0.5) * pitch);

  // Horizontal bars are segmented in Y (bar runs full panel width)
  fScintBarCenterY.clear();
  fScintBarCenterY.reserve(fNumScintBarsPerPanel);
  const G4double yMin = -0.5 * fScintillatorHeight + fScintDetectorOffsetY;
  for (G4int j = 0; j < fNumScintBarsPerPanel; ++j)
    fScintBarCenterY.push_back(yMin + (j + 0.5) * pitch);

  G4cout << "Computed scint bar centers: "
         << fScintBarCenterX.size() << " x-bars, "
         << fScintBarCenterY.size() << " y-bars" << G4endl;
}

