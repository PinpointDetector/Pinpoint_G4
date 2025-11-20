#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.8...3.18)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()
project(pinpoint)

##----------------------------------------------------------------------------
# Find Pythia8 (required package)
set(PYTHIA8_PREFIX "/cvmfs/sft.cern.ch/lcg/views/LCG_107/x86_64-el9-gcc11-opt")

add_library(Pythia8::Pythia8 UNKNOWN IMPORTED)

set_target_properties(Pythia8::Pythia8 PROPERTIES
    IMPORTED_LOCATION "${PYTHIA8_PREFIX}/lib/libpythia8.so"
    INTERFACE_INCLUDE_DIRECTORIES "${PYTHIA8_PREFIX}/include"
)

#----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default
# You can set WITH_GEANT4_UIVIS to OFF via the command line or ccmake/cmake-gui
# to build a batch mode only executable
#
option(WITH_GEANT4_UIVIS "Build example with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

#----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
#
include(${Geant4_USE_FILE})

#----------------------------------------------------------------------------
# Find HepMC3 (required package)
#
find_package(HepMC3 REQUIRED)  # <-- For HepMC3
# find_package(HepMC REQUIRED) # <-- For HepMC2

#----------------------------------------------------------------------------
# Find ROOT (required package)
#
find_package(ROOT REQUIRED COMPONENTS Geom EG RIO) 
include(${ROOT_USE_FILE})
message(STATUS "Set ROOT : ${ROOT_USE_FILE}")
message(STATUS "ROOT : ${ROOT_LIBRARIES}")

#----------------------------------------------------------------------------
# Locate sources and headers for this project
#
include_directories(${PROJECT_SOURCE_DIR}/include 
                    ${Geant4_INCLUDE_DIR}
                    ${HEPMC3_INCLUDE_DIR}
                    ${ROOT_INCLUDE_DIR}
                    ${PYTHIA8_INCLUDE_DIRS}
                    )

file(GLOB_RECURSE sources ${PROJECT_SOURCE_DIR}/src/*.cc 
                    ${PROJECT_SOURCE_DIR}/src/reco/*.cc
                    ${PROJECT_SOURCE_DIR}/src/generators/*.cc)
file(GLOB_RECURSE headers ${PROJECT_SOURCE_DIR}/include/*.hh 
                          ${PROJECT_SOURCE_DIR}/include/reco/*.hh
                          ${PROJECT_SOURCE_DIR}/include/generators/*.hh)
file(GLOB_RECURSE macros RELATIVE ${PROJECT_SOURCE_DIR} macros/*.mac)
#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build pinpoint. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
# Enable macros for out-of-source build
foreach(_file ${macros})
  configure_file(
    ${_file}
    ${PROJECT_BINARY_DIR}/${_file}
    COPYONLY
    )
    #message(STATUS "Copying ${_file} in ${PROJECT_BINARY_DIR}/${_file}")
endforeach()

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(pinpoint Pinpoint.cc ${sources})

target_link_libraries(pinpoint 
                      ${Geant4_LIBRARIES}
                      ${HEPMC3_LIBRARIES} 
                      ${HEPMC3_FIO_LIBRARIES} 
                      ${HEPMC3_LIB}
                      ${ROOT_LIBRARIES}
                      Pythia8::Pythia8
                      )

#----------------------------------------------------------------------------
# Install the executable to 'bin' directory under CMAKE_INSTALL_PREFIX
#
install(TARGETS pinpoint DESTINATION bin)

