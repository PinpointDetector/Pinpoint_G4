#ifndef pinpoint_DetectorConstruction_hh
#define pinpoint_DetectorConstruction_hh

#include "G4VUserDetectorConstruction.hh"
#include "G4GDMLParser.hh"
#include "G4SystemOfUnits.hh"
#include "DetectorConstructionMessenger.hh"
#include "G4RunManager.hh"
#include "G4OpticalSurface.hh"

class G4VPhysicalVolume;

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
    std::vector<G4VPhysicalVolume*> GetTargetPhysVols() const { return fTarget_phys; }
    G4VPhysicalVolume* GetLayerPhysVol() const { return fLayerPV; }

    G4int GetNlayers() const { return fTarget_phys.size(); }

    void SetTungstenThickness(G4double thickness) { 
      if (thickness <= 0) {
        G4cerr << "Error: Tungsten thickness must be positive." << G4endl;
        return;
    }

    fTungstenThickness = thickness; 
    G4cout << "Set tungsten thickness to " << fTungstenThickness/mm << " mm" << G4endl;

    // Optional: trigger geometry rebuild
    // G4RunManager::GetRunManager()->ReinitializeGeometry();
}
    void SetSiliconThickness(G4double thickness) { fSiliconThickness = thickness; }
    void SetNLayers(G4int nLayers) { fNLayers = nLayers; }
    void SetPixelHeight(G4double height) { fPixelHeight = height; }
    void SetPixelWidth(G4double width) { fPixelWidth = width; }
    void SetDetectorWidth(G4double width) { fDetectorWidth = width; }
    void SetDetectorHeight(G4double height) { fDetectorHeight = height; }
    void SetCheckOverlaps(G4bool check) { fCheckOverlaps = check; }
    void SetGDMLFile(const G4String& filename) { fWriteFile = filename; }
    void SetSimFlag(G4int flag) { sim_flag = flag; }
    void SetScintBarFlag(G4bool flag) { scint_bar_flag = flag; }


    std::vector<G4double> GetPixelXPositions() const {
      std::vector<G4double> xPositions;
      G4int nPixelsX = static_cast<G4int>(fDetectorWidth / fPixelWidth);
      G4double startX = -fDetectorWidth / 2 + fPixelWidth / 2;
      for (G4int i = 0; i < nPixelsX; ++i) {
        xPositions.push_back(startX + i * fPixelWidth);
      }
      return xPositions;
    }
    std::vector<G4double> GetPixelYPositions() const
    {
      std::vector<G4double> yPositions;
      G4int nPixelsY = static_cast<G4int>(fDetectorHeight / fPixelHeight);
      G4double startY = -fDetectorHeight / 2 + fPixelHeight / 2;
      for (G4int i = 0; i < nPixelsY; ++i) {
        yPositions.push_back(startY + i * fPixelHeight);
      }
      return yPositions;
    };
    std::vector<G4double> GetPixelZPositions() const
    {
      std::vector<G4double> zPositions;
      G4double layerSpacing = fTungstenThickness + fSiliconThickness;
      G4double startZ = -((fNLayers - 1) * layerSpacing) / 2;
      for (G4int i = 0; i < fNLayers; ++i) {
        zPositions.push_back(startZ + i * layerSpacing);
      }
      return zPositions;
    };

    G4double GetTungstenThickness() const { return fTungstenThickness; }
    G4double GetSiliconThickness() const { return fSiliconThickness; }
    G4int GetNumberOfLayers() const { return fNLayers; }
    G4double GetPixelHeight() const { return fPixelHeight; }
    G4double GetPixelWidth() const { return fPixelWidth; }
    G4double GetDetectorWidth() const { return fDetectorWidth; }
    G4double GetDetectorHeight() const { return fDetectorHeight; }
    G4int GetSimFlag() const {return sim_flag;}
    G4int GetScintBarFlag() const {
    if (false) {
        return 0;
    }
    if (true) {
        return 1;
    }
    return -1; // fallback to avoid warnings
    }

    

  private:
    G4String fWriteFile = "pinpoint.gdml";
    G4GDMLParser fParser;
    G4LogicalVolume* fPixelLV;

    DetectorConstructionMessenger* messenger;

    G4double fTungstenThickness = 5 * mm;
    G4double fSiliconThickness = 50 * um;
    G4int fNLayers = 100;
    G4double fPixelHeight = 22.8 * um;
    G4double fPixelWidth = 20.8 * um;
    G4double fDetectorWidth = 26.6 * cm;
    G4double fDetectorHeight = 19.6 * cm;
    // G4double fScintBarWidth = 10.0 * mm;
    // G4double fScintBarHeight = 10.0 * mm;
    G4double fScintBarWidth = 9.85 * mm; 
    G4double fScintBarHeight = 9.80 * mm;
    G4double fScintThickness = 5.0 * mm;
    G4int sim_flag = 0;
    G4bool scint_bar_flag = true;

    G4bool fCheckOverlaps = true;

    std::vector<G4LogicalVolume*> scintLVs;
    std::vector<G4VPhysicalVolume*> fTarget_phys;
    G4VPhysicalVolume* fLayerPV;

    G4OpticalSurface* scintWrap;

    G4Material* scintillator = nullptr;
};


#endif // pinpoint_DetectorConstruction_hh