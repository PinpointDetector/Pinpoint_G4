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
    void PrintLayerVolumePositions();

  
    void SetReadFile(const G4String& File);
    void SetWriteFile(const G4String& File);
    // std::vector<G4VPhysicalVolume*> GetTargetPhysVols() const { return fTarget_phys; }
    G4VPhysicalVolume* GetLayerPhysVol() const { return fLayerPV; }

    G4int GetNFortuneBlocks() const { return fNFortuneBlocks; }
    G4int GetNPinpointBlocks() const { return fNPinpointBlocks; }
    G4int GetNLayers() const { return fNPinpointBlocks * 2 + fNFortuneBlocks * (fNumScintLayers + 1) + 1; }
    // G4int GetNumberOfLayers() const { return fNLayers; }

    void SetFortuneTungstenThickness(G4double thickness) { 
      if (thickness <= 0) {
        G4cerr << "Error: Tungsten thickness must be positive." << G4endl;
        return;
      }

      fFortuneTungstenThickness = thickness;
      G4cout << "Set tungsten thickness to " << fFortuneTungstenThickness/mm << " mm" << G4endl;

      // Optional: trigger geometry rebuild
      // G4RunManager::GetRunManager()->ReinitializeGeometry();
    }
    void SetSiliconThickness(G4double thickness) { fSiliconThickness = thickness; }
    void SetBoxThickness(G4double thickness) { fBoxThickness = thickness; }
    void SetPixelHeight(G4double height) { fPixelHeight = height; }
    void SetPixelWidth(G4double width) { fPixelWidth = width; }
    void SetPixelDetectorWidth(G4double width) { fPixelDetectorWidth = width; }
    void SetPixelDetectorHeight(G4double height) { fPixelDetectorHeight = height; }
    void SetCheckOverlaps(G4bool check) { fCheckOverlaps = check; }
    void SetGDMLFile(const G4String& filename) { fWriteFile = filename; }
    void SetNumScintPanelsPerLayer(G4int n) { fNumScintPanelsPerLayer = n; }
    void SetScintBarFlag(G4bool flag) { scint_bar_flag = flag; }
    void SetScintDetectorHeight(G4double height) { fScintDetectorHeight = height; }
    void SetScintDetectorWidth(G4double width) { fScintDetectorWidth = width; }
    void SetScintBarWidth(G4double w) { fScintBarWidth = w; }
    void SetScintBarHeight(G4double h) { fScintBarHeight = h; }
    void SetScintThickness(G4double t) { fScintThickness = t; }
    void SetNumScintLayers(G4int n) { fNumScintLayers = n; }
    void SetMaxDetectorThickness(G4double t) { fMaxDetectorThickness = t; }
    void SetPinpointThickness(G4double w) { fPinpointThickness = w; }
    void SetPinpointTungstenThickness(G4double t) { fPinpointTungstenThickness = t; }
    void SetEnableFaserSpectrometer(G4bool enable) { fEnableFaserSpectrometer = enable; }
    void SetNIPTLayers(G4int n) { fNIPTLayers = n; }
    G4int GetNIPTLayers() const { return fNIPTLayers; }


    std::vector<G4double> GetPixelXPositions() const {
      std::vector<G4double> xPositions;
      G4int nPixelsX = static_cast<G4int>(fPixelDetectorWidth / fPixelWidth);
      G4double startX = -fPixelDetectorWidth / 2 + fPixelWidth / 2;
      for (G4int i = 0; i < nPixelsX; ++i) {
        xPositions.push_back(startX + i * fPixelWidth);
      }
      return xPositions;
    }
    std::vector<G4double> GetPixelYPositions() const
    {
      std::vector<G4double> yPositions;
      G4int nPixelsY = static_cast<G4int>(fPixelDetectorHeight / fPixelHeight);
      G4double startY = -fPixelDetectorHeight / 2 + fPixelHeight / 2;
      for (G4int i = 0; i < nPixelsY; ++i) {
        yPositions.push_back(startY + i * fPixelHeight);
      }
      return yPositions;
    };
    std::vector<G4double> GetPixelZPositions() const
    {
      // FIXME
      std::vector<G4double> zPositions;
      // G4double startZ = -0.5 * fNLayers * fLayerThickness;
      // for (G4int i = 0; i < fNLayers; ++i) {
      //   // zPositions.push_back(startZ + i * fLayerThickness + fFortuneTungstenThickness + fSiliconThickness / 2);
      //   // FIXME!
      //   zPositions.push_back(startZ + i * fLayerThickness + 0.5 * fLayerThickness);
      // }
      return zPositions;
    };

    G4double GetFortuneTungstenThickness() const { return fFortuneTungstenThickness; }
    G4double GetSiliconThickness() const { return fSiliconThickness; }
    G4double GetBoxThickness() const { return fBoxThickness; }
    G4double GetPixelHeight() const { return fPixelHeight; }
    G4double GetPixelWidth() const { return fPixelWidth; }
    G4double GetPixelDetectorWidth() const { return fPixelDetectorWidth; }
    G4double GetPixelDetectorHeight() const { return fPixelDetectorHeight; }
    G4double GetLayerThickness() const { return fFortunePixelBlockThickness; }  // pixel-layer thickness
    G4double GetPixelLayerThickness() const { return fFortunePixelBlockThickness; }
    G4double GetScintLayerThickness() const { return fFortuneScintBlockThickness; }
    G4double GetScintDetectorWidth() const { return fScintDetectorWidth; }
    G4double GetScintBarWidth() const { return fScintBarWidth; }
    G4double GetScintBarHeight() const { return fScintBarHeight; }
    G4double GetScintPanelThickness() const { return fScintThickness; }
    // G4int GetTotalNLayers() const { return fTotalNLayers; }
    const std::vector<G4bool>& GetLayerIsPixel() const { return fLayerIsPixel; }
    G4int GetNumScintPanelsPerLayer() const { return fNumScintPanelsPerLayer; }
    G4int GetScintBarFlag() const {
    if (false) {
        return 0;
    }
    if (true) {
        return 1;
    }
    return -1; // fallback to avoid warnings
    }
    G4double GetScintDetectorHeight() const { return fScintDetectorHeight; }
    G4int GetNumScintLayers() const { return fNumScintLayers; }
    G4double GetMaxDetectorThickness() const { return fMaxDetectorThickness; }
    G4double GetPinpointThickness() const { return fPinpointThickness; }
    G4double GetPinpointTungstenThickness() const { return fPinpointTungstenThickness; }
    G4int GetNPinpointLayers() const { return fNPinpointBlocks; }

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
  
  private:
    
    G4double GetSiliconOffsetInLayer() const;
    void ComputeSiliconZPositions();
    std::vector<G4double> fSiliconZPositions;
    std::vector<G4double> fTungstenZPositions;
    std::vector<G4double> fTungstenThicknesses;
    std::vector<G4double> fScintZPositions;
    
    void ComputePixelCentersXY();
    std::vector<G4double> fPixelCenterX;
    std::vector<G4double> fPixelCenterY;

    void ComputeScintCentersXY();
    std::vector<G4double> fScintBarCenterX;  // vertical bars, segmented in X
    std::vector<G4double> fScintBarCenterY;  // horizontal bars, segmented in Y (world coords)

    G4String fWriteFile = "pinpoint.gdml";
    G4GDMLParser fParser;
    G4LogicalVolume* fPixelLayerLV;

    DetectorConstructionMessenger* messenger;

    G4double fFortuneTungstenThickness = 5 * mm;
    G4double fSiliconThickness = 10 * um;
    G4double fBoxThickness = 4.990 * mm;
    // G4int fNLayers = 100;
    G4double fPixelHeight = 22.8 * um;
    G4double fPixelWidth = 20.8 * um;
    G4double fPixelDetectorWidth = 26.6 * cm;
    G4double fPixelDetectorHeight = 19.6 * cm;
    G4double fScintBarWidth = 10.0 * mm;
    G4double fScintBarHeight = 10.0 * mm;
    // G4double fScintBarWidth = 9.85 * mm; 
    // G4double fScintBarHeight = 9.80 * mm;
    G4double fScintThickness = 5.0 * mm;
    G4double fScintDetectorHeight = 20 * cm;   // height of scintillator panels (may differ from fPixelDetectorHeight)
    G4double fScintDetectorWidth = 27 * cm;   // height of scintillator panels (may differ from fPixelDetectorHeight)
    G4int fNumScintLayers = 0;           // number of scint groups per detector layer (0 = no scintillators)
    G4double fLayerThickness = 0.0 * mm; // kept for backward compat; equals fFortunePixelBlockThickness
    G4double fFortunePixelBlockThickness = 0.0 * mm; // thickness of T + gap + P layer
    G4double fPinpointTungstenThickness = 1.0 * mm;   // tungsten thickness for the Pinpoint sub-detector
    G4double fPinpointPixelBlockThickness = 0.0 * mm; // computed: T_pp + gap + P
    G4double fPinpointScintBlockThickness = 0.0 * mm; // computed: T_pp + S
    G4double fFortuneScintBlockThickness = 0.0 * mm; // thickness of T + N_panels*S layer
    // G4int fTotalNLayers = 0;             // total physical layers (pixel + scint)
    std::vector<G4bool> fLayerIsPixel;   // true if layer i (in sequence) is a pixel layer
    G4int fNumScintPanelsPerLayer = 1;  // 0=no scint, 1=single panel, 2=double panel
    G4double fMaxDetectorThickness = 111.5 * cm;
    G4double fPinpointThickness = 10.5 * cm;  // if >0, initial pixel-only section (no scint)
    G4int fNPinpointBlocks = 4;           // computed: floor(fPinpointThickness / fFortunePixelBlockThickness)
    G4int fNFortuneBlocks = 6;           // computed: floor(fPinpointThickness / fFortunePixelBlockThickness)
    G4bool scint_bar_flag = true;
    G4bool fEnableFaserSpectrometer = true;
    G4double fLongMagnetLength = 1500.0 * mm;
    G4double fShortMagnetLength = 1000.0 * mm;
    G4double fInnerRadius = 100.0 * mm;
    G4double fOuterRadius = 215.0 * mm;
    // Position of FASER magnets and tracking stations relative to VetoNu scintillator
    G4double fVetoNuPosition = -3112 * mm;
    G4double fMagnet0Position = -815.3 * mm - fVetoNuPosition;
    G4double fMagnet1Position = 637.4 * mm - fVetoNuPosition;
    G4double fMagnet2Position = 1837.4 * mm - fVetoNuPosition;
    G4double fTrackerSize = 250.0 * mm;
    G4double fTracker1Position = 47.4 * mm - fVetoNuPosition;
    G4double fTracker2Position = 1237.4 * mm - fVetoNuPosition;
    G4double fTracker3Position = 2427.4 * mm - fVetoNuPosition;
    G4int fNIPTLayers = 3;

    G4bool fCheckOverlaps = true;

    G4int fNPixelsX;
    G4int fNPixelsY;

    std::vector<G4LogicalVolume*> scintLVs;
    // std::vector<G4VPhysicalVolume*> fTarget_phys;
    G4VPhysicalVolume* fLayerPV;

    G4OpticalSurface* scintWrap;

    G4Material* scintillator = nullptr;

    static G4ThreadLocal MagneticField* fMagneticField;
    static G4ThreadLocal G4FieldManager* fFieldMgr;
};


#endif // pinpoint_DetectorConstruction_hh