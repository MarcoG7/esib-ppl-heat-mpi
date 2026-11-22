#include <mpi.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <vector>

const double PI = 3.141592653589793;

// Convert a 2D position (i, j) into a 1D array index
int index(int i, int j, int width) {
    return j * width + i;
}

// Initial temperature and fixed boundary conditions
double initialTemperature(char testCase, int i, int j, int N) {
    bool edge = (i == 0 || i == N || j == 0 || j == N);

    if (testCase == '1')
        return edge ? 1.0 : 0.0;

    if (testCase == '2')
        return (j == 0 || j == N) ? 1.0 : 0.0;

    // Case V: sin(pi*x)*sin(pi*y)
    if (edge)
        return 0.0;

    double h = 1.0 / N;
    return sin(PI * i * h) * sin(PI * j * h);
}

// Divide n interior points between p processes
void split(int n, int p, int position, int &start, int &count) {
    int base = n / p;
    int extra = n % p;

    count = base + (position < extra ? 1 : 0);
    start = 1 + position * base + std::min(position, extra);
}

// Exchange boundary data with neighboring processes
void exchangeHalos(std::vector<double> &u,
                   int nx, int ny, int width,
                   int down, int up, int left, int right,
                   MPI_Datatype column, MPI_Comm cart) {

    double *data = u.data();

    // Exchange top and bottom rows
    MPI_Sendrecv(
        data + index(1, ny, width), nx, MPI_DOUBLE, up, 0,
        data + index(1, 0, width), nx, MPI_DOUBLE, down, 0,
        cart, MPI_STATUS_IGNORE
    );

    MPI_Sendrecv(
        data + index(1, 1, width), nx, MPI_DOUBLE, down, 1,
        data + index(1, ny + 1, width), nx, MPI_DOUBLE, up, 1,
        cart, MPI_STATUS_IGNORE
    );

    // Exchange left and right columns
    MPI_Sendrecv(
        data + index(nx, 1, width), 1, column, right, 2,
        data + index(0, 1, width), 1, column, left, 2,
        cart, MPI_STATUS_IGNORE
    );

    MPI_Sendrecv(
        data + index(1, 1, width), 1, column, left, 3,
        data + index(nx + 1, 1, width), 1, column, right, 3,
        cart, MPI_STATUS_IGNORE
    );
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, P;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &P);

    // 1. Read program options
    char testCase = '1';
    int N = 128;
    int steps = 100;
    double r = 0.2;
    double T = -1;
    bool unsafe = false;

    std::string decomp = "blocks";
    std::string output = "";

    for (int a = 1; a < argc; a++) {
        std::string option = argv[a];

        if (option == "--unsafe") {
            unsafe = true;
            continue;
        }

        if (a + 1 >= argc)
            break;

        std::string value = argv[++a];

        if (option == "--case") testCase = value[0];
        else if (option == "--N") N = std::stoi(value);
        else if (option == "--steps") steps = std::stoi(value);
        else if (option == "--r") r = std::stod(value);
        else if (option == "--T") T = std::stod(value);
        else if (option == "--decomp") decomp = value;
        else if (option == "--out") output = value;
    }

    // 2. Check parameters
    if (N < 2 || r <= 0 || (decomp == "serial" && P != 1)) {
        if (rank == 0)
            printf("Invalid parameters (need N >= 2, r > 0, and 1 process for serial).\n");

        MPI_Finalize();
        return 1;
    }

    if (r > 0.25 && !unsafe) {
        if (rank == 0)
            printf("r = %g > 1/4 is unstable. Use --unsafe to run anyway.\n", r);
        MPI_Finalize();
        return 1;
    }

    if (decomp != "serial" && decomp != "strips" && decomp != "blocks") {
        if (rank == 0)
            printf("Unknown --decomp %s (use serial, strips or blocks).\n", decomp.c_str());
        MPI_Finalize();
        return 1;
    }

    double h = 1.0 / N;
    double dt = r * h * h;

    if (T > 0) {
        steps = (int)ceil(T / dt);
        dt = T / steps;
        r = dt / (h * h);
    }

    // 3. Create the grid of MPI processes
    int dims[2] = {0, 0};

    if (decomp == "strips" || decomp == "serial") {
        dims[0] = P;
        dims[1] = 1;
    } else {
        MPI_Dims_create(P, 2, dims);
    }

    // Each process must own at least one interior point
    if (dims[0] > N - 1 || dims[1] > N - 1) {
        if (rank == 0)
            printf("Too many processes for this grid.\n");

        MPI_Finalize();
        return 1;
    }

    int periods[2] = {0, 0};
    MPI_Comm cart;

    MPI_Cart_create(MPI_COMM_WORLD, 2, dims,
                    periods, 0, &cart);

    int coords[2];
    MPI_Cart_coords(cart, rank, 2, coords);

    // Find my four neighbors
    int down, up, left, right;
    MPI_Cart_shift(cart, 0, 1, &down, &up);
    MPI_Cart_shift(cart, 1, 1, &left, &right);

    // 4. Find my part of the heat plate
    int i0, j0, nx, ny;

    split(N - 1, dims[1], coords[1], i0, nx);
    split(N - 1, dims[0], coords[0], j0, ny);

    int width = nx + 2;

    // +2 for the ghost cells on each dimension
    std::vector<double> u((nx + 2) * (ny + 2));
    std::vector<double> next = u;

    // Initialize local temperatures and ghost cells
    for (int j = 0; j <= ny + 1; j++) {
        for (int i = 0; i <= nx + 1; i++) {
            int globalI = i0 + i - 1;
            int globalJ = j0 + j - 1;

            u[index(i, j, width)] =
                initialTemperature(testCase, globalI, globalJ, N);
        }
    }

    next = u;

    // Create MPI datatype for one column
    MPI_Datatype column;

    MPI_Type_vector(ny, 1, width, MPI_DOUBLE, &column);
    MPI_Type_commit(&column);

    // 5. Solve the heat equation
    double computeTime = 0;
    double communicationTime = 0;

    exchangeHalos(u, nx, ny, width, down, up, left, right, column, cart);   // warm-up, not timed

    MPI_Barrier(cart);
    double totalStart = MPI_Wtime();

    for (int step = 0; step < steps; step++) {

        // A. Exchange ghost cells
        double start = MPI_Wtime();

        exchangeHalos(u, nx, ny, width,
                      down, up, left, right, column, cart);

        communicationTime += MPI_Wtime() - start;

        // B. Compute the new temperatures
        start = MPI_Wtime();

        for (int j = 1; j <= ny; j++) {
            for (int i = 1; i <= nx; i++) {

                int c = index(i, j, width);

                double neighbors =
                    u[c - 1] + u[c + 1] +
                    u[c - width] + u[c + width];

                next[c] = u[c] + r * (neighbors - 4.0 * u[c]);
            }
        }

        // C. Use the new temperatures in the next step
        std::swap(u, next);

        computeTime += MPI_Wtime() - start;
    }

    double totalTime = MPI_Wtime() - totalStart;

    // Get the slowest process's time
    double myTimes[3] = {
        totalTime, computeTime, communicationTime
    };
    double maxTimes[3];

    MPI_Reduce(myTimes, maxTimes, 3, MPI_DOUBLE,
               MPI_MAX, 0, cart);

    // 6. Calculate errors for case V
    double errorInf = 0;
    double error2 = 0;

    if (testCase == 'V') {
        double decay = exp(-2.0 * PI * PI * steps * dt);

        for (int j = 1; j <= ny; j++) {
            for (int i = 1; i <= nx; i++) {

                int globalI = i0 + i - 1;
                int globalJ = j0 + j - 1;

                double exact =
                    sin(PI * globalI * h) *
                    sin(PI * globalJ * h) * decay;

                double difference =
                    fabs(u[index(i, j, width)] - exact);

                errorInf = std::max(errorInf, difference);
                error2 += difference * difference;
            }
        }

        MPI_Allreduce(MPI_IN_PLACE, &errorInf, 1,
                      MPI_DOUBLE, MPI_MAX, cart);

        MPI_Allreduce(MPI_IN_PLACE, &error2, 1,
                      MPI_DOUBLE, MPI_SUM, cart);

        error2 = sqrt(h * h * error2);
    }

    // 7. Collect the complete heat plate on rank 0
    if (!output.empty()) {

        // Pack my interior temperatures (without ghosts)
        std::vector<double> mine(nx * ny);

        for (int j = 0; j < ny; j++) {
            for (int i = 0; i < nx; i++) {
                mine[j * nx + i] =
                    u[index(i + 1, j + 1, width)];
            }
        }

        if (rank != 0) {
            MPI_Send(mine.data(), nx * ny, MPI_DOUBLE,
                     0, 9, cart);
        }
        else {
            // Full plate, including fixed boundary nodes
            int fullWidth = N + 1;
            std::vector<double> full(fullWidth * fullWidth);

            for (int j = 0; j <= N; j++) {
                for (int i = 0; i <= N; i++) {
                    full[index(i, j, fullWidth)] =
                        initialTemperature(testCase, i, j, N);
                }
            }

            // Receive each process's block
            for (int q = 0; q < P; q++) {
                int position[2];
                MPI_Cart_coords(cart, q, 2, position);

                int startI, startJ, countI, countJ;

                split(N - 1, dims[1], position[1],
                      startI, countI);

                split(N - 1, dims[0], position[0],
                      startJ, countJ);

                std::vector<double> block(countI * countJ);

                if (q == 0) {
                    block = mine;
                }
                else {
                    MPI_Recv(block.data(), countI * countJ,
                             MPI_DOUBLE, q, 9, cart,
                             MPI_STATUS_IGNORE);
                }

                // Place this block into the full plate
                for (int j = 0; j < countJ; j++) {
                    for (int i = 0; i < countI; i++) {
                        full[index(startI + i, startJ + j,
                                   fullWidth)] =
                            block[j * countI + i];
                    }
                }
            }

            // Write raw double values to a binary file
            FILE *file = fopen(output.c_str(), "wb");

            if (file != nullptr) {
                fwrite(full.data(), sizeof(double),
                       full.size(), file);
                fclose(file);
            }
            else {
                printf("Could not open output file.\n");
            }
        }
    }

    // 8. Print results
    if (rank == 0) {
        printf("decomp=%s P=%d grid=%dx%d N=%d r=%g steps=%d",
               decomp.c_str(), P, dims[1], dims[0],
               N, r, steps);

        printf(" t_total=%.4f t_comp=%.4f t_comm=%.4f",
               maxTimes[0], maxTimes[1], maxTimes[2]);

        if (testCase == 'V') {
            printf(" err_inf=%.3e err_2h=%.3e",
                   errorInf, error2);
        }

        printf("\n");
    }

    MPI_Type_free(&column);
    MPI_Comm_free(&cart);
    MPI_Finalize();

    return 0;
}
