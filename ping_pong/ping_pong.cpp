#include <mpi.h>
#include <cstdio>
#include <vector>

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (size != 2)
  {
    if (rank == 0)
      std::fprintf(stderr, "Run with exactly 2 MPI processes.\n");
    MPI_Finalize();
    return 1;
  }

  if (rank == 0)
    std::printf("bytes,one_way_seconds\n");

  for (int bytes = 8; bytes <= 16 * 1024 * 1024; bytes *= 2)
  {
    std::vector<char> buffer(bytes, 0);

    // Fewer repetitions for larger messages
    int repetitions = (bytes <= 65536) ? 20 : 10;
    int warmup = 2;

    for (int k = 0; k < warmup; ++k)
    {
      if (rank == 0)
      {
        MPI_Send(buffer.data(), bytes, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        MPI_Recv(buffer.data(), bytes, MPI_CHAR, 1, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      else
      {
        MPI_Recv(buffer.data(), bytes, MPI_CHAR, 0, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(buffer.data(), bytes, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
      }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    for (int k = 0; k < repetitions; ++k)
    {
      if (rank == 0)
      {
        MPI_Send(buffer.data(), bytes, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        MPI_Recv(buffer.data(), bytes, MPI_CHAR, 1, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      else
      {
        MPI_Recv(buffer.data(), bytes, MPI_CHAR, 0, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(buffer.data(), bytes, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
      }
    }

    if (rank == 0)
    {
      double elapsed = MPI_Wtime() - start;
      double one_way = elapsed / (2.0 * repetitions);
      std::printf("%d,%.9f\n", bytes, one_way);
      std::fflush(stdout);
    }
  }

  MPI_Finalize();
  return 0;
}
