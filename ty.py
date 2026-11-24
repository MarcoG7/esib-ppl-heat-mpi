# Part 4, experiment 1: compute speed of each PC.
# usage: python3 tc.py base_heat1.txt base_heat2.txt base_heat3.txt
import sys, statistics

for name in sys.argv[1:]:
    tc = {}                                       # N -> list of T_C values (one per repetition)
    for line in open(name):
        v = dict(x.split("=") for x in line.split() if "=" in x)
        if "t_comp" not in v:
            continue
        N, steps = int(v["N"]), int(v["steps"])
        nodes = (N - 1) ** 2                      # interior nodes updated per step
        tc.setdefault(N, []).append(float(v["t_comp"]) / (steps * nodes))
    print(name)
    for N in sorted(tc):
        print(f"  N={N:5d}  T_C = {statistics.median(tc[N]):.2e} s per node per step")
