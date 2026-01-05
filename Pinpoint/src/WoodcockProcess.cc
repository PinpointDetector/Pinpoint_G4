#include "WoodcockProcess.hh"

#include "G4Version.hh"
#include "G4Region.hh"
#include "G4VSolid.hh"
#include "G4RegionStore.hh"


#include "G4TransportationManager.hh"
#include "G4Navigator.hh"
#include "G4MaterialCutsCouple.hh"


WoodcockProcess::WoodcockProcess(const G4String& wdckRegionName, G4double wdckLowELimit)
: GammaGeneralProcess("Woodcock-GG"),
  fWDCKRegionName(wdckRegionName),
  fWDCKLowEnergyThreshold(wdckLowELimit),
  fWDCKNumRootLVolume(0),
  fWDCKRegion(nullptr),
  fWDCKCouple(nullptr),
  fOnWDCKRegionBoundary(false),
  fInWDCKRegion(false),
  fWDCKRootLVolumeIndx(-1),
  fWDCKStepLength(0.0) {
  preStepKinEnergy = -1.0;
  preStepLambda    = 0.0;
  currentCouple    = nullptr;
}


WoodcockProcess::~WoodcockProcess() {
  // delete the local navigator used to locate the post-step point in WDCK tracking
  for (std::size_t iv=0; iv<fWDCKNumRootLVolume; ++iv) {
    if (fWDCKDataPerRootLVolume[iv].fNavigator) {
      delete fWDCKDataPerRootLVolume[iv].fNavigator;
    }
  }
}


void WoodcockProcess::StartTracking(G4Track* track) {
  // set initial values of some flags
  fOnWDCKRegionBoundary = false;
  fWDCKRootLVolumeIndx  = -1;
  // call the base class method
  GammaGeneralProcess::StartTracking(track);
}


void WoodcockProcess::PreparePhysicsTable(const G4ParticleDefinition& part) {
  GammaGeneralProcess::PreparePhysicsTable(part);
}

// Init the WDCK tracking if the detector region, specified by the user giving
// its name, was found. Only partial initialisation is done here: finding the
// heaviest material of the region and setting the size of containers that stores
// the individual root logical volume related information. The other part of the
// init is done when the fist gamma track enters to the given volumes (this is
// because the geometry must be closed for getting further require information).
void WoodcockProcess::BuildPhysicsTable(const G4ParticleDefinition& part) {
  // try to find the WDCK region in the store by its name given by the user
  fWDCKRegion = G4RegionStore::GetInstance()->GetRegion(fWDCKRegionName);
  if (fWDCKRegion != nullptr) {
    fWDCKNumRootLVolume = fWDCKRegion->GetNumberOfRootVolumes();
    G4AffineTransform aTransform;
    fWDCKDataPerRootLVolume.resize(fWDCKNumRootLVolume, {aTransform, nullptr, nullptr, nullptr, true});
    // build the root logical volume ID --> vector index (in the fWDCKDataPerRootLVolume) map
    std::vector<G4LogicalVolume*>::const_iterator itrLV = fWDCKRegion->GetRootLogicalVolumeIterator();
    for (std::size_t ilv = 0; ilv<fWDCKNumRootLVolume; ++ilv) {
      fWDCKRootLVolIDToIndxMap[(*itrLV)->GetInstanceID()] = ilv;
      fWDCKDataPerRootLVolume[ilv].fLogicalVolume         = (*itrLV);
      ++itrLV;
    }
    // loop over the materials of this region and get the couple of the heaviest
    std::vector<G4Material*>::const_iterator itr = fWDCKRegion->GetMaterialIterator();
    const G4int nmat = fWDCKRegion->GetNumberOfMaterials();
    G4double maxDensity =0.0;
    for (G4int im=0; im<nmat; ++im) {
      const G4double density = (*itr)->GetDensity();
      if (density>maxDensity) {
        maxDensity  = density;
        fWDCKCouple = fWDCKRegion->FindCouple((*itr));
      }
      ++itr;
    }
  }
  // call the underlying Gamma-general process `BuildPhysicsTable`
  GammaGeneralProcess::BuildPhysicsTable(part);
}


G4double
WoodcockProcess::PostStepGetPhysicalInteractionLength(const G4Track& track, G4double previousStepSize, G4ForceCondition* condition) {
  // check if this step will be performed within the Woodcock region
  *condition          = G4ForceCondition::NotForced;
  G4double stepLength = DBL_MAX;
  fInWDCKRegion       = false;
  // NOTE: this is in order to get out from the Woodcock Region
  //
  //       Since we make a dirty hack of the navigation/transportation,
  //       the GPIL and DoIt methods of transportation are not called properly
  //       and boundary crossing is not recognised even if we `put` the track on
  //       the boundary. So we just detect when it happenes based on this flag
  //       set in the previous step limit:
  //        - setting the `fOnWDCKRegionBoundary` flag to be true when reaching
  //          the WDCK top volume boundary (see well below)
  //        - shorten a tiny bit of the overall step length inside the WDCK top
  //          volume in order to arrive in the vicinity of the WDCK top volume
  //          bundary (at the end of the pevious step)
  //        - also note, that the DoIt method detects this case and there is no
  //          interaction in at the end of the previous step
  //       So now we are very close to the WDCK top volume boundary and just
  //       giving a step length that ensures that Transportation wins that makes
  //       sure the appropriate exit from the WDCK region.
  //       Therefore, at the very end of the WDCK tracking of a photon in the
  //       WDCK root volume, this top volume is left in 2 "transportation" steps:
  //       one fake that just brings close (< 1.0E-4) to the boundary and a real
  //       one that will make an appropriate boundary crossing step.
  if (fOnWDCKRegionBoundary) {
    fOnWDCKRegionBoundary = false;
    // we are already reached the WDCK region boundary at the end of the previous
    // step: just give a step length > 1.0E-4 and let the transportation win this
    //       time normaly (this will set everything properly)
    fWDCKStepLength      = 10.0;
    fWDCKRootLVolumeIndx = -1;
    return fWDCKStepLength;
  }

  // Check if WDCK tracking was required ,i.e. if the WDCK region was found at init.,
  // and the current step is performed inside this region with a high enough energy.
  if (fWDCKRegion !=nullptr && fWDCKRegion == track.GetVolume()->GetLogicalVolume()->GetRegion() && track.GetKineticEnergy()>fWDCKLowEnergyThreshold) {
    fInWDCKRegion = true;
    // Need to find the root logical volume of this branch: only in the very
    // first step in this branch, i.e. when entering or just first step after
    // created in this branch.) After this, fWDCKRootLVolumeIndx is set for sure.
    if (fWDCKNumRootLVolume > 1) {
      FindWDCKRootLogicalVolume(track);
    } else {
      fWDCKRootLVolumeIndx = 0;
    }
    // Compute the distance to WDCK region root volume boundary
    // - get direction and starting position
    const G4ThreeVector& r0 = track.GetPosition();
    const G4ThreeVector& v0 = track.GetMomentumDirection();
    // This method is called only once when the very firts gamma that makes its
    // fist step inside this branch (i.e. below this root logical volume) of the
    // WDKC region: initialize the local navigation and transformation for this
    // root logical volume of the WDCK region.
    if (fWDCKDataPerRootLVolume[fWDCKRootLVolumeIndx].fNeedsInit) {
      InitWoodcockTracking(r0, v0);
      fWDCKDataPerRootLVolume[fWDCKRootLVolumeIndx].fNeedsInit = false;
    }
    G4double distToBoundary = ComputeStepToWDCKRegionBoundary(r0, v0);
    // Compute the total macroscopic cross section in the WDCK material
    // (result will be in `preStepLambda`)
    const G4double ekin  = track.GetKineticEnergy();
    const G4double lekin = track.GetDynamicParticle()->GetLogKineticEnergy();
    ComputeTotalMacrCrossSection(fWDCKCouple, ekin, lekin);
    const G4double invWDCKMXsec = preStepLambda > 0.0 ? 1.0/preStepLambda : DBL_MAX;
    // Set initial values for the step length and the flag that indicates ending
    // the WDCK root volume boundary on boundary
    fWDCKStepLength       = 0.0;
    fOnWDCKRegionBoundary = false;
    stepLength            = 0.0;
    G4bool doStop         = false;
    G4Navigator* localNav = fWDCKDataPerRootLVolume[fWDCKRootLVolumeIndx].fNavigator;
    const G4Material* wdckMaterial = fWDCKCouple->GetMaterial();
    // Untill we either interact or reach the boundary of the WDCK region:
    while (true) {
      // Compute the step length till the next interaction in the WDCK material
      const G4double pstep = invWDCKMXsec < DBL_MAX ? -G4Log( G4UniformRand() )*invWDCKMXsec : DBL_MAX;
      // Take the minimum of this and the distance to the WDCK root volume boundary
      // Check if this step will end up on the WDCK region boundary
      if (distToBoundary < pstep) {
        stepLength           += distToBoundary;
        fOnWDCKRegionBoundary = true;
        doStop = true;
      } else {
        // We will move the particle by a step length of `pstep` so we reduce the
        // distance to boundary with teh same length.
        stepLength     += pstep;
        distToBoundary -= pstep;
        // Locate the actual post step point in order to get the real material.
        // Use relative search (true) since we already located a point at the
        // local navigator init inside the WDCK region and ignore direction (true)
        const G4VPhysicalVolume* pVol = localNav->LocateGlobalPointAndSetup(r0+stepLength*v0, nullptr, true, true);
        const G4LogicalVolume*   lVol = pVol->GetLogicalVolume();
        const G4Material* postStepMat = lVol->GetMaterial();
        // Check if the real material of the post-step point is the WDCK one
        if (wdckMaterial != postStepMat) {
          // The post step point is not in the WDCK material: need to check if we interact
          // Compute the total macroscopic cross section for the real, non-wdck material
          // (the result will be in `preStepLambda`)
          const G4MaterialCutsCouple* couple = lVol->GetMaterialCutsCouple();
          ComputeTotalMacrCrossSection(couple, ekin, lekin);
          // P(interact) = preStepLambda/wdckMXsec note: preStepLambda <= wdckMXsec
          doStop = (preStepLambda*invWDCKMXsec > G4UniformRand()); // interact
        } else {
            // Interact since the post step point is in the wdck material which cross section was used
            // Make sure that all members of the underlying gamma-general set properly so re-compute
            // the partial macroscopic cross sections (if needed)
            ComputeTotalMacrCrossSection(fWDCKCouple, ekin, lekin);
            doStop = true;
        }
      }
      if (doStop) {
        // Reached the end, i.e. either interaction happens at the current post step point
        // or it is locating at the WDCK reagion boundary (fOnWDCKRegionBoundary=true).
        // Update the track position and return with zero step lenght (transportation/navigation)
        // will correctly re-locate the post-step point of the track while we get back to the DoIt()
        // BUT we do it by our self in order to avoid Warnings!
        // NOTE: the material-cuts couple and the material that will be used at
        //       the post-step DoIt in case of interaction are those that are in
        //       `currentCouple` and `currentMaterial`, etc. and not taken from
        //       the track. Therefore, as long as we are inside the given WDCK
        //       root volume, we do not care much if Transporation relocates or
        //       not the track. If the gamma leaves the given WDCK root volume,
        //       we make sure that a normal transportation step will transverse
        //       the volume boundary.
        G4Track& theTrack = const_cast<G4Track&>(track);
        // shorten a bit this step length if we would reach the boundary:
        //  - WoodcockProcess limits the step but we see in the DoIt that actually its boundary
        //  - the next step will be real Transportation since Woodcock will propose a longer step
        //  - this will bring us properly to the next boundary in the next step
        double sl = stepLength;
        if (fOnWDCKRegionBoundary) {
          sl = sl > 1.0E-4 ? sl-1.0E-4 : sl*0.999;
        }
        fWDCKStepLength = sl;
        theTrack.SetPosition(track.GetPosition()+sl*track.GetMomentumDirection());
        // relocate the moved G4Track in order to avoid Navigator warnings
        G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking()->LocateGlobalPointAndUpdateTouchableHandle(track.GetPosition(), track.GetMomentumDirection(), const_cast<G4TouchableHandle&>(track.GetTouchableHandle()));
//        theTrack.GetStep()->GetPostStepPoint()->SetTouchableHandle(track.GetTouchableHandle());
        //
        return 0.0;
      }
      // If the WDCK region boundary has not been reached in this step then delta interaction
      // happend so just keepKeep moving the post-step point toward to the WDCK region boundary otherwise by
      // sampling and performing yet an other step
    };
  } else {
    // This step will be performed OUTSIDE the WDK region: compute the step exactly as the
    // the Gamma-general process PostStepGetPhysicalInteractionLength
    return GammaGeneralProcess::PostStepGetPhysicalInteractionLength(track, previousStepSize, condition);
  }
}


// Computes the path length to the WDCK root volume boundary.
// NOTE: assumes that there is a single WDCK region with a single root volume.
G4double
WoodcockProcess::ComputeStepToWDCKRegionBoundary(const G4ThreeVector& pGlobalPoint, const G4ThreeVector& pDirection) {
   const G4AffineTransform& transform = fWDCKDataPerRootLVolume[fWDCKRootLVolumeIndx].fAffineTransform;
   const G4ThreeVector localPoint     = transform.TransformPoint(pGlobalPoint);
   const G4ThreeVector localDirection = transform.TransformAxis(pDirection);
   return  fWDCKDataPerRootLVolume[fWDCKRootLVolumeIndx].fSolid->DistanceToOut(localPoint, localDirection );
}


G4VParticleChange*
WoodcockProcess::PostStepDoIt(const G4Track& track, const G4Step& step) {
  // Check if the step was done inside the WDCK region with DWCK tracking conditions
  if (fInWDCKRegion) {
    // Set the step length in the track in order to give correct value for the user
    const_cast<G4Track&>(track).SetStepLength( fWDCKStepLength );
    const_cast<G4Step&>(step).SetStepLength( fWDCKStepLength );
    // Do nothing else if the step ended on the WDCK region boundary
    if (fOnWDCKRegionBoundary) {
      // boundary limited: no interaction
      // set the number of interaction legth left to restart tracking correctly
      theNumberOfInteractionLengthLeft = -1.0;
      fParticleChange.InitializeForPostStep(track);
//      std::cout << " I should set Transportation as limiter becasue actually its the WoodcockProcess itself" << std::endl;
      return &fParticleChange;
    }
    // NOTE: make the user SteppingAction of THIS APPLICATION to see the material
    //       in which the intercation happend! This is becuase of the WDCK!!!
    step.GetPreStepPoint()->SetMaterialCutsCouple(currentCouple);
    step.GetPreStepPoint()->SetMaterial(const_cast<G4Material*>(currentCouple->GetMaterial()));
//    step.GetPostStepPoint()->SetMaterialCutsCouple(currentCouple);
//    step.GetPostStepPoint()->SetMaterial(const_cast<G4Material*>(currentCouple->GetMaterial()));
    return GammaGeneralProcess::PostStepDoIt(track, step);
  } else {
    // Normal, i.e. non-WDCK tracking otherwise
    return GammaGeneralProcess::PostStepDoIt(track, step);
  }
  return &fParticleChange;
}


//
// NOTE: - we need to get the touchable of the Woodcock region root volume in order
//         to obtain the transformations needed at run time to compute the distance
//         to the root volume boundary (i.e. calorimeter boundary)
//       - we obtain it by locating a point that we know that for sure is inside
//         the Woodckock region and we move up till we are in the Woodcock region
//       - the problem is that:
//         1. we need to know a position that is inside
//         2. we need to do this after the geometry is closed (i.e. cannot be done
//            at normal process init) only here at start tracking
//
// This is why we do these initialisations (only once) when the very first gamma
// makes its very first step inside the given branch of the WDCK region, i.e. under
// the given root logical volume of the WDCK region: --> we know a global point
// that is inside this branch the geometry is already closed.
//
void
WoodcockProcess::InitWoodcockTracking(const G4ThreeVector& aPointInsideTheWDCKregion, const G4ThreeVector& aDirectionInto) {
    // create the local navigator for this branch of the WDCK region and locate
    // a point inside this branch then obtain the root volume transformation of this branch
    G4TouchableHandle theWDCKRegionTouchable;
    G4Navigator* nav = new G4Navigator();
    fWDCKDataPerRootLVolume[fWDCKRootLVolumeIndx].fNavigator = nav;
    nav->SetWorldVolume(G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking()->GetWorldVolume());
    nav->LocateGlobalPointAndUpdateTouchableHandle( aPointInsideTheWDCKregion, aDirectionInto, theWDCKRegionTouchable, false);
    // Keep moving up till we reach the root logical volume (of the given branch) of the WDCK region
    const G4LogicalVolume* wdckRootLVol = fWDCKDataPerRootLVolume[fWDCKRootLVolumeIndx].fLogicalVolume;
    while (wdckRootLVol != theWDCKRegionTouchable->GetVolume()->GetLogicalVolume()) {
      theWDCKRegionTouchable->MoveUpHistory();
    }
    // Obtain the transformation and the solid of the root volume of the WDCK region
    // that are required for the run-time computation of the distance to out/boundary.
    const G4NavigationHistory* navHistory = theWDCKRegionTouchable->GetHistory();
    fWDCKDataPerRootLVolume[fWDCKRootLVolumeIndx].fAffineTransform = navHistory->GetTopTransform();
    // Get the solid of the WDCK root volume
    const G4VPhysicalVolume* motherPhysical = navHistory->GetTopVolume();
    const G4LogicalVolume*   motherLogical  = motherPhysical->GetLogicalVolume();
    fWDCKDataPerRootLVolume[fWDCKRootLVolumeIndx].fSolid = motherLogical->GetSolid();
}


void WoodcockProcess::ComputeTotalMacrCrossSection(const G4MaterialCutsCouple* couple, const G4double energy, const G4double logenergy) {
  // compute macroscopic cross section only if the material and/or energy has changed
  G4bool isRecompute = false;
  if (couple != currentCouple) {
    currentCouple      = couple;
    currentCoupleIndex = couple->GetIndex();
    currentMaterial    = couple->GetMaterial();
#if G4VERSION_NUMBER>=1100
    basedCoupleIndex   = DensityIndex(currentCoupleIndex);
    factor             = DensityFactor(currentCoupleIndex);
#else
    basedCoupleIndex   = (*theDensityIdx)[currentCoupleIndex];
    factor             = (*theDensityFactor)[currentCoupleIndex];
#endif
    isRecompute = true;
  }
  if (energy != preStepKinEnergy) {
    preStepKinEnergy = energy;
    preStepLogE      = logenergy;
    isRecompute = true;
  }
  if (isRecompute) {
    preStepLambda = std::max(0.0, TotalCrossSectionPerVolume());
  }
}


// This method is supposed to be called only from inside the WDCK region and it
// will set the index of the root logical volume inside which the current step
// will be done.
void WoodcockProcess::FindWDCKRootLogicalVolume(const G4Track& track) {
  // no need to reset since we have not left the branch yet, i.e. the same root LV
  if (fWDCKRootLVolumeIndx > -1) { return; }
  // get the navigation history and keep moving up till a root logical volume of
  // the WDCK region is found, then set the current root logical volume index
  const G4VTouchable* tchb = track.GetTouchable();
  std::map<G4int, G4int>::iterator itr;
  G4int depth = -1;
  do {
    ++depth;
    G4int id = tchb->GetVolume(depth)->GetLogicalVolume()->GetInstanceID();
    itr = fWDCKRootLVolIDToIndxMap.find(id);
  } while ( itr == fWDCKRootLVolIDToIndxMap.end() );
  fWDCKRootLVolumeIndx = itr->second;
}
