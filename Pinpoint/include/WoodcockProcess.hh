
#ifndef WoodcockProcess_h
#define WoodcockProcess_h 1

#include "GammaGeneralProcess.hh"

#include "G4AffineTransform.hh"

#include <map>
#include <vector>

class G4Region;
class G4VSolid;
class G4Navigator;
class G4MaterialCutsCouple;

/**
 * @file    WoodcockProcess.hh
 * @author  Mihaly Novak
 * @date    29.11.2021
 *
 * Geant4 process for utilising Woodcock tracking in a given detector region.
 *
 * It is assumed that Woodcock tracking is required in a single detector
 * region and the process requires the name of this region. Then Woodcock
 * tracking will be used whenever the Gamma particle is inside this detector
 * region and its energy is above the low energy threshold (that is also
 * an input argument of the process).
 *
 * Requires: (at least) Geant4-11.0 that contains an updated version of the base
 *           G4GammaGeneralProcess. In order to be able to utilise even before
 *           Geant4-11.0 is avaibale, this version of the `WoodcockProcess` is
 *           dervived from a local version of that `G4GammaGeneralProcess` with
 *           the name of `GammaGeneralProcess`. This `GammaGeneralProcess` is
 *           (almost) identical to that will be available in Geant4-11.0.
 */

class WoodcockProcess : public GammaGeneralProcess {

public:

   WoodcockProcess(const G4String& wdckRegionName, G4double wdckLowELimit=0.0);
  ~WoodcockProcess() override;


  void     StartTracking(G4Track*) override;

  void     PreparePhysicsTable(const G4ParticleDefinition&) override;

  void     BuildPhysicsTable(const G4ParticleDefinition&) override;

  G4double PostStepGetPhysicalInteractionLength(const G4Track& track, G4double previousStepSize, G4ForceCondition* condition) override;

  G4VParticleChange* PostStepDoIt(const G4Track&, const G4Step&) override;



private:

  // This will be called only once when the very first gamma makes the very first
  // step inside the WDCK region and all the WDCK related initialisations, that
  // requires a closed geometry (and a global point inside the WDCK region), will
  // be done.
  void     InitWoodcockTracking(const G4ThreeVector& aPointInsideTheWDCKregion, const G4ThreeVector& aDirectionInto);

  // After the above init, this method can be used to compute the Distance to the
  // boundary of the (single) root volume of the WDCK region from the given (global)
  // point along the given direction.
  G4double ComputeStepToWDCKRegionBoundary(const G4ThreeVector& pGlobalpoint, const G4ThreeVector& pDirection);

  // Computes the total (i.e. sum of individual processes) macroscopic cross section
  // for the given material and photon energy (computed only if any of those has changed)
  void     ComputeTotalMacrCrossSection(const G4MaterialCutsCouple* coupe, const G4double energy, const G4double logenergy);

  void     FindWDCKRootLogicalVolume(const G4Track& track);

private:

  // WDCK tracking data for a single root logical volume of the WDCK region
  struct WDCKDataPerRootLVolume {
    G4AffineTransform  fAffineTransform;
    G4LogicalVolume*   fLogicalVolume;
    G4Navigator*       fNavigator;
    G4VSolid*          fSolid;
    G4bool             fNeedsInit;
  };

  // name of the region where WDCK tracking is required: given at construction
  // NOTE: if region is not found with the given name then no WDCK tracking is used
  const G4String         fWDCKRegionName;
  // low energy limit below which normal tracking will be used even inside the WDCK region
  G4double               fWDCKLowEnergyThreshold;

  // number of root logical volumes of the WDCK region
  std::size_t            fWDCKNumRootLVolume;
  // a `map` of the WDCK root logical volume IDs to continous indices
  std::map<G4int, G4int> fWDCKRootLVolIDToIndxMap;

  // pointer to the WDCK region (single region is assumed)
  G4Region*              fWDCKRegion;
  // the heaviest material-cuts of the (single) WDCK region
  G4MaterialCutsCouple*  fWDCKCouple;

  // flag to indicate that a WDCK region step was limited by the WDCK region boundary
  G4bool                 fOnWDCKRegionBoundary;
  // flag to indicate that a step was performed inside the WDCK region
  G4bool                 fInWDCKRegion;
  // the current root logical volume index: under which the current tracking
  // happens inside the WDCK region
  G4int                  fWDCKRootLVolumeIndx;
  // length of the step done in the WDCK region
  G4double               fWDCKStepLength;
  

  // data for each of the root logical volume of the WDCK region
  std::vector<WDCKDataPerRootLVolume>  fWDCKDataPerRootLVolume;

};

#endif // WoodcockProcess_h
