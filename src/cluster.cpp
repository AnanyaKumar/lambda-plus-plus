#include <mpi.h>
#include <omp.h>
#include <cstdio>

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
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);
    printf("%s\n", processor_name);
    omp_set_num_threads(6);
  }

  void close () {
    MPI_Finalize();
  }
};
