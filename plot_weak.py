# Part 4, experiment 3: weak scaling plot.   usage: python3 plot_weak.py weak.txt
import sys, statistics
import matplotlib.pyplot as plt

times = {}                                        # P -> list of t_total
for line in open(sys.argv[1]):
    v = dict(x.split("=") for x in line.split() if "=" in x)
    if "t_total" in v:
        times.setdefault(int(v["P"]), []).append(float(v["t_total"]))

Ps = sorted(times)
T = [statistics.median(times[P]) for P in Ps]
for P, t in zip(Ps, T):
    print(f"P={P}  t_total (median) = {t:.3f} s")
plt.plot(Ps, T, "o-", label="measured (median of 3)")
plt.plot(Ps, [T[0]] * len(Ps), "k--", label="ideal: constant")
plt.xlabel("P (N grows so each process keeps ~1024 x 1024 nodes)"); plt.ylabel("run time [s]")
plt.yscale("log"); plt.legend(); plt.savefig("weak.png", dpi=120)
