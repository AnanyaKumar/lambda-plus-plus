#ifndef SEQUENCE_H
#define SEQUENCE_H

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

template<typename T>
class Sequence
{
  // Data stored by the current node
  T* data;
  int startIndex; // data is inclusive of the element at startIndex
  int numElements;

  // Common information about the Sequence
  int size;

public:
  Sequence () {
    size = 0;
  }

  Sequence (T *array, int n) {
    size = n;
    data = new T[size];
    for (int i = 0; i < size; i++) {
      data[i] = array[i];
    }
  }

  void initialize(int n) {
    int numLeftOverElements = n % Cluster.procs;
    int myLeftOver = Cluster.procId < numLeftOverElements;
    int myNormalElements = n / Cluster.procs;
    numElements = myNormalElements + myLeftOver;
    if (myLeftOver) {
      startIndex = Cluster.procId * (myNormalElements + 1);
    } else {
      startIndex = Cluster.procId * myNormalElements + numLeftOverElements;
    }
  }

  void tabulate (function<T(int)> generator, int n) {
    initialize(n);
    for (int i = 0; i < numElements; i++) {
      data[i] = generator(startIndex + i);
    }
  }

  void transform (function<T(T)> mapper) {
    for (int i = 0; i < size; i++) {
      data[i] = mapper(data[i]);
    }
  }

  template<typename S> 
  Sequence<S> map(function<S(T)> mapper) {
    Sequence S = new Sequence;
    auto tabulateFunction = [&](int index) {
      return mapper(this.get(index));
    };
    S.tabulate(tabulateFunction, size);
    return S;
  }

  T reduce (function<T(T,T)> combiner, T init) {
    T value = init;
    for (int i = 0; i < size; i++) {
      value = combiner(value, data[i]);
    }
    return value;
  }

  void scan (function<T(T,T)> combiner, T init) {
    if (size > 0) {
      data[0] = combiner(init, data[0]);
    }
    for (int i = 1; i < size; i++) {
      data[i] = combiner(data[i-1], data[i]);
    }
  }

  T get (int index) {
    return data[index];
  }

  void set (int index, T value) {
    data[index] = value;
  }

  void print () {
    cout << "Node " << (Cluster::procId+1) << "/" << Cluster::procs << ":" << endl;
    int i;
    for (i = 0; i < numElements; i++) {
      cout << data[i] <<  " ";
      if (i % 10 == 9) cout << endl;
    }
    if (i % 10 != 9) cout << endl;
  }
};

#endif
