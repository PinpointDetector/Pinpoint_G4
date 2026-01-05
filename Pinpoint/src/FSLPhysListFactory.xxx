/*
  Copyright (C) 2002-2022 CERN for the benefit of the ATLAS collaboration
*/
#include "FSLPhysListFactory.hh"

#include "G4VModularPhysicsList.hh"
#include "G4PhysListFactory.hh"
#include "G4NeutronTrackingCut.hh"

#include "StandardEmWithWoodcock.hh"
#include "EmExtraPhysics.hh"

#include "GeoModelKernel/GeoPluginLoader.h"
#include "FullSimLight/FSLPhysicsListPlugin.h"

#include <string>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

std::unique_ptr<FSLPhysListFactory> FSLPhysListFactory::fgInstance;

bool FSLPhysListFactory::fActivateRegionsFlag;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

const FSLPhysListFactory* FSLPhysListFactory::GetInstance()
{
  return fgInstance.get();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

FSLPhysListFactory::FSLPhysListFactory()
{
  if (!fgInstance) {
    fgInstance=std::unique_ptr<FSLPhysListFactory>(this);
    fActivateRegionsFlag = false;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

FSLPhysListFactory::~FSLPhysListFactory()
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VModularPhysicsList* FSLPhysListFactory::GetPhysList(const std::string physListNameOrPluginPath)
{

    G4VModularPhysicsList* physList = nullptr;

    if ( (physListNameOrPluginPath.find(".dylib") != std::string::npos) || //for macOS devices
         (physListNameOrPluginPath.find(".so") != std::string::npos   )  ) //for Linux devices
    {   //if the physics list comes from the plugin
        std::string pluginPath = physListNameOrPluginPath;

        GeoPluginLoader<FSLPhysicsListPlugin> loader;
        FSLPhysicsListPlugin *plugin = loader.load(pluginPath);

        physList = plugin->GetPhysicsList();
        fActivateRegionsFlag = plugin->GetActivateRegionsFlag();

        if (!physList)
        {
            G4cerr << "ERROR: The physics list is not set through the plugin" << pluginPath <<G4endl;
            exit(-1);
        }
    }
    else 
    {   //if the physics list comes from a predefined name
        
        std::string parPhysListName = physListNameOrPluginPath;

        G4PhysListFactory factory;
        if (factory.IsReferencePhysList(parPhysListName)) {
            physList = factory.GetReferencePhysList(parPhysListName);
        } else if (parPhysListName==G4String("FTFP_BERT_ATL_WDCK")) {
            G4cout << "<<< Geant4 FTFP_BERT_ATL physics list with the local Woodcock settings " << G4endl;
            physList = factory.GetReferencePhysList("FTFP_BERT_ATL");
            // the local em-standard physics with Woodcock tracking for gamma
            StandardEmWithWoodcock* em0AndWDCK = new StandardEmWithWoodcock;
            // set the region name and low energy limit for Woodcock tracking
            em0AndWDCK->SetRegionNameForWoodcockTracking("EMEC");
            em0AndWDCK->SetLowEnergyLimitForWoodcockTracking(200.0*CLHEP::keV);
            physList->ReplacePhysics(em0AndWDCK);
            // the local version of the `G4EmExtraPhysics` that will use the local `GammaGeneralProcess`
            G4VPhysicsConstructor* emExtra = new EmExtraPhysics;
            physList->ReplacePhysics(emExtra);
            //physList->RemovePhysics("G4GammaLeptoNuclearPhys");
            // make sure that regions will also be added to the detector
            fActivateRegionsFlag = true;
        } else {
            G4cerr << "ERROR: Physics List " << parPhysListName << " UNKNOWN!" << G4endl;
            exit(-1);
        }
        // In cases of ATLAS physics lists, set the neutron tracking cut to be 150 [ns] as in Athena
        if (parPhysListName.find("ATL") != std::string::npos) {
            G4NeutronTrackingCut* neutronCut = new G4NeutronTrackingCut("neutronCutphysics", 1);
            neutronCut->SetTimeLimit(150.0*CLHEP::ns);
            physList->ReplacePhysics(neutronCut);
        }
    }

    return physList;

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
