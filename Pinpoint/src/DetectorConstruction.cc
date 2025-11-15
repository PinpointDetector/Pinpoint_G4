#include "G4SystemOfUnits.hh"
#include "G4NistManager.hh"
#include "DetectorConstruction.hh"
#include "PixelSD.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4PVReplica.hh"
#include "G4PVParameterised.hh"
#include "G4SDManager.hh"
#include <fstream>
#include "G4VisAttributes.hh"

DetectorConstruction::DetectorConstruction()
  : G4VUserDetectorConstruction()
{
  messenger = new DetectorConstructionMessenger(this);
}

DetectorConstruction::~DetectorConstruction()
{
  delete messenger;
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  // Geometry parameters
  // https://iopscience.iop.org/article/10.1088/1748-0221/20/02/C02015
  G4double boxThickness = fTungstenThickness - fSiliconThickness;
  G4bool fCheckOverlaps = true;
  
  // G4int nPixelsX = 12788; // 20.8um pixel pitch
  // G4int nPixelsY = 8596;  // 22.8um pixel pitch
  G4int nPixelsX = static_cast<G4int>(fDetectorWidth / fPixelWidth);
  G4int nPixelsY = static_cast<G4int>(fDetectorHeight / fPixelHeight);
  G4cout << "Detector dimensions: " << fDetectorWidth/cm << " cm x " << fDetectorHeight/cm << " cm" << G4endl;
  G4cout << "Number of layers: " << fNLayers << G4endl;
  G4cout << "Tungsten thickness per layer: " << fTungstenThickness/mm << " mm" << G4endl;
  G4cout << "Silicon thickness per layer: " << fSiliconThickness/um << " um" << G4endl;
  G4cout << "Creating " << nPixelsX << " x " << nPixelsY << " pixels per silicon layer" << G4endl;
  G4cout << "Pixel size: " << fPixelWidth/micrometer << " x " << fPixelHeight/micrometer << " μm" << G4endl;
  auto layerThickness = fTungstenThickness + boxThickness + fSiliconThickness;
  auto detectorThickness = fNLayers * layerThickness;
  auto worldSizeX = 1.2 * fDetectorWidth;
  auto worldSizeY = 1.2 * fDetectorHeight;
  auto worldSizeZ = 1.2 * detectorThickness;

  // Get materials
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* worldMaterial = nist->FindOrBuildMaterial("G4_AIR");
  G4Material* tungstenMaterial = nist->FindOrBuildMaterial("G4_W");
  G4Material* siliconMaterial = nist->FindOrBuildMaterial("G4_Si");

  // World
  G4Box* worldS = new G4Box("World", 0.5 * worldSizeX, 0.5 * worldSizeY, worldSizeZ);
  G4LogicalVolume* worldLV = new G4LogicalVolume(worldS, worldMaterial, "World");
  G4VPhysicalVolume* worldPV = new G4PVPlacement(nullptr, G4ThreeVector(), worldLV, "World", nullptr, false, 0, fCheckOverlaps);

  G4VisAttributes* experimentalHallVisAtt = new G4VisAttributes(G4Colour(1.,1.,1.));
  experimentalHallVisAtt->SetForceWireframe(true);
  worldLV->SetVisAttributes(experimentalHallVisAtt);

  // Detector
  auto detectorS = new G4Box("Detector", 0.5 * fDetectorWidth, 0.5 * fDetectorHeight, 0.5 * detectorThickness);
  auto detectorLV = new G4LogicalVolume(detectorS, worldMaterial, "Detector");
  // Position detector so that first layer starts at z=0 (shift by half detector thickness)
  new G4PVPlacement(0, G4ThreeVector(0., 0., 0.5 * detectorThickness), detectorLV, "Detector", worldLV, false, 0, fCheckOverlaps);

  // Layer
  auto layerS = new G4Box("Layer", 0.5 * fDetectorWidth, 0.5 * fDetectorHeight, layerThickness / 2);
  auto layerLV = new G4LogicalVolume(layerS, worldMaterial, "Layer");
  new G4PVReplica("Layer", layerLV, detectorLV, kZAxis, fNLayers, layerThickness);

  // Tungsten
  auto tungstenS = new G4Box("Tungsten", 0.5 * fDetectorWidth, 0.5 * fDetectorHeight, 0.5 * fTungstenThickness);
  auto tungstenLV = new G4LogicalVolume(tungstenS, tungstenMaterial, "Tungsten");
  fTarget_phys.push_back(new G4PVPlacement(0, G4ThreeVector(0., 0., -0.5 * layerThickness + 0.5 * fTungstenThickness), tungstenLV, "Tungsten", layerLV, false, 0, fCheckOverlaps));
  
  G4VisAttributes* TargetVisAtt =  new G4VisAttributes(G4Colour::Red());
  TargetVisAtt->SetForceWireframe(true);
  tungstenLV->SetVisAttributes(TargetVisAtt);


  // Silicon layer (will contain pixels)
  G4VisAttributes* invisAtrrib = new G4VisAttributes();
  invisAtrrib->SetVisibility(false);
  invisAtrrib->SetForceSolid(false);

  G4VisAttributes* LayerAtrrib = new G4VisAttributes(G4Colour::Green());
  LayerAtrrib->SetVisibility(true);
  LayerAtrrib->SetForceSolid(true);

  auto silicofNLayers = new G4Box("SiliconLayer", 0.5 * fDetectorWidth, 0.5 * fDetectorHeight, 0.5 * fSiliconThickness);
  auto siliconLayerLV = new G4LogicalVolume(silicofNLayers, siliconMaterial, "SiliconLayer");  // Changed to siliconMaterial
  new G4PVPlacement(nullptr, G4ThreeVector(0., 0., -0.5 * layerThickness + fTungstenThickness + 0.5 * fSiliconThickness), siliconLayerLV, "SiliconLayer", layerLV, false, 0, fCheckOverlaps);
  siliconLayerLV->SetVisAttributes(LayerAtrrib);

  // Create pixel row (Y direction)
  auto pixelRowS = new G4Box("SiliconPixelRow", 0.5 * fDetectorWidth, 0.5 * fPixelHeight, 0.5 * fSiliconThickness);
  auto pixelRowLV = new G4LogicalVolume(pixelRowS, siliconMaterial, "SiliconPixelRow");  // Changed to siliconMaterial
  new G4PVReplica("SiliconPixelRow", pixelRowLV, siliconLayerLV, kYAxis, nPixelsY, fPixelHeight);
  pixelRowLV->SetVisAttributes(invisAtrrib);

  // Create individual pixels (X direction)
  auto pixelS = new G4Box("SiliconPixel", 0.5 * fPixelWidth, 0.5 * fPixelHeight, 0.5 * fSiliconThickness);
  fPixelLV = new G4LogicalVolume(pixelS, siliconMaterial, "SiliconPixel");
  new G4PVReplica("SiliconPixel", fPixelLV, pixelRowLV, kXAxis, nPixelsX, fPixelWidth);
  fPixelLV->SetVisAttributes(invisAtrrib);

  // // Box
  // auto boxS = new G4Box("Box", 0.5 * fDetectorWidth, 0.5 * fDetectorHeight, 0.5 * boxThickness);
  // auto boxLV = new G4LogicalVolume(boxS, worldMaterial, "Box");
  // new G4PVPlacement(nullptr, G4ThreeVector(0., 0., -0.5 * layerThickness + fTungstenThickness + fSiliconThickness + 0.5 * boxThickness), boxLV, "Box", layerLV, false, 0, fCheckOverlaps);

  // G4cout << "Detector consists of " << fNLayers << " layers of: [ " << fTungstenThickness / mm << "mm of " << tungstenMaterial->GetName() << " + " << fSiliconThickness / mm << "mm of "
  //        << siliconMaterial->GetName() <<  " + " << boxThickness / mm << "mm of " << worldMaterial->GetName() << " ] " << G4endl;

  // // Write GDML file if it doesn't exist
  // std::ifstream file(fWriteFile);
  // if (!file.good()) {
  //   fParser.Write(fWriteFile, worldPV);
  // }
  // file.close();
  
  return worldPV;
}

void DetectorConstruction::ConstructSDandField()
{
  if (fPixelLV) {
    auto pixelSD = new PixelSD("PixelDetector", "PixelHitsCollection");
    G4SDManager::GetSDMpointer()->AddNewDetector(pixelSD);
    fPixelLV->SetSensitiveDetector(pixelSD);
  }
}