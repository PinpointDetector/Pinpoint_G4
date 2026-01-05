//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
//
//
//---------------------------------------------------------------------------
//
// ClassName:   StandardEmWithWoodcock
//
// Author:      V.Ivanchenko 09.11.2005
//
// Modified:
// 05.12.2005 V.Ivanchenko add controlled verbosity
// 23.11.2006 V.Ivanchenko remove mscStepLimit option and improve cout
// 17.11.2021 M. Novak: added Woodcock tracking in a given detector region
//
//
// NOTE: M. Novak. 27.11.2021
//
// This EM physics constructor is almost identical to the EM standard (Em-opt0)
// physics constructor with the possibility of utilising Woodcock-tracking in a
// given detector region. The name of the region needs to be provided to the
// `WoodcockProcess` at construction. The `WoodckockProcess` falls into the base
// Gamma-General process (a local version of the `G4GammaGeneralProcess`) when:
// - the given detector region cannot be found in the detector
// - the gamma energy is below a given threshold (by default 0; can be set at
//   construction time)
// As in the case of the base Gamma-General process, all the processes neeeds to
// be assigned to the `G4Gamma` particle through the `WoodckockProcess` (and not
// directly!).
//
//----------------------------------------------------------------------------
//
// This class provides construction of default EM standard physics
//

#ifndef StandardEmWithWoodcock_h
#define StandardEmWithWoodcock_h 1

#include "G4VPhysicsConstructor.hh"
#include "G4EmParticleList.hh"
#include "globals.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class StandardEmWithWoodcock : public G4VPhysicsConstructor
{
public:

  explicit StandardEmWithWoodcock(G4int ver=1, const G4String& name="StandardEmWithWoodcock");

  ~StandardEmWithWoodcock() override;

  void ConstructParticle() override;
  void ConstructProcess() override;


  void SetRegionNameForWoodcockTracking(const G4String& rname) { fWDCKRegionName = rname; }
  void SetLowEnergyLimitForWoodcockTracking(G4double val)      { fWDCKLowEnergyThreshold = val; }

private:

  G4int verbose;
  G4EmParticleList partList;

  G4String         fWDCKRegionName;
  G4double         fWDCKLowEnergyThreshold;

};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
