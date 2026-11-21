#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

// One round trip: rank 0 -> rank 1 -> rank 0
void pingpong(int rank, char* buffer, int bytes) {
    if (rank == 0) {
        MPI_Send(buffer, bytes, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        MPI_Recv(buffer, bytes, MPI_CHAR, 1, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    else {
        MPI_Recv(buffer, bytes, MPI_CHAR, 0, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(buffer, bytes, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // The experiment needs exactly two processes
    if (size != 2) {
        if (rank == 0)
            printf("Please run with exactly 2 processes.\n");
        MPI_Finalize();
        return 1;
    }

    // Maximum message size: 1 MiB by default
    int maxBytes = (argc > 1) ? atoi(argv[1]) : 1024 * 1024;
    if (maxBytes < 8) {
        MPI_Finalize();
        return 1;
    }

    std::vector<char> buffer(maxBytes);

    // Warm-up before measuring
    pingpong(rank, buffer.data(), 8);

    if (rank == 0)
        printf("# bytes  one_way_s  MB_per_s\n");

    // Test message sizes: 8, 16, 32, ... up to maxBytes
    for (int bytes = 8; bytes <= maxBytes; bytes *= 2) {

        int repetitions = (bytes <= 8192) ? 20 : 5;

        MPI_Barrier(MPI_COMM_WORLD);

        double start = MPI_Wtime();

        for (int i = 0; i < repetitions; i++) {
            pingpong(rank, buffer.data(), bytes);
        }

        double elapsed = MPI_Wtime() - start;
        double oneWay = elapsed / (2 * repetitions);

        if (rank == 0) {
            double bandwidth = bytes / oneWay / 1e6;
            printf("%8d  %.3e  %8.2f\n", bytes, oneWay, bandwidth);
        }
    }

    MPI_Finalize();
    return 0;
}
