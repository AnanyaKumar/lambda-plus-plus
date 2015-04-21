#ifndef _PARALLEL_SEQUENCE_H_
#define _PARALLEL_SEQUENCE_H_

#include <functional>
#include <mpi.h>

#include "sequence.h"

using namespace std;

template<typename T>
class ParallelSequence : public Sequence
{
private:
  int startIndex; // data is inclusive of the element at startIndex
  int numElements;
  MPI_Win data_window; // gives nodes access to each others' data

private:
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
    if (size != 0) {
      free(data);
    }
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

  T *getPartialReduces (function<T(T,T)> combiner) {
    // TODO: Consider possibly faster ways of transfering data
    T myValue = data[0]; // TODO: what if there are 0 elements?
    for (int i = 1; i < numElements; i++) {
      myValue = combiner(myValue, data[i]);
    }
    MPI_Barrier(MPI_COMM_WORLD); // Is the barrier necessary?

    T *recvbuf;
    int *recvcounts;
    int *displs;
    recvbuf = new T[Cluster::procs];
    recvcounts = new T[Cluster::procs];
    displs = new T[Cluster::procs];

    recvcounts[0] = sizeof(T);
    displs[0] = 0;
    for (int i = 1; i < Cluster::procs; i++) {
      recvcounts[i] = sizeof(T);
      displs[i] = displs[i-1] + sizeof(T);
    }

    MPI_Allgatherv(&myValue, sizeof(T), MPI_BYTE, recvbuf, recvcounts,
      displs, MPI_BYTE, MPI_COMM_WORLD);

    free(recvcounts);
    free(displs);

    return recvbuf;
  }

public:
  ParallelSequence (T *array, int n) {
    initialize(n);
    for (int i = 0; i < numElements; i++) {
      data[i] = array[startIndex + i];
    }
    endMethod();
  }

  ParallelSequence (function<T(int)> generator, int n) {
    initialize(n);
    for (int i = 0; i < numElements; i++) {
      data[i] = generator(startIndex + i);
    }
    endMethod();
  }

  ~ParallelSequence() {
    destroy();
  }

  void transform (function<T(T)> mapper) {
    for (int i = 0; i < numElements; i++) {
      data[i] = mapper(data[i]);
    }
    endMethod();
  }

  template<typename S>
  ParallelSequence<S> map(function<S(T)> mapper) {
    auto nop [](int _) {
      return 42;
    };
    return ParallelSequence<S>(nop, 0);
  }

  T reduce (function<T(T,T)> combiner, T init) {
    T *recvbuf = getPartialReduces(combiner);

    // Compute the final answer
    T value = init;
    for (int i = 0; i < Cluster::procs; i++) {
      value = combiner(value, recvbuf[i]);
    }

    free(recvbuf);
    return value;
  }

  void scan (function<T(T,T)> combiner, T init) {
    T *recvbuf = getPartialReduces(combiner);

    // Get the combination of all values before values in current node
    T scan = init;
    for (int i = 0; i < Cluster::procId; i++) {
      scan = combiner(scan, recvbuf[i]);
    }

    // Apply the scan to elements in the current node
    data[0] = combiner(scan, data[0]);
    for (int i = 1; i < numElements; i++) {
      data[i] = combiner(data[i-1], data[i]);
    }

    free(recvbuf);
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

  void set (int index, T value) {
    return;
  }

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
