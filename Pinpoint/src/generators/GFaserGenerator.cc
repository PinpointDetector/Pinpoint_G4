#include "generators/GFaserGenerator.hh"
#include "generators/GFaserGeneratorMessenger.hh"
#include "DetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4Box.hh"
#include "G4Event.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "Randomize.hh"
#include "G4String.hh"
#include "G4Types.hh"
#include "G4IonTable.hh"
#include "G4TransportationManager.hh"

#include "TFile.h"
#include "TTree.h"
#include "TApplication.h"
#include "TROOT.h"

#include <string>

GFaserGenerator::GFaserGenerator()
{

  fGeneratorName = "gfaser";
  fMessenger = new GFaserGeneratorMessenger(this);

  fGfaserFile = nullptr;
  fGfaserTree = nullptr;
}


GFaserGenerator::~GFaserGenerator()
{
  delete fName; fName = nullptr;
  delete fPdgc; fPdgc = nullptr;
  delete fStatus; fStatus = nullptr;
  delete fPx; fPx = nullptr;
  delete fPy; fPy = nullptr;
  delete fPz; fPz = nullptr;
  delete fE; fE = nullptr;
  delete fGfaserTree; fGfaserTree = nullptr;
  delete fMessenger; fMessenger = nullptr;
  if (fGfaserFile) {
    fGfaserFile->Close();
    delete fGfaserFile;
    fGfaserFile = nullptr;
  }
}


void GFaserGenerator::LoadData()
{
  if (fInputFileName.empty()) {
    G4cout << "Error: Missing *.gfaser.root file." << G4endl;
    return;
  }
  
  fGfaserFile = TFile::Open(fInputFileName.c_str(), "READ");
  if (!fGfaserFile || fGfaserFile->IsZombie()) {
    G4ExceptionDescription msg;
    msg << "Cannot open Gfaser file: " << fInputFileName;
    G4Exception("GFaserGenerator::LoadData()", "FileError", FatalErrorInArgument, msg);
    return;
  }
  
  fGfaserTree = (TTree*)fGfaserFile->Get("gFaser");
  if (!fGfaserTree) {
    G4ExceptionDescription msg;
    msg << "Cannot find gFaser tree in file: " << fInputFileName;
    G4Exception("GFaserGenerator::LoadData()", "FileError", FatalErrorInArgument, msg);
    return;
  }

  fGfaserTree->SetBranchAddress("n", &fN);
  fGfaserTree->SetBranchAddress("vx", &fVx);
  fGfaserTree->SetBranchAddress("vy", &fVy);
  fGfaserTree->SetBranchAddress("vz", &fVz);
  fGfaserTree->SetBranchAddress("name", &fName);
  fGfaserTree->SetBranchAddress("pdgc", &fPdgc);
  fGfaserTree->SetBranchAddress("status", &fStatus);
  fGfaserTree->SetBranchAddress("px", &fPx);
  fGfaserTree->SetBranchAddress("py", &fPy);
  fGfaserTree->SetBranchAddress("pz", &fPz);
  fGfaserTree->SetBranchAddress("E", &fE);
  fGfaserTree->SetBranchAddress("pxv", &fPxv);
  fGfaserTree->SetBranchAddress("pyv", &fPyv);
  fGfaserTree->SetBranchAddress("pzv", &fPzv);
  fGfaserTree->SetBranchAddress("Ev", &fEv);
  fGfaserTree->SetBranchAddress("Q2", &fQ2);
  fGfaserTree->SetBranchAddress("W", &fW);
  fGfaserTree->SetBranchAddress("x", &fX);
  fGfaserTree->SetBranchAddress("y", &fY);
  fGfaserTree->SetBranchAddress("xsec", &xsec);
  fGfaserTree->SetBranchAddress("intType", &fIntType);
  fGfaserTree->SetBranchAddress("scatteringType", &fScatType);
  fGfaserTree->SetBranchAddress("cc", &fIsCc);
  fGfaserTree->SetBranchAddress("nc", &fIsNc);
  fGfaserTree->SetBranchAddress("Z", &fTgtZ);
  fGfaserTree->SetBranchAddress("A", &fTgtA);
  fGfaserTree->SetBranchAddress("tgt", &fTgtPdg);
  fGfaserTree->SetBranchAddress("neu", &fNuPdg);
  fGfaserTree->SetBranchAddress("fspl", &fLeptonPdg);
  fGfaserTree->SetBranchAddress("hitnuc", &fHitPdg);

  fCurrentEvent = fFirstEvent;
  fTotalEvents = fGfaserTree->GetEntries();
  
  G4cout << "Opened Gfaser file: " << fInputFileName << G4endl;
  G4cout << "Total events: " << fTotalEvents << G4endl;
}


G4String GFaserGenerator::EncodeProcessName() const
{
  G4String process = "";
  if (fIsCc) process += "WeakCC";
  else if (fIsNc) process += "WeakNC";

  if(fScatType == 2) process += " QE";
  else if(fScatType == 5) process += " RES";
  else if(fScatType == 4) process += " DIS";
  else if(fScatType == 6) process += " COH";
  else if(fScatType == 9) process += " IMD";
  else if(fScatType == 11) process += " MEC";
  else if(fScatType == 8) process += " nuELASTIC";

  return process;
}


void GFaserGenerator::GeneratePrimaries(G4Event* event)
{
  // complete line from PrimaryGeneratorAction...
  G4cout << ") : GFaser Generator ===oooOOOooo===" << G4endl;
  G4cout << "oooOOOooo Event # " << fCurrentEvent << "/" << fTotalEvents << " oooOOOooo" << G4endl;
  G4cout << "GeneratePrimaries from file " << fInputFileName << G4endl;

  event->SetEventID(fCurrentEvent);
  if (fCurrentEvent >= fTotalEvents) {
    G4cerr << "** event index beyond range !! **" << G4endl;
  }

  fGfaserTree->GetEntry(fCurrentEvent);
  if (fUseFixedZPosition) {
    fVz = GenerateRandomZVertex(fLayerId);
  }
  G4ThreeVector vertexPosition(fVx * m, fVy * m, fVz * m);
  G4PrimaryVertex* vertex = new G4PrimaryVertex(vertexPosition, 0.);
  // Add primary particles
  for (int i = 0; i < fN; i++) {
    if ((*fStatus)[i] != 1) continue;
    G4int pdg = (*fPdgc)[i];
    G4ParticleDefinition* particleDefinition;
    if ( !FindParticleDefinition( pdg, particleDefinition) ) continue; //skip bad pdgs
    // Create primary particle
    G4LorentzVector p( (*fPx)[i]*GeV, (*fPy)[i]*GeV, (*fPz)[i]*GeV, (*fE)[i]*GeV );
    G4PrimaryParticle* primaryParticle = new G4PrimaryParticle(particleDefinition, p.x(), p.y(), p.z(), p.e());
    vertex->SetPrimary(primaryParticle);
  }
  // Add vertex to event
  event->AddPrimaryVertex(vertex);

  GeneratorVertexMetadata metadata;
  metadata.generatorType = fGeneratorName;
  metadata.processName = EncodeProcessName();
  metadata.weight = 1.0;
  metadata.pdg = fNuPdg; 
  metadata.x4 = G4LorentzVector(fVx*m, fVy*m, fVz*m, 0.);
  metadata.p4 = G4LorentzVector(fPxv*GeV, fPyv*GeV, fPzv*GeV, fEv*GeV);
  metadata.mass = 0.; // neutrinos always massless
  metadata.charge = 0.; // neutrinos always no charge
  metadata.intType = fIntType;
  metadata.scatteringType = fScatType;
  metadata.fsl_pdg = fLeptonPdg;
  metadata.tgt_pdg = fTgtPdg;
  metadata.tgt_A = fTgtA;
  metadata.tgt_Z = fTgtZ;
  metadata.hitnuc_pdg = fHitPdg;
  metadata.Q2 = fQ2;
  metadata.xBj = fX;
  metadata.y = fY;
  metadata.W = fW;
  metadata.xs = xsec;
  fVertexMetadata.push_back(metadata);

  fCurrentEvent++;
}


G4bool GFaserGenerator::FindParticleDefinition(G4int const pdg, G4ParticleDefinition* &particleDefinition) const
{
  // unknown pgd codes in GENIE --> skip it!
  // ref: https://internal.dunescience.org/doxygen/ConvertMCTruthToG4_8cxx_source.html
  // This has been a known issue with GENIE
  const int genieLo = 2000000001;
  const int genieHi = 2000000202;
  if ( pdg >= genieLo && pdg <= genieHi) {
    G4cout<< "This unknown PDG code [" << pdg << "] was present in the GENIE input, "
             << "but will not be processed by Geant4."
             << G4endl;
    return false; // return bad
  }
  
  if ( pdg == 0) {
    particleDefinition = G4ParticleTable::GetParticleTable()->FindParticle("opticalphoton");
  } else {
    particleDefinition = G4ParticleTable::GetParticleTable()->FindParticle(pdg);
  }
  
  if ( pdg > 1000000000) { // If the particle is a nucleus
    // If the particle table doesn't have a definition yet, ask the ion
    // table for one. This will create a new ion definition as needed.
    if (!particleDefinition) {
      int Z = (pdg % 10000000) / 10000; // atomic number
      int A = (pdg % 10000) / 10;       // mass number
      particleDefinition = G4ParticleTable::GetParticleTable()->GetIonTable()->GetIon(Z, A, 0.);
    }
  }

  return true; //return good
}


G4double GFaserGenerator::GenerateRandomZVertex(G4int layerIndex) const {
  auto *runManager = G4RunManager::GetRunManager();
  auto detector = (DetectorConstruction*) (runManager->GetUserDetectorConstruction());

  G4double layerThickness = detector->GetLayerThickness();
  G4double tungstenThickness = detector->GetTungstenThickness();
  G4int nLayers = detector->GetNumberOfLayers();
  G4double startZ = -0.5 * nLayers * layerThickness;
  G4double z = startZ + layerIndex * layerThickness + tungstenThickness * G4UniformRand();
  return z/m; // convert to meters
}

