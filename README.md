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

There are a number of user-defined macro commands (Geant4 UI commands, defined by this app's own
messenger classes) which control the simulation. These must be issued *before* `/run/initialize` in
a macro file (see `Pinpoint/macros/geom.mac` and `Pinpoint/macros/gps.mac` for worked examples).
Defaults below are the values used if a command is never issued at all (i.e. the C++ member's
built-in default), which is not always the same as the `G4UIcommand`'s own internal default value.

### Geometry commands (`/det/`)

#### Detector layout

|Command |Description | Default |
|:--|:--|:--|
|`/det/setNumPinpointLayers`| Number of initial Pinpoint (tungsten + pixel) blocks placed before the first Fortune block | `6` |
|`/det/setNumFortuneBlocks`| Number of Fortune (scintillator) blocks; an intermediate Pinpoint block is automatically interleaved between each pair | `7` |
|`/det/setNumIPTLayers`| Number of trailing Interface Pixel Tracker (IPT) pixel layers | `3` |
|`/det/enableFaserSpectrometer`| Enable the FASER spectrometer magnets, tracking stations and magnetic field | `true` |

#### Pinpoint module (tungsten + pixel sensor)

|Command |Description | Default |
|:--|:--|:--|
|`/det/setTungstenThickness` | Tungsten plate thickness (shared by Pinpoint and Fortune modules) in mm | `5 mm`|
|`/det/setSiliconThickness` | Pixel silicon sensor thickness in $\mu$m | `10 um`|
|`/det/setPixelHeight`| Height of an individual pixel in $\mu$m | `22.8 um` |
|`/det/setPixelWidth`| Width of an individual pixel in $\mu$m | `20.8 um` |
|`/det/setDetectorWidth` | Width of the pixel sensor in cm (also sets the number of pixel columns, `width / pixelWidth`) | `25 cm` |
|`/det/setDetectorHeight` | Height of the pixel sensor in cm (also sets the number of pixel rows, `height / pixelHeight`) | `20 cm` |
|`/det/setPixelDetectorOffsetX`| X offset of the pixel sensor (shifts pixel columns) in mm | `0 mm` |
|`/det/setPixelDetectorOffsetY`| Y offset of the pixel sensor (shifts pixel rows) in mm | `0 mm` |

#### Fortune module (scintillator)

|Command |Description | Default |
|:--|:--|:--|
|`/det/setNumScintLayers`| Number of scintillator layer-pairs (tungsten + up to 2 scintillator panels) per Fortune block | `8` |
|`/det/setScintThickness`| Thickness of a single scintillator panel in mm | `5 mm` |
|`/det/setScintDetectorOffsetX`| X offset of the vertical scintillator bars in mm | `0 mm` |
|`/det/setScintDetectorOffsetY`| Y offset of the horizontal scintillator bars in mm | `0 mm` |
|`/det/setNumScintPanelsPerLayer` | Number of scintillator panels built per scint layer: `0`=neither (tungsten sampling only), `1`=vertical panel only, `2`=both vertical and horizontal | `2` |
|`/det/setScintWidth` | Scintillator panel X-extent in cm. Drives the horizontal bars' length and the vertical bars' pitch/segmentation | `42 cm` |
|`/det/setScintHeight` | Scintillator panel Y-extent in cm. Drives the vertical bars' length and the horizontal bars' pitch/segmentation | `42 cm` |
|`/det/setScintBarWidth` | Vertical bar's narrow (X-segmented) cross-axis width in mm | `10 mm` |
|`/det/setScintBarHeight` | Horizontal bar's narrow (Y-segmented) cross-axis width in mm | `10 mm` |

> **Note:** each of the `setNumScintLayers` layers is built, in Z order, as: tungsten plate,
> vertical bar plane, air gap, horizontal bar plane. The air gap sits between the vertical and
> horizontal bar planes, and layers are back-to-back (no extra gap between one layer's horizontal
> plane and the next layer's tungsten plate). The whole stack of layers is sandwiched between the
> two aluminum walls.

> **Note:** bar width/height must stay comfortably below the pitch implied by the panel
> extent divided by the (currently fixed, not user-settable) number of bars per panel — 40 by
> default, giving a 10.5mm pitch at the default 42cm panel size. Setting a bar width/height at or
> above that pitch will produce overlapping volumes, which Geant4's overlap checking (already
> enabled throughout this geometry) will flag at construction time.

#### Aluminum walls

|Command |Description | Default |
|:--|:--|:--|
|`/det/setAluminumWallThickness`| Thickness of the aluminum wall placed before and after each Pinpoint/Fortune block, in mm | `2 mm` |
|`/det/setAluminumWallWidth`| Transverse width of the aluminum walls in cm | `55 cm` |
|`/det/setAluminumWallHeight`| Transverse height of the aluminum walls in cm | `60 cm` |

#### Output

|Command |Description | Default |
|:--|:--|:--|
|`/det/setGDMLFile`| Output filename for the `gdml` geometry dump | `pinpoint.gdml` |

### Generator commands (`/gen/`)

|Command |Description | Default |
|:--|:--|:--|
|`/gen/select`| Select the primary generator: `gun`, `genie`, `hepmc` or `gfaser` | `gun` |

The `gun` generator is configured with Geant4's built-in General Particle Source `/gps/...`
commands (see the [Geant4 GPS documentation](https://geant4-userdoc.web.cern.ch/UsersGuides/ForApplicationDeveloper/html/GettingStarted/generalParticleSource.html));
`Pinpoint/macros/gps.mac` has a worked example. The other three generators have their own
sub-directories:

#### `/gen/hepmc/`

|Command |Description | Default |
|:--|:--|:--|
|`/gen/hepmc/hepmcInput`| Input filename for the HepMC generator | *(none — required)* |
|`/gen/hepmc/vtxOffset`| Offset `x y z <unit>` applied to the primary vertex; useful when there's a mismatch in the geometry | `0 0 0 mm` |
|`/gen/hepmc/useHepMC2`| Read the input file as HepMC2 instead of HepMC3 | `false` |
|`/gen/hepmc/placeInDecayVolume`| Translate the vertex into the FASER2 decay volume. Assumes HepMC vertices start from `(0,0,0)` (set `vtxOffset` if not) and that decay volume lengths match | `true` |

#### `/gen/gfaser/`

|Command |Description | Default |
|:--|:--|:--|
|`/gen/gfaser/inputFile`| Input filename for the gfaser generator | *(none — required)* |
|`/gen/gfaser/hitPixelArea`| If `true`, skip events whose neutrino transverse position lands outside the pixel detector footprint | `false` |

#### `/gen/genie/`

|Command |Description | Default |
|:--|:--|:--|
|`/gen/genie/genieInput`| Input `.ghep`/`.gst` filename for the GENIE generator | *(none — required)* |
|`/gen/genie/genieIStart`| Index of the starting event in the input file | `0` |
|`/gen/genie/randomVtx`| Randomize the vertex position within the fiducial volume | `false` |

### Output file commands (`/out/`)

|Command |Description | Default |
|:--|:--|:--|
|`/out/fileName`     | Name of the output ROOT file that all analysis variables are saved to | `test.root` |
|`/out/saveTrack`    | If `true`, save all track information (requires `/tracking/storeTrajectory 1` to also be set) | `false` |
|`/out/saveTruthHits`| If `true`, save truth hit x/y/z position | `false` |

> **Note:** `/out/saveTruthHits` currently has no effect — the code path that would use it
> (`AnalysisManager.cc`) is commented out, so this command is a no-op for now regardless of value.

### Submission
Use the `Pinpoint/submission/submit.py` script to submit jobs to HTCondor. The script uses the Pinpoint version build in `/eos/project/f/fasersim-bonn/public/pinpoint/Pinpoint_G4/Pinpoint/build` and expects a macro file in `/eos/project/f/fasersim-bonn/public/pinpoint/data/run` with the details. You can specify the run number, total number of events and number of events per jobs as command line arguments. The CERN HTCondor supports only submissions from a `afs` space, so you have to copy the submission script to your personal `afs` directory, but the samples will be produced in the common `eos` space.
