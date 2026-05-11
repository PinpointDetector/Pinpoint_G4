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

void DetectorConstruction::DefineMaterial()
{
  //Scintillator Material and Properties
  G4NistManager* nist = G4NistManager::Instance();
  scintillator = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
	scintillator->GetIonisation()->SetBirksConstant(0.126 * mm / MeV);
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  G4bool fCheckOverlaps = true;
  //cleaning scintillator logical volume container
  scintLVs.clear();

  // MOSAIX silcion pixel detectors for ALICE ITS3
  // https://iopscience.iop.org/article/10.1088/1748-0221/20/02/C02015
  fNPixelsX = static_cast<G4int>(fPixelDetectorWidth / fPixelWidth);
  fNPixelsY = static_cast<G4int>(fPixelDetectorHeight / fPixelHeight);

  if (fNumScintLayers == 0) fNumScintPanelsPerLayer = 0;
  fFortunePixelBlockThickness  = fFortuneTungstenThickness + fBoxThickness + fSiliconThickness;
  fPinpointPixelBlockThickness = fPinpointTungstenThickness + fBoxThickness + fSiliconThickness;
  fPinpointScintBlockThickness = fPinpointTungstenThickness + fScintThickness;
  fFortuneScintBlockThickness = (fNumScintPanelsPerLayer > 0)
                                ? fFortuneTungstenThickness + fNumScintPanelsPerLayer * fScintThickness
                                : 0.0 * mm;
  G4double pinpointBlockThickness = fPinpointPixelBlockThickness + fPinpointScintBlockThickness;
  G4double fortuneBlockThickness = fFortunePixelBlockThickness + fNumScintLayers * fFortuneScintBlockThickness;

  // Each pinpoint block is T + P + T + S
  fNPinpointBlocks = static_cast<G4int>(fPinpointThickness / pinpointBlockThickness);
  fPinpointThickness = fNPinpointBlocks * pinpointBlockThickness;

  fNFortuneBlocks = static_cast<int>((fMaxDetectorThickness - fPinpointThickness) / fortuneBlockThickness);
  G4double fortuneThickness = fNFortuneBlocks * fortuneBlockThickness + fFortunePixelBlockThickness;
  auto detectorThickness = fPinpointThickness + fortuneThickness;
  G4double detEnvelopeSizeY = std::max(fPixelDetectorHeight, fScintDetectorHeight);
  G4double detEnvelopeSizeX = std::max(fPixelDetectorWidth, fScintDetectorWidth);
  // detectorYCenter > 0: the detector envelope is shifted upward so its bottom aligns with
  // the pixel detector bottom (world Y = -0.5*fPixelDetectorHeight).  The top of the envelope
  // therefore reaches world Y = detEnvelopeSizeY - 0.5*fPixelDetectorHeight.
  G4double detectorYCenter = -0.5*fPixelDetectorHeight + 0.5*detEnvelopeSizeY;
  // World must cover: detector (±half-width in X, shifted in Y) and magnets (±fOuterRadius in X/Y)
  G4double worldHalfX = std::max(0.5*detEnvelopeSizeX, fOuterRadius);
  G4double worldHalfY = std::max(detEnvelopeSizeY - 0.5*fPixelDetectorHeight, fOuterRadius);
  auto worldSizeX = 2.0 * 1.2 * worldHalfX;
  auto worldSizeY = 2.0 * 1.2 * worldHalfY;
  auto worldSizeZ = 1.2 * (detectorThickness + fTracker3Position);

  // Get materials
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
  G4Material* worldMaterial = nist->FindOrBuildMaterial("G4_AIR");
  G4Material* tungstenMaterial = nist->FindOrBuildMaterial("G4_W");
  G4Material* siliconMaterial = nist->FindOrBuildMaterial("G4_Si");
    
  //Calling scintillator, wrapping and related variables
  DefineMaterial();

  G4int nScintBarsX = static_cast<G4int>(fScintDetectorWidth / fScintBarWidth);
  G4int nScintBarsY = static_cast<G4int>(fScintDetectorHeight / fScintBarHeight);

  // World
  G4Box* worldS = new G4Box("World", 0.5 * worldSizeX, 0.5 * worldSizeY, worldSizeZ);
  G4LogicalVolume* worldLV = new G4LogicalVolume(worldS, worldMaterial, "World");
  G4VPhysicalVolume* worldPV = new G4PVPlacement(nullptr, G4ThreeVector(), worldLV, "World", nullptr, false, 0, fCheckOverlaps);

  G4VisAttributes* experimentalHallVisAtt = new G4VisAttributes(G4Colour(1.,1.,1.));
  experimentalHallVisAtt->SetForceWireframe(true);
  worldLV->SetVisAttributes(experimentalHallVisAtt);

  // Detector envelope – Y sized to max(fPixelDetectorHeight, fScintDetectorHeight) so scint bars fit.
  // Shifted in Y so the bottom edge of the envelope aligns with the bottom of the pixel detector,
  // which bottom-aligns the scintillators with the pixel detector.
  auto detectorS = new G4Box("Detector", 0.5 * detEnvelopeSizeX, 0.5 * detEnvelopeSizeY, 0.5 * detectorThickness);
  auto detectorLV = new G4LogicalVolume(detectorS, worldMaterial, "Detector");
  // Front face at z=0, back face at z=detectorThickness in world coords
  new G4PVPlacement(0, G4ThreeVector(0., detectorYCenter, 0.5 * detectorThickness), detectorLV, "Detector", worldLV, false, 0, fCheckOverlaps);

  // ---------------------------------------------------------------
  // Tungsten
  // ---------------------------------------------------------------
  G4VisAttributes* TargetVisAtt = new G4VisAttributes(G4Colour::Red());
  TargetVisAtt->SetForceWireframe(true);

  // PINPOINT pixel tungsten
  auto pinpointTungstenS = new G4Box("PinpointTungsten", 0.5*fPixelDetectorWidth, 0.5*fPixelDetectorHeight, 0.5*fPinpointTungstenThickness);
  auto pinpointTungstenLV = new G4LogicalVolume(pinpointTungstenS, tungstenMaterial, "PinpointTungsten");
  pinpointTungstenLV->SetVisAttributes(TargetVisAtt);

  // PINPOINT scintillator tungsten
  auto pinpointScintTungstenS = new G4Box("PinpointScintTungsten", 0.5*fScintDetectorWidth, 0.5*fScintDetectorHeight, 0.5*fPinpointTungstenThickness);
  auto pinpointScintTungstenLV = new G4LogicalVolume(pinpointScintTungstenS, tungstenMaterial, "PinpointScintTungsten");
  pinpointScintTungstenLV->SetVisAttributes(TargetVisAtt);

  // FORTUNE Pixel Tungsten
  auto fortuneTungstenS = new G4Box("Tungsten", 0.5 * fPixelDetectorWidth, 0.5 * fPixelDetectorHeight, 0.5 * fFortuneTungstenThickness);
  auto fortuneTungstenLV = new G4LogicalVolume(fortuneTungstenS, tungstenMaterial, "Tungsten");
  fortuneTungstenLV->SetVisAttributes(TargetVisAtt);

  // FORTUNE Scintillator Tungsten
  auto fortuneScintTungstenS = new G4Box("ScintTungsten", 0.5 * fScintDetectorWidth, 0.5 * fScintDetectorHeight, 0.5 * fFortuneTungstenThickness);
  auto fortuneScintTungstenLV = new G4LogicalVolume(fortuneScintTungstenS, tungstenMaterial, "ScintTungsten");
  fortuneScintTungstenLV->SetVisAttributes(TargetVisAtt);

  // ---------------------------------------------------------------
  // Silicon pixel layer
  // ---------------------------------------------------------------
  G4VisAttributes* LayerAtrrib = new G4VisAttributes(G4Colour::Green());
  LayerAtrrib->SetVisibility(true);
  LayerAtrrib->SetForceSolid(true);

  auto pixelLayerS = new G4Box("PixelLayer", 0.5*fPixelDetectorWidth, 0.5*fPixelDetectorHeight, 0.5*fSiliconThickness);
  fPixelLayerLV = new G4LogicalVolume(pixelLayerS, siliconMaterial, "pixelLayer");
  fPixelLayerLV->SetVisAttributes(LayerAtrrib);

  // ---------------------------------------------------------------
  // FORTUNE pixel block: T + 0.5 * box + P + 0.5 * box
  // ---------------------------------------------------------------
  auto fortunePixelBlockS  = new G4Box("FortunePixelBlock", 0.5*fPixelDetectorWidth, 0.5*fPixelDetectorHeight, 0.5*fFortunePixelBlockThickness);
  auto fortunePixelBlockLV = new G4LogicalVolume(fortunePixelBlockS, worldMaterial, "PixelBlock");
  fortunePixelBlockLV->SetVisAttributes(G4VisAttributes::GetInvisible());
  // Tungsten
  G4double z_T_pxl = -0.5*fFortunePixelBlockThickness + 0.5*fFortuneTungstenThickness;
  new G4PVPlacement(0, G4ThreeVector(0., 0., z_T_pxl), fortuneTungstenLV, "Tungsten", fortunePixelBlockLV, false, 0, fCheckOverlaps);
  // Pixel
  G4double z_Si_pxl = -0.5*fFortunePixelBlockThickness + fFortuneTungstenThickness + 0.5*fBoxThickness + 0.5*fSiliconThickness;
  new G4PVPlacement(nullptr, G4ThreeVector(0., 0., z_Si_pxl), fPixelLayerLV, "PixelLayer", fortunePixelBlockLV, false, 0, fCheckOverlaps);

  // ---------------------------------------------------------------
  // PINPOINT pixel block: T_pp + 0.5 * box + P + 0.5 * box
  // ---------------------------------------------------------------
  auto pinpointPixelBlockS  = new G4Box("PinpointPixelLayer", 0.5*fPixelDetectorWidth, 0.5*fPixelDetectorHeight, 0.5*fPinpointPixelBlockThickness);
  auto pinpointPixelBlockLV = new G4LogicalVolume(pinpointPixelBlockS, worldMaterial, "PinpointPixelLayer");
  pinpointPixelBlockLV->SetVisAttributes(G4VisAttributes::GetInvisible());
  // Tungsten
  G4double z_T_pp = -0.5*fPinpointPixelBlockThickness + 0.5*fPinpointTungstenThickness;
  new G4PVPlacement(0, G4ThreeVector(0., 0., z_T_pp), pinpointTungstenLV, "PinpointTungsten", pinpointPixelBlockLV, false, 0, fCheckOverlaps);
  // Pixel
  G4double z_Si_pp = -0.5*fPinpointPixelBlockThickness + fPinpointTungstenThickness + 0.5 * fBoxThickness + 0.5*fSiliconThickness;
  new G4PVPlacement(nullptr, G4ThreeVector(0., 0., z_Si_pp), fPixelLayerLV, "PixelLayer", pinpointPixelBlockLV, false, 0, fCheckOverlaps);

  // Scintillator visual attributes (shared by Pinpoint and Fortune)
  G4VisAttributes* ScintLayerAtrrib = new G4VisAttributes(G4Colour::Blue());
  ScintLayerAtrrib->SetVisibility(true);
  ScintLayerAtrrib->SetForceSolid(true);

  // ---------------------------------------------------------------
  // Shared scintillator pixel LV (used by all bar panels when scint_bar_flag is true).
  // Pixel size = fScintBarWidth × fScintBarHeight (bar width × bar height).
  // Bar containers (ScintColumn / ScintRow) are air-filled; pixels are the active material.
  // ---------------------------------------------------------------
  G4LogicalVolume* scintPixelLV = nullptr;
  if (scint_bar_flag) {
    auto scintPixelS = new G4Box("ScintPixel", 0.5*fScintBarWidth, 0.5*fScintBarHeight, 0.5*fScintThickness);
    scintPixelLV = new G4LogicalVolume(scintPixelS, scintillator, "ScintPixel");
    scintPixelLV->SetVisAttributes(ScintLayerAtrrib);
    scintLVs.push_back(scintPixelLV);
  }

  // ---------------------------------------------------------------
  // PINPOINT scintillator layer LVs: T_pp + S
  // ---------------------------------------------------------------
  G4LogicalVolume* pinpointHorizontalScintBlockLV = nullptr;
  G4LogicalVolume* pinpointVerticalScintBlockLV   = nullptr;
  if (fNPinpointBlocks > 0) {
    G4double z_T_ppsc = -0.5*fPinpointScintBlockThickness + 0.5*fPinpointTungstenThickness;
    G4double z_ppsc   = -0.5*fPinpointScintBlockThickness + fPinpointTungstenThickness + 0.5*fScintThickness;

    // --- Horizontal block (vertical bars: columns segmented in X, each filled with pixels in Y) ---
    auto pinpointHorizontalScintBlockS = new G4Box("PinpointHorizontalScintLayer", 0.5*fScintDetectorWidth, 0.5*fScintDetectorHeight, 0.5*fPinpointScintBlockThickness);
    pinpointHorizontalScintBlockLV = new G4LogicalVolume(pinpointHorizontalScintBlockS, worldMaterial, "PinpointHorizontalScintLayer");
    pinpointHorizontalScintBlockLV->SetVisAttributes(G4VisAttributes::GetInvisible());
    new G4PVPlacement(0, G4ThreeVector(0., 0., z_T_ppsc), pinpointScintTungstenLV, "PinpointScintTungsten", pinpointHorizontalScintBlockLV, false, 0, fCheckOverlaps);
    if (!scint_bar_flag) {
      auto ppHScintS = new G4Box("PinpointHorizontalScintBlock", 0.5*fScintDetectorWidth, 0.5*fScintDetectorHeight, 0.5*fScintThickness);
      auto ppHScintLV = new G4LogicalVolume(ppHScintS, scintillator, "PinpointHorizontalScintBlock");
      ppHScintLV->SetVisAttributes(ScintLayerAtrrib);
      new G4PVPlacement(0, G4ThreeVector(0., 0., z_ppsc), ppHScintLV, "PinpointHorizontalScintBlock", pinpointHorizontalScintBlockLV, false, 0, fCheckOverlaps);
      scintLVs.push_back(ppHScintLV);
    } else {
      // Air-filled column container; pixels stacked in Y inside each column
      auto ppHScintColS = new G4Box("ScintColumn", 0.5*fScintBarWidth, 0.5*fScintDetectorHeight, 0.5*fScintThickness);
      auto ppHScintColLV = new G4LogicalVolume(ppHScintColS, worldMaterial, "ScintColumn");
      ppHScintColLV->SetVisAttributes(G4VisAttributes::GetInvisible());
      G4double yFirst_ppH = -0.5*fScintDetectorHeight + 0.5*fScintBarHeight;
      for (G4int jBar = 0; jBar < nScintBarsY; ++jBar) {
        G4double yPixel = yFirst_ppH + jBar * fScintBarHeight;
        new G4PVPlacement(0, G4ThreeVector(0., yPixel, 0.), scintPixelLV, "ScintPixel", ppHScintColLV, false, jBar, fCheckOverlaps);
      }
      G4double xpos = -0.5*fScintDetectorWidth + 0.5*fScintBarWidth;
      for (G4int iBar = 0; iBar < nScintBarsX; ++iBar) {
        new G4PVPlacement(0, G4ThreeVector(xpos, 0., z_ppsc), ppHScintColLV, "ScintColumn", pinpointHorizontalScintBlockLV, false, iBar, fCheckOverlaps);
        xpos += fScintBarWidth;
      }
    }

    // --- Vertical block (horizontal bars: rows segmented in Y, each filled with pixels in X) ---
    auto pinpointVerticalScintBlockS = new G4Box("PinpointVerticalScintLayer", 0.5*fScintDetectorWidth, 0.5*fScintDetectorHeight, 0.5*fPinpointScintBlockThickness);
    pinpointVerticalScintBlockLV = new G4LogicalVolume(pinpointVerticalScintBlockS, worldMaterial, "PinpointVerticalScintLayer");
    pinpointVerticalScintBlockLV->SetVisAttributes(G4VisAttributes::GetInvisible());
    new G4PVPlacement(0, G4ThreeVector(0., 0., z_T_ppsc), pinpointScintTungstenLV, "PinpointScintTungsten", pinpointVerticalScintBlockLV, false, 0, fCheckOverlaps);
    if (!scint_bar_flag) {
      auto ppVScintS = new G4Box("PinpointVerticalScintBlock", 0.5*fScintDetectorWidth, 0.5*fScintDetectorHeight, 0.5*fScintThickness);
      auto ppVScintLV = new G4LogicalVolume(ppVScintS, scintillator, "PinpointVerticalScintBlock");
      ppVScintLV->SetVisAttributes(ScintLayerAtrrib);
      new G4PVPlacement(0, G4ThreeVector(0., 0., z_ppsc), ppVScintLV, "PinpointVerticalScintBlock", pinpointVerticalScintBlockLV, false, 0, fCheckOverlaps);
      scintLVs.push_back(ppVScintLV);
    } else {
      // Air-filled row container; pixels stacked in X inside each row
      auto ppVScintRowS = new G4Box("ScintRow", 0.5*fScintDetectorWidth, 0.5*fScintBarHeight, 0.5*fScintThickness);
      auto ppVScintRowLV = new G4LogicalVolume(ppVScintRowS, worldMaterial, "ScintRow");
      ppVScintRowLV->SetVisAttributes(G4VisAttributes::GetInvisible());
      G4double xFirst_ppV = -0.5*fScintDetectorWidth + 0.5*fScintBarWidth;
      for (G4int iBar = 0; iBar < nScintBarsX; ++iBar) {
        G4double xPixel = xFirst_ppV + iBar * fScintBarWidth;
        new G4PVPlacement(0, G4ThreeVector(xPixel, 0., 0.), scintPixelLV, "ScintPixel", ppVScintRowLV, false, iBar, fCheckOverlaps);
      }
      G4double yFirst = -0.5*fScintDetectorHeight + 0.5*fScintBarHeight;
      for (G4int jBar = 0; jBar < nScintBarsY; ++jBar) {
        G4double yBar = yFirst + jBar * fScintBarHeight;
        new G4PVPlacement(0, G4ThreeVector(0., yBar, z_ppsc), ppVScintRowLV, "ScintRow", pinpointVerticalScintBlockLV, false, jBar, fCheckOverlaps);
      }
    }
  }

  // ---------------------------------------------------------------
  // FORTUNE scintillator block: T + fNumScintPanelsPerLayer × S
  // ---------------------------------------------------------------
  G4LogicalVolume* fortuneScintBlockLV = nullptr;
  if (fNumScintPanelsPerLayer > 0 && fNumScintLayers > 0) {
    auto fortuneScintBlockS  = new G4Box("ScintLayer", 0.5*fScintDetectorWidth, 0.5*fScintDetectorHeight, 0.5*fFortuneScintBlockThickness);
    fortuneScintBlockLV = new G4LogicalVolume(fortuneScintBlockS, worldMaterial, "ScintLayer");
    fortuneScintBlockLV->SetVisAttributes(G4VisAttributes::GetInvisible());
    // Tungsten
    G4double z_T_scnt = -0.5*fFortuneScintBlockThickness + 0.5*fFortuneTungstenThickness;
    new G4PVPlacement(0, G4ThreeVector(0., 0., z_T_scnt), fortuneScintTungstenLV, "ScintTungsten", fortuneScintBlockLV, false, 0, fCheckOverlaps);
    // Scintillators
    // Y offset so scintillator bottom-edge aligns with detector bottom-edge
    G4double yScintOffset = -0.5*fPixelDetectorHeight + 0.5*fScintDetectorHeight;
    for (G4int iPanel = 0; iPanel < fNumScintPanelsPerLayer; ++iPanel) {
      G4bool verticalBars = (iPanel == 0); // first panel: vertical bars; second panel: horizontal bars
      G4double z_panel = -0.5*fFortuneScintBlockThickness + fFortuneTungstenThickness + (iPanel + 0.5)*fScintThickness;
      G4String blockName = "";
      if (!scint_bar_flag) {
        // Solid block
        blockName = "solidScint";
        auto scintBlockS  = new G4Box(blockName, 0.5*fScintDetectorWidth, 0.5*fScintDetectorHeight, 0.5*fScintThickness);
        auto scintBlockLV = new G4LogicalVolume(scintBlockS, scintillator, blockName);
        scintBlockLV->SetVisAttributes(ScintLayerAtrrib);
        new G4PVPlacement(0, G4ThreeVector(0., 0., z_panel),
                          scintBlockLV, blockName, fortuneScintBlockLV, false, iPanel, fCheckOverlaps);
        scintLVs.push_back(scintBlockLV);
      } else if (verticalBars) {
        // Vertical bars: air-filled columns with pixels stacked in Y
        blockName = "verticalBars";
        auto scintColS  = new G4Box("ScintColumn", 0.5*fScintBarWidth, 0.5*fScintDetectorHeight, 0.5*fScintThickness);
        auto scintColLV = new G4LogicalVolume(scintColS, worldMaterial, "ScintColumn");
        scintColLV->SetVisAttributes(G4VisAttributes::GetInvisible());
        G4double yFirst_fort = -0.5*fScintDetectorHeight + 0.5*fScintBarHeight;
        for (G4int jBar = 0; jBar < nScintBarsY; ++jBar) {
          G4double yPixel = yFirst_fort + jBar * fScintBarHeight;
          new G4PVPlacement(0, G4ThreeVector(0., yPixel, 0.),
                            scintPixelLV, "ScintPixel", scintColLV, false, jBar, fCheckOverlaps);
        }
        G4double xpos = -0.5*fScintDetectorWidth + 0.5*fScintBarWidth;
        for (G4int iBar = 0; iBar < nScintBarsX; ++iBar) {
          new G4PVPlacement(0, G4ThreeVector(xpos, 0., z_panel),
                            scintColLV, "ScintColumn", fortuneScintBlockLV, false, iBar, fCheckOverlaps);
          xpos += fScintBarWidth;
        }
      } else {
        // Horizontal bars: air-filled rows with pixels stacked in X
        blockName = "horizontalBars";
        auto scintRowS  = new G4Box("ScintRow", 0.5*fScintDetectorWidth, 0.5*fScintBarHeight, 0.5*fScintThickness);
        auto scintRowLV = new G4LogicalVolume(scintRowS, worldMaterial, "ScintRow");
        scintRowLV->SetVisAttributes(G4VisAttributes::GetInvisible());
        G4double xFirst_fort = -0.5*fScintDetectorWidth + 0.5*fScintBarWidth;
        for (G4int iBar = 0; iBar < nScintBarsX; ++iBar) {
          G4double xPixel = xFirst_fort + iBar * fScintBarWidth;
          new G4PVPlacement(0, G4ThreeVector(xPixel, 0., 0.),
                            scintPixelLV, "ScintPixel", scintRowLV, false, iBar, fCheckOverlaps);
        }
        G4double yFirst = -0.5*fScintDetectorHeight + 0.5*fScintBarHeight;
        for (G4int jBar = 0; jBar < nScintBarsY; ++jBar) {
          G4double yBar = yFirst + jBar * fScintBarHeight;
          new G4PVPlacement(0, G4ThreeVector(0., yBar, z_panel),
                            scintRowLV, "ScintRow", fortuneScintBlockLV, false, jBar, fCheckOverlaps);
        }
      }
    }
  }

  // ---------------------------------------------------------------
  // Add blocks to detector geometry
  // ---------------------------------------------------------------
  fLayerIsPixel.clear();
  // Pixel layers must be offset in Y within the (possibly enlarged) detector envelope
  // so that they remain at world Y=0 (centred on the beam axis).
  G4double pixelLayerYInDet = -detectorYCenter;
  // Scint layers are placed so their bottom edge aligns with the pixel detector bottom
  // (world Y = -0.5*fPixelDetectorHeight).  When fScintDetectorHeight >= fPixelDetectorHeight this is 0.
  G4double scintYInDet = 0.5 * (fScintDetectorHeight - detEnvelopeSizeY);
  // ---------------------------------------------------------------
  // PINPOINT
  // ---------------------------------------------------------------
  for (G4int i = 0; i < fNPinpointBlocks; ++i) {
    G4double zGroupStart = -0.5*detectorThickness + i * pinpointBlockThickness;
    // Pixel bloks
    G4double zCenter = zGroupStart + 0.5*fPinpointPixelBlockThickness;
    fLayerPV = new G4PVPlacement(0, G4ThreeVector(0., pixelLayerYInDet, zCenter),
                                  pinpointPixelBlockLV, "PinpointPixelBlock", detectorLV, false, i, fCheckOverlaps);
    fLayerIsPixel.push_back(true);
    // Scintillator blocks (alternating: horizontal if i%2==0, vertical otherwise)
    G4LogicalVolume* ppScintLV = (i % 2 == 0) ? pinpointHorizontalScintBlockLV : pinpointVerticalScintBlockLV;
    if (ppScintLV) {
      G4double zScintCenter = zGroupStart + fPinpointPixelBlockThickness + 0.5*fPinpointScintBlockThickness;
      new G4PVPlacement(0, G4ThreeVector(0., scintYInDet, zScintCenter),
                        ppScintLV, "ScintLayer", detectorLV, false, i, fCheckOverlaps);
      fLayerIsPixel.push_back(false);
    }
  }
  // ---------------------------------------------------------------
  // FORTUNE
  // ---------------------------------------------------------------
  for (G4int i = 0; i < fNFortuneBlocks; ++i) {
    G4double zGroupStart = -0.5*detectorThickness + fPinpointThickness + i * fortuneBlockThickness;
    // Pixel block
    G4double zPixelCenter = zGroupStart + 0.5*fFortunePixelBlockThickness;
    fLayerPV = new G4PVPlacement(0, G4ThreeVector(0., pixelLayerYInDet, zPixelCenter),
                                  fortunePixelBlockLV, "PinpointPixelLayer", detectorLV, false, fNPinpointBlocks + i, fCheckOverlaps);
    fLayerIsPixel.push_back(true);
    // Scint blocks
    for (G4int j = 0; j < fNumScintLayers; ++j) {
      G4double zScintCenter = zGroupStart + fFortunePixelBlockThickness + j * fFortuneScintBlockThickness + 0.5*fFortuneScintBlockThickness;
      G4int scintCopyNum = fNPinpointBlocks + i * fNumScintLayers + j;
      new G4PVPlacement(0, G4ThreeVector(0., scintYInDet, zScintCenter),
                        fortuneScintBlockLV, "ScintLayer", detectorLV, false, scintCopyNum, fCheckOverlaps);
      fLayerIsPixel.push_back(false);
    }
  }

  // Trailing pixel layer (after last fortune group)
  {
    G4double zTrailingCenter = -0.5*detectorThickness + fPinpointThickness + fNFortuneBlocks * fortuneBlockThickness + 0.5*fFortunePixelBlockThickness;
    fLayerPV = new G4PVPlacement(0, G4ThreeVector(0., pixelLayerYInDet, zTrailingCenter),
                                  fortunePixelBlockLV, "FortunePixelLayer", detectorLV, false, fNPinpointBlocks + fNFortuneBlocks, fCheckOverlaps);
    fLayerIsPixel.push_back(true);
  }

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
    // bottom at world Y = -0.5*fPixelDetectorHeight.
    G4double fieldYOffset = -0.5 * fPixelDetectorHeight + fInnerRadius;
    auto fieldRegion0LV = new G4LogicalVolume(longFieldS, worldMaterial, "FieldRegion0");
    new G4PVPlacement(nullptr, G4ThreeVector(0., fieldYOffset, fMagnet0Position), fieldRegion0LV, "FieldRegion0", worldLV, false, 0, fCheckOverlaps);
    fieldRegion0LV->SetVisAttributes(G4VisAttributes::GetInvisible());

    auto fieldRegion1LV = new G4LogicalVolume(shortFieldS, worldMaterial, "FieldRegion1");
    new G4PVPlacement(nullptr, G4ThreeVector(0., fieldYOffset, fMagnet1Position), fieldRegion1LV, "FieldRegion1", worldLV, false, 1, fCheckOverlaps);
    fieldRegion1LV->SetVisAttributes(G4VisAttributes::GetInvisible());

    auto fieldRegion2LV = new G4LogicalVolume(shortFieldS, worldMaterial, "FieldRegion2");
    new G4PVPlacement(nullptr, G4ThreeVector(0., fieldYOffset, fMagnet2Position), fieldRegion2LV, "FieldRegion2", worldLV, false, 2, fCheckOverlaps);
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
    G4double trackerYOffset = -0.5 * fPixelDetectorHeight + 0.5 * fTrackerSize;
    auto trackerS = new G4Box("Tracker", 0.5 * fTrackerSize, 0.5 * fTrackerSize, 0.5 * fSiliconThickness);
  
    // Tracker 1
    auto tracker1LV = new G4LogicalVolume(trackerS, siliconMaterial, "Tracker1");
    new G4PVPlacement(nullptr, G4ThreeVector(0., trackerYOffset, fTracker1Position), tracker1LV, "Tracker1", worldLV, false, 0, fCheckOverlaps);
    tracker1LV->SetVisAttributes(G4VisAttributes(G4Colour(0.0, 1.0, 0.0, 0.7))); // Green, semi-transparent
  
    // Tracker 2
    auto tracker2LV = new G4LogicalVolume(trackerS, siliconMaterial, "Tracker2");
    new G4PVPlacement(nullptr, G4ThreeVector(0., trackerYOffset, fTracker2Position), tracker2LV, "Tracker2", worldLV, false, 1, fCheckOverlaps);
    tracker2LV->SetVisAttributes(G4VisAttributes(G4Colour(0.0, 1.0, 0.0, 0.7))); // Green, semi-transparent
  
    // Tracker 3
    auto tracker3LV = new G4LogicalVolume(trackerS, siliconMaterial, "Tracker3");
    new G4PVPlacement(nullptr, G4ThreeVector(0., trackerYOffset, fTracker3Position), tracker3LV, "Tracker3", worldLV, false, 2, fCheckOverlaps);
    tracker3LV->SetVisAttributes(G4VisAttributes(G4Colour(0.0, 1.0, 0.0, 0.7))); // Green, semi-transparent
  }

  G4cout << G4endl;
  G4cout << "========================================" << G4endl;
  G4cout << "===      Detector Configuration      ===" << G4endl;
  G4cout << "========================================" << G4endl;

  G4cout << G4endl;
  G4cout << "--- Pixel Detector ---" << G4endl;
  G4cout << "  Active area:        " << fPixelDetectorWidth/mm  << " x " << fPixelDetectorHeight/mm  << " mm" << G4endl;
  G4cout << "  Pixel size:         " << fPixelWidth/um << " x " << fPixelHeight/um << " um" << G4endl;
  G4cout << "  Pixel grid:         " << fNPixelsX << " x " << fNPixelsY << " pixels per layer" << G4endl;
  G4cout << "  Silicon thickness:  " << fSiliconThickness/um << " um  (" << siliconMaterial->GetName() << ")" << G4endl;
  G4cout << "  Air gap (box):      " << fBoxThickness/um << " um" << G4endl;

  G4cout << G4endl;
  G4cout << "--- Scintillator Detector ---" << G4endl;
  G4cout << "  Active area:        " << fScintDetectorWidth/mm << " x " << fScintDetectorHeight/mm << " mm" << G4endl;
  G4cout << "  Panel thickness:    " << fScintThickness/mm << " mm  (" << scintillator->GetName() << ")" << G4endl;
  G4cout << "  Bar segmentation:   " << nScintBarsX << " x-bars (" << fScintBarWidth/mm << " mm wide)"
         << "  x  " << nScintBarsY << " y-bars (" << fScintBarHeight/mm << " mm tall)" << G4endl;

  if (std::abs(nScintBarsX * fScintBarWidth - fScintDetectorWidth) > 0.01*mm)
    G4Exception("DetectorConstruction", "Geom001", JustWarning,
                "X bars do not exactly tile the scintillator width!");
  if (std::abs(nScintBarsY * fScintBarHeight - fScintDetectorHeight) > 0.01*mm)
    G4Exception("DetectorConstruction", "Geom002", JustWarning,
                "Y bars do not exactly tile the scintillator height!");

  G4cout << G4endl;
  G4cout << "--- Pinpoint Section (" << fNPinpointBlocks << " blocks) ---" << G4endl;
  G4cout << "  Tungsten thickness: " << fPinpointTungstenThickness/mm << " mm  (" << tungstenMaterial->GetName() << ")" << G4endl;
  G4cout << "  Pixel block (T+gap+Si): " << fPinpointPixelBlockThickness/mm << " mm" << G4endl;
  G4cout << "  Scint block (T+S):      " << fPinpointScintBlockThickness/mm << " mm" << G4endl;
  G4cout << "  Total Pinpoint thickness: " << fPinpointThickness/mm << " mm" << G4endl;

  G4cout << G4endl;
  G4cout << "--- Fortune Section (" << fNFortuneBlocks << " pixel+scint groups + 1 trailing pixel layer) ---" << G4endl;
  G4cout << "  Tungsten thickness: " << fFortuneTungstenThickness/mm << " mm  (" << tungstenMaterial->GetName() << ")" << G4endl;
  G4cout << "  Pixel block (T+gap+Si): " << fFortunePixelBlockThickness/mm << " mm" << G4endl;
  G4cout << "  Scint layers per group: " << fNumScintLayers << G4endl;
  G4cout << "  Scint panels per layer: " << fNumScintPanelsPerLayer << G4endl;
  if (fNumScintLayers > 0 && fNumScintPanelsPerLayer > 0)
    G4cout << "  Scint block (T+" << fNumScintPanelsPerLayer << "xS): " << fFortuneScintBlockThickness/mm << " mm" << G4endl;

  G4cout << G4endl;
  G4cout << "--- Total Layer Count ---" << G4endl;
  G4cout << "  Pinpoint pixel layers:  " << fNPinpointBlocks << G4endl;
  G4cout << "  Fortune pixel layers:   " << fNFortuneBlocks + 1 << "  (incl. trailing)" << G4endl;
  G4cout << "  Total pixel layers:     " << fNPinpointBlocks + fNFortuneBlocks + 1 << G4endl;
  G4cout << "  Fortune scint layers:   " << fNFortuneBlocks * fNumScintLayers << G4endl;
  G4cout << "  Pinpoint scint layers:  " << fNPinpointBlocks << G4endl;

  G4cout << G4endl;
  if (fEnableFaserSpectrometer) {
    G4cout << "--- FASER Spectrometer Magnets ---" << G4endl;
    G4cout << "  Bore radius:  " << fInnerRadius/mm << " mm,  shell outer radius: " << fOuterRadius/mm << " mm" << G4endl;
    G4cout << "  Magnet 0: z = " << fMagnet0Position/mm << " mm, length = " << fLongMagnetLength/mm  << " mm" << G4endl;
    G4cout << "  Magnet 1: z = " << fMagnet1Position/mm << " mm, length = " << fShortMagnetLength/mm << " mm" << G4endl;
    G4cout << "  Magnet 2: z = " << fMagnet2Position/mm << " mm, length = " << fShortMagnetLength/mm << " mm" << G4endl;

    G4cout << G4endl;
    G4cout << "--- FASER Tracking Stations ---" << G4endl;
    G4cout << "  Active area:   " << fTrackerSize/mm << " x " << fTrackerSize/mm << " mm" << G4endl;
    G4cout << "  Thickness:     " << fSiliconThickness/um << " um  (" << siliconMaterial->GetName() << ")" << G4endl;
    G4cout << "  Tracker 1: z = " << fTracker1Position/mm << " mm" << G4endl;
    G4cout << "  Tracker 2: z = " << fTracker2Position/mm << " mm" << G4endl;
    G4cout << "  Tracker 3: z = " << fTracker3Position/mm << " mm" << G4endl;
  }
  G4cout << "========================================" << G4endl << G4endl;

  // Always overwrite the GDML file
  if (std::ifstream(fWriteFile).good()) {
    std::remove(fWriteFile.c_str());
  }
  fParser.Write(fWriteFile, worldPV);

  //PrintLayerVolumePositions();
  
  return worldPV;
}

void DetectorConstruction::ConstructSDandField()
{
    
  // Create Scintillator SD
  auto scintSD = new ScintillatorSD("ScintillatorDetector", "ScintHitsCollection", "ScintPixelHitsCollection");
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
    
    // Make lookup tables to extract x,y,z position of pixels
    ComputeSiliconZPositions();
    ComputePixelCentersXY();
    ComputeScintCentersXY();
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

  const G4double pinpointBlockThickness = fPinpointPixelBlockThickness + fPinpointScintBlockThickness;
  const G4double fortuneBlockThickness  = fFortunePixelBlockThickness  + fNumScintLayers * fFortuneScintBlockThickness;

  // Offsets from the front face of a pixel block to the centre of each sub-layer
  const G4double ppTungstenOffset = 0.5 * fPinpointTungstenThickness;
  const G4double ppSiliconOffset  = fPinpointTungstenThickness + 0.5 * fBoxThickness + 0.5 * fSiliconThickness;
  const G4double ftTungstenOffset = 0.5 * fFortuneTungstenThickness;
  const G4double ftSiliconOffset  = fFortuneTungstenThickness + 0.5 * fBoxThickness + 0.5 * fSiliconThickness;

  // ---- Pinpoint blocks ----
  for (G4int i = 0; i < fNPinpointBlocks; ++i) {
    const G4double blockFront = i * pinpointBlockThickness;

    // Pixel sub-block
    fTungstenZPositions.push_back(blockFront + ppTungstenOffset);
    fTungstenThicknesses.push_back(fPinpointTungstenThickness);
    fSiliconZPositions.push_back( blockFront + ppSiliconOffset);

    // Scint sub-block (same tungsten thickness as pixel tungsten in pinpoint)
    const G4double scintBlockFront = blockFront + fPinpointPixelBlockThickness;
    fTungstenZPositions.push_back(scintBlockFront + ppTungstenOffset);
    fTungstenThicknesses.push_back(fPinpointTungstenThickness);
    fScintZPositions.push_back(   scintBlockFront + fPinpointTungstenThickness + 0.5 * fScintThickness);
  }

  // ---- Fortune blocks ----
  for (G4int i = 0; i < fNFortuneBlocks; ++i) {
    const G4double blockFront = fPinpointThickness + i * fortuneBlockThickness;

    // Pixel sub-block
    fTungstenZPositions.push_back(blockFront + ftTungstenOffset);
    fTungstenThicknesses.push_back(fFortuneTungstenThickness);
    fSiliconZPositions.push_back( blockFront + ftSiliconOffset);

    // Scint sub-blocks
    for (G4int j = 0; j < fNumScintLayers; ++j) {
      const G4double scintBlockFront = blockFront + fFortunePixelBlockThickness + j * fFortuneScintBlockThickness;
      fTungstenZPositions.push_back(scintBlockFront + ftTungstenOffset);
      fTungstenThicknesses.push_back(fFortuneTungstenThickness);
      for (G4int p = 0; p < fNumScintPanelsPerLayer; ++p) {
        fScintZPositions.push_back(scintBlockFront + fFortuneTungstenThickness + (p + 0.5) * fScintThickness);
      }
    }
  }

  // ---- Trailing pixel layer ----
  {
    const G4double blockFront = fPinpointThickness + fNFortuneBlocks * fortuneBlockThickness;
    fTungstenZPositions.push_back(blockFront + ftTungstenOffset);
    fTungstenThicknesses.push_back(fFortuneTungstenThickness);
    fSiliconZPositions.push_back( blockFront + ftSiliconOffset);
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
  const G4double xMin = -0.5 * fPixelDetectorWidth;
  for (G4int col = 0; col < fNPixelsX; ++col)
    fPixelCenterX.push_back(xMin + (col + 0.5) * fPixelWidth);

  // Y centres: pixel detector is centred on beam axis (world Y = 0)
  fPixelCenterY.clear();
  fPixelCenterY.reserve(fNPixelsY);
  const G4double yMin = -0.5 * fPixelDetectorHeight;
  for (G4int row = 0; row < fNPixelsY; ++row)
    fPixelCenterY.push_back(yMin + (row + 0.5) * fPixelHeight);

  G4cout << "Computed pixel centers: "
         << fPixelCenterX.size() << " x "
         << fPixelCenterY.size() << G4endl;
}


void DetectorConstruction::ComputeScintCentersXY()
{
  // Vertical bars are segmented in X (bar runs full scintillator height)
  // X centres: symmetric about world X = 0
  fScintBarCenterX.clear();
  const G4int nBarsX = static_cast<G4int>(fScintDetectorWidth / fScintBarWidth);
  fScintBarCenterX.reserve(nBarsX);
  const G4double xMin = -0.5 * fScintDetectorWidth;
  for (G4int i = 0; i < nBarsX; ++i)
    fScintBarCenterX.push_back(xMin + (i + 0.5) * fScintBarWidth);

  // Horizontal bars are segmented in Y (bar runs full scintillator width)
  // Y centres in world coords: scintillator bottom aligns with pixel detector bottom
  // at world Y = -0.5 * fPixelDetectorHeight.
  fScintBarCenterY.clear();
  const G4int nBarsY = static_cast<G4int>(fScintDetectorHeight / fScintBarHeight);
  fScintBarCenterY.reserve(nBarsY);
  const G4double yMin = -0.5 * fPixelDetectorHeight;
  for (G4int j = 0; j < nBarsY; ++j)
    fScintBarCenterY.push_back(yMin + (j + 0.5) * fScintBarHeight);

  G4cout << "Computed scint bar centers: "
         << fScintBarCenterX.size() << " x-bars, "
         << fScintBarCenterY.size() << " y-bars" << G4endl;
}

