#include <cassert>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <algorithm>

#include "libxml/parser.h"
#include "libxml/xmlmemory.h"

#include <TSystem.h>
#include <TFile.h>
#include <TTree.h>
#include <TFolder.h>
#include <TBits.h>
#include <TObjString.h>
#include <TMath.h>
#include "Framework/ParticleData/BaryonResonance.h"
#include "Framework/ParticleData/BaryonResUtils.h"
#include "Framework/Conventions/GBuild.h"
#include "Framework/Conventions/Constants.h"
#include "Framework/Conventions/Units.h"
#include "Framework/EventGen/EventRecord.h"
#include "Framework/GHEP/GHepStatus.h"
#include "Framework/GHEP/GHepParticle.h"
#include "Framework/GHEP/GHepUtils.h"
#include "Framework/Ntuple/NtpMCFormat.h"
#include "Framework/Ntuple/NtpMCTreeHeader.h"
#include "Framework/Ntuple/NtpMCEventRecord.h"
#include "Framework/Ntuple/NtpWriter.h"
#include "Framework/Numerical/RandomGen.h"
#include "Framework/Messenger/Messenger.h"
#include "Framework/ParticleData/PDGCodes.h"
#include "Framework/ParticleData/PDGUtils.h"
#include "Framework/ParticleData/PDGLibrary.h"
#include "Framework/Utils/AppInit.h"
#include "Framework/Utils/RunOpt.h"
#include "Framework/Utils/CmdLnArgParser.h"
#include "Framework/Utils/SystemUtils.h"
#include "Framework/Utils/T2KEvGenMetaData.h"

// Helper function to decode interaction type from process flags
int DecodeInteractionType(bool cc, bool nc, bool em)
{
  // Interaction types from GENIE InteractionType.h
  enum InteractionType {
    kIntNull   = 0,
    kIntEM,          
    kIntWeakCC,      
    kIntWeakNC,      
    kIntWeakMix,     
    kIntDarkMatter,  
    kIntNDecay,      
    kIntNOsc,        
    kIntNHL,         
    kIntDarkNC       
  };
  
  if (cc) return kIntWeakCC;
  else if (nc) return kIntWeakNC;
  else if (em) return kIntEM;
  
  return kIntNull;
}

// Helper function to decode scattering type from process flags
int DecodeScatteringType(bool qel, bool res, bool dis, bool coh, bool dfr, 
                         bool imd, bool imdanh, bool mec, bool nuel, 
                         bool singlek, bool amnugamma)
{
  // Scattering types from GENIE ScatteringType.h
  enum ScatteringType {
    kScUnknown = -100,
    kScNull = 0,
    kScQuasiElastic,
    kScSingleKaon,
    kScDeepInelastic,
    kScResonant,
    kScCoherentProduction,
    kScDiffractive,
    kScNuElectronElastic,
    kScInverseMuDecay,
    kScAMNuGamma,
    kScMEC,
    kScCoherentElastic,
    kScInverseBetaDecay,
    kScGlashowResonance,
    kScIMDAnnihilation,
    kScPhotonCoherent,
    kScPhotonResonance,
    kScSinglePion,
    kScDarkMatterElastic = 101,
    kScDarkMatterDeepInelastic,
    kScDarkMatterElectron,
    kScNorm
  };
  
  if (qel) return kScQuasiElastic;
  else if (res) return kScResonant;
  else if (dis) return kScDeepInelastic;
  else if (coh) return kScCoherentProduction;
  else if (dfr) return kScDiffractive;
  else if (imd) return kScInverseMuDecay;
  else if (imdanh) return kScIMDAnnihilation;
  else if (mec) return kScMEC;
  else if (nuel) return kScNuElectronElastic;
  else if (singlek) return kScSingleKaon;
  else if (amnugamma) return kScAMNuGamma;
  
  return kScUnknown;
}

void convertGHEPPinPoint(const std::string& inputFilename, const std::string& outputFilename, bool ccOnly = false)
{
  // recite the incantations necessary to read Genie's ntuple

  using namespace genie;
  TFile* inFile = TFile::Open(inputFilename.c_str(), "READ");
  TTree* inTree = (TTree*) inFile->Get("gtree");
  NtpMCEventRecord* pRecord = nullptr;
  TBranch* b_gmcrec = inTree->GetBranch("gmcrec");
  b_gmcrec->SetAddress(&pRecord);

  // set up the output tree
  
  TFile* outFile = TFile::Open(outputFilename.c_str(), "RECREATE");
  outFile->cd();
  TTree* outTree = new TTree("gFaser","gFaserTitle");

  // buffers 

  int nFinal;
  int iev;
  int neu;
  int fspl;
  int tgt;
  int Z;
  int A;
  int hitnuc;
  bool cc;
  bool nc;
  int intType;
  int scatteringType;
  // double vtxx;
  // double vtxy;
  // double vtxz;
  double vx;
  double vy;
  double vz;
  double pxv;
  double pyv;
  double pzv;
  double Ev;
  double Q2;
  double W;
  double x;
  double y;
  double Xsec;
  double crossingAngle;
  bool isVerticalCrossingAngle;
  // to do: add interaction information
  std::vector<std::string> name;
  std::vector<int> pdgc;
  std::vector<int> status;
  std::vector<int> firstMother;
  std::vector<int> lastMother;
  std::vector<int> firstDaughter;
  std::vector<int> lastDaughter;
  std::vector<double> px;
  std::vector<double> py;
  std::vector<double> pz;
  std::vector<double> E;
  std::vector<double> m;
  std::vector<double> M;
  
  outTree->Branch("iev",&iev,"iev/I");
  outTree->Branch("neu",&neu,"neu/I");
  outTree->Branch("fspl",&fspl,"fspl/I");
  outTree->Branch("tgt",&tgt,"tgt/I");
  outTree->Branch("Z",&Z,"Z/I");
  outTree->Branch("A",&A,"A/I");
  outTree->Branch("hitnuc",&hitnuc,"hitnuc/I");
  outTree->Branch("cc",&cc,"cc/O");
  outTree->Branch("nc",&nc,"nc/O");
  outTree->Branch("intType",&intType,"intType/I");
  outTree->Branch("scatteringType",&scatteringType,"scatteringType/I");
  // outTree->Branch("vtxx",&vtxx,"vtxx/D");
  // outTree->Branch("vtxy",&vtxy,"vtxy/D");
  // outTree->Branch("vtxz",&vtxz,"vtxz/D");
  outTree->Branch("vx",&vx,"vx/D");
  outTree->Branch("vy",&vy,"vy/D");
  outTree->Branch("vz",&vz,"vz/D");
  outTree->Branch("pxv",&pxv,"pxv/D");
  outTree->Branch("pyv",&pyv,"pyv/D");
  outTree->Branch("pzv",&pzv,"pzv/D");
  outTree->Branch("Ev",&Ev,"Ev/D");
  outTree->Branch("Q2",&Q2,"Q2/D");
  outTree->Branch("W",&W,"W/D");
  outTree->Branch("x",&x,"x/D");
  outTree->Branch("y",&y,"y/D");
  outTree->Branch("XSec", &Xsec, "XSec/D");
  outTree->Branch("crossingAngle",&crossingAngle,"crossingAngle/D");
  outTree->Branch("isVerticalCrossingAngle",&isVerticalCrossingAngle,
                  "isVerticalCrossingAngle/O");
  outTree->Branch("n",&nFinal,"n/I");
  outTree->Branch("name","std::vector<std::string>",&name);
  outTree->Branch("pdgc","std::vector<int>",&pdgc);
  outTree->Branch("status","std::vector<int>",&status);
  outTree->Branch("firstMother","std::vector<int>",&firstMother);
  outTree->Branch("lastMother","std::vector<int>",&lastMother);
  outTree->Branch("firstDaughter","std::vector<int>",&firstDaughter);
  outTree->Branch("lastDaughter","std::vector<int>",&lastDaughter);
  outTree->Branch("px","std::vector<double>",&px);
  outTree->Branch("py","std::vector<double>",&py);
  outTree->Branch("pz","std::vector<double>",&pz);
  outTree->Branch("E","std::vector<double>",&E);
  outTree->Branch("m","std::vector<double>",&m);
  outTree->Branch("M","std::vector<double>",&m);

  // now loop over the input tree and fill the output tree
  int nEntries = inTree->GetEntries();
  for (int i = 0; i < nEntries; i++)
  {
    if (i%100 == 0)
      std::cout << "Event: " << i+1 << std::endl;

    // Clear buffers
    nFinal = 0;
    iev = 0;
    neu = 0;
    fspl = 0;
    tgt = 0;
    Z = 0;
    A = 0;
    hitnuc = 0;
    cc = false;
    nc = false;
    intType = 0;
    scatteringType = 0;
    // vtxx = 0.0;
    // vtxy = 0.0;
    // vtxz = 0.0;
    vx = 0.0;
    vy = 0.0;
    vz = 0.0;
    pxv = 0.0;
    pyv = 0.0;
    pzv = 0.0;
    Ev = 0.0;
    Q2 = 0.0;
    W = 0.0;
    x = 0.0;
    y = 0.0;
    crossingAngle = 0.0;
    isVerticalCrossingAngle = false;
    name.clear();
    pdgc.clear();
    status.clear();
    firstMother.clear();
    lastMother.clear();
    firstDaughter.clear();
    lastDaughter.clear();
    px.clear();
    py.clear();
    pz.clear();
    E.clear();
    m.clear();
    M.clear();
    
    // read and fill
    inTree->GetEntry(i);
    EventRecord& event = *(pRecord->event);
    
    // Extract particles
    GHepParticle* neutrino = event.Probe();
    GHepParticle* target = event.Particle(1);
    GHepParticle* fsl = event.FinalStatePrimaryLepton();
    GHepParticle* hitnucl = event.HitNucleon();
    
    // Get interaction summary
    const Interaction* interaction = event.Summary();
    const ProcessInfo& proc_info = interaction->ProcInfo();
    const Kinematics& kine = interaction->Kine();
    
    // Fill event number
    iev = i;
    
    // Fill particle info
    neu = (neutrino) ? neutrino->Pdg() : 0;
    fspl = (fsl) ? fsl->Pdg() : 0;
    tgt = target->Pdg();
    
    // Extract target Z and A
    if(pdg::IsIon(target->Pdg())) {
      Z = pdg::IonPdgCodeToZ(target->Pdg());
      A = pdg::IonPdgCodeToA(target->Pdg());
    }
    if(target->Pdg() == kPdgProton)   { Z = 1; A = 1; }
    if(target->Pdg() == kPdgNeutron)  { Z = 0; A = 1; }
    
    // Hit nucleon
    hitnuc = (hitnucl) ? hitnucl->Pdg() : 0;

    Xsec  = event.XSec() * (1E+38/units::cm2);
    
    // Process type
    cc = proc_info.IsWeakCC();
    nc = proc_info.IsWeakNC();
    bool em = proc_info.IsEM();
    bool qel = proc_info.IsQuasiElastic();
    bool res = proc_info.IsResonant();
    bool dis = proc_info.IsDeepInelastic();
    bool coh = proc_info.IsCoherentProduction();
    bool dfr = proc_info.IsDiffractive();
    bool imd = proc_info.IsInverseMuDecay();
    bool imdanh = proc_info.IsIMDAnnihilation();
    bool mec = proc_info.IsMEC();
    bool nuel = proc_info.IsNuElectronElastic();
    bool singlek = proc_info.IsSingleKaon();
    bool amnugamma = proc_info.IsAMNuGamma();
    
    // Decode interaction and scattering types
    intType = DecodeInteractionType(cc, nc, em);
    scatteringType = DecodeScatteringType(qel, res, dis, coh, dfr, imd, 
                                          imdanh, mec, nuel, singlek, amnugamma);
    
    // Vertex
    TLorentzVector* vertex = event.Vertex();
    vx = vertex->X();
    vy = vertex->Y();
    vz = vertex->Z();
    
    // Neutrino kinematics
    if(neutrino) {
      pxv = neutrino->Px();
      pyv = neutrino->Py();
      pzv = neutrino->Pz();
      Ev = neutrino->E();
    }
    
    // Calculate kinematic variables
    const double kNucleonMass = genie::constants::kNucleonMass;
    TLorentzVector pdummy(0,0,0,0);
    const TLorentzVector& k1 = (neutrino) ? *(neutrino->P4()) : pdummy;
    const TLorentzVector& k2 = (fsl) ? *(fsl->P4()) : pdummy;
    const TLorentzVector& p1 = (hitnucl) ? *(hitnucl->P4()) : pdummy;
    
    double Mn = kNucleonMass;
    TLorentzVector q = k1 - k2;
    Q2 = -1 * q.M2();
    
    bool is_hnl = proc_info.IsHNLDecay();
    
    if(!coh && !is_hnl) {
      double v = (hitnucl) ? q.Energy() : -1;
      x = (hitnucl) ? 0.5*Q2/(Mn*v) : -1;
      y = (hitnucl) ? v/k1.Energy() : -1;
      double W2 = (hitnucl) ? Mn*Mn + 2*Mn*v - Q2 : -1;
      W = (hitnucl) ? TMath::Sqrt(W2) : -1;
    } else if(is_hnl) {
      x = -1;
      y = -1;
      double W2 = k1.M2();
      W = TMath::Sqrt(W2);
    } else {
      double v = q.Energy();
      x = 0.5*Q2/(Mn*v);
      y = v/k1.Energy();
      double W2 = Mn*Mn + 2*Mn*v - Q2;
      W = TMath::Sqrt(W2);
    }
    
    // Crossing angle info
    crossingAngle = event.CrossingAngle();
    isVerticalCrossingAngle = event.IsVerticalCrossingAngle();

    nFinal = event.GetEntries();

    // Check for CC if requested
    bool keep = true;
    if (ccOnly)
    {
      GHepParticle* nu = (GHepParticle*) event[0];
      uint nuPDG = abs(nu->Pdg());
      if (nFinal >= 3) 
      {
        GHepParticle* p = (GHepParticle*) event[2];
        if (abs(p->Pdg()) == nuPDG)
        {
          keep = false;
        }
        else if (nFinal >= 5)
        {
          p = (GHepParticle*) event[4];
          if (abs(p->Pdg()) == nuPDG) keep = false;
        }
      }
    }
    if (!keep) continue;
    for (int j = 0; j < nFinal; j++)
    {
      GHepParticle* p = (GHepParticle*) event[j];
      name.push_back(p->Name());
      pdgc.push_back(p->Pdg());
      status.push_back(p->Status());
      firstMother.push_back(p->FirstMother());
      lastMother.push_back(p->LastMother());
      firstDaughter.push_back(p->FirstDaughter());
      lastDaughter.push_back(p->LastDaughter());
      px.push_back(p->Px());
      py.push_back(p->Py());
      pz.push_back(p->Pz());
      E.push_back(p->E());
      m.push_back(p->Mass());
      if (p->IsOnMassShell())
      {
	M.push_back(p->Mass());
      }
      else
      {
	M.push_back(p->P4()->M());
      }
    }  // Loop over particles
    outTree->Fill();
  }  //Loop over events

  outTree->Write();
  outFile->Close();
}

