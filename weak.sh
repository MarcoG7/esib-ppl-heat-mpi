#!/bin/bash
# Part 4, experiment 3: weak scaling (about 1024 x 1024 points per process).
# Run from heat1:  bash weak.sh | tee weak.txt
MCA="--mca oob_tcp_if_include tailscale0 --mca btl_tcp_if_include tailscale0"
PS=(1 2 3 4 6)
NS=(1025 1449 1775 2049 2509)  # N - 1 = 1024 * sqrt(P)
RUN="./heat --case 1 --r 0.25 --steps 10"

for rep in 1 2 3; do
  for i in 0 1 2 3 4; do
    mpirun --hostfile ~/hosts --map-by node -np ${PS[$i]} $MCA $RUN --N ${NS[$i]} --decomp strips
  done
done
