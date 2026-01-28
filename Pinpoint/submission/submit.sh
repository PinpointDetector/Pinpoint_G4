#!/bin/bash
# arguments: run: int, job: int, first_event: int 

date

# Check if correct number of commandline arguments is given
if [ "$#" -ne 3 ]; then
  echo "Usage: $0 <run> <job> <first_event>"
  exit 1
fi

RUN=$1
JOB=$2
FIRST_EVENT=$3
PINPOINT_DIR=/eos/project/f/fasersim-bonn/public/pinpoint

source ${PINPOINT_DIR}/Pinpoint_G4/Pinpoint/setup.sh
mkdir ${PINPOINT_DIR}/data/${RUN}/${JOB}
cd ${PINPOINT_DIR}/data/${RUN}/${JOB}
${PINPOINT_DIR}/Pinpoint_G4/Pinpoint/build/pinpoint ${PINPOINT_DIR}/data/${RUN}/run.mac -f ${FIRST_EVENT}
