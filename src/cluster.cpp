#include <mpi.h>

#include "cluster.h"

namespace Cluster {
  // Information about this node
  int procs;
  int procId;
  int blocksPerProc;

  // TODO: Add static information about the cluster

  void init (int *argc, char ***argv) {
    MPI_Init(argc, argv);
    MPI_Comm_size(MPI_COMM_WORLD, &procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &procId);
    blocksPerProc = 5;
  }

  void close () {
    MPI_Finalize();
  }
};
