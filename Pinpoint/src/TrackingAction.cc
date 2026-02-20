#include "TrackingAction.hh"
#include "TrackInformation.hh"
#include "AnalysisManager.hh"

#include "G4TrackingManager.hh"
#include "G4Track.hh"

TrackingAction::TrackingAction() : G4UserTrackingAction() {;}

void TrackingAction::PreUserTrackingAction(const G4Track* aTrack)
{
}

void TrackingAction::PostUserTrackingAction(const G4Track* aTrack)
{
  if (aTrack->GetParentID()==0) 
  {
    AnalysisManager::GetInstance()->AddOnePrimaryTrack();
  }
  if (aTrack->GetParentID()==0) 
  {
    if (aTrack->GetParticleDefinition()->GetPDGEncoding()==111) 
    {
      // in case of pizero in the list of primary track
      // its decay products are also counted as primary particles, mostly 2 gammas
      G4TrackVector* secondaries = fpTrackingManager->GimmeSecondaries();
      if (secondaries) 
      {
        size_t nSeco = secondaries->size();
        if (nSeco>0) 
        {
          for (size_t i=0; i<nSeco; ++i) 
          {
            if ((*secondaries)[i]->GetCreatorProcess()->GetProcessName()=="Decay") 
            {
              TrackInformation* info =  new TrackInformation();
              info->SetTrackIsFromPrimaryPizero(1);
              (*secondaries)[i]->SetUserInformation(info);
              AnalysisManager::GetInstance()->AddOnePrimaryTrack();
            }
          }
        }
      }
    }
  }

  if (aTrack->GetTrackID()==1 &&
      (abs(aTrack->GetParticleDefinition()->GetPDGEncoding())==15 ||
       abs(aTrack->GetParticleDefinition()->GetPDGEncoding())==13)) 
  {
    // in case of the lepton decays, the decay products are counted as primary particles
    // * tau- decay (dominant)
    // * mu- decay
    G4TrackVector* secondaries = fpTrackingManager->GimmeSecondaries();
    if (secondaries) 
    {
      size_t nSeco = secondaries->size();
      if (nSeco>0) 
      {
        for (size_t i=0; i<nSeco; ++i) 
        {
          if ((*secondaries)[i]->GetCreatorProcess()->GetProcessName()=="Decay") 
          {
            TrackInformation* info =  new TrackInformation();
            info->SetTrackIsFromPrimaryLepton(1);
            (*secondaries)[i]->SetUserInformation(info);
            AnalysisManager::GetInstance()->AddOnePrimaryTrack();
          }
        }
      }
    }
  }

  if (aTrack->GetTrackID()==1 && abs(aTrack->GetParticleDefinition()->GetPDGEncoding())==11)
  {
    const auto* info =static_cast<const TrackInformation*>(aTrack->GetUserInformation());
    TrackInformation* newInfo =  new TrackInformation();
    if (info) {
      newInfo->SetTrackIsFromPrimaryPizero(info->IsTrackFromPrimaryPizero());
      newInfo->SetTrackIsFromFSLPizero(info->IsTrackFromFSLPizero());
      newInfo->SetTrackIsFromPrimaryLepton(info->IsTrackFromPrimaryLepton());
    }
    newInfo->SetTrackIsFromPrimaryEMShower(1);
    aTrack->SetUserInformation(newInfo);
  }

  const auto* info =static_cast<const TrackInformation*>(aTrack->GetUserInformation());
  G4bool fromPrimaryEMShower = info && info->IsTrackFromPrimaryEMShower();

  if (fromPrimaryEMShower &&
      (abs(aTrack->GetParticleDefinition()->GetPDGEncoding())==11 ||
       abs(aTrack->GetParticleDefinition()->GetPDGEncoding())==22))
  {
    G4TrackVector* secondaries = fpTrackingManager->GimmeSecondaries();
    if (secondaries)
    {
      size_t nSeco = secondaries->size();
      if (nSeco>0)
      {
        for (size_t i=0; i<nSeco; ++i)
        {
          TrackInformation* info =  new TrackInformation();
          info->SetTrackIsFromPrimaryEMShower(1);
          (*secondaries)[i]->SetUserInformation(info);
        }
      }
    }
  }

  // Track charmed hadrons and their decay products (including through neutral particles)
  // Charmed hadron PDG codes: 411 (D+), 421 (D0), 431 (Ds+), 4122 (Lambda_c+), 4112 (Sigma_c0), 4212 (Sigma_c+), 4222 (Sigma_c++), 4132 (Xi_c0), 4232 (Xi_c+)
  G4int absPDG = std::abs(aTrack->GetParticleDefinition()->GetPDGEncoding());
  G4bool isCharmedHadron = (absPDG == 411 || absPDG == 421 || absPDG == 431 || 
                             absPDG == 4122 || absPDG == 4112 || absPDG == 4212 || 
                             absPDG == 4222 || absPDG == 4132 || absPDG == 4232);
  
  G4bool fromCharmedHadron = info && info->IsTrackFromCharmedHadron();
  
  // If this is a primary charmed hadron, its decay products are counted as primary particles
  if (isCharmedHadron && (aTrack->GetParentID() == 0 || aTrack->GetParentID() == 1))
  {
    G4TrackVector* secondaries = fpTrackingManager->GimmeSecondaries();
    if (secondaries)
    {
      size_t nSeco = secondaries->size();
      if (nSeco>0)
      {
        for (size_t i=0; i<nSeco; ++i)
        {
          if ((*secondaries)[i]->GetCreatorProcess()->GetProcessName()=="Decay")
          {
            TrackInformation* newInfo = new TrackInformation();
            newInfo->SetTrackIsFromCharmedHadron(1);
            (*secondaries)[i]->SetUserInformation(newInfo);
            AnalysisManager::GetInstance()->AddOnePrimaryTrack();
          }
        }
      }
    }
  }
  // If this track comes from a charmed hadron, tag all its decay products
  else if (isCharmedHadron || fromCharmedHadron)
  {
    G4TrackVector* secondaries = fpTrackingManager->GimmeSecondaries();
    if (secondaries)
    {
      size_t nSeco = secondaries->size();
      if (nSeco>0)
      {
        for (size_t i=0; i<nSeco; ++i)
        {
          TrackInformation* newInfo = new TrackInformation();
          newInfo->SetTrackIsFromCharmedHadron(1);
          (*secondaries)[i]->SetUserInformation(newInfo);
        }
      }
    }
  }

  if (aTrack->GetParentID()==1 && aTrack->GetCreatorProcess()->GetProcessName()=="Decay") 
  {
    // in case of tau decay pizero
    // decay products of this pizero are also counted as primary particles, mostly 2 gammas
    if (aTrack->GetParticleDefinition()->GetPDGEncoding()==111) 
    {
      G4TrackVector* secondaries = fpTrackingManager->GimmeSecondaries();
      if (secondaries) 
      {
        size_t nSeco = secondaries->size();
        if (nSeco>0) 
        {
          for (size_t i=0; i<nSeco; ++i) 
          {
            if ((*secondaries)[i]->GetCreatorProcess()->GetProcessName()=="Decay") 
            {
              TrackInformation* info =  new TrackInformation();
              info->SetTrackIsFromFSLPizero(1);
              (*secondaries)[i]->SetUserInformation(info);
              AnalysisManager::GetInstance()->AddOnePrimaryTrack();
            }
          }
        }
      }
    }
  }
  
  //TrackInformation* aTrackInfo = (TrackInformation*)(aTrack->GetUserInformation());
  //if (aTrackInfo) {
  //  if (aTrackInfo->IsTrackFromPrimaryTau() | aTrackInfo->IsTrackFromPrimaryPizero()) {
  //    std::cout<<aTrack->GetParentID()<<" "<<aTrack->GetParticleDefinition()->GetPDGEncoding()<<std::endl;
  //    aTrackInfo->Print();
  //  }
  //}
}
