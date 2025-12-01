#include "EventAction.hh"

#include <G4Event.hh>
#include <G4AccumulableManager.hh>
#include "G4VVisManager.hh"
#include "G4Circle.hh"
#include "G4VisAttributes.hh"
#include "AnalysisManager.hh"
#include "ProfilingManager.hh"

using namespace std;

EventAction::EventAction() :
  G4UserEventAction(),
  fNPrimaryTrack("NPrimaryTrack", 0),
  fNSecondaryTrack("NSecondaryTrack", 0),
  fNSecondaryTrackNotGamma("NSecondaryTrackNotGamma", 0) 
{
  // Register created accumulables
  G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Register(fNPrimaryTrack);
  accumulableManager->Register(fNSecondaryTrack);
  accumulableManager->Register(fNSecondaryTrackNotGamma);
}

EventAction::~EventAction() {;}

void EventAction::BeginOfEventAction(const G4Event* event)
{
  PROFILE_START("Event");

  // Reset all accumulables to their initial values
  G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Reset();

  PROFILE_START("AnalysisManager::BeginOfEvent");
  AnalysisManager* ana = AnalysisManager::GetInstance();
  ana->BeginOfEvent();
  PROFILE_STOP("AnalysisManager::BeginOfEvent");
}

void EventAction::EndOfEventAction(const G4Event* event)
{
  G4cout << "This is the " << event->GetEventID() << "th event"<<G4endl;

  if (fNPrimaryTrack.GetValue()) 
    G4cout << " * Produced "<< fNPrimaryTrack.GetValue() << " primary tracks." << G4endl;
  else 
    G4cout << " * No primary tracks produced" << G4endl;

  if (fNSecondaryTrack.GetValue()) 
    G4cout << " * Produced "<< fNSecondaryTrack.GetValue() << " secondary tracks." << G4endl;
  else
    G4cout << " * No secondary tracks produced" << G4endl;

  if (fNSecondaryTrackNotGamma.GetValue()) 
    G4cout << " * Produced "<< fNSecondaryTrackNotGamma.GetValue() << " secondary tracks (excluding gamma)." << G4endl;
  else
    G4cout << " * No secondary tracks (excluding gamma) produced" << G4endl;

  // skip AnalysisManager if there are no tracks at all!
  if(!fNPrimaryTrack.GetValue() && !fNSecondaryTrack.GetValue() && !fNSecondaryTrackNotGamma.GetValue()) {
    PROFILE_STOP("Event");
    return;
  }

  PROFILE_START("AnalysisManager::EndOfEvent");
  AnalysisManager* ana = AnalysisManager::GetInstance();
  ana->EndOfEvent(event);
  PROFILE_STOP("AnalysisManager::EndOfEvent");

  PROFILE_STOP("Event");
}

void EventAction::AddPrimaryTrack() 
{
  fNPrimaryTrack += 1;
}

void EventAction::AddSecondaryTrack() 
{
  fNSecondaryTrack += 1;
}

void EventAction::AddSecondaryTrackNotGamma() 
{
  fNSecondaryTrackNotGamma += 1;
}
