# usage: python3 plot_convergence.py convergence.txt   -> convergence.png (log-log error vs h, slope 2)
import sys, matplotlib.pyplot as plt

N, einf, e2 = [], [], []

for line in open(sys.argv[1]):
    kv = dict(x.split("=", 1) for x in line.split() if "=" in x)
    N.append(int(kv["N"])); einf.append(float(kv["err_inf"])); e2.append(float(kv["err_2h"]))

h = [1 / n for n in N]

for i in range(1, len(N)):
    print(f"N={N[i]:4d}  err_inf={einf[i]:.3e}  ratio={einf[i-1] / einf[i]:.2f}")   # ~4 means 2nd order

plt.loglog(h, einf, "o-", label="max error"); plt.loglog(h, e2, "s-", label="2-norm error")
plt.loglog(h, [einf[0] * (x / h[0]) ** 2 for x in h], "k--", label="slope 2")
plt.xlabel("h"); plt.ylabel("error at T = 0.05"); plt.legend(); plt.savefig("convergence.png", dpi=120)
