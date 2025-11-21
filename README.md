<h1 align="center">
<img src="docs/logo.png" width="300">
</h1><br>

# Pixel Instrument For Precision Neutrino Observations

[![CI - Build Check](https://github.com/PinpointDetector/Pinpoint_G4/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/PinpointDetector/Pinpoint_G4/actions/workflows/build.yml)

## Introduction

This is the GEANT4 simulation code for Pinpoint, a pixel-based neutrino detector concept for the FASER Run 4 upgrade.

This code is based off the [`FPFSim` GEANT4 simulation](https://github.com/FPFSoftware/FPFSim) written by [Matteo Vicenzi](https://github.com/mvicenzi) and [Wenjie Wu](https://github.com/WenjieWu-Sci).

## Getting started

This code can be compiled on `lxplus` or any Alma Linux9 machine with `cvmfs` access. To install this code do:

```bash
git clone https://github.com/PinpointDetector/Pinpoint_G4.git
cd Pinpoint_G4
source Pinpoint/setup.sh
mkdir build && cd build
cmake ../Pinpoint
make -j 8
```

In case your machine is not suitable, you can compile and run the code in a Docker container. An el9 Docker container which mimics the lxplus environment is available from [DockerHub](https://hub.docker.com/layers/benw22022/faser/el9-cvmfs/images/sha256-e6cffa8f752e192eae60b134dd28fb34682d257e02eed9355d17986c186ae116?context=repo).

A repository containing a script to easily run the container and mount `cvmfs` is available from [github.com/benw22022/el9-cvmfs-docker](https://github.com/benw22022/el9-cvmfs-docker?tab=readme-ov-file)

To get started do:

```bash
git clone https://github.com/benw22022/el9-cvmfs-docker.git
cd el9-cvmfs-docker/
./run_container /path/to/Pinpoint_G4
```

## Macro commands

There are a number of user defined macro commands which can be used to control the simulation.

### Geometry Commands

|Command |Description | Default |
|:--|:--|:--|
|`/det/setTungstenThickness` | Tungsten sheet thickness in mm | `5 mm`|
|`/det/setSiliconThickness` | Silicon layer thickness in $\mu$m | `50 um`|
|`/det/setNLayers` | Number of tungsten/pixel layer | `100`|
|`/det/setPixelHeight`| Set the height of an individual pixel in $\mu$m | `22.8 um` |
|`/det/setPixelWidth`| Set the width of an individual pixel in $\mu$m | `20.8 um` |
|`/det/setDetectorWidth` | Set the width of the detector in cm | `26.6` |
|`/det/setDetectorHeight` | Set height of the detector in cm | `19.6` |
|`/det/setGDMLFile`| Set the output file for the `gdml` file | `pinpoint.gdml` |

### Output file commands

|Command |Description |
|:--|:--|
|/out/fileName     | option for AnalysisManagerMessenger, set name of the file saving all analysis variables|
|/out/saveTrack    | if `true` save all tracks, `false` by default, requires `\tracking\storeTrajectory 1`|
|/out/saveTruthHits| if `true` save truth hit x, y, z position, `false` by default|

### Next steps
- [ ] Geometry (Dhruv)
  - [ ] Add scintillator layers
  - [ ] Check deposited energy (smear with Landau distribution)
  - [ ] Add messenger to enable/disable from macro files
- [ ] Neutrino Interactions (Tobi)
  - [x] Set z-position of vertex to random position within one layer
  - [ ] Check charm decays (Tobi)
- [ ] Output (Ben)
  - [x] Add parent track id, or write out tau decay products
  - [ ] Check file size including true position of all particles
- [ ] Add HTCondor submission script
