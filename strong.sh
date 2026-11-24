#!/bin/bash
# Part 4, experiment 2: strong scaling (same N, more processes).
# Run from heat1:  bash strong.sh | tee strong.txt
MCA="--mca oob_tcp_if_include tailscale0 --mca btl_tcp_if_include tailscale0"
NS="512 2048"                  # N = 8192 would add about 1 hour on our links
STEPS=10                       # few steps: one step over our links costs up to seconds
RUN="./heat --case 1 --r 0.25 --steps $STEPS"

for rep in 1 2 3; do           # repetitions outside: Ctrl+C keeps finished ones
  for N in $NS; do
    mpirun -np 1 $RUN --N $N --decomp serial                    # T1, on heat1
    for MAP in node slot; do
      for P in 2 3 4 6; do
        echo -n "map=$MAP "
        mpirun --hostfile ~/hosts --map-by $MAP -np $P $MCA $RUN --N $N --decomp strips
        if [ $P -ge 4 ]; then  # for P = 2, 3 blocks are the same as strips
          echo -n "map=$MAP "
          mpirun --hostfile ~/hosts --map-by $MAP -np $P $MCA $RUN --N $N --decomp blocks
        fi
      done
    done
  done
done
