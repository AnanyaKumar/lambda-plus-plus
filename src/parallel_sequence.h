#ifndef _PARALLEL_SEQUENCE_H_
#define _PARALLEL_SEQUENCE_H_

#include <iostream>
#include <mpi.h>

#include "sequence.h"

using namespace std;

template<typename T>
class ParallelSequence: public Sequence<T>
{
private:
  int procs;
  int procId;

  int startIndex; // data is inclusive of the element at startIndex
  int numElements;
  MPI_Win data_window; // gives nodes access to each others' data

private:
  void initialize (int n) {
    this->size = n;
    int equalSplit = this->size / procs;
    int numLeftOverElements = this->size % procs;
    int myLeftOver = procId < numLeftOverElements;
    numElements = equalSplit + myLeftOver;
    if (myLeftOver) {
      startIndex = procId * (equalSplit + 1);
    } else {
      startIndex = procId * equalSplit + numLeftOverElements;
    }

    this->data = new T[numElements];
    MPI_Win_create(this->data, numElements * sizeof(T), sizeof(T),
      MPI_INFO_NULL, MPI_COMM_WORLD, &data_window);
    MPI_Win_fence(0, data_window);
  }

  void destroy () {
    if (this->size != 0) {
      delete[] this->data;
    }
  }

  bool isMine (int index) {
    return startIndex <= index && index < startIndex + numElements;
  }

  int getNodeWithData (int index) {
    int equalSplit = this->size / procs;
    int numLeftOverElements = this->size % procs;
    int block = (equalSplit + 1) * numLeftOverElements;
    if (index < block) {
      return index / (equalSplit + 1);
    } else {
      return (index - block) / equalSplit + numLeftOverElements;
    }
  }

  int getDataDisp (int index) {
    int equalSplit = this->size / procs;
    int numLeftOverElements = this->size % procs;
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
    T myValue = this->data[0]; // TODO: what if there are 0 elements?
    for (int i = 1; i < numElements; i++) {
      myValue = combiner(myValue, this->data[i]);
    }
    MPI_Barrier(MPI_COMM_WORLD); // Is the barrier necessary?

    T *recvbuf;
    int *recvcounts;
    int *displs;
    recvbuf = new T[procs];
    recvcounts = new T[procs];
    displs = new T[procs];

    recvcounts[0] = sizeof(T);
    displs[0] = 0;
    for (int i = 1; i < procs; i++) {
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
    MPI_Comm_size(MPI_COMM_WORLD, procs);
    MPI_Comm_rank(MPI_COMM_WORLD, procId);

    initialize(n);
    for (int i = 0; i < numElements; i++) {
      this->data[i] = array[startIndex + i];
    }
    endMethod();
  }

  ParallelSequence (function<T(int)> generator, int n) {
    initialize(n);
    for (int i = 0; i < numElements; i++) {
      this->data[i] = generator(startIndex + i);
    }
    endMethod();
  }

  ~ParallelSequence() {
    destroy();
  }

  void transform (function<T(T)> mapper) {
    for (int i = 0; i < numElements; i++) {
      this->data[i] = mapper(this->data[i]);
    }
    endMethod();
  }

  template<typename S>
  ParallelSequence<S> map(function<S(T)> mapper) {
    auto nop = [](int _) {
      return 42;
    };
    return ParallelSequence<S>(nop, 0);
  }

  T reduce (function<T(T,T)> combiner, T init) {
    T *recvbuf = getPartialReduces(combiner);

    // Compute the final answer
    T value = init;
    for (int i = 0; i < procs; i++) {
      value = combiner(value, recvbuf[i]);
    }

    free(recvbuf);
    return value;
  }

  void scan (function<T(T,T)> combiner, T init) {
    T *recvbuf = getPartialReduces(combiner);

    // Get the combination of all values before values in current node
    T scan = init;
    for (int i = 0; i < procId; i++) {
      scan = combiner(scan, recvbuf[i]);
    }

    // Apply the scan to elements in the current node
    this->data[0] = combiner(scan, this->data[0]);
    for (int i = 1; i < numElements; i++) {
      this->data[i] = combiner(this->data[i-1], this->data[i]);
    }

    free(recvbuf);
  }

  T get (int index) {
    T value;
    if (procId == 0) {
      value = this->data[index - startIndex];
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
    cout << "Node " << (procId+1) << "/" << procs << ":" << endl;
    int i;
    for (i = 0; i < numElements; i++) {
      cout << this->data[i] <<  " ";
      if (i % 10 == 9) cout << endl;
    }
    if (i % 10 != 9) cout << endl;
    cout << endl;
  }
};

#endif
