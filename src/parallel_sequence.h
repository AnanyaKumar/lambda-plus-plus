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
  MPI_Win data_window; // gives nodes access to each others' data

  // Common information about the Sequence
  int size;

  // Private functions

  void initialize (int n) {
    int size = n;
    int equalSplit = size / Cluster::procs;
    int numLeftOverElements = size % Cluster::procs;
    int myLeftOver = Cluster::procId < numLeftOverElements;
    numElements = equalSplit + myLeftOver;
    if (myLeftOver) {
      startIndex = Cluster::procId * (equalSplit + 1);
    } else {
      startIndex = Cluster::procId * equalSplit + numLeftOverElements;
    }
    
    data = new T[numElements];
    MPI_Win_create(data, numElements * sizeof(T), sizeof(T), 
      MPI_INFO_NULL, MPI_COMM_WORLD, &data_window);
    MPI_Win_fence(0, data_window); 
  }

  void destroy () {
    // Destroy stuff
    // Probably need to case on whether we've created stuff
  }

  bool isMine (int index) {
    return startIndex <= index && index < startIndex + numElements;
  }

  int getNodeWithData (int index) {
    int equalSplit = size / Cluster::procs;
    int numLeftOverElements = size % Cluster::procs;
    int block = (equalSplit + 1) * numLeftOverElements;
    if (index < block) {
      return index / (equalSplit + 1);
    } else {
      return (index - block) / equalSplit + numLeftOverElements;
    }
  }

  int getDataDisp (int index) {
    int equalSplit = size / Cluster::procs;
    int numLeftOverElements = size % Cluster::procs;
    int block = (equalSplit + 1) * numLeftOverElements;
    if (index < block) {
      return index % (equalSplit + 1);
    } else {
      return (index - block) % equalSplit;
    }
  }

  void endMethod () {
    MPI_Barrier(MPI_COMM_WORLD);
  }

public:
  Sequence () {
    size = 0;
  }

  Sequence (T *array, int n) {
    initialize(n);
    for (int i = 0; i < numElements; i++) {
      data[i] = array[startIndex + i];
    }
    endMethod();
  }

  Sequence (function<T(int)> generator, int n) {
    initialize(n);
    for (int i = 0; i < numElements; i++) {
      data[i] = generator(startIndex + i);
    }
    endMethod();
  }

  void transform (function<T(T)> mapper) {
    for (int i = 0; i < numElements; i++) {
      data[i] = mapper(data[i]);
    }
    endMethod();
  }

  // template<typename S> 
  // Sequence<S> map (function<S(T)> mapper) {
  //   Sequence S = new Sequence;
  //   auto tabulateFunction = [&](int index) {
  //     return mapper(this.get(index));
  //   };
  //   S.tabulate(tabulateFunction, size);
  //   return S;
  // }

  // T reduce (function<T(T,T)> combiner, T init) {
  //   T value = init;
  //   for (int i = 0; i < size; i++) {
  //     value = combiner(value, data[i]);
  //   }
  //   return value;
  // }

  void scan (function<T(T,T)> combiner, T init) {
    if (size > 0) {
      data[0] = combiner(init, data[0]);
    }
    for (int i = 1; i < size; i++) {
      data[i] = combiner(data[i-1], data[i]);
    }
  }

  T get (int index) {
    T value;
    if (Cluster::procId == 0) {
      value = data[index - startIndex];
    } else {
      MPI_Get(&value, sizeof(T), MPI_BYTE, getNodeWithData(index), getDataDisp(index), 
        sizeof(T), MPI_BYTE, data_window);
    }
    MPI_Win_fence(0, data_window); 
    return value;
  }

  // void set (int index, T value) {
  //   if (isMine(index)) {
  //     data[index - startIndex] = value;
  //   }
  // }

  void print () {
    cout << "Node " << (Cluster::procId+1) << "/" << Cluster::procs << ":" << endl;
    int i;
    for (i = 0; i < numElements; i++) {
      cout << data[i] <<  " ";
      if (i % 10 == 9) cout << endl;
    }
    if (i % 10 != 9) cout << endl;
    cout << endl;
  }
};

#endif
