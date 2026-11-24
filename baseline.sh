#!/bin/bash
# Part 4, experiment 1: compute speed of ONE PC (serial runs, 3 repetitions).
# Serial runs use no network, so no --mca flags are needed.
#   on heat1:              bash baseline.sh | tee base_heat1.txt
#   heat2/heat3 from heat1: ssh heat2 "cd $PWD && bash baseline.sh" | tee base_heat2.txt
for rep in 1 2 3; do
  for N in 512 1024 2048 4096 8192; do
    mpirun -np 1 ./heat --case 1 --r 0.25 --steps 20 --N $N --decomp serial
  done
done
