#include <G4RunManager.hh>
#include <G4UImanager.hh>
#include <G4VisExecutive.hh>
#include <G4UIExecutive.hh>
#include <G4String.hh>
#include <G4PhysListFactoryAlt.hh>

#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "AnalysisManager.hh"
// #include "PhysicsList.hh"
#include "FTFP_BERT.hh"
#include "Py8DecayerPhysics.hh"
// allow ourselves to extend the short names for physics ctor addition/replace
// along the same lines as EMX, EMY, etc
#include "G4PhysListRegistry.hh"

// allow ourselves to give the user extra info about available physics ctors
#include "G4PhysicsConstructorFactory.hh"

// forward declaration
void PrintAvailable(G4int verb = 1);

/* Main function that enables to:
 * - run macros
 * - start interactive UI mode (no arguments)
 */
int main(int argc, char** argv) {
  G4cout<<"Application starting..."<<G4endl;
//  G4long myseed = 345354;
//  CLHEP::HepRandom::setTheSeed(myseed);

  // pick physics list
  std::string physListName = "FTFP_BERT+PY8DK";

  for (G4int i = 1; i < argc; i = i + 2) {
    G4String g4argv(argv[i]);  // convert only once
    if (g4argv == "-p") physListName = argv[i + 1];
  }

  // Choose the Random engine
  //
  G4Random::setTheEngine(new CLHEP::RanecuEngine);

  // invoke analysis manager before ui manager to invoke analysis manager messenger
  AnalysisManager* analysis = AnalysisManager::GetInstance();

  // Create the run manager (MT or non-MT) and make it a bit verbose.
  auto runManager = new G4RunManager();
  runManager->SetVerboseLevel(1);

  // Set mandatory initialization classes
  runManager->SetUserInitialization(new DetectorConstruction());
  
  // Set Physics list
  // G4VUserPhysicsList* physics = new FTFP_BERT;
  // runManager->SetUserInitialization(physics);
  g4alt::G4PhysListFactory plFactory;
  G4VModularPhysicsList* physicsList = nullptr;
  plFactory.SetDefaultReferencePhysList("NO_DEFAULT_PHYSLIST");

  // set a short name for the plugin
  G4PhysListRegistry* plReg = G4PhysListRegistry::Instance();
  plReg->AddPhysicsExtension("PY8DK", "Py8DecayerPhysics");

  physicsList = plFactory.GetReferencePhysList(physListName);

  if (!physicsList) {
    PrintAvailable(1);

    // if we can't get what the user asked for...
    //    don't go on to use something else, that's confusing
    G4ExceptionDescription ed;
    ed << "The factory for the physicslist [" << physListName << "] does not exist!" << G4endl;
    G4Exception("extensibleFactory", "extensibleFactory001", FatalException, ed);
    exit(42);
  }
  runManager->SetUserInitialization(physicsList);

  // Set user action classes
  runManager->SetUserInitialization(new ActionInitialization());

  // Initialize visualization
  G4VisManager* visManager = new G4VisExecutive();
  visManager->SetVerboseLevel(1);   // Default, you can always override this using macro commands
  visManager->Initialize();

  G4UImanager* UImanager = G4UImanager::GetUIpointer();

  // Parse command line arguments
  if (argc==1) {
    G4UIExecutive* ui = new G4UIExecutive(argc, argv);
    UImanager->ApplyCommand("/control/execute macros/vis.mac");
    ui->SessionStart();
    delete ui;
  } else {
    //G4UIExecutive* ui = new G4UIExecutive(argc, argv);
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command+fileName);
    if (argc==3) {
      G4String mode = argv[2];
      if (mode == "vis") {
        G4UIExecutive* ui = new G4UIExecutive(argc, argv);
        ui->SessionStart();
        delete ui;
      } else {
        G4cout<<"Please specify the second argument as vis to visualize the event"<<G4endl;
        return 0;
      }
    }
  }

  delete visManager;
  delete runManager;

  G4cout<<"Application sucessfully ended.\nBye :-)"<<G4endl;

  return 0;
}


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrintAvailable(G4int verbosity)
{
  G4cout << G4endl;
  G4cout << "extensibleFactory: here are the available physics lists:" << G4endl;
  g4alt::G4PhysListFactory factory;
  factory.PrintAvailablePhysLists();

  // if user asked for extra verbosity then print physics ctors as well
  if (verbosity > 1) {
    G4cout << G4endl;
    G4cout << "extensibleFactory: "
           << "here are the available physics ctors that can be added:" << G4endl;
    G4PhysicsConstructorRegistry* g4pctorFactory = G4PhysicsConstructorRegistry::Instance();
    g4pctorFactory->PrintAvailablePhysicsConstructors();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
