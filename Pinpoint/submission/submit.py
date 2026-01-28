import argparse
import os
import subprocess

import numpy as np


def submit(run: int, num_events: int, num_events_per_job: int) -> None:
    njobs = (num_events + num_events_per_job - 1) // num_events_per_job

    f = open(f"submit_pinpoint_{run:05d}.condor", "w")
    print("Executable = submit.sh", file=f)
    print(f"Log = submit_{run:05d}.log", file=f)
    print("Input = /dev/null", file=f)
    print(f"Output = submit_{run:05d}.out", file=f)
    print(f"Error = submit_{run:05d}.err", file=f)
    print(f"initialdir = {os.getcwd()}", file=f)
    print('+JobFlavour = "workday"', file=f)
    print('+AccountingGroup = "group_u_FASER.users"', file=f)
    print("Rank = -SlotID", file=f)
    for job in range(njobs):
        print(f"arguments = {run:05d} {job:03d} {num_events_per_job * job}", file=f)
        print("queue", file=f)
    f.close()

    print(f"Submitting condor job: submit_pinpoint_{run:05d}.condor")
    proc = subprocess.run(
        ["/usr/bin/condor_submit", f"submit_pinpoint_{run:05d}.condor"],
        capture_output=True,
        text=True,
    )
    for line in proc.stdout.split("\n"):
        print(line)


if __name__ == "__main__":
    np.random.seed(0)

    parser = argparse.ArgumentParser()
    parser.add_argument("--run", "-r", default=0, type=int)
    parser.add_argument("--num-events", "-n", default=10000, type=int)
    parser.add_argument("--num-events-per-job", default=1000, type=int)
    args = parser.parse_args()

    submit(
        run=args.run,
        num_events=args.num_events,
        num_events_per_job=args.num_events_per_job,
    )
