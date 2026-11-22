# usage: python3 convergence.py convergence.txt
import sys, math
import matplotlib.pyplot as plt

rows = []
for line in open(sys.argv[1]):
    values = dict(item.split("=") for item in line.split() if "=" in item)
    rows.append((int(values["N"]), int(values["steps"]),
                 float(values["err_inf"]), float(values["err_2h"])))

print("N     h          steps   err_inf     err_2h      p_obs")
for k, (N, steps, einf, e2) in enumerate(rows):
    p = "-" if k == 0 else f"{math.log2(rows[k-1][3] / e2):.2f}"   # p_obs = log2(E(h)/E(h/2))
    print(f"{N:<5} {1/N:<10.5f} {steps:<7} {einf:<11.3e} {e2:<11.3e} {p}")

h  = [1 / N for N, _, _, _ in rows]
e2 = [row[3] for row in rows]

plt.loglog(h, e2, "o-", label="error (2,h norm)")
plt.loglog(h, [e2[0] * (x / h[0]) ** 2 for x in h], "k--", label="slope 2 (reference)")
plt.xlabel("h = 1/N"); plt.ylabel("error at T = 0.05"); plt.legend(); plt.grid(True, which="both", alpha=0.3)
plt.savefig("convergence.png", dpi=120)
