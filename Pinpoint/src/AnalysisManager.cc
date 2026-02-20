#include <functional>
#include <iostream>
#include <string>
#include <map>
#include <iomanip>
#include <random>

#include <G4Event.hh>
#include <G4SDManager.hh>
#include <G4SystemOfUnits.hh>
#include <Randomize.hh>
#include <G4Poisson.hh>
#include <G4Trajectory.hh>
#include <G4LorentzVector.hh>
#include "G4SDManager.hh"
#include "G4THitsCollection.hh"
#include "G4VVisManager.hh"
#include "G4Circle.hh"


#include <TDirectory.h>
#include <TFile.h>
#include <TTree.h>
#include <TH2F.h>
#include <THnSparse.h>
#include <TString.h>
#include <Math/ProbFunc.h>
#include "G4RunManager.hh"
#include "DetectorConstruction.hh"
#include "EventInformation.hh"
#include "AnalysisManager.hh"
#include "reco/Barcode.hh"
#include "FPFParticle.hh"
#include "PixelHit.hh"
#include "ScintHit.hh"
#include "FaserHit.hh"


//---------------------------------------------------------------------
//---------------------------------------------------------------------
// AnalysisManager "singleton" instance
// once initialized, can be used to point to AnalysisManager
// from anywhere else in the codebase
AnalysisManager *AnalysisManager::fInstance = 0;

AnalysisManager *AnalysisManager::GetInstance()
{
  if (!fInstance)
  {
    G4cout << "AnalysisManager: Re-initialization" << G4endl;
    fInstance = new AnalysisManager();
  }
  return fInstance;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
AnalysisManager::AnalysisManager()
{
  fFile = nullptr;
  fFilename = "test.root";

  fMessenger = new AnalysisManagerMessenger(this);

  fEvt = nullptr;
  fTrk = nullptr;
  fPrim = nullptr;
  fPixelHitsTree = nullptr;
  fTauTree = nullptr;
  fCharmTree = nullptr;
  // fActsParticlesTree = nullptr;
  
  fSaveTrack = false;
}

AnalysisManager::~AnalysisManager() {}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

void AnalysisManager::bookEvtTree()
{
  fEvt = new TTree("event", "event info");
  fEvt->Branch("evtID", &evtID, "evtID/I");
  fEvt->Branch("vtxID", &vertexID, "vtxID/I");
  fEvt->Branch("weight", &weight, "weight/D");
  fEvt->Branch("genType", &genType);
  fEvt->Branch("processName", &processName);
  fEvt->Branch("initPDG", &initPDG, "initPDG/I");
  fEvt->Branch("initX", &initX, "initX/D");
  fEvt->Branch("initY", &initY, "initY/D");
  fEvt->Branch("initZ", &initZ, "initZ/D");
  fEvt->Branch("initT", &initT, "initT/D");
  fEvt->Branch("initPx", &initPx, "initPx/D");
  fEvt->Branch("initPy", &initPy, "initPy/D");
  fEvt->Branch("initPz", &initPz, "initPz/D"); 
  fEvt->Branch("initE", &initE, "initE/D");
  fEvt->Branch("initM", &initM, "initM/D");
  fEvt->Branch("initQ", &initQ, "initQ/D");
  fEvt->Branch("intType", &intType, "intType/I");
  fEvt->Branch("scatteringType", &scatteringType, "scatteringType/I");
  fEvt->Branch("fslPDG", &fslPDG, "fslPDG/I");
  fEvt->Branch("tgtPDG", &tgtPDG, "tgtPDG/I");
  fEvt->Branch("tgtA", &tgtA, "tgtA/I");
  fEvt->Branch("tgtZ", &tgtZ, "tgtZ/I");
  fEvt->Branch("hitnucPDG", &hitnucPDG, "hitnucPDG/I");
  fEvt->Branch("xs", &xs, "xs/D");
  fEvt->Branch("Q2", &Q2, "Q2/D");
  fEvt->Branch("xBj", &xBj, "xBj/D");
  fEvt->Branch("y", &y, "y/D");
  fEvt->Branch("W", &W, "W/D");
}

void AnalysisManager::bookPrimTree()
{
  fPrim = new TTree("primaries", "primaries info");
  fPrim->Branch("evtID", &evtID, "evtID/I");
  fPrim->Branch("vtxID", &primVtxID, "vtxID/I");
  fPrim->Branch("PDG", &primPDG, "PDG/I");
  fPrim->Branch("trackID", &primTrackID, "trackID/I");
  fPrim->Branch("barcode", &primParticleID, "bardcode/I");
  fPrim->Branch("mass", &primM, "mass/F");
  fPrim->Branch("charge", &primQ, "charge/F");
  fPrim->Branch("Vx", &primVx, "Vx/F"); // position
  fPrim->Branch("Vy", &primVy, "Vy/F");
  fPrim->Branch("Vz", &primVz, "Vz/F");
  fPrim->Branch("Vt", &primVt, "Vt/F");
  fPrim->Branch("Px", &primPx, "Px/F"); // momentum
  fPrim->Branch("Py", &primPy, "Py/F");
  fPrim->Branch("Pz", &primPz, "Pz/F");
  fPrim->Branch("E", &primE, "E/F");    // initial total energy
  fPrim->Branch("KE", &primKE, "KE/F"); // initial kinetic energy
  fPrim->Branch("Eta", &primEta, "Eta/F");
  fPrim->Branch("Phi", &primPhi, "Phi/F");
  fPrim->Branch("Pt", &primPt, "Pt/F");
  fPrim->Branch("P", &primP, "P/F");
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

void AnalysisManager::bookTrkTree()
{
  fTrk = new TTree("trajectories", "trajectories info");
  fTrk->Branch("evtID", &evtID, "evtID/I");
  fTrk->Branch("trackTID", &trackTID, "trackTID/I");
  fTrk->Branch("trackPID", &trackPID, "trackPID/I");
  fTrk->Branch("trackPDG", &trackPDG, "trackPDG/I");
  fTrk->Branch("trackKinE", &trackKinE, "trackKinE/D");
  fTrk->Branch("trackNPoints", &trackNPoints, "trackNPoints/I");
  fTrk->Branch("trackTheta", &trackTheta, "trackTheta/D");
  fTrk->Branch("trackProdX", &trackProdX, "trackProdX/D");
  fTrk->Branch("trackProdY", &trackProdY, "trackProdY/D");
  fTrk->Branch("trackProdZ", &trackProdZ, "trackProdZ/D");
  fTrk->Branch("trackDecayX", &trackDecayX, "trackDecayX/D");
  fTrk->Branch("trackDecayY", &trackDecayY, "trackDecayY/D");
  fTrk->Branch("trackDecayZ", &trackDecayZ, "trackDecayZ/D");
}


//---------------------------------------------------------------------
//---------------------------------------------------------------------
void AnalysisManager::bookGeomTree()
{
  fGeom = new TTree("geometry", "geometry info");
  fGeom->Branch("detector_width", &detectorWidth, "detectorWidth/F");
  fGeom->Branch("detector_height", &detectorHeight, "detectorHeight/F");
  fGeom->Branch("tungsten_thickness", &tungstenThickness, "tungstenThickness/F");
  fGeom->Branch("silicon_thickness", &siliconThickness, "siliconThickness/F");
  fGeom->Branch("nLayers", &nLayers, "nLayers/I");
  fGeom->Branch("pixel_Xpos", &pixelsXPos);
  fGeom->Branch("pixel_Ypos", &pixelsYPos);
  fGeom->Branch("pixel_Zpos", &pixelsZPos);
  fGeom->Branch("sim_flag", &simFlag, "simFlag/I");
  fGeom->Branch("scint_bar_flag", &scintBarFlag, "scintBarFlag/F");
}



//---------------------------------------------------------------------
//---------------------------------------------------------------------

void AnalysisManager::bookHitsTrees()
{
  // create subdirectory in file
  fHits = fFile->mkdir("Hits","Hits output",kTRUE);
  fFile->cd(fHits->GetName());

  //* Reco Hits Tree [i == unsigned int; F == float; l == Long unsigned 64 int]
  fPixelHitsTree = new TTree("pixelHits", "pixelHits_Tree");
  fPixelHitsTree->Branch("event_id", &fPixelEventID, "event_id/i");
  fPixelHitsTree->Branch("hit_rowID", &fPixelRowIDs);
  fPixelHitsTree->Branch("hit_colID", &fPixelColIDs);
  fPixelHitsTree->Branch("hit_layerID", &fPixelLayerIDs);
  fPixelHitsTree->Branch("hit_pdgc", &fPixelPDGCs);
  fPixelHitsTree->Branch("hit_trackID", &fPixelTrackIDs);
  fPixelHitsTree->Branch("hit_parentID", &fPixelParentIDs);
  // fPixelHitsTree->Branch("hit_px", &fPixelPxs);
  // fPixelHitsTree->Branch("hit_py", &fPixelPys);
  // fPixelHitsTree->Branch("hit_pz", &fPixelPzs);
  // fPixelHitsTree->Branch("hit_energy", &fPixelEnergies);
  // fPixelHitsTree->Branch("hit_charge", &fPixelCharges);
  fPixelHitsTree->Branch("hit_edep", & fPixelEDep);
  // fPixelHitsTree->Branch("hit_fromPrimaryPizero", &fPixelFromPrimaryPizero);
  // fPixelHitsTree->Branch("hit_fromFSLPizero", &fPixelFromFSLPizero);
  fPixelHitsTree->Branch("hit_fromPrimaryLepton", &fPixelFromPrimaryLepton);
  fPixelHitsTree->Branch("hit_fromPrimaryEMShower", &fPixelFromPrimaryEMShower);

  // if (fSaveTruthHits)
  // {
  //   fPixelHitsTree->Branch("hit_truth_x", &fPixelTruthX);
  //   fPixelHitsTree->Branch("hit_truth_y", &fPixelTruthY);
  //   fPixelHitsTree->Branch("hit_truth_z", &fPixelTruthZ);
  // }

  fFile->cd();
}

//// --- NEW FOR SCINTILLATORS ---
void AnalysisManager::bookScintTrees()
{
    fFile->cd(fHits->GetName());

    fScintTree = new TTree("scintHits", "scintillator hits");

    fScintTree->Branch("event_id", &fScintEventID, "event_id/i");
    fScintTree->Branch("layerID", &fScintLayerID);
    fScintTree->Branch("colID", &fScintColID);
    fScintTree->Branch("rowID", &fScintRowID);
    fScintTree->Branch("trackID", &fScintTrackID);
    fScintTree->Branch("parentID", &fScintParentID);
    fScintTree->Branch("pdg", &fScintPDG);
    fScintTree->Branch("edep", &fScintEdep);
    fScintTree->Branch("fromMuon", &fScintFromMuon);
    fScintTree->Branch("fromPrimaryLepton", &fScintFromPrimaryLepton);
}

//// --- FASER tracking spectrometer hits ---
void AnalysisManager::bookFaserTree()
{
    fFile->cd(fHits->GetName());

    fFaserHitsTree = new TTree("faserHits", "FASER spectrometer hits");
    fFaserHitsTree->Branch("event_id", &fFaserEventID, "event_id/i");
    fFaserHitsTree->Branch("trackerID", &fFaserTrackerID);
    fFaserHitsTree->Branch("trackID", &fFaserTrackID);
    fFaserHitsTree->Branch("pdg", &fFaserPDG);
    fFaserHitsTree->Branch("x", &fFaserX);
    fFaserHitsTree->Branch("y", &fFaserY);
    fFaserHitsTree->Branch("z", &fFaserZ);
    fFaserHitsTree->Branch("px", &fFaserPx);
    fFaserHitsTree->Branch("py", &fFaserPy);
    fFaserHitsTree->Branch("pz", &fFaserPz);
    fFaserHitsTree->Branch("energy", &fFaserE);
    fFaserHitsTree->Branch("edep", &fFaserEdep);
    fFaserHitsTree->Branch("charge", &fFaserCharge);
}

//// --- Children of primary tau decay ---
void AnalysisManager::bookTauTree()
{
  fTauTree = new TTree("tau", "Children of tau decay");
  fTauTree->Branch("evtID", &fPythiaEventID, "evtID/i");
  fTauTree->Branch("PID", &fPythiaPID, "PID/I");
  fTauTree->Branch("TID", &fPythiaTID, "TID/I");
  fTauTree->Branch("PDG", &fPythiaTPDG, "PDG/I");
  fTauTree->Branch("ParentPDG", &fPythiaPPDG, "ParentPDG/I");
  fTauTree->Branch("ProdX", &fPythiaProdX, "ProdX/D");
  fTauTree->Branch("ProdY", &fPythiaProdY, "ProdY/D");
  fTauTree->Branch("ProdZ", &fPythiaProdZ, "ProdZ/D");
  fTauTree->Branch("DecayX", &fPythiaDecayX, "DecayX/D");
  fTauTree->Branch("DecayY", &fPythiaDecayY, "DecayY/D");
  fTauTree->Branch("DecayZ", &fPythiaDecayZ, "DecayZ/D");
  fTauTree->Branch("Px", &fPythiaPx, "Px/D");
  fTauTree->Branch("Py", &fPythiaPy, "Py/D");
  fTauTree->Branch("Pz", &fPythiaPz, "Pz/D");
}

//// --- Children of primary tau decay ---
void AnalysisManager::bookCharmTree()
{
  fCharmTree = new TTree("charm", "Children of tau decay");
  fCharmTree->Branch("evtID", &fPythiaEventID, "evtID/i");
  fCharmTree->Branch("PID", &fPythiaPID, "PID/I");
  fCharmTree->Branch("TID", &fPythiaTID, "TID/I");
  fCharmTree->Branch("PDG", &fPythiaTPDG, "PDG/I");
  fCharmTree->Branch("ParentPDG", &fPythiaPPDG, "ParentPDG/I");
  fCharmTree->Branch("ProdX", &fPythiaProdX, "ProdX/D");
  fCharmTree->Branch("ProdY", &fPythiaProdY, "ProdY/D");
  fCharmTree->Branch("ProdZ", &fPythiaProdZ, "ProdZ/D");
  fCharmTree->Branch("DecayX", &fPythiaDecayX, "DecayX/D");
  fCharmTree->Branch("DecayY", &fPythiaDecayY, "DecayY/D");
  fCharmTree->Branch("DecayZ", &fPythiaDecayZ, "DecayZ/D");
  fCharmTree->Branch("Px", &fPythiaPx, "Px/D");
  fCharmTree->Branch("Py", &fPythiaPy, "Py/D");
  fCharmTree->Branch("Pz", &fPythiaPz, "Pz/D");
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

void AnalysisManager::BeginOfRun()
{
  G4cout << "Run has been started, preparing output" << G4endl;

  if (fFile)
    delete fFile;

  // Preparing output file
  fFile = new TFile(fFilename.c_str(), "RECREATE");
  
  // Booking common output trees
  bookEvtTree();
  bookPrimTree();
  bookGeomTree();
  if (fSaveTrack) bookTrkTree();
  bookTauTree();
  bookCharmTree();

  bookHitsTrees();
  bookScintTrees();
  bookFaserTree();

}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

void AnalysisManager::EndOfRun()
{
  G4cout << "Run has ended, closing output" << G4endl;
  // save common trees at the top of the output file
  fFile->cd();
  fEvt->Write();
  fPrim->Write();
  fTauTree->Write();
  fCharmTree->Write();
  FillGeomTree();
  fGeom->Write();
  if (fSaveTrack) fTrk->Write();

  fFile->cd(fHits->GetName());
  fPixelHitsTree->Write();
  fScintTree->Write();
  fFaserHitsTree->Write();

  // fActsParticlesTree->Write();
  fFile->cd(); // go back to top

  fFile->Close();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

void AnalysisManager::BeginOfEvent()
{
  G4cout << "Starting new event, resetting variables" << G4endl;
  // reset vectors that need to be cleared for a new event
  // only reset arrays or vectors, tipically no need for other defaults

  primaries.clear();
  primaryIDs.clear();

  // track ID to primary ancestor association
  trackToPrimaryAncestor.clear();

  // trackPointX.clear();
  // trackPointY.clear();
  // trackPointZ.clear();

  fPixelRowIDs.clear();
  fPixelColIDs.clear();
  fPixelLayerIDs.clear();
  fPixelPDGCs.clear();
  fPixelTrackIDs.clear();
  fPixelParentIDs.clear();
  // fPixelPxs.clear();
  // fPixelPys.clear();
  // fPixelPzs.clear();
  // fPixelEnergies.clear();
  // fPixelCharges.clear();
  fPixelEDep.clear();
  // fPixelFromPrimaryPizero.clear();
  // fPixelFromFSLPizero.clear();
  fPixelFromPrimaryLepton.clear();
  fPixelFromPrimaryEMShower.clear();
  // fPixelTruthX.clear();
  // fPixelTruthY.clear();
  // fPixelTruthZ.clear();

  //clean scintillator hits
  //// --- NEW FOR SCINTILLATORS ---
  fScintLayerID.clear();
  fScintColID.clear();
  fScintRowID.clear();
  fScintTrackID.clear();
  fScintParentID.clear();
  fScintPDG.clear();
  fScintEdep.clear();
  fScintFromMuon.clear();
  fScintFromPrimaryLepton.clear();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

void AnalysisManager::EndOfEvent(const G4Event *event)
{
  G4cout << "Ending event, filling output trees" << G4endl;
  /// evtID
  evtID = event->GetEventID();

  // FILL EVENT TREE
  FillEventTree(event);

  //-----------------------------------------------------------

  // FILL PRIMARIES/TRAJECTORIES TREE
  FillPrimariesTree(event);

  fPythiaEventID = evtID;
  FillPythiaTree(event, fTauTree, {15});
  FillPythiaTree(event, fCharmTree, {411, 421, 431, 4122, 4112, 4212, 4222, 4132, 4232});
  if(fSaveTrack) FillTrajectoriesTree(event);

  //-----------------------------------------------------------

  // Get the hit collections
  // If there is no hit collection, there is nothing to be done
  fHCofEvent = event->GetHCofThisEvent();
  if (!fHCofEvent)
  {
    G4cout << "No hits recorded in any sensitive volume --> nothing to save!" << G4endl;
    return;
  }

  FillHitsOutput();
  FillScintOutput();
  FillFaserOutput();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

void AnalysisManager::FillEventTree(const G4Event *event)
{
  G4cout << "Filling event tree" << G4endl;
  EventInformation* eventInfo = static_cast<EventInformation*>(event->GetUserInformation());
  eventInfo->Print();
  auto metadata = eventInfo->GetEventMetadata();
  for(int i=0; i<metadata.size(); i++)
  {
    vertexID = i;
    weight = metadata[i].weight;
    genType = metadata[i].generatorType;
    processName = metadata[i].processName;
    initPDG = metadata[i].pdg;
    initX = metadata[i].x4.x();
    initY = metadata[i].x4.y();
    initZ = metadata[i].x4.z();
    initT = metadata[i].x4.t();
    initPx = metadata[i].p4.x();
    initPy = metadata[i].p4.y();
    initPz = metadata[i].p4.z();
    initE = metadata[i].p4.e();
    initM = metadata[i].mass;
    initQ = metadata[i].charge;
    intType = metadata[i].intType;     
    scatteringType = metadata[i].scatteringType;   
    fslPDG = metadata[i].fsl_pdg;           
    tgtPDG = metadata[i].tgt_pdg;  
    tgtZ = metadata[i].tgt_Z;     
    tgtA = metadata[i].tgt_A;     
    hitnucPDG = metadata[i].hitnuc_pdg;  
    xs = metadata[i].xs;
    Q2 = metadata[i].Q2;  
    xBj = metadata[i].xBj;
    y = metadata[i].y; 
    W = metadata[i].W; 

    fEvt->Fill();
  }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

void AnalysisManager::FillPrimariesTree(const G4Event *event)
{
  G4cout << "Filling primaries tree" << G4endl;
  nPrimaryVertex = event->GetNumberOfPrimaryVertex();
  G4cout << "\nNumber of primary vertices  : " << nPrimaryVertex << G4endl;
  
  /// loop over the vertices, and then over primary particles,
  /// neutrino truth info from event generator.
  for (G4int ivtx = 0; ivtx < event->GetNumberOfPrimaryVertex(); ++ivtx)
  {
    G4cout << "=== Vertex " << ivtx+1 << " of " << nPrimaryVertex << " -> " 
           << event->GetPrimaryVertex(ivtx)->GetNumberOfParticle() << " primaries ===" << G4endl;
    for (G4int ipp = 0; ipp < event->GetPrimaryVertex(ivtx)->GetNumberOfParticle(); ++ipp)
    {
      G4PrimaryParticle *primary_particle = event->GetPrimaryVertex(ivtx)->GetPrimary(ipp);
      if (primary_particle)
      {
 
        primVtxID = ivtx;
        primTrackID = ipp + 1; // confirm matches track id?

        auto particleId = ActsFatras::Barcode();
        particleId.setVertexPrimary(ivtx);
        particleId.setGeneration(0);
        particleId.setSubParticle(0);
        particleId.setParticle(primTrackID - 1);

        primParticleID = particleId.value();
        primPDG = primary_particle->GetPDGcode();
        primVx = event->GetPrimaryVertex(ivtx)->GetPosition().x();
        primVy = event->GetPrimaryVertex(ivtx)->GetPosition().y();
        primVz = event->GetPrimaryVertex(ivtx)->GetPosition().z();
        primVt = event->GetPrimaryVertex(ivtx)->GetT0();
        primPx = primary_particle->GetMomentum().x();
        primPy = primary_particle->GetMomentum().y();
        primPz = primary_particle->GetMomentum().z();
        primM = primary_particle->GetMass()/MeV;
        primQ = primary_particle->GetCharge();

        G4double energy = GetTotalEnergy(primPx, primPy, primPz, primM);
        G4LorentzVector p4(primPx,primPy,primPz,energy);
        primEta = p4.eta();
        primPhi = p4.phi();
        primPt = p4.perp();
        primP = p4.vect().mag();
        primE = energy;
        primKE = energy - primM;

        // store a copy as a FPFParticle for further processing
        primaryIDs.push_back(primTrackID); //store to avoid duplicates
        primaries.push_back(FPFParticle(primPDG, 0, 
		                        primTrackID, primaryIDs.size()-1, 1,
		                        primM,
                            primVx, primVy, primVz, primVt,
                            primPx, primPy, primPz,energy));

        // G4cout << G4endl;
        // G4cout << "PrimaryParticleInfo: PDG code " << primPDG << G4endl
        //   << "Particle unique ID : " << primTrackID << G4endl
        //   << "Momentum : (" << primPx << ", " << primPy << ", " << primPz << ") MeV" << G4endl
        //   << "Vertex : (" << primVx << ", " << primVy << ", " << primVz << ") mm" << G4endl;

        fPrim->Fill();
      }
    }
  }

  G4cout << "\nNumber of primaries  : " << primaryIDs.size() << G4endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

void AnalysisManager::FillTrajectoriesTree(const G4Event* event)
{
  G4cout << "Filling trajectories tree" << G4endl;
  int count_tracks = 0;

  G4cout << "==== Saving track information to tree ====" << G4endl; 
  auto trajectoryContainer = event->GetTrajectoryContainer(); 
  if (!trajectoryContainer)
  {
    G4cout << "No tracks found: did you enable their storage with '/tracking/storeTrajectory 1'?" << G4endl;
    return;
  }

  for (size_t i = 0; i < trajectoryContainer->entries(); ++i) 
  { 
    auto trajectory = static_cast<G4Trajectory*>((*trajectoryContainer)[i]); 
    trackKinE = trajectory->GetInitialKineticEnergy(); 
    if (trackKinE < 10*GeV) continue;
    trackTID = trajectory->GetTrackID();
    trackPID = trajectory->GetParentID();
    trackPDG = trajectory->GetPDGEncoding(); 
    trackNPoints = trajectory->GetPointEntries(); 
    trackTheta = trajectory->GetInitialMomentum().theta();
    count_tracks++; 
    
    // Find start and decay points based on z values
    if (trackNPoints > 0) {
      G4double minZ = std::numeric_limits<G4double>::max();
      G4double maxZ = std::numeric_limits<G4double>::lowest();
      G4ThreeVector prodPos, decayPos;
      
      for (size_t j = 0; j < trackNPoints; ++j) 
      { 
        G4ThreeVector pos = trajectory->GetPoint(j)->GetPosition(); 
        if (pos.z() < minZ) {
          minZ = pos.z();
          prodPos = pos;
        }
        if (pos.z() > maxZ) {
          maxZ = pos.z();
          decayPos = pos;
        }
      }
      
      trackProdX = prodPos.x();
      trackProdY = prodPos.y();
      trackProdZ = prodPos.z();
      trackDecayX = decayPos.x();
      trackDecayY = decayPos.y();
      trackDecayZ = decayPos.z();
    }
    fTrk->Fill();
  }
  G4cout << "Total number of recorded track: " << count_tracks << std::endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

void AnalysisManager::FillGeomTree()
{
  G4cout << "Filling geometry tree" << G4endl;
  const DetectorConstruction* det =
    static_cast<const DetectorConstruction*>(
        G4RunManager::GetRunManager()->GetUserDetectorConstruction()
    );
  detectorWidth = det->GetDetectorWidth()/mm;
  detectorHeight = det->GetDetectorHeight()/mm;
  tungstenThickness = det->GetTungstenThickness()/mm;
  siliconThickness = det->GetSiliconThickness()/um;
  nLayers = det->GetNumberOfLayers();
  simFlag = det->GetSimFlag();
  scintBarFlag = det->GetScintBarFlag();
  
  // get pixel positions: Idea is that we can get the x,y,z of all hits by indexing into these arrays
  // pixelsXPos = det->GetPixelXPositions();
  // pixelsYPos = det->GetPixelYPositions();
  // pixelsZPos = det->GetPixelZPositions();
  pixelsXPos = det->GetPixelCenterX();
  pixelsYPos = det->GetPixelCenterY();
  pixelsZPos = det->GetSiliconZPositions();
  

  fGeom->Fill();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------

void AnalysisManager::FillHitsOutput()
{
  G4cout << "==== Filling Hits output trees ====" << G4endl;
  int nHits = 0;
  G4int nHC = fHCofEvent->GetNumberOfCollections();
  for (G4int i = 0; i < nHC; ++i) {
      auto* hc = fHCofEvent->GetHC(i);
      auto* pixelHitCollection = dynamic_cast<PixelHitsCollection*>(hc);
      if (pixelHitCollection && pixelHitCollection->GetName() == "PixelHitsCollection") {
        
        G4cout << "Found hit collection: " << pixelHitCollection->GetName() << G4endl;

        G4cout << "Number of hits in collection: " << pixelHitCollection->GetSize() << G4endl;
        for (auto hit : *pixelHitCollection->GetVector())
        {
          nHits++;
          fPixelEventID = evtID;
          fPixelRowIDs.push_back(hit->GetRowID());
          fPixelColIDs.push_back(hit->GetColID());
          fPixelLayerIDs.push_back(hit->GetLayerID());
          fPixelPDGCs.push_back(hit->GetPDGCode());
          fPixelTrackIDs.push_back(hit->GetTrackID());
          fPixelParentIDs.push_back(hit->GetParentID());
          // fPixelPxs.push_back(hit->GetPx());
          // fPixelPys.push_back(hit->GetPy());
          // fPixelPzs.push_back(hit->GetPz());
          // fPixelEnergies.push_back(hit->GetEnergy());
          // fPixelCharges.push_back(hit->GetCharge());
          fPixelEDep.push_back(hit->GetEnergyDeposit());
          // fPixelFromPrimaryPizero.push_back(hit->GetFromPrimaryPizero());
          // fPixelFromFSLPizero.push_back(hit->GetFromFSLPizero());
          fPixelFromPrimaryLepton.push_back(hit->GetFromPrimaryLepton());
          fPixelFromPrimaryEMShower.push_back(hit->GetFromPrimaryEMShower());

          // if (fSaveTruthHits)
          // {
          //   fPixelTruthX.push_back(hit->GetTruthHitPos().x());
          //   fPixelTruthY.push_back(hit->GetTruthHitPos().y());
          //   fPixelTruthZ.push_back(hit->GetTruthHitPos().z());
          // }

          // G4cout << "Filling hit: TrackID=" << hit->GetTrackID() 
          //        << " PDG=" << hit->GetPDGCode() 
          //        << " Layer=" << hit->GetLayerID() 
          //        << " Row=" << hit->GetRowID() 
          //        << " Col=" << hit->GetColID() 
          //        << " E=" << hit->GetEnergy()/keV << " keV" 
          //        << G4endl;

      }
  
      fPixelHitsTree->Fill();
      
    } 
  } // Close loop over hit collections
}

//// --- NEW FOR SCINTILLATORS ---
void AnalysisManager::FillScintOutput()
{
    fScintEventID = evtID;

    G4int nHC = fHCofEvent->GetNumberOfCollections();

    for(G4int i = 0; i < nHC; ++i)
    {
        auto* hc = fHCofEvent->GetHC(i);
        auto* scintHC = dynamic_cast<ScintHitsCollection*>(hc);
        if(!scintHC) continue;
        if(scintHC->GetName() != "ScintHitsCollection") continue;

        for(size_t h = 0; h < scintHC->entries(); ++h)
        {
            auto* hit = (*scintHC)[h];

            fScintLayerID.push_back(hit->GetLayerID());
            fScintColID.push_back(hit->GetColID());
            fScintRowID.push_back(hit->GetRowID());
            fScintTrackID.push_back(hit->GetTrackID());
            fScintParentID.push_back(hit->GetParentID());
            fScintPDG.push_back(hit->GetPDGCode());
            fScintEdep.push_back(hit->GetEnergyDeposit());
            fScintFromMuon.push_back(hit->GetFromMuon());
            fScintFromPrimaryLepton.push_back(hit->GetFromPrimaryLepton());
        }
    }

    fScintTree->Fill();
}

//// FASER SPECTROMETER HITS ---
void AnalysisManager::FillFaserOutput()
{
    fFaserEventID = evtID;

    // Clear vectors
    fFaserTrackerID.clear();
    fFaserTrackID.clear();
    fFaserPDG.clear();
    fFaserX.clear();
    fFaserY.clear();
    fFaserZ.clear();
    fFaserPx.clear();
    fFaserPy.clear();
    fFaserPz.clear();
    fFaserE.clear();
    fFaserEdep.clear();
    fFaserCharge.clear();

    G4int nHC = fHCofEvent->GetNumberOfCollections();

    for(G4int i = 0; i < nHC; ++i)
    {
        auto* hc = fHCofEvent->GetHC(i);
        auto* faserHC = dynamic_cast<FaserHitsCollection*>(hc);
        if(!faserHC) continue;
        if(faserHC->GetName() != "FaserHitsCollection") continue;

        G4cout << "Found FASER tracker hit collection with " << faserHC->GetSize() << " hits" << G4endl;

        for(size_t h = 0; h < faserHC->entries(); ++h)
        {
            auto* hit = (*faserHC)[h];

            fFaserTrackerID.push_back(hit->GetTrackerID());
            fFaserTrackID.push_back(hit->GetTrackID());
            fFaserPDG.push_back(hit->GetPDGCode());
            fFaserX.push_back(hit->GetX());
            fFaserY.push_back(hit->GetY());
            fFaserZ.push_back(hit->GetZ());
            fFaserPx.push_back(hit->GetPx());
            fFaserPy.push_back(hit->GetPy());
            fFaserPz.push_back(hit->GetPz());
            fFaserE.push_back(hit->GetEnergy());
            fFaserEdep.push_back(hit->GetEnergyDeposit());
            fFaserCharge.push_back(hit->GetCharge());
        }
    }

    fFaserHitsTree->Fill();
}


void AnalysisManager::FillPythiaTree(const G4Event* event, TTree *tree, std::vector<int> pdg_ids)
{
    auto trajectoryContainer = event->GetTrajectoryContainer(); 
    if (!trajectoryContainer)
    {
        G4cout << "No tracks found: did you enable their storage with '/tracking/storeTrajectory 1'?" << G4endl;
        return;
    }
    for (size_t parentIdx = 0; parentIdx < trajectoryContainer->entries(); ++parentIdx) 
    { 
        // Check for primary tau /charm
        G4Trajectory *parent = static_cast<G4Trajectory*>((*trajectoryContainer)[parentIdx]); 
        int parentPDG = std::abs(parent->GetPDGEncoding());
        if ((parent->GetParentID() == 0) && (std::find(pdg_ids.begin(), pdg_ids.end(), std::abs(parentPDG)) != pdg_ids.end())) {
            G4int parentID = parent->GetTrackID();
            fPythiaPPDG = parent->GetPDGEncoding();
            // std::cout << "Found primary tau/charm with pdg " << parentPDG << ", track id " << parentID << std::endl;
            // Check for children of this primary tauy lepton
            for (size_t trajIdx = 0; trajIdx < trajectoryContainer->entries(); ++trajIdx) {
                G4Trajectory *traj = static_cast<G4Trajectory*>((*trajectoryContainer)[trajIdx]); 
                if (traj->GetParentID() == parentID) {
                    int trajPDG = traj->GetParticleDefinition()->GetPDGEncoding();
                    // std::cout << "Found child of primary tau/charm with PDG ID " << trajPDG << std::endl;
                    // Write out trajecotry information
                    fPythiaTPDG = traj->GetParticleDefinition()->GetPDGEncoding();
                    fPythiaTID = traj->GetTrackID();
                    fPythiaPID = traj->GetParentID();
                    G4int trackNPoints = traj->GetPointEntries(); 
                    // Get production and decay vertex
                    G4double minZ = std::numeric_limits<G4double>::max();
                    G4double maxZ = std::numeric_limits<G4double>::lowest();
                    G4ThreeVector prodPos, decayPos;
                    for (size_t j = 0; j < trackNPoints; ++j) 
                    { 
                        G4ThreeVector pos = traj->GetPoint(j)->GetPosition(); 
                        if (pos.z() < minZ) {
                        minZ = pos.z();
                        prodPos = pos;
                        }
                        if (pos.z() > maxZ) {
                        maxZ = pos.z();
                        decayPos = pos;
                        }
                    }
                    fPythiaProdX = prodPos.x();
                    fPythiaProdY = prodPos.y();
                    fPythiaProdZ = prodPos.z();
                    fPythiaDecayX = decayPos.x();
                    fPythiaDecayY = decayPos.y();
                    fPythiaDecayZ = decayPos.z();
                    G4ThreeVector momentum = traj->GetInitialMomentum();
                    fPythiaPx = momentum.x();
                    fPythiaPy = momentum.y();
                    fPythiaPz = momentum.z();
                    tree->Fill();
                }
            }
        }
    }
}

float_t AnalysisManager::GetTotalEnergy(float_t px, float_t py, float_t pz, float_t m)
{
  return TMath::Sqrt(px * px + py * py + pz * pz + m * m);
}

