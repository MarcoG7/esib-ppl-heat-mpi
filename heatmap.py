# usage: python3 heatmap.py u.bin   -> writes u.bin.png
import sys, numpy as np, matplotlib.pyplot as plt

u = np.fromfile(sys.argv[1])                # raw little-endian doubles
n = int(round(u.size ** 0.5))               # (N+1) x (N+1)

plt.imshow(u.reshape(n, n), origin="lower", extent=[0, 1, 0, 1], cmap="inferno")
plt.colorbar(label="u"); plt.xlabel("x"); plt.ylabel("y")
plt.savefig(sys.argv[1] + ".png", dpi=120)
