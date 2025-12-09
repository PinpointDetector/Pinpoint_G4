#include "G4SystemOfUnits.hh"
#include "G4NistManager.hh"
#include "DetectorConstruction.hh"
#include "PixelSD.hh"
#include "ScintSD.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4PVReplica.hh"
#include "G4PVParameterised.hh"
#include "G4SDManager.hh"
#include <fstream>
#include "G4VisAttributes.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4PhysicalVolumeStore.hh"
#include <algorithm>
#include <iomanip>


DetectorConstruction::DetectorConstruction()
  : G4VUserDetectorConstruction()
{
  messenger = new DetectorConstructionMessenger(this);
}

DetectorConstruction::~DetectorConstruction()
{
  delete messenger;
}

void DetectorConstruction::DefineMaterial()
{
  //Scintillator Material and Properties
  G4NistManager* nist = G4NistManager::Instance();
  scintillator = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
	scintillator->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);  
}

// Function to print layer volume positions
void DetectorConstruction::PrintLayerVolumePositions()
{
  G4cout << "\n" << G4String(80, '=') << G4endl;
  G4cout << "LAYER VOLUME POSITIONS (from Physical Volumes)" << G4endl;
  G4cout << G4String(80, '=') << G4endl;

  // Get the PhysicalVolumeStore
  auto pvStore = G4PhysicalVolumeStore::GetInstance();
  
  G4cout << "\nSearching for volumes in layer structure..." << G4endl;
  
  // Containers to store found volumes
  std::vector<std::pair<G4String, G4ThreeVector>> tungsten_positions;
  std::vector<std::pair<G4String, G4ThreeVector>> silicon_positions;
  std::vector<std::pair<G4String, G4ThreeVector>> scint_positions;
  
  // Iterate through all physical volumes
  for (auto pv : *pvStore) {
    G4String pvName = pv->GetName();
    G4LogicalVolume* lv = pv->GetLogicalVolume();
    G4ThreeVector pos = pv->GetTranslation();
    
    // Check for Tungsten
    if (pvName.find("Tungsten") != std::string::npos) {
      tungsten_positions.push_back({pvName, pos});
      G4cout << "\nTungsten: " << pvName << G4endl;
      G4cout << "  Position (X, Y, Z): (" 
             << pos.x()/mm << ", " 
             << pos.y()/mm << ", " 
             << pos.z()/mm << ") mm" << G4endl;
      if (lv) {
        G4Box* box = dynamic_cast<G4Box*>(lv->GetSolid());
        if (box) {
          G4cout << "  Dimensions (HX, HY, HZ): (" 
                 << 2*box->GetXHalfLength()/mm << ", " 
                 << 2*box->GetYHalfLength()/mm << ", " 
                 << 2*box->GetZHalfLength()/mm << ") mm" << G4endl;
        }
      }
    }
    
    // Check for Silicon
    else if (pvName.find("SiliconLayer") != std::string::npos) {
      silicon_positions.push_back({pvName, pos});
      G4cout << "\nSilicon Layer: " << pvName << G4endl;
      G4cout << "  Position (X, Y, Z): (" 
             << pos.x()/mm << ", " 
             << pos.y()/mm << ", " 
             << pos.z()/mm << ") mm" << G4endl;
      if (lv) {
        G4Box* box = dynamic_cast<G4Box*>(lv->GetSolid());
        if (box) {
          G4cout << "  Dimensions (HX, HY, HZ): (" 
                 << 2*box->GetXHalfLength()/mm << ", " 
                 << 2*box->GetYHalfLength()/mm << ", " 
                 << 2*box->GetZHalfLength()/um << ") um" << G4endl;
        }
      }
    }
    
    // Check for Scintillators
    else if (pvName.find("ScintContainer") != std::string::npos) {
      scint_positions.push_back({pvName, pos});
      G4cout << "\nScintillator Container: " << pvName << G4endl;
      G4cout << "  Position (X, Y, Z): (" 
             << pos.x()/mm << ", " 
             << pos.y()/mm << ", " 
             << pos.z()/mm << ") mm" << G4endl;
      if (lv) {
        G4Box* box = dynamic_cast<G4Box*>(lv->GetSolid());
        if (box) {
          G4cout << "  Dimensions (HX, HY, HZ): (" 
                 << 2*box->GetXHalfLength()/mm << ", " 
                 << 2*box->GetYHalfLength()/mm << ", " 
                 << 2*box->GetZHalfLength()/mm << ") mm" << G4endl;
        }
      }
    }
  }
  
  // Summary
  G4cout << "\n" << G4String(80, '-') << G4endl;
  G4cout << "SUMMARY:" << G4endl;
  G4cout << "Tungsten layers found: " << tungsten_positions.size() << G4endl;
  G4cout << "Silicon layers found: " << silicon_positions.size() << G4endl;
  G4cout << "Scintillator containers found: " << scint_positions.size() << G4endl;
  
  // Z-position ordering
  G4cout << "\n" << G4String(80, '-') << G4endl;
  G4cout << "Z-POSITION ORDERING (layer sequence):" << G4endl;
  
  std::vector<std::tuple<G4double, G4String, G4String>> z_ordered;
  
  for (auto& [name, pos] : tungsten_positions) {
    z_ordered.push_back({pos.z(), "Tungsten", name});
  }
  for (auto& [name, pos] : silicon_positions) {
    z_ordered.push_back({pos.z(), "Silicon", name});
  }
  for (auto& [name, pos] : scint_positions) {
    z_ordered.push_back({pos.z(), "Scintillator", name});
  }
  
  // Sort by z position
  std::sort(z_ordered.begin(), z_ordered.end(),
    [](const auto& a, const auto& b) { return std::get<0>(a) < std::get<0>(b); });
  
  for (auto& [z, type, name] : z_ordered) {
    G4cout << "  Z = " << std::setw(8) << std::fixed << std::setprecision(2) 
           << z/mm << " mm  |  " << std::setw(15) << type << "  |  " << name << G4endl;
  }
  
  G4cout << "\n" << G4String(80, '=') << G4endl << G4endl;
}


G4VPhysicalVolume* DetectorConstruction::Construct()
{
  // Geometry parameters
  // https://iopscience.iop.org/article/10.1088/1748-0221/20/02/C02015
  G4bool fCheckOverlaps = true;

  //cleaning scintillator logical volume container
  scintLVs.clear();
  
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
  fLayerThickness = fTungstenThickness + fBoxThickness + fSiliconThickness;
  if(sim_flag == -1) { fLayerThickness = fTungstenThickness + fBoxThickness + fSiliconThickness;}  //only pixel, TPTPTPTP...
  if(sim_flag == 0)  { fLayerThickness = 2.0*fTungstenThickness + fBoxThickness + fSiliconThickness + fScintThickness;}  //pixel + single scintillator, TPTSTPTS...
  if(sim_flag == 1)  { fLayerThickness = 2.0*fTungstenThickness + fBoxThickness + fSiliconThickness  + 2.0*fScintThickness;}  //pixel + double scintillator, TPTSSTPTSS...
  //auto fLayerThickness = fTungstenThickness + fBoxThickness + fSiliconThickness;
  auto maxLayers = static_cast<int>(100.0*cm / fLayerThickness); // maximum layers allowed
  if(fNLayers > maxLayers) {
      G4cout << "Warning: Reducing number of layers from " << fNLayers 
            << " to " << maxLayers << " to keep detector thickness <= 100 cm." << G4endl;
      fNLayers = maxLayers;
  }
  auto detectorThickness = fNLayers * fLayerThickness;
  auto worldSizeX = 1.2 * fDetectorWidth;
  auto worldSizeY = 1.2 * fDetectorHeight;
  auto worldSizeZ = 1.2 * detectorThickness;

  // Get materials
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
  G4Material* worldMaterial = nist->FindOrBuildMaterial("G4_AIR");
  G4Material* tungstenMaterial = nist->FindOrBuildMaterial("G4_W");
  G4Material* siliconMaterial = nist->FindOrBuildMaterial("G4_Si");
  
  //Calling scintillator, wrapping and related variables
  DefineMaterial();
  // G4int nScintBarsX = static_cast<G4int>(fDetectorWidth / fScintBarWidth);
  // G4int nScintBarsY = static_cast<G4int>(fDetectorHeight / fScintBarHeight);

  G4int nScintBarsX = 27; fScintBarWidth = fDetectorWidth/nScintBarsX;  //~9.85 mm each bar
  G4int nScintBarsY = 20; fScintBarHeight = fDetectorHeight/nScintBarsY; //~9.80 mm each bar

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
  auto layerS = new G4Box("Layer", 0.5 * fDetectorWidth, 0.5 * fDetectorHeight, fLayerThickness / 2);
  auto layerLV = new G4LogicalVolume(layerS, worldMaterial, "Layer");
  fLayerPV = new G4PVReplica("Layer", layerLV, detectorLV, kZAxis, fNLayers, fLayerThickness);

  //Cursor for placements inside each layer
  G4double zCursor = -0.5*fLayerThickness;

  // Tungsten
  auto tungstenS = new G4Box("Tungsten", 0.5 * fDetectorWidth, 0.5 * fDetectorHeight, 0.5 * fTungstenThickness);
  auto tungstenLV = new G4LogicalVolume(tungstenS, tungstenMaterial, "Tungsten");
  zCursor += 0.5*fTungstenThickness;
  //fTarget_phys.push_back(new G4PVPlacement(0, G4ThreeVector(0., 0., -0.5 * fLayerThickness + 0.5 * fTungstenThickness), tungstenLV, "Tungsten", layerLV, false, 0, fCheckOverlaps));
  new G4PVPlacement(0, G4ThreeVector(0., 0., zCursor), tungstenLV, "Tungsten", layerLV, false, 0, fCheckOverlaps);

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

  G4VisAttributes* ScintLayerAtrrib = new G4VisAttributes(G4Colour::Blue());
  ScintLayerAtrrib->SetVisibility(true);
  ScintLayerAtrrib->SetForceSolid(true);

  zCursor += 0.5*fTungstenThickness + fBoxThickness + 0.5*fSiliconThickness;

  auto silicofNLayers = new G4Box("SiliconLayer", 0.5 * fDetectorWidth, 0.5 * fDetectorHeight, 0.5 * fSiliconThickness);
  auto siliconLayerLV = new G4LogicalVolume(silicofNLayers, siliconMaterial, "SiliconLayer");  // Changed to siliconMaterial
  new G4PVPlacement(nullptr, G4ThreeVector(0., 0.,zCursor), siliconLayerLV, "SiliconLayer", layerLV, false, 0, fCheckOverlaps);
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

  zCursor += 0.5*fSiliconThickness;

  //Tungsten again
  zCursor += 0.5*fTungstenThickness;
  new G4PVPlacement(0, G4ThreeVector(0., 0., zCursor), tungstenLV, "Tungsten", layerLV, false, 0, fCheckOverlaps);
  zCursor += 0.5*fTungstenThickness;
  
  //Scintillator : no scint config, flag = -1 | single scint config, flag = 0 | double scint config, flag = 1
  // ---------------------------------------------------------
  // Scintillator : no scint, single, or double
  // ---------------------------------------------------------
  if(sim_flag == -1) {
      G4cout << " No scintillator created in this configuration." << G4endl;
      zCursor = 0.0;
  }

  // At least one scintillator panel
  if(sim_flag == 0 || sim_flag == 1)
  {
      zCursor += 0.5 * fScintThickness;

      // --- Create container for FIRST scintillator layer ---
      auto scintContainerS = new G4Box("ScintContainer1",0.5 * fDetectorWidth,0.5 * fDetectorHeight,0.5 * fScintThickness);
      auto scintContainerLV = new G4LogicalVolume(scintContainerS, scintillator, "ScintContainer1");
      auto scintContainerPV = new G4PVPlacement(0, G4ThreeVector(0.,0.,zCursor),scintContainerLV, "ScintContainer1",layerLV, false, 100, fCheckOverlaps);
      scintContainerLV->SetVisAttributes(ScintLayerAtrrib);

      if(scint_bar_flag == false)
      {
        // Solid block placed inside the container
        auto scintS = new G4Box("ScintillatorBlock1",0.5 * fDetectorWidth,0.5 * fDetectorHeight,0.5 * fScintThickness);
        auto scintLV = new G4LogicalVolume(scintS, scintillator, "ScintillatorBlock1");
        auto scintPV = new G4PVPlacement(0, G4ThreeVector(0.,0.,0.),scintLV, "ScintillatorBlock1",scintContainerLV, false, 0, fCheckOverlaps);
        scintLVs.push_back(scintLV);
        //new G4LogicalSkinSurface("Wrap1_Block", scintLV, scintWrap);
      }
      else
      {
        // Vertical bars inside the container
        auto scintColS = new G4Box("ScintColumn2",0.5 * fScintBarWidth, 0.5 * fDetectorHeight, 0.5 * fScintThickness);
        auto scintColLV =new G4LogicalVolume(scintColS, scintillator, "ScintColumn2");
        new G4PVReplica("ScintColumn2", scintColLV, scintContainerLV, kXAxis, nScintBarsX, fScintBarWidth);
        scintLVs.push_back(scintColLV);
        //new G4LogicalSkinSurface("Wrap1_BarX", scintColLV, scintWrap);
      }
    zCursor += 0.5 * fScintThickness;
    G4cout << "=== Scintillator Bar Check ===" << G4endl;
    G4cout << "Container width: " << fDetectorWidth/mm << " mm" << G4endl;
    G4cout << "Bar width: " << fScintBarWidth/mm << " mm" << G4endl;
    G4cout << "Number of bars: " << nScintBarsX << G4endl;
    G4cout << "Total bar coverage: " << nScintBarsX * fScintBarWidth/mm << " mm" << G4endl;
    G4cout << "Gap: " << (fDetectorWidth - nScintBarsX * fScintBarWidth)/mm << " mm" << G4endl;

    if (std::abs(nScintBarsX * fScintBarWidth - fDetectorWidth) > 0.01*mm) {
      G4Exception("DetectorConstruction", "Geom001", JustWarning,
                  "Bars do not exactly tile the container!");
    }

  }

// ---------------------------------------------------------
// Second scintillator layer (sim_flag == 1)
// ---------------------------------------------------------
if(sim_flag == 1)
{
    zCursor += 0.5 * fScintThickness;

    // --- Container for second scintillator layer ---
    auto scintContainer2S = new G4Box("ScintContainer2", 0.5 * fDetectorWidth,0.5 * fDetectorHeight,0.5 * fScintThickness);
    auto scintContainer2LV = new G4LogicalVolume(scintContainer2S, scintillator, "ScintContainer2");
    auto scintContainer2PV = new G4PVPlacement(0, G4ThreeVector(0.,0.,zCursor),scintContainer2LV, "ScintContainer2",layerLV, false, 101, fCheckOverlaps);
    scintContainer2LV->SetVisAttributes(ScintLayerAtrrib);

    if(scint_bar_flag == false)
    {
        auto scintS2 = new G4Box("ScintillatorBlock2",0.5 * fDetectorWidth,0.5 * fDetectorHeight,0.5 * fScintThickness);
        auto scintLV2 =new G4LogicalVolume(scintS2, scintillator, "ScintillatorBlock2");
        auto scintPV2 = new G4PVPlacement(0, G4ThreeVector(0.,0.,0.),scintLV2, "ScintillatorBlock2",scintContainer2LV, false, 0, fCheckOverlaps);
        scintLVs.push_back(scintLV2);
        //new G4LogicalSkinSurface("Wrap2_Block", scintLV2, scintWrap);
    }
    else
    {
        // Horizontal bars inside container
        auto scintRowS = new G4Box("ScintRow1",0.5 * fDetectorWidth,0.5 * fScintBarHeight,0.5 * fScintThickness);
        auto scintRowLV = new G4LogicalVolume(scintRowS, scintillator, "ScintRow1");
        auto scintRowPV = new G4PVReplica("ScintRow1", scintRowLV, scintContainer2LV, kYAxis, nScintBarsY, fScintBarHeight);
        scintLVs.push_back(scintRowLV);
        //new G4LogicalSkinSurface("Wrap2_BarY", scintRowLV, scintWrap);
    }

    zCursor = 0.0;
}

  // // Box
  // auto boxS = new G4Box("Box", 0.5 * fDetectorWidth, 0.5 * fDetectorHeight, 0.5 * fBoxThickness);
  // auto boxLV = new G4LogicalVolume(boxS, worldMaterial, "Box");
  // new G4PVPlacement(nullptr, G4ThreeVector(0., 0., -0.5 * fLayerThickness + fTungstenThickness + fSiliconThickness + 0.5 * fBoxThickness), boxLV, "Box", layerLV, false, 0, fCheckOverlaps);

  // G4cout << "Detector consists of " << fNLayers << " layers of: [ " << fTungstenThickness / mm << "mm of " << tungstenMaterial->GetName() << " + " << fSiliconThickness / mm << "mm of "
  //        << siliconMaterial->GetName() <<  " + " << fBoxThickness / mm << "mm of " << worldMaterial->GetName() << " ] " << G4endl;

  // Write GDML file if it doesn't exist
  std::ifstream file(fWriteFile);
  if (!file.good()) {
    fParser.Write(fWriteFile, worldPV);
  }
  file.close();

  PrintLayerVolumePositions();
  
  return worldPV;
}

void DetectorConstruction::ConstructSDandField()
{
    // Create Scintillator SD
    auto scintSD = new ScintillatorSD("ScintillatorDetector", "ScintHitsCollection");
    G4SDManager::GetSDMpointer()->AddNewDetector(scintSD);

    // Assign SD to all scintillator LVs
    if (sim_flag == -1) {
        G4cout << "No scintillator layers to assign SD." << G4endl;
    } else if (sim_flag == 0 || sim_flag == 1) {
        // Block configuration
        if (!scint_bar_flag) {
            for (auto lv : scintLVs) {   // scintLVs is a std::vector<G4LogicalVolume*> storing all created scintillator blocks
                if (lv) lv->SetSensitiveDetector(scintSD);
            }
        } else {
            // Bar replicas: need to assign SD to each bar LV
            for (auto lv : scintLVs) {  
                if (lv) lv->SetSensitiveDetector(scintSD);
            }
        }
    }

    // Pixel SD
    if(fPixelLV) {
        auto pixelSD = new PixelSD("PixelDetector", "PixelHitsCollection");
        G4SDManager::GetSDMpointer()->AddNewDetector(pixelSD);
        fPixelLV->SetSensitiveDetector(pixelSD);
    }
}










//Extra : Scintillator Material Propoerty Removed
/*
std::vector<G4double> refractiveIndexScint = { 1.58, 1.58 };
  std::vector<G4double> absorptionScint = {0.1*cm, 0.1*cm};
  //std::vector<G4double> absorptionScint = {210.0*cm, 210.0*cm};
  std::vector<G4double> energiesSmall = { 1.907*eV, 3.542*eV };

  std::vector<G4double> energyScint = { 
	    1.907 * eV, 1.926 * eV, 1.944 * eV, 1.963 * eV, 1.982 * eV, 2.002 * eV, 2.022 * eV, 2.042 * eV, 
	    2.063 * eV, 2.084 * eV, 2.106 * eV, 2.128 * eV, 2.150 * eV, 2.174 * eV, 2.197 * eV, 2.221 * eV, 
	    2.246 * eV, 2.271 * eV, 2.297 * eV, 2.323 * eV, 2.350 * eV, 2.378 * eV, 2.406 * eV, 2.435 * eV, 
	    2.465 * eV, 2.495 * eV, 2.526 * eV, 2.558 * eV, 2.591 * eV, 2.624 * eV, 2.659 * eV, 2.694 * eV, 
	    2.730 * eV, 2.768 * eV, 2.806 * eV, 2.845 * eV, 2.886 * eV, 2.928 * eV, 2.971 * eV, 3.015 * eV, 
	    3.060 * eV, 3.107 * eV, 3.156 * eV, 3.206 * eV, 3.257 * eV, 3.311 * eV, 3.366 * eV, 3.423 * eV, 
	    3.481 * eV, 3.542 * eV 
	};
	
	std::vector<G4double> emissionIntensityScint = { 
	    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 
	    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
	    0.0, 0.0, 0.0, 0.0, 0.0, 0.1, 0.15, 0.2, 
	    0.25, 0.3, 0.35, 0.4, 0.43, 0.46, 0.6, 0.69, 
	    0.82, 0.9, 1.0, 0.97, 0.9, 0.83, 0.75, 0.41, 
	    0.21, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 
	    0.0, 0.0 
	};

  //MPT for Scintillator
	G4MaterialPropertiesTable *MPT = new G4MaterialPropertiesTable();  
	MPT->AddProperty("RINDEX",energiesSmall, refractiveIndexScint);
	MPT->AddProperty("ABSLENGTH",energiesSmall, absorptionScint);
	MPT->AddProperty("SCINTILLATIONCOMPONENT1", energyScint, emissionIntensityScint);    									
	MPT->AddConstProperty("SCINTILLATIONYIELD", 144.0/MeV);
	MPT->AddConstProperty("RESOLUTIONSCALE", 1.0);  //FWHM divided by Energy, also Fano Factor
	MPT->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.1*ns);   //was previous 2.1, chaged to 0.9 where scintillator time reaches peak
	scintillator->SetMaterialPropertiesTable(MPT);



  Scintillator Wrapping
  scintWrap = new G4OpticalSurface("ScintWrap",glisur, ground, dielectric_metal, 1.0);
  G4double pp[2]           = { 2.0 * eV, 3.47 * eV };
  G4double reflectivity[2] = { 1.0, 1.0 };
  G4double efficiency[2]   = { 0.0, 0.0 };

  G4MaterialPropertiesTable* scintWrapProperty = new G4MaterialPropertiesTable();

  scintWrapProperty->AddProperty("REFLECTIVITY", pp, reflectivity,2);
  scintWrapProperty->AddProperty("EFFICIENCY", pp, efficiency,2);
  scintWrap->SetMaterialPropertiesTable(scintWrapProperty);

  */