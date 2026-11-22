#!/bin/bash
# Part 3 correctness: blocks must give the same file as serial, byte for byte.
# Run from heat1:  bash check_blocks.sh
MCA="--mca oob_tcp_if_include tailscale0 --mca btl_tcp_if_include tailscale0"

for N in 8 50 101; do
  mpirun -np 1 ./heat --case 2 --N $N --steps 20 --decomp serial \
         --out serial_$N.bin > /dev/null
  for P in 4 6; do
    mpirun --hostfile ~/hosts --map-by node -np $P $MCA \
           ./heat --case 2 --N $N --steps 20 --decomp blocks \
           --out blocks_${N}_$P.bin > /dev/null
    if cmp -s serial_$N.bin blocks_${N}_$P.bin; then
      echo "N=$N P=$P identical"
    else
      echo "N=$N P=$P DIFFERENT"
    fi
  done
done
