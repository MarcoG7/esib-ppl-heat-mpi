import sys
import statistics
import matplotlib.pyplot as plt

# 1. Network and computation parameters

TC = 1.5e-9       # Computation time per grid point per step
TS = 0.07         # Network latency in seconds
BW = 110e3        # Network bandwidth in bytes/second

REAL_TS = 1e-6    # Latency of a real cluster
REAL_BW = 10e9    # Bandwidth of a real cluster

# Process grid dimensions (rows, columns)
DIMS = {
    1: (1, 1),
    2: (2, 1),
    3: (3, 1),
    4: (2, 2),
    6: (3, 2)
}


# 2. Communication and performance model

def message_time(bytes, latency, bandwidth):
    return latency + bytes / bandwidth


def model(decomp, N, P, latency=TS, bandwidth=BW):

    n = N - 1  # Number of interior points per direction

    if decomp == "serial" or decomp == "strips":
        rows, cols = P, 1
    else:
        rows, cols = DIMS[P]

    # Time spent computing
    compute = TC * n * n / P

    # Time spent communicating
    comm = 0

    if rows > 1:
        bytes = 8 * n / cols
        comm += 2 * message_time(bytes, latency, bandwidth)

    if cols > 1:
        bytes = 8 * n / rows
        comm += 2 * message_time(bytes, latency, bandwidth)

    return compute + comm


def speedup(decomp, N, P, latency=TS, bandwidth=BW):

    serial_time = model("serial", N, 1)
    parallel_time = model(decomp, N, P, latency, bandwidth)

    return serial_time / parallel_time


def crossover(decomp, P, latency, bandwidth):

    for N in range(8, 1000001, 8):
        if speedup(decomp, N, P, latency, bandwidth) > 1:
            return f"N = {N}"

    return "never (N > 1,000,000)"


# 3. Part 3: Compare strips and blocks

print("Part 3 question: model time per step [s], strips vs blocks (your network)")

for P in (4, 6):
    for N in (512, 2048, 8192):

        strips = model("strips", N, P)
        blocks = model("blocks", N, P)

        faster = "strips" if strips < blocks else "blocks"

        print(f"  P={P} N={N:5d}  strips {strips:.3f}  blocks {blocks:.3f}  -> {faster} faster")


# 4. Find when parallel execution becomes faster

print("\nModel: smallest N where spreading over the PCs pays off (speed-up > 1)")
print(f"  {'case':14s} {'your network':24s} real cluster (ts=1us, 10 GB/s)")

tests = [
    ("strips", 2),
    ("strips", 3),
    ("strips", 4),
    ("strips", 6),
    ("blocks", 4),
    ("blocks", 6)
]

for decomp, P in tests:

    your_network = crossover(decomp, P, TS, BW)
    real_cluster = crossover(decomp, P, REAL_TS, REAL_BW)

    print(f"  {decomp} P={P:<5d} {your_network:24s} {real_cluster}")


# Stop here if no measurement file was provided
if len(sys.argv) < 2:
    sys.exit()


# 5. Read measured results from strong.txt

runs = {}

with open(sys.argv[1]) as file:
    for line in file:

        values = dict(
            item.split("=", 1)
            for item in line.split()
            if "=" in item
        )

        if "t_total" not in values:
            continue

        key = (
            values.get("map", "-"),
            values["decomp"],
            int(values["N"]),
            int(values["P"])
        )

        total = float(values["t_total"])
        comm = float(values["t_comm"])

        if key not in runs:
            runs[key] = []

        runs[key].append((total, comm))


# 6. Calculate the median execution time

def median_total(key):
    times = [total for total, comm in runs[key]]
    return statistics.median(times)


# 7. Print the strong scaling results

print(f"\n{'map':5s} {'decomp':7s} {'N':>5s} {'P':>2s} {'t_total':>8s} {'spread':>7s} "
      f"{'S(P)':>6s} {'E(P)':>6s} {'comm%':>6s} {'model S':>8s}")

grid_sizes = sorted(set(key[2] for key in runs))

for N in grid_sizes:

    # Serial execution time
    serial_key = ("-", "serial", N, 1)

    if serial_key not in runs:
        raise ValueError(f"Missing serial result for N={N}")

    T1 = median_total(serial_key)

    # Results for all parallel configurations
    parallel_keys = sorted(
        key for key in runs
        if key[2] == N and key[1] != "serial"
    )

    for key in parallel_keys:

        mapping, decomp, _, P = key

        times = [total for total, comm in runs[key]]
        comm_times = [comm for total, comm in runs[key]]

        total = statistics.median(times)
        comm = statistics.median(comm_times)

        spread = (max(times) - min(times)) / total * 100
        comm_percent = comm / total * 100

        S = T1 / total
        E = S / P
        predicted = speedup(decomp, N, P)

        print(f"{mapping:5s} {decomp:7s} {N:5d} {P:2d} {total:8.3f} "
              f"{spread:6.0f}% {S:6.3f} {E:6.3f} "
              f"{comm_percent:5.0f}% {predicted:8.3f}")


    # 8. Create speedup and efficiency graphs

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 4.2))

    # Measured values
    for mapping in ("node", "slot"):
        for decomp in ("strips", "blocks"):

            Ps = sorted(
                key[3] for key in runs
                if key[:3] == (mapping, decomp, N)
            )

            if not Ps:
                continue

            Ps = [1] + Ps

            speedups = [1.0]

            for P in Ps[1:]:
                key = (mapping, decomp, N, P)
                speedups.append(T1 / median_total(key))

            efficiencies = [
                S / P for S, P in zip(speedups, Ps)
            ]

            ax1.plot(Ps, speedups, "o-",
                     label=f"{decomp}, map-by {mapping}")

            ax2.plot(Ps, efficiencies, "o-",
                     label=f"{decomp}, map-by {mapping}")

    # Model predictions
    for decomp, Ps in (
        ("strips", [1, 2, 3, 4, 6]),
        ("blocks", [1, 4, 6])
    ):

        predicted = [
            speedup(decomp, N, P) for P in Ps
        ]

        efficiency = [
            S / P for S, P in zip(predicted, Ps)
        ]

        ax1.plot(Ps, predicted, "--",
                 label=f"model {decomp} (map-by node)")

        ax2.plot(Ps, efficiency, "--",
                 label=f"model {decomp}")

    # Graph formatting
    ax1.set_yscale("log")
    ax2.set_yscale("log")

    ax1.set_xlabel("P")
    ax1.set_ylabel("speed-up S(P) = T1 / TP")
    ax1.axhline(1, color="gray", lw=0.6)

    ax2.set_xlabel("P")
    ax2.set_ylabel("efficiency E(P) = S(P) / P")

    ax1.set_xticks([1, 2, 3, 4, 6])
    ax2.set_xticks([1, 2, 3, 4, 6])

    ax1.set_title(f"N = {N}")
    ax1.legend(fontsize=8)

    plt.tight_layout()
    plt.savefig(f"strong_N{N}.png", dpi=120)
    plt.close()
