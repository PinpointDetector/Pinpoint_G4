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
  if (fNumScintPanelsPerLayer == 0) fNumScintLayers = 0;
  fFortunePixelBlockThickness  = fFortuneTungstenThickness + fBoxThickness + fSiliconThickness;
  fPinpointPixelBlockThickness = fPinpointTungstenThickness + fBoxThickness + fSiliconThickness;
  fPinpointScintBlockThickness = fPinpointTungstenThickness + fScintThickness;
  // Tungsten is split into quarter-thickness pieces, each panel independently wrapped by
  // one before and one after (adjacent panels therefore share a double-thickness boundary).
  fFortuneScintBlockThickness = (fNumScintPanelsPerLayer > 0)
                                ? fNumScintPanelsPerLayer * (fScintThickness + 0.5 * fFortuneTungstenThickness)
                                : 0.0 * mm;
  // Pinpoint block: pixel sub-block and scint sub-block each independently wrapped in an aluminum wall pair.
  G4double pinpointBlockThickness = fPinpointPixelBlockThickness + fPinpointScintBlockThickness + 4 * fAluminumWallThickness;
  // Fortune block: pixel block + each scint layer independently wrapped in an aluminum wall pair.
  G4double fortuneBlockThickness = fFortunePixelBlockThickness + fNumScintLayers * fFortuneScintBlockThickness
                                  + 2 * (fNumScintLayers + 1) * fAluminumWallThickness;

  // Each pinpoint block is T + P + T + S
  // fNPinpointBlocks = static_cast<G4int>(fPinpointThickness / pinpointBlockThickness);
  fPinpointThickness = fNPinpointBlocks * pinpointBlockThickness;

  // fNFortuneBlocks = static_cast<int>((fMaxDetectorThickness - fPinpointThickness) / fortuneBlockThickness);
  G4double fortuneThickness = fNFortuneBlocks * fortuneBlockThickness + fFortunePixelBlockThickness;
  G4double IPTPixelBlockThickness = fPinpointPixelBlockThickness;
  G4double IPTThickness = fNIPTLayers * IPTPixelBlockThickness;
  auto detectorThickness = fPinpointThickness + fortuneThickness + IPTThickness;
  G4double detEnvelopeSizeY = std::max({fPixelDetectorHeight, fScintDetectorHeight, fAluminumWallHeight});
  G4double detEnvelopeSizeX = std::max({fPixelDetectorWidth, fScintDetectorWidth, fAluminumWallWidth});
  // Detector envelope is centred at (0, 0) on the beam axis.
  // World must cover: detector (±half-width in X/Y) and magnets (±fOuterRadius in X/Y)
  G4double worldHalfX = std::max(0.5*detEnvelopeSizeX, fOuterRadius);
  G4double worldHalfY = std::max(0.5*detEnvelopeSizeY, fOuterRadius);
  auto worldSizeX = 2.0 * 1.2 * worldHalfX;
  auto worldSizeY = 2.0 * 1.2 * worldHalfY;
  auto worldSizeZ = 1.2 * (detectorThickness + fTracker3Position);

  // Get materials
  G4NistManager* nist = G4NistManager::Instance();
  G4Material* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
  G4Material* worldMaterial = nist->FindOrBuildMaterial("G4_AIR");
  G4Material* tungstenMaterial = nist->FindOrBuildMaterial("G4_W");
  G4Material* siliconMaterial = nist->FindOrBuildMaterial("G4_Si");
  G4Material* aluminumMaterial = nist->FindOrBuildMaterial("G4_Al");
    
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

  // Detector envelope – Y/X sized to max(pixel, scint) extents so all layers fit; centred at (0, 0).
  auto detectorS = new G4Box("Detector", 0.5 * detEnvelopeSizeX, 0.5 * detEnvelopeSizeY, 0.5 * detectorThickness);
  auto detectorLV = new G4LogicalVolume(detectorS, worldMaterial, "Detector");
  // Front face at z=0, back face at z=detectorThickness in world coords
  new G4PVPlacement(0, G4ThreeVector(0., 0., 0.5 * detectorThickness), detectorLV, "Detector", worldLV, false, 0, fCheckOverlaps);

  // ---------------------------------------------------------------
  // Tungsten
  // ---------------------------------------------------------------
  G4VisAttributes* TargetVisAtt = new G4VisAttributes(G4Colour::Red());
  TargetVisAtt->SetForceWireframe(true);
  TargetVisAtt->SetVisibility(true);
  // TargetVisAtt->SetVisibility(false);

  // PINPOINT pixel tungsten (transverse size matches the scintillator plates): split into
  // half-thickness pieces, one before and one after the pixel silicon (see below).
  G4double pinpointTungstenHalfThickness = 0.5*fPinpointTungstenThickness;
  fPinpointTungstenWidth = std::max(fPinpointTungstenWidth, fScintDetectorWidth);
  fPinpointTungstenHeight = std::max(fPinpointTungstenHeight, fScintDetectorHeight);
  auto pinpointTungstenS = new G4Box("PinpointTungsten", 0.5*fPinpointTungstenWidth, 0.5*fPinpointTungstenHeight, 0.5*pinpointTungstenHalfThickness);
  auto pinpointTungstenLV = new G4LogicalVolume(pinpointTungstenS, tungstenMaterial, "PinpointTungsten");
  pinpointTungstenLV->SetVisAttributes(TargetVisAtt);

  // PINPOINT scintillator tungsten: split into half-thickness pieces, one before and one
  // after the scintillator (see pinpointHorizontalScintBlockLV / pinpointVerticalScintBlockLV below).
  auto pinpointScintTungstenS = new G4Box("PinpointScintTungsten", 0.5*fScintDetectorWidth, 0.5*fScintDetectorHeight, 0.5*pinpointTungstenHalfThickness);
  auto pinpointScintTungstenLV = new G4LogicalVolume(pinpointScintTungstenS, tungstenMaterial, "PinpointScintTungsten");
  pinpointScintTungstenLV->SetVisAttributes(TargetVisAtt);

  // FORTUNE Pixel Tungsten
  auto fortuneTungstenS = new G4Box("Tungsten", 0.5 * fScintDetectorWidth, 0.5 * fScintDetectorHeight, 0.5 * fFortuneTungstenThickness);
  auto fortuneTungstenLV = new G4LogicalVolume(fortuneTungstenS, tungstenMaterial, "Tungsten");
  fortuneTungstenLV->SetVisAttributes(TargetVisAtt);

  // FORTUNE Scintillator Tungsten: split into quarter-thickness pieces, one before and one
  // after each scintillator panel (see fortuneScintBlockLV below).
  G4double fortuneScintTungstenQuarterThickness = 0.25 * fFortuneTungstenThickness;
  auto fortuneScintTungstenS = new G4Box("ScintTungsten", 0.5 * fScintDetectorWidth, 0.5 * fScintDetectorHeight, 0.5 * fortuneScintTungstenQuarterThickness);
  auto fortuneScintTungstenLV = new G4LogicalVolume(fortuneScintTungstenS, tungstenMaterial, "ScintTungsten");
  fortuneScintTungstenLV->SetVisAttributes(TargetVisAtt);

  // ---------------------------------------------------------------
  // Aluminum wall: placed before and after each Pinpoint/Fortune block, spanning the full
  // transverse extent of the detector envelope.
  // ---------------------------------------------------------------
  G4LogicalVolume* aluminumWallLV = nullptr;
  if (fAluminumWallThickness > 0.) {
    G4VisAttributes* AlWallVisAtt = new G4VisAttributes(G4Colour::Grey());
    AlWallVisAtt->SetForceSolid(true);
    AlWallVisAtt->SetVisibility(false);
    // AlWallVisAtt->SetVisibility(true);
    auto aluminumWallS = new G4Box("AluminumWall", 0.5*fAluminumWallWidth, 0.5*fAluminumWallHeight, 0.5*fAluminumWallThickness);
    aluminumWallLV = new G4LogicalVolume(aluminumWallS, aluminumMaterial, "AluminumWall");
    aluminumWallLV->SetVisAttributes(AlWallVisAtt);
  }

  // ---------------------------------------------------------------
  // Silicon pixel layer
  // ---------------------------------------------------------------
  G4VisAttributes* LayerAtrrib = new G4VisAttributes(G4Colour::Green());
  LayerAtrrib->SetVisibility(false);
  // LayerAtrrib->SetVisibility(true);
  LayerAtrrib->SetForceSolid(true);

  auto pixelLayerS = new G4Box("PixelLayer", 0.5*fPixelDetectorWidth, 0.5*fPixelDetectorHeight, 0.5*fSiliconThickness);
  fPixelLayerLV = new G4LogicalVolume(pixelLayerS, siliconMaterial, "pixelLayer");
  fPixelLayerLV->SetVisAttributes(LayerAtrrib);

  // ---------------------------------------------------------------
  // FORTUNE / PINPOINT pixel blocks (T + 0.5*box + P + 0.5*box) are assembled directly
  // in the detector volume (see below) rather than as a wrapper logical volume, since the
  // tungsten (centred at the scintillator offset) and the silicon (centred at the pixel
  // offset) generally sit at different transverse positions.
  // ---------------------------------------------------------------
  G4double ftTungstenOffset = 0.5*fFortuneTungstenThickness;
  G4double ftSiliconOffset  = fFortuneTungstenThickness + 0.5*fBoxThickness + 0.5*fSiliconThickness;
  // Pinpoint pixel tungsten is split in half: one piece before, one after the silicon.
  G4double ppTungstenFrontOffset = 0.5*pinpointTungstenHalfThickness;
  G4double ppSiliconOffset       = pinpointTungstenHalfThickness + 0.5*fBoxThickness + 0.5*fSiliconThickness;
  G4double ppTungstenBackOffset  = fPinpointPixelBlockThickness - 0.5*pinpointTungstenHalfThickness;

  // ---------------------------------------------------------------
  // Interface Pixel Tracker (IPT)
  // ---------------------------------------------------------------
  auto IPTPixelBlockS  = new G4Box("AirPixelBlock", 0.5*fPixelDetectorWidth, 0.5*fPixelDetectorHeight, 0.5*IPTPixelBlockThickness);
  auto IPTPixelBlockLV = new G4LogicalVolume(IPTPixelBlockS, worldMaterial, "IPTPixelBlock");
  IPTPixelBlockLV->SetVisAttributes(G4VisAttributes::GetInvisible());
  G4double z_Si_IPT = -0.5*IPTPixelBlockThickness + fPinpointTungstenThickness + 0.5*fBoxThickness + 0.5*fSiliconThickness;
  new G4PVPlacement(nullptr, G4ThreeVector(0., 0., z_Si_IPT), fPixelLayerLV, "PixelLayer", IPTPixelBlockLV, false, 0, fCheckOverlaps);

  // Scintillator visual attributes (shared by Pinpoint and Fortune)
  G4VisAttributes* ScintLayerAtrrib = new G4VisAttributes(G4Colour::Blue());
  ScintLayerAtrrib->SetVisibility(true);
  // ScintLayerAtrrib->SetVisibility(false);
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
    // Scint tungsten split in half: one piece before, one after the scintillator.
    G4double z_T_ppsc_front = -0.5*fPinpointScintBlockThickness + 0.5*pinpointTungstenHalfThickness;
    G4double z_ppsc         = -0.5*fPinpointScintBlockThickness + pinpointTungstenHalfThickness + 0.5*fScintThickness;
    G4double z_T_ppsc_back  =  0.5*fPinpointScintBlockThickness - 0.5*pinpointTungstenHalfThickness;

    // --- Horizontal block (vertical bars: columns segmented in X, each filled with pixels in Y) ---
    auto pinpointHorizontalScintBlockS = new G4Box("PinpointHorizontalScintLayer", 0.5*fScintDetectorWidth, 0.5*fScintDetectorHeight, 0.5*fPinpointScintBlockThickness);
    pinpointHorizontalScintBlockLV = new G4LogicalVolume(pinpointHorizontalScintBlockS, worldMaterial, "PinpointHorizontalScintLayer");
    pinpointHorizontalScintBlockLV->SetVisAttributes(G4VisAttributes::GetInvisible());
    new G4PVPlacement(0, G4ThreeVector(0., 0., z_T_ppsc_front), pinpointScintTungstenLV, "PinpointScintTungsten", pinpointHorizontalScintBlockLV, false, 0, fCheckOverlaps);
    new G4PVPlacement(0, G4ThreeVector(0., 0., z_T_ppsc_back), pinpointScintTungstenLV, "PinpointScintTungsten", pinpointHorizontalScintBlockLV, false, 1, fCheckOverlaps);
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
    new G4PVPlacement(0, G4ThreeVector(0., 0., z_T_ppsc_front), pinpointScintTungstenLV, "PinpointScintTungsten", pinpointVerticalScintBlockLV, false, 0, fCheckOverlaps);
    new G4PVPlacement(0, G4ThreeVector(0., 0., z_T_ppsc_back), pinpointScintTungstenLV, "PinpointScintTungsten", pinpointVerticalScintBlockLV, false, 1, fCheckOverlaps);
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

  G4cout << "Scint detector size: " << fScintDetectorWidth << ", " << fScintDetectorHeight << G4endl;

  // ---------------------------------------------------------------
  // FORTUNE scintillator block: T + fNumScintPanelsPerLayer × S
  // ---------------------------------------------------------------
  G4LogicalVolume* fortuneScintBlockLV = nullptr;
  if (fNumScintPanelsPerLayer > 0 && fNumScintLayers > 0) {
    auto fortuneScintBlockS  = new G4Box("ScintLayer", 0.5*fScintDetectorWidth, 0.5*fScintDetectorHeight, 0.5*fFortuneScintBlockThickness);
    fortuneScintBlockLV = new G4LogicalVolume(fortuneScintBlockS, worldMaterial, "ScintLayer");
    fortuneScintBlockLV->SetVisAttributes(G4VisAttributes::GetInvisible());
    // Tungsten quarter-pieces and scintillator panels, interleaved: each panel is wrapped by
    // its own tungsten quarter before and after (adjacent panels share a double-thickness
    // tungsten boundary), e.g. for 2 panels: Tq, S1, Tq, Tq, S2, Tq.
    G4double zCursorScint = -0.5*fFortuneScintBlockThickness;
    G4int scintTungstenCopy = 0;
    for (G4int iPanel = 0; iPanel < fNumScintPanelsPerLayer; ++iPanel) {
      new G4PVPlacement(0, G4ThreeVector(0., 0., zCursorScint + 0.5*fortuneScintTungstenQuarterThickness),
                        fortuneScintTungstenLV, "ScintTungsten", fortuneScintBlockLV, false, scintTungstenCopy++, fCheckOverlaps);
      zCursorScint += fortuneScintTungstenQuarterThickness;

      G4bool verticalBars = (iPanel == 0); // first panel: vertical bars; second panel: horizontal bars
      G4double z_panel = zCursorScint + 0.5*fScintThickness;
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
      zCursorScint += fScintThickness;

      new G4PVPlacement(0, G4ThreeVector(0., 0., zCursorScint + 0.5*fortuneScintTungstenQuarterThickness),
                        fortuneScintTungstenLV, "ScintTungsten", fortuneScintBlockLV, false, scintTungstenCopy++, fCheckOverlaps);
      zCursorScint += fortuneScintTungstenQuarterThickness;
    }
  }

  // ---------------------------------------------------------------
  // Add blocks to detector geometry
  // ---------------------------------------------------------------
  fLayerIsPixel.clear();
  // ---------------------------------------------------------------
  // PINPOINT
  // ---------------------------------------------------------------
  // Pixel layers are centred at (fPixelDetectorOffsetX, fPixelDetectorOffsetY);
  // scintillator layers and tungsten plates are centred at (fScintDetectorOffsetX, fScintDetectorOffsetY).
  G4double pixelX = fPixelDetectorOffsetX;
  G4double pixelY = fPixelDetectorOffsetY;
  G4double scintX = fScintDetectorOffsetX;
  G4double scintY = fScintDetectorOffsetY;

  // Each pinpoint block wraps the pixel sub-block and the scint sub-block in its own
  // dedicated wall pair; the two sub-blocks therefore have two walls back-to-back
  // at their shared boundary.
  G4int pinpointWallCopy = 0;
  for (G4int i = 0; i < fNPinpointBlocks; ++i) {
    G4double zCursor = -0.5*detectorThickness + i * pinpointBlockThickness;

    // Pixel block: wall, tungsten (scint offset) + silicon (pixel offset), wall
    if (aluminumWallLV) {
      new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zCursor + 0.5*fAluminumWallThickness),
                        aluminumWallLV, "AluminumWall", detectorLV, false, pinpointWallCopy++, fCheckOverlaps);
      zCursor += fAluminumWallThickness;
    }
    new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zCursor + ppTungstenFrontOffset),
                      pinpointTungstenLV, "PinpointTungsten", detectorLV, false, 2*i, fCheckOverlaps);
    fLayerPV = new G4PVPlacement(0, G4ThreeVector(pixelX, pixelY, zCursor + ppSiliconOffset),
                                  fPixelLayerLV, "PixelLayer", detectorLV, false, i, fCheckOverlaps);
    new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zCursor + ppTungstenBackOffset),
                      pinpointTungstenLV, "PinpointTungsten", detectorLV, false, 2*i + 1, fCheckOverlaps);
    fLayerIsPixel.push_back(true);
    zCursor += fPinpointPixelBlockThickness;
    if (aluminumWallLV) {
      new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zCursor + 0.5*fAluminumWallThickness),
                        aluminumWallLV, "AluminumWall", detectorLV, false, pinpointWallCopy++, fCheckOverlaps);
      zCursor += fAluminumWallThickness;
    }

    // Scint block: wall, tungsten + scintillator, wall (alternating: horizontal if i%2==0, vertical otherwise)
    G4LogicalVolume* ppScintLV = (i % 2 == 0) ? pinpointHorizontalScintBlockLV : pinpointVerticalScintBlockLV;
    if (ppScintLV) {
      if (aluminumWallLV) {
        new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zCursor + 0.5*fAluminumWallThickness),
                          aluminumWallLV, "AluminumWall", detectorLV, false, pinpointWallCopy++, fCheckOverlaps);
        zCursor += fAluminumWallThickness;
      }
      G4double zScintCenter = zCursor + 0.5*fPinpointScintBlockThickness;
      new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zScintCenter),
                        ppScintLV, "ScintLayer", detectorLV, false, i, fCheckOverlaps);
      fLayerIsPixel.push_back(false);
      zCursor += fPinpointScintBlockThickness;
      if (aluminumWallLV) {
        new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zCursor + 0.5*fAluminumWallThickness),
                          aluminumWallLV, "AluminumWall", detectorLV, false, pinpointWallCopy++, fCheckOverlaps);
        zCursor += fAluminumWallThickness;
      }
    }
  }
  // ---------------------------------------------------------------
  // FORTUNE
  // ---------------------------------------------------------------
  // Each fortune block wraps every sub-component (pixel block, and each scint block)
  // in its own dedicated wall pair; adjacent components therefore have two walls
  // back-to-back at their shared boundary.
  G4int fortuneWallCopy = pinpointWallCopy;
  for (G4int i = 0; i < fNFortuneBlocks; ++i) {
    G4double zCursor = -0.5*detectorThickness + fPinpointThickness + i * fortuneBlockThickness;
    G4int copyNum = fNPinpointBlocks + i;

    // Pixel block: wall, tungsten (scint offset) + silicon (pixel offset), wall
    if (aluminumWallLV) {
      new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zCursor + 0.5*fAluminumWallThickness),
                        aluminumWallLV, "AluminumWall", detectorLV, false, fortuneWallCopy++, fCheckOverlaps);
      zCursor += fAluminumWallThickness;
    }
    new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zCursor + ftTungstenOffset),
                      fortuneTungstenLV, "Tungsten", detectorLV, false, copyNum, fCheckOverlaps);
    fLayerPV = new G4PVPlacement(0, G4ThreeVector(pixelX, pixelY, zCursor + ftSiliconOffset),
                                  fPixelLayerLV, "PixelLayer", detectorLV, false, copyNum, fCheckOverlaps);
    fLayerIsPixel.push_back(true);
    zCursor += fFortunePixelBlockThickness;
    if (aluminumWallLV) {
      new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zCursor + 0.5*fAluminumWallThickness),
                        aluminumWallLV, "AluminumWall", detectorLV, false, fortuneWallCopy++, fCheckOverlaps);
      zCursor += fAluminumWallThickness;
    }

    // Scint blocks: wall, tungsten + N_panels*scintillator, wall (repeated per scint layer)
    for (G4int j = 0; j < fNumScintLayers; ++j) {
      if (aluminumWallLV) {
        new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zCursor + 0.5*fAluminumWallThickness),
                          aluminumWallLV, "AluminumWall", detectorLV, false, fortuneWallCopy++, fCheckOverlaps);
        zCursor += fAluminumWallThickness;
      }
      G4double zScintCenter = zCursor + 0.5*fFortuneScintBlockThickness;
      G4int scintCopyNum = fNPinpointBlocks + i * fNumScintLayers + j;
      new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zScintCenter),
                        fortuneScintBlockLV, "ScintLayer", detectorLV, false, scintCopyNum, fCheckOverlaps);
      fLayerIsPixel.push_back(false);
      zCursor += fFortuneScintBlockThickness;
      if (aluminumWallLV) {
        new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zCursor + 0.5*fAluminumWallThickness),
                          aluminumWallLV, "AluminumWall", detectorLV, false, fortuneWallCopy++, fCheckOverlaps);
        zCursor += fAluminumWallThickness;
      }
    }
  }

  // Trailing pixel layer (after last fortune group)
  {
    G4double zGroupStart = -0.5*detectorThickness + fPinpointThickness + fNFortuneBlocks * fortuneBlockThickness;
    G4int copyNum = fNPinpointBlocks + fNFortuneBlocks;
    new G4PVPlacement(0, G4ThreeVector(scintX, scintY, zGroupStart + ftTungstenOffset),
                      fortuneTungstenLV, "Tungsten", detectorLV, false, copyNum, fCheckOverlaps);
    fLayerPV = new G4PVPlacement(0, G4ThreeVector(pixelX, pixelY, zGroupStart + ftSiliconOffset),
                                  fPixelLayerLV, "PixelLayer", detectorLV, false, copyNum, fCheckOverlaps);
    fLayerIsPixel.push_back(true);
  }

  // Interface Pixel Tracker
  {
    G4double zIPTLayerStart = -0.5*detectorThickness + fPinpointThickness + fNFortuneBlocks * fortuneBlockThickness + fFortunePixelBlockThickness;
    for (G4int i = 0; i < fNIPTLayers; ++i) {
      G4double zCenter = zIPTLayerStart + (i + 0.5) * IPTPixelBlockThickness;
      fLayerPV = new G4PVPlacement(0, G4ThreeVector(pixelX, pixelY, zCenter),
                                    IPTPixelBlockLV, "IPTPixelLayer", detectorLV, false, fNPinpointBlocks + fNFortuneBlocks + 1 + i, fCheckOverlaps);
      fLayerIsPixel.push_back(true);
    }
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
  G4cout << "  Center (X, Y):      " << pixelX/mm << ", " << pixelY/mm << " mm" << G4endl;
  G4cout << "  Pixel size:         " << fPixelWidth/um << " x " << fPixelHeight/um << " um" << G4endl;
  G4cout << "  Pixel grid:         " << fNPixelsX << " x " << fNPixelsY << " pixels per layer" << G4endl;
  G4cout << "  Silicon thickness:  " << fSiliconThickness/um << " um  (" << siliconMaterial->GetName() << ")" << G4endl;
  G4cout << "  IPT gap (box):      " << fBoxThickness/um << " um" << G4endl;

  G4cout << G4endl;
  G4cout << "--- Scintillator Detector ---" << G4endl;
  G4cout << "  Active area:        " << fScintDetectorWidth/mm << " x " << fScintDetectorHeight/mm << " mm" << G4endl;
  G4cout << "  Center (X, Y):      " << scintX/mm << ", " << scintY/mm << " mm" << G4endl;
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
  G4cout << "  Tungsten width:     " << fPinpointTungstenWidth/mm << " mm" << G4endl;
  G4cout << "  Tungsten height:    " << fPinpointTungstenHeight/mm << " mm" << G4endl;
  G4cout << "  Pixel block (T+gap+Si): " << fPinpointPixelBlockThickness/mm << " mm" << G4endl;
  G4cout << "  Scint block (T+S):      " << fPinpointScintBlockThickness/mm << " mm" << G4endl;
  G4cout << "  Aluminum wall thickness: " << fAluminumWallThickness/mm << " mm  (x2 per block)" << G4endl;
  G4cout << "  Total Pinpoint thickness: " << fPinpointThickness/mm << " mm" << G4endl;

  G4cout << G4endl;
  G4cout << "--- Fortune Section (" << fNFortuneBlocks << " pixel+scint groups + 1 trailing pixel layer) ---" << G4endl;
  G4cout << "  Tungsten thickness: " << fFortuneTungstenThickness/mm << " mm  (" << tungstenMaterial->GetName() << ")" << G4endl;
  G4cout << "  Pixel block (T+gap+Si): " << fFortunePixelBlockThickness/mm << " mm" << G4endl;
  G4cout << "  Scint layers per group: " << fNumScintLayers << G4endl;
  G4cout << "  Scint panels per layer: " << fNumScintPanelsPerLayer << G4endl;
  if (fNumScintLayers > 0 && fNumScintPanelsPerLayer > 0)
    G4cout << "  Scint block (T+" << fNumScintPanelsPerLayer << "xS): " << fFortuneScintBlockThickness/mm << " mm" << G4endl;
  G4cout << "  Aluminum wall thickness: " << fAluminumWallThickness/mm << " mm  (x2 per block)" << G4endl;

  G4cout << G4endl;
  G4cout << "--- Total Layer Count ---" << G4endl;
  G4cout << "  Pinpoint pixel layers:  " << fNPinpointBlocks << G4endl;
  G4cout << "  Fortune pixel layers:   " << fNFortuneBlocks + 1 << "  (incl. trailing)" << G4endl;
  G4cout << "  Trailing IPT pixel layers: " << fNIPTLayers << G4endl;
  G4cout << "  Total pixel layers:     " << fNPinpointBlocks + fNFortuneBlocks + 1 + fNIPTLayers << G4endl;
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
  scintSD->SetLayerIndexing(fNPinpointBlocks, fNumScintPanelsPerLayer); 
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

  const G4double pinpointBlockThickness = fPinpointPixelBlockThickness + fPinpointScintBlockThickness + 4 * fAluminumWallThickness;
  const G4double fortuneBlockThickness  = fFortunePixelBlockThickness  + fNumScintLayers * fFortuneScintBlockThickness
                                          + 2 * (fNumScintLayers + 1) * fAluminumWallThickness;

  // Offsets from the front face of a pixel block to the centre of each sub-layer.
  // Pinpoint tungsten is split in half: one piece before, one after the silicon/scint.
  const G4double pinpointTungstenHalfThickness = 0.5 * fPinpointTungstenThickness;
  const G4double ppTungstenFrontOffset = 0.5 * pinpointTungstenHalfThickness;
  const G4double ppSiliconOffset       = pinpointTungstenHalfThickness + 0.5 * fBoxThickness + 0.5 * fSiliconThickness;
  const G4double ppTungstenBackOffset  = fPinpointPixelBlockThickness - 0.5 * pinpointTungstenHalfThickness;
  const G4double ppScintTungstenBackOffset = fPinpointScintBlockThickness - 0.5 * pinpointTungstenHalfThickness;
  const G4double ftTungstenOffset = 0.5 * fFortuneTungstenThickness;
  const G4double ftSiliconOffset  = fFortuneTungstenThickness + 0.5 * fBoxThickness + 0.5 * fSiliconThickness;

  // ---- Pinpoint blocks ----
  // Each pinpoint block wraps the pixel sub-block and the scint sub-block in its own
  // dedicated wall pair (see Construct()).
  for (G4int i = 0; i < fNPinpointBlocks; ++i) {
    G4double cursor = i * pinpointBlockThickness + fAluminumWallThickness;

    // Pixel sub-block: tungsten half before + half after the silicon
    fTungstenZPositions.push_back(cursor + ppTungstenFrontOffset);
    fTungstenThicknesses.push_back(pinpointTungstenHalfThickness);
    fSiliconZPositions.push_back( cursor + ppSiliconOffset);
    fTungstenZPositions.push_back(cursor + ppTungstenBackOffset);
    fTungstenThicknesses.push_back(pinpointTungstenHalfThickness);
    cursor += fPinpointPixelBlockThickness + 2 * fAluminumWallThickness;

    // Scint sub-block: tungsten half before + half after the scintillator
    fTungstenZPositions.push_back(cursor + ppTungstenFrontOffset);
    fTungstenThicknesses.push_back(pinpointTungstenHalfThickness);
    fScintZPositions.push_back(   cursor + pinpointTungstenHalfThickness + 0.5 * fScintThickness);
    fTungstenZPositions.push_back(cursor + ppScintTungstenBackOffset);
    fTungstenThicknesses.push_back(pinpointTungstenHalfThickness);
  }

  // ---- Fortune blocks ----
  // Each fortune block wraps the pixel sub-block and every scint sub-block in its own
  // dedicated wall pair (see Construct()).
  for (G4int i = 0; i < fNFortuneBlocks; ++i) {
    G4double cursor = fPinpointThickness + i * fortuneBlockThickness + fAluminumWallThickness;

    // Pixel sub-block
    fTungstenZPositions.push_back(cursor + ftTungstenOffset);
    fTungstenThicknesses.push_back(fFortuneTungstenThickness);
    fSiliconZPositions.push_back( cursor + ftSiliconOffset);
    cursor += fFortunePixelBlockThickness + 2 * fAluminumWallThickness;

    // Scint sub-blocks: tungsten split into quarter-thickness pieces, one before and one
    // after each scintillator panel (see fortuneScintBlockLV in Construct()).
    const G4double scintTungstenQuarter = 0.25 * fFortuneTungstenThickness;
    for (G4int j = 0; j < fNumScintLayers; ++j) {
      G4double scintCursor = cursor;
      for (G4int p = 0; p < fNumScintPanelsPerLayer; ++p) {
        fTungstenZPositions.push_back(scintCursor + 0.5 * scintTungstenQuarter);
        fTungstenThicknesses.push_back(scintTungstenQuarter);
        scintCursor += scintTungstenQuarter;

        fScintZPositions.push_back(scintCursor + 0.5 * fScintThickness);
        scintCursor += fScintThickness;

        fTungstenZPositions.push_back(scintCursor + 0.5 * scintTungstenQuarter);
        fTungstenThicknesses.push_back(scintTungstenQuarter);
        scintCursor += scintTungstenQuarter;
      }
      cursor += fFortuneScintBlockThickness + 2 * fAluminumWallThickness;
    }
  }

  // ---- Trailing pixel layer ----
  {
    const G4double blockFront = fPinpointThickness + fNFortuneBlocks * fortuneBlockThickness;
    fTungstenZPositions.push_back(blockFront + ftTungstenOffset);
    fTungstenThicknesses.push_back(fFortuneTungstenThickness);
    fSiliconZPositions.push_back( blockFront + ftSiliconOffset);
  }

  // ---- Interface pixel tracker (fNIPTLayers layers, no tungsten) ----
  {
    const G4double IPTLayersStart = fPinpointThickness + fNFortuneBlocks * fortuneBlockThickness + fFortunePixelBlockThickness;
    const G4double IPTPixelBlockThickness = fPinpointPixelBlockThickness;
    for (G4int i = 0; i < fNIPTLayers; ++i) {
      const G4double blockFront = IPTLayersStart + i * IPTPixelBlockThickness;
      fSiliconZPositions.push_back(blockFront + fPinpointTungstenThickness + 0.5*fBoxThickness + 0.5*fSiliconThickness);
    }
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
  const G4double xMin = -0.5 * fPixelDetectorWidth + fPixelDetectorOffsetX;
  for (G4int col = 0; col < fNPixelsX; ++col)
    fPixelCenterX.push_back(xMin + (col + 0.5) * fPixelWidth);

  // Y centres: pixel detector is centred on beam axis (world Y = 0)
  fPixelCenterY.clear();
  fPixelCenterY.reserve(fNPixelsY);
  const G4double yMin = -0.5 * fPixelDetectorHeight + fPixelDetectorOffsetY;
  for (G4int row = 0; row < fNPixelsY; ++row)
    fPixelCenterY.push_back(yMin + (row + 0.5) * fPixelHeight);

  G4cout << "Computed pixel centers: "
         << fPixelCenterX.size() << " x " << fPixelCenterY.size() << " centered at ("
         << fPixelCenterX[0] << ", " << fPixelCenterY[0] << ")" << G4endl;
}


void DetectorConstruction::ComputeScintCentersXY()
{
  // Vertical bars are segmented in X (bar runs full scintillator height)
  // X centres: symmetric about world X = 0
  fScintBarCenterX.clear();
  const G4int nBarsX = static_cast<G4int>(fScintDetectorWidth / fScintBarWidth);
  fScintBarCenterX.reserve(nBarsX);
  const G4double xMin = -0.5 * fScintDetectorWidth + fScintDetectorOffsetX;
  for (G4int i = 0; i < nBarsX; ++i)
    fScintBarCenterX.push_back(xMin + (i + 0.5) * fScintBarWidth);

  // Horizontal bars are segmented in Y (bar runs full scintillator width)
  // Y centres in world coords: scintillator layer is centred at fScintDetectorOffsetY.
  fScintBarCenterY.clear();
  const G4int nBarsY = static_cast<G4int>(fScintDetectorHeight / fScintBarHeight);
  fScintBarCenterY.reserve(nBarsY);
  const G4double yMin = -0.5 * fScintDetectorHeight + fScintDetectorOffsetY;
  for (G4int j = 0; j < nBarsY; ++j)
    fScintBarCenterY.push_back(yMin + (j + 0.5) * fScintBarHeight);

  G4cout << "Computed scint bar centers: "
         << fScintBarCenterX.size() << " x-bars, "
         << fScintBarCenterY.size() << " y-bars" << G4endl;
}

