#ifndef ANALYSISMANAGER_HH
#define ANALYSISMANAGER_HH

#include <set>
#include <vector>
#include <string>

#include "G4Event.hh"
#include "TFile.h"
#include "TTree.h"
#include "TH2F.h"


#include "AnalysisManagerMessenger.hh"
#include "FPFParticle.hh"

class AnalysisManager {
  public:

    AnalysisManager();
    ~AnalysisManager();
    static AnalysisManager* GetInstance();

    //------------------------------------------------
    // Functions executed at specific times
    void BeginOfRun(); 
    void EndOfRun();
    void BeginOfEvent();
    void EndOfEvent(const G4Event* event);

    //------------------------------------------------
    // functions for controlling from the configuration file
    void setFileName(std::string val) { fFilename = val; }
    void saveTrack(G4bool val) { fSaveTrack = val; }
    void saveTruthHits(G4bool val) { fSaveTruthHits = val; }

    // build TID to primary ancestor association
    // filled progressively from StackingAction
    void SetTrackPrimaryAncestor(G4int trackID, G4int ancestorID) { trackToPrimaryAncestor[trackID] = ancestorID; }
    G4int GetTrackPrimaryAncestor(G4int trackID) { return trackToPrimaryAncestor.at(trackID); }

    // TODO: needed???
    void AddOnePrimaryTrack() { nTestNPrimaryTrack++; }

  private:

    //------------------------------------------------
    // Book ROOT output TTrees
    // common + detector specific
    void bookEvtTree();
    void bookTrkTree();
    void bookPrimTree();
    void bookGeomTree();
    void bookHitsTrees();
    void bookScintTrees();

    void FillEventTree(const G4Event* event);
    void FillPrimariesTree(const G4Event* event);
    void FillTrajectoriesTree(const G4Event* event);
    void FillGeomTree();
    void FillHitsOutput();
    void FillScintOutput();
    
    float_t GetTotalEnergy(float_t px, float_t py, float_t pz, float_t m);

    static AnalysisManager* fInstance;
    AnalysisManagerMessenger* fMessenger{nullptr};

    G4bool fSaveTrack;
    G4bool fSaveTruthHits;
    
    std::map<int, std::string> fSDNamelist;

    G4HCofThisEvent* fHCofEvent{nullptr};
    
    G4int nPrimaryVertex;
    std::vector<FPFParticle> primaries;
    std::vector<int> primaryIDs;

    //------------------------------------------------
    // output files and trees
    std::string fFilename;
    TFile*   fFile;
    TTree*   fEvt;
    TTree*   fTrk;
    TTree*   fPrim;
    TTree*   fGeom;

    TDirectory* fHits;
    TTree*   fPixelHitsTree;

    TTree* fScintTree = nullptr;

    // track to primary ancestor
    std::map<G4int, G4int> trackToPrimaryAncestor;

    // TODO: no longer needed?
    G4int nTestNPrimaryTrack;

    //---------------------------------------------------
    // OUTPUT VARIABLES FOR COMMON TREES

    G4int evtID;
    G4int vertexID;
    double weight;
    std::string genType;
    std::string processName;    
    int initPDG;           
    double initX, initY, initZ, initT;
    double initPx, initPy, initPz, initE;
    double initM;     
    double initQ;    
    int intType;           
    int scatteringType;    
    int fslPDG;           
    int tgtPDG;     
    int tgtA;      
    int tgtZ;      
    int hitnucPDG; 
    double xs;
    double Q2;  
    double xBj; 
    double y;   
    double W;  
 
    //---------------------------------------------------
    // Output variables for TRAJECTORIES tree
    int trackTID;
    int trackPID;
    int trackPDG;
    double trackKinE;
    int trackNPoints;
    std::vector<double> trackPointX;
    std::vector<double> trackPointY;
    std::vector<double> trackPointZ;

    //---------------------------------------------------
    // Output variables for PRIMARIES tree
    UInt_t primVtxID;
    UInt_t primParticleID;
    UInt_t primTrackID;
    UInt_t primPDG; // why unsigned?
    float_t primM;
    float_t primQ;
    float_t primEta;
    float_t primPhi;
    float_t primPt;
    float_t primP;
    float_t primVx;
    float_t primVy;
    float_t primVz;
    float_t primVt;
    float_t primPx;
    float_t primPy;
    float_t primPz;
    float_t primE;
    float_t primKE;

    //---------------------------------------------------
    // Output variables for GEOMETRY tree
    float_t detectorWidth;
    float_t detectorHeight;
    float_t tungstenThickness;
    float_t siliconThickness;
    Int_t nLayers;
    Int_t simFlag;
    Int_t scintBarFlag;
    std::vector<double_t> pixelsXPos;
    std::vector<double_t> pixelsYPos;
    std::vector<double_t> pixelsZPos;

    //---------------------------------------------------
    // OUTPUT VARIABLES FOR Hits TREES

    //* Reco space points
    UInt_t fPixelEventID;
    std::vector<UInt_t> fPixelRowIDs;
    std::vector<UInt_t> fPixelColIDs;
    std::vector<UInt_t> fPixelLayerIDs;
    std::vector<UInt_t> fPixelPDGCs;
    std::vector<UInt_t> fPixelTrackIDs;
    std::vector<UInt_t> fPixelParentIDs;
    std::vector<Float_t> fPixelPxs;
    std::vector<Float_t> fPixelPys;
    std::vector<Float_t> fPixelPzs;
    std::vector<Float_t> fPixelEnergies;
    std::vector<Float_t> fPixelCharges;
    std::vector<Float_t> fPixelEDep;
    // std::vector<G4bool> fPixelFromPrimaryPizero;
    // std::vector<G4bool> fPixelFromFSLPizero;
    std::vector<G4bool> fPixelFromPrimaryLepton;

    // Truth position of hit in x, y, z
    std::vector<Float_t> fPixelTruthX;
    std::vector<Float_t> fPixelTruthY;
    std::vector<Float_t> fPixelTruthZ;

    //----------------------------------------------------
    //OUTPUT VARIABLES FOR SCINTILLATOR
    UInt_t fScintEventID;
    std::vector<int> fScintLayerID;
    std::vector<int> fScintTrackID;
    std::vector<int> fScintParentID;
    std::vector<int> fScintPDG;
    std::vector<float> fScintEdep;
    std::vector<int> fScintFromMuon;
    std::vector<int> fScintFromPrimaryLepton;
    
  };

#endif
