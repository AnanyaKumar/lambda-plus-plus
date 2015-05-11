#include <mpi.h>
#include <omp.h>
#include <cstdio>
#include <iostream>

#include "cluster.h"
#include "CycleTimer.h"

namespace Cluster {
  // Information about the cluster
  int procs;
  int blocksPerProc;
  int threadsPerProc;
  int systemTime;
  int *procTimes;

  // Information about this node
  int procId;

  void init (int *argc, char ***argv) {
    MPI_Init(argc, argv);
    MPI_Comm_size(MPI_COMM_WORLD, &procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &procId);
    blocksPerProc = 5;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);
    printf("%s\n", processor_name);
    threadsPerProc = 2;
    omp_set_num_threads(threadsPerProc);

    // Get the time for a simple loop
    double start_time = CycleTimer::currentSeconds();
    int compSize = 10000;
    int arraySize = 10000;
    for (volatile int i = 0; i < compSize; i++) {
      int *A = new int[arraySize];
      for (volatile int j = 0; j < arraySize; j++) {
        A[j] = 0;
      }
      delete[] A;
    }
    double total_time_parallel = CycleTimer::currentSeconds() - start_time;
    int procTime = int(total_time_parallel * 1000);
    if (procTime < 1) {
      procTime = 1;
    }

    // Collect results from accross the cluster
    procTimes = new int[procs];
    MPI_Allgather(&procTime, sizeof(int), MPI_BYTE, procTimes, sizeof(int), MPI_BYTE, 
      MPI_COMM_WORLD);
    systemTime = 0;
    for (int i = 0; i < procs; i++) {
      systemTime += procTimes[i];
    }
  }

  void close () {
    delete[] procTimes;
    MPI_Finalize();
  }
};
