#ifndef _SEQUENCE_H_
#define _SEQUENCE_H_

#include <functional>
#include <mpi.h>

using namespace std;

namespace Cluster {
  // Information about this node
  int procs;
  int procId;

  // TODO: Add static information about the cluster

  void init (int *argc, char ***argv) {
    MPI_Init(argc, argv);
    MPI_Comm_size(MPI_COMM_WORLD, &procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &procId);
  }

  void close () {
    MPI_Finalize();
  }
};

/*
 * Abstract Sequence class
 *
 * Implemented by SerialSequence and ParallelSequence
 */
template<typename T>
class Sequence
{
protected:
  // Data stored by the current node
  T* data;

  // Common information about the Sequence
  int size;

public:
  Sequence<T> (T *array, int n) = 0;
  Sequence<T> (function<T(int)> generator, int n) = 0;

  void transform(function<T(T)> mapper);

  template<typename S>
  Sequence<S> map(function<S(T)> mapper);

  T reduce (function<T(T,T)> combiner, T init);
  void scan (function<T(T,T)> combiner, T init);

  T get (int index);
  void set (int index, T value);

  void print ();
};

#endif
