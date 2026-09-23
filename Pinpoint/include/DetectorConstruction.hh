#ifndef pinpoint_DetectorConstruction_hh
#define pinpoint_DetectorConstruction_hh

#include "G4VUserDetectorConstruction.hh"
#include "G4GDMLParser.hh"
#include "G4SystemOfUnits.hh"
#include "DetectorConstructionMessenger.hh"
#include "G4RunManager.hh"
#include "G4OpticalSurface.hh"

class G4VPhysicalVolume;
class G4FieldManager;
class MagneticField;

class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;
    void DefineMaterial();

  
    void SetReadFile(const G4String& File);
    void SetWriteFile(const G4String& File);
    // std::vector<G4VPhysicalVolume*> GetTargetPhysVols() const { return fTarget_phys; }
    G4VPhysicalVolume* GetLayerPhysVol() const { return fLayerPV; }

    G4int GetNFortuneBlocks() const { return nFortuneLayers; }
    void SetNumFortuneBlocks(G4int n) { nFortuneLayers = n; }
    
    G4int GetNPinpointBlocks() const { return nPinpointLayers; }
    void SetNumPinpointLayers(G4int n) { nPinpointLayers = n; }

    void SetFortuneTungstenThickness(G4double thickness) { 
      if (thickness <= 0) {
        G4cerr << "Error: Tungsten thickness must be positive." << G4endl;
        return;
      }

      fTungstenPlateThickness = thickness;
      G4cout << "Set tungsten thickness to " << fTungstenPlateThickness/mm << " mm" << G4endl;

      // Optional: trigger geometry rebuild
      // G4RunManager::GetRunManager()->ReinitializeGeometry();
    }
    G4double GetFortuneTungstenThickness() const { return fTungstenPlateThickness; }

    void SetSiliconThickness(G4double thickness) { fPixelSensorThickness = thickness; }
    void SetPixelHeight(G4double height) { fPixelHeight = height; }
    void SetPixelWidth(G4double width) { fPixelWidth = width; }
    void SetPixelDetectorWidth(G4double width) { fPixelSensorWidth = width; }
    void SetPixelDetectorHeight(G4double height) { fPixelSensorHeight = height; }
    void SetCheckOverlaps(G4bool check) { fCheckOverlaps = check; }
    void SetGDMLFile(const G4String& filename) { fWriteFile = filename; }
    void SetNumScintPanelsPerLayer(G4int n) { fNumScintPanelsPerLayer = n; }
    void SetScintDetectorHeight(G4double height) { fScintDetectorHeight = height; }
    void SetScintDetectorWidth(G4double width) { fScintDetectorWidth = width; }
    void SetScintBarWidth(G4double w) { fScintBarWidth = w; }
    void SetScintBarHeight(G4double h) { fScintBarHeight = h; }
    void SetScintThickness(G4double t) { fScintThickness = t; }
    void SetNumScintLayers(G4int n) { fNumScintLayersPerModule = n; }
    // void SetMaxDetectorThickness(G4double t) { fMaxDetectorThickness = t; }

    void SetAluminumWallThickness(G4double t) { fAlWallThickness = t; }
    G4double GetAluminumWallThickness() const { return fAlWallThickness; }
    void SetAluminumWallWidth(G4double w) { fAlWallWidth = w; }
    G4double GetAluminumWallWidth() const { return fAlWallWidth; }
    void SetAluminumWallHeight(G4double h) { fAlWallHeight = h; }
    G4double GetAluminumWallHeight() const { return fAlWallHeight; }
    void SetEnableFaserSpectrometer(G4bool enable) { fEnableFaserSpectrometer = enable; }
    void SetNIPTLayers(G4int n) { fNIPTLayers = n; }
    G4int GetNIPTLayers() const { return fNIPTLayers; }

    void SetPixelDetectorOffsetX(G4double x) { fPixelDetectorOffsetX = x; }
    void SetPixelDetectorOffsetY(G4double y) { fPixelDetectorOffsetY = y; }
    void SetScintDetectorOffsetX(G4double x) { fScintDetectorOffsetX = x; }
    void SetScintDetectorOffsetY(G4double y) { fScintDetectorOffsetY = y; }
    G4double GetPixelDetectorOffsetX() const { return fPixelDetectorOffsetX; }
    G4double GetPixelDetectorOffsetY() const { return fPixelDetectorOffsetY; }
    G4double GetScintDetectorOffsetX() const { return fScintDetectorOffsetX; }
    G4double GetScintDetectorOffsetY() const { return fScintDetectorOffsetY; }


    G4double GetTungstenPlateThickness() const { return fTungstenPlateThickness; }
    G4double GetSiliconThickness() const { return fPixelSensorThickness; }
    G4double GetPixelHeight() const { return fPixelHeight; }
    G4double GetPixelWidth() const { return fPixelWidth; }
    G4double GetPixelDetectorWidth() const { return fPixelSensorWidth; }
    G4double GetPixelDetectorHeight() const { return fPixelSensorHeight; }
    G4double GetFortuneModuleLength() const { return fFortuneModuleLength; }
    G4double GetFortuneModuleWidth() const { return fFortuneModuleWidth; }
    G4double GetFortuneModuleHeight() const { return fFortuneModuleHeight; }
    G4double GetLayerThickness() const { return fPinpointBlockLength; }  // representative single-module thickness (crude per-layer stride)
    G4double GetPixelLayerThickness() const { return fPinpointBlockLength; }
    G4double GetScintLayerThickness() const { return fScintThickness; }
    G4double GetScintDetectorWidth() const { return fScintDetectorWidth; }
    G4double GetScintBarWidth() const { return fScintBarWidth; }
    G4double GetScintBarHeight() const { return fScintBarHeight; }
    G4double GetScintPanelThickness() const { return fScintThickness; }
    // G4int GetTotalNLayers() const { return fTotalNLayers; }
    const std::vector<G4bool>& GetLayerIsPixel() const { return fLayerIsPixel; }
    G4int GetNumScintPanelsPerLayer() const { return fNumScintPanelsPerLayer; }
    G4double GetScintDetectorHeight() const { return fScintDetectorHeight; }
    G4int GetNumScintLayers() const { return fNumScintLayersPerModule; }
    // G4double GetMaxDetectorThickness() const { return fMaxDetectorThickness; }
    G4int GetNPinpointLayers() const { return nPinpointLayers; }
    G4int GetNLayers() const { return static_cast<G4int>(fSiliconZPositions.size()); }

    G4int GetNPixelsX() const { return fNPixelsX; }
    G4int GetNPixelsY() const { return fNPixelsY; }

    const std::vector<G4double>& GetSiliconZPositions() const {
      return fSiliconZPositions;
    }
    const std::vector<G4double>& GetTungstenZPositions() const {
      return fTungstenZPositions;
    }
    const std::vector<G4double>& GetTungstenThicknesses() const {
      return fTungstenThicknesses;
    }
    const std::vector<G4double>& GetScintZPositions() const {
      return fScintZPositions;
    }
    const std::vector<G4double>& GetPixelCenterX() const {
      return fPixelCenterX;
    }
    const std::vector<G4double>& GetPixelCenterY() const {
      return fPixelCenterY;
    }
    // X centres of vertical scint bars (segmented in X)
    const std::vector<G4double>& GetScintBarCenterX() const {
      return fScintBarCenterX;
    }
    // Y centres of horizontal scint bars (segmented in Y), in world coords
    const std::vector<G4double>& GetScintBarCenterY() const {
      return fScintBarCenterY;
    }

    // panelID/layerID lookup tables for scintillator hits (used by ScintSD::ProcessHits()),
    // indexed directly by the placed PinpointBlock/FortuneBlock G4 copy number. Built in
    // ComputeSiliconZPositions(), in the exact same loop that builds fScintZPositions, so they
    // can never drift out of sync with the true physical panel ordering the way ScintSD's own
    // independently-derived index arithmetic did (twice).
    const std::vector<G4int>& GetScintPanelIDForPinpointBlock() const {
      return fScintPanelIDForPinpointBlock;
    }
    const std::vector<G4int>& GetScintLayerIDForPinpointBlock() const {
      return fScintLayerIDForPinpointBlock;
    }
    // Base panelID for a FortuneBlock hit; ScintSD adds groupCopy*2 + (isHorizontal?1:0) on top.
    const std::vector<G4int>& GetScintPanelIDBaseForFortuneBlock() const {
      return fScintPanelIDBaseForFortuneBlock;
    }
    const std::vector<G4int>& GetScintLayerIDForFortuneBlock() const {
      return fScintLayerIDForFortuneBlock;
    }
  
  private:
    
    void ComputeSiliconZPositions();
    std::vector<G4double> fSiliconZPositions;
    std::vector<G4double> fTungstenZPositions;
    std::vector<G4double> fTungstenThicknesses;
    std::vector<G4double> fScintZPositions;
    std::vector<G4bool>   fLayerIsPixel;

    // See the getters above: filled inside ComputeSiliconZPositions().
    std::vector<G4int> fScintPanelIDForPinpointBlock;
    std::vector<G4int> fScintLayerIDForPinpointBlock;
    std::vector<G4int> fScintPanelIDBaseForFortuneBlock;
    std::vector<G4int> fScintLayerIDForFortuneBlock;
    
    void ComputePixelCentersXY();
    std::vector<G4double> fPixelCenterX;
    std::vector<G4double> fPixelCenterY;

    void ComputeScintCentersXY();
    std::vector<G4double> fScintBarCenterX;  // vertical bars, segmented in X
    std::vector<G4double> fScintBarCenterY;  // horizontal bars, segmented in Y (world coords)

    G4String fWriteFile = "pinpoint.gdml";
    G4GDMLParser fParser;

    DetectorConstructionMessenger* messenger;
    
    // Fixed geometry parameters for FASER subdetectors (magnets and tracking stations)
    // ==========================
    G4double fLongMagnetLength = 1500.0 * mm; 
    G4double fShortMagnetLength = 1000.0 * mm; 
    G4double fInnerRadius = 100.0 * mm;
    G4double fOuterRadius = 215.0 * mm;
    // Position of FASER magnets and tracking stations relative to VetoNu scintillator
    G4double fVetoNuPosition = -3112 * mm;
    G4double fMagnet0Position = -815.3 * mm;
    G4double fMagnet1Position = 637.4 * mm; 
    G4double fMagnet2Position = 1837.4 * mm;
    G4double fTrackerSize = 250.0 * mm;
    G4double fTracker1Position = 47.4 * mm;
    G4double fTracker2Position = 1237.4 * mm;
    G4double fTracker3Position = 2427.4 * mm;
    // ==========================


    G4double fPixelDetectorOffsetX = 0.0 * mm;
    G4double fPixelDetectorOffsetY = 0.0 * mm;
    G4double fScintDetectorOffsetX = 0.0 * mm;
    G4double fScintDetectorOffsetY = 0.0 * mm;

    G4bool fCheckOverlaps = true;
    G4bool fEnableFaserSpectrometer = true;

    G4int fNPixelsX;
    G4int fNPixelsY;

    G4int nPinpointLayers = 6;
    G4int nFortuneLayers = 7;
    G4int nIntermidiatePixelLayers = nFortuneLayers - 1;
    G4int fNIPTLayers = 3;

    G4double fWorldSizeX = 0.0 * mm;
    G4double fWorldSizeY = 0.0 * mm;
    G4double fWorldSizeZ = 0.0 * mm;
    G4LogicalVolume* fWorldLV;
    void ConstructWorldLV();

    G4double fTungstenPlateThickness = 5 * mm; // thickness of tungsten plate, shared by the pixel and scintillator modules
    G4double fTungstenHeight = 42 * cm; // height of tungsten plate in pixel module
    G4double fTungstenWidth = 42 * cm; // width of tungsten
    G4LogicalVolume* fTungstenPlateLV;
    void ConstructTungstenPlateLV();

    G4double fAlWallThickness = 2 * mm; // thickness of aluminum wall, shared by the pixel and scintillator modules
    G4double fAlWallHeight = 60 * cm; // height of aluminum wall (pixel module)
    G4double fAlWallWidth = 55 * cm; // width of aluminum wall (pixel module)
    G4LogicalVolume* fAlWallLV;
    void ConstructAlWallLV();

    G4double fPixelSensorThickness = 10 * um; // thickness of silicon sensor in pixel module
    G4double fPixelHeight = 22.8 * um; // height of silicon pixel
    G4double fPixelWidth = 20.8 * um; // width of silicon pixel
    
    G4double fPixelSensorWidth = 25 * cm;
    G4double fPixelSensorHeight = 20 * cm;
    
    G4LogicalVolume* fPixelSensorLV;
    void ConstructPixelSensorLV();
    
    G4double fAlCoolingPlateThickness = 5 * mm; // thickness of aluminum cooling plate in pixel module
    G4LogicalVolume* fAlCoolingPlateLV;
    void ConstructAlCoolingPlateLV();
    
    G4double fScintillatorThickness = 5 * mm; // thickness of scintillator in pixel module
    G4LogicalVolume* fScintillatorVertLV;
    G4LogicalVolume* fScintillatorHorizLV;
    void ConstructScintillatorBarLVs();

    G4int fNumScintBarsPerPanel = 40;
    G4LogicalVolume* fScintillatorVertPanelLV;
    G4LogicalVolume* fScintillatorHorizPanelLV;
    void ConstructScintillatorPanelLVs();

    G4double fIPTPixelBlockThickness = 0.0 * mm; // thickness of the Interface Pixel Tracker (IPT) pixel block
    G4LogicalVolume* fIPTPixelBlockLV;
    void ConstructIPTPixelBlockLV();
  
    // pixel module LV should be 3.6 cm thick, consisting of: 
    // - 2 mm Al Wall
    // - 5 mm Scintillator
    // - 5 mm Tungsten
    // - 12 mm Air Gap
    // - 10 um Silicon Sensor
    // - 5 mm Al Cooling Plate
    // - 5 mm Tungsten
    // - 2 mm Al Wall
    G4double fPixelAirGapThickness = 6 * mm; // thickness of air gap in pixel module
    G4LogicalVolume* fPixelModuleLV;
    G4LogicalVolume* fPixelLayerLV; // alias for the sensitive silicon LV; used for SD attachment and true-position bookkeeping
    G4double fPinpointBlockLength = 0.0 * mm; // length of each Pinpoint block, initially set to 0, will be computed based on the specified thicknesses
    G4double fPinpointBlockWidth = 0.0 * mm; // width of each Pinpoint block, initially set to 0, will be computed based on the specified thicknesses
    G4double fPinpointBlockHeight = 0.0 * mm; // height of each Pinpoint block, initially set to 0, will be computed based on the specified thicknesses
    void ConstructPixelModuleLV();

    G4double fScintThickness = 5 * mm; // thickness of a single scintillator panel in the scintillator (Fortune) module
    G4int fNumScintPanelsPerLayer = 2; // number of scintillator panels per scint layer: 0=none, 1=vertical only, 2=both (see ConstructScintModuleLV())
    G4double fScintDetectorWidth = 42 * cm; // scintillator panel X-extent: drives horizontal-bar length and vertical-bar pitch/segmentation
    G4double fScintDetectorHeight = 42 * cm; // scintillator panel Y-extent: drives vertical-bar length and horizontal-bar pitch/segmentation
    G4double fScintBarWidth = 10.0 * mm; // vertical bar's narrow (X-segmented) cross-axis width
    G4double fScintBarHeight = 10.0 * mm; // horizontal bar's narrow (Y-segmented) cross-axis width
    G4double fScintAirGapThickness = 0.5 * mm; // thickness of air gap in scintillator module
    G4double fFortuneModuleLength = 0.0 * mm; // length of each scintillator module, initially set to 0, will be computed based on the specified thicknesses
    G4double fFortuneModuleWidth = 0.0 * mm; // width of each scintillator module, initially set to 0, will be computed based on the specified thicknesses
    G4double fFortuneModuleHeight = 0.0 * mm; // height of each scintillator module, initially set to 0, will be computed based on the specified thickness
    G4int fNumScintLayersPerModule = 8; // number of scintillator layers per module
    G4LogicalVolume* scintModuleLV;
    void ConstructScintModuleLV();
    

    std::vector<G4LogicalVolume*> scintLVs;
    // std::vector<G4VPhysicalVolume*> fTarget_phys;
    G4VPhysicalVolume* fLayerPV;

    G4OpticalSurface* scintWrap;

    G4Material* scintillator = nullptr;
    G4Material* matFR4 = nullptr;

    static G4ThreadLocal MagneticField* fMagneticField;
    static G4ThreadLocal G4FieldManager* fFieldMgr;
};


#endif // pinpoint_DetectorConstruction_hh
