#include "PrimaryGeneratorMessenger.hh"
#include "PrimaryGeneratorAction.hh"

#include "generators/GeneratorBase.hh"
#include "generators/GENIEGenerator.hh"
#include "generators/HepMCGenerator.hh"
#include "generators/GFaserGenerator.hh"
#include "generators/GPSGenerator.hh"

#include "EventInformation.hh"
#include "ProfilingManager.hh"

#include "G4Event.hh"
#include "G4Exception.hh"


PrimaryGeneratorAction::PrimaryGeneratorAction()
{
  // create a messenger for this class
  fGenMessenger = new PrimaryGeneratorMessenger(this);

  // start with default generator
  fGenerator = new GPSGenerator();
  fInitialized = false;

}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fGenerator;
  delete fGenMessenger;
}

void PrimaryGeneratorAction::SetGenerator(G4String name)
{
  G4StrUtil::to_lower(name);

  if( name == "genie" )
    fGenerator = new GENIEGenerator();
  else if ( name == "gfaser" )
    fGenerator = new GFaserGenerator();
  else if( name == "hepmc" )
    fGenerator = new HepMCGenerator();
  else if ( name == "gun" )
    fGenerator = new GPSGenerator();
  else{
    G4String err = "Unknown generator option " + name;
    G4Exception("PrimaryGeneratorAction",
                "UnknownOption",
                FatalErrorInArgument,
                err.c_str());
  }
}


void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  PROFILE_START("PrimaryGeneratorAction");

  // load generator data at first event
  // this function opens files, reads trees, etc (if required)
  if(!fInitialized){
    PROFILE_START("Generator::LoadData");
    fGenerator->LoadData();
    PROFILE_STOP("Generator::LoadData");
    fInitialized = true;
  }

  G4cout << G4endl;
  G4cout << "===oooOOOooo=== Event Generator (# " << anEvent->GetEventID();

  // reset event metadata
  fGenerator->ResetEventMetadata();

  // produce an event with current generator
  PROFILE_START("Generator::GeneratePrimaries");
  fGenerator->GeneratePrimaries(anEvent);
  PROFILE_STOP("Generator::GeneratePrimaries");

  // save vertex metadata information into the event
  anEvent->SetUserInformation(new EventInformation(fGenerator->GetEventMetadata()));

  PROFILE_STOP("PrimaryGeneratorAction");
}
