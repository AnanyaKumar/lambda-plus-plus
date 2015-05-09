#ifndef _UBER_SEQUENCE_H_
#define _UBER_SEQUENCE_H_

#include <iostream>
#include <mpi.h>

#include "sequence.h"
#include "cluster.h"

using namespace std;

struct Responsibility
{
  int procId;
  int startIndex;
  int numElements;
};

template<typename T>
struct SeqPart
{
  int startIndex;
  int numElements;
  T* data;
};

template<typename T>
class UberSequence
{
  // Get rid of these
  T *data;
  int startIndex; // data is inclusive of the element at startIndex
  int numElements;

  // The new stuff
  int size;
  int numResponsibilities;
  Responsibility *responsibilities;
  int numParts;
  SeqPart<T> *mySeqParts;

  MPI_Win data_window; // gives nodes access to each others' data

  void computeResponsibilities () {
    int totalBlocks = Cluster::blocksPerProc * Cluster::procs;
    this->numResponsibilities = totalBlocks;
    this->numParts = Cluster::blocksPerProc;
    this->responsibilities = new Responsibility[totalBlocks];

    int blockSize = this->size / totalBlocks;
    int numLeftOverElements = this->size % totalBlocks;
    int curStartIndex = 0;
    for (int i = 0; i < totalBlocks; i++) {
      this->responsibilities[i].procId = i % Cluster::procs;
      this->responsibilities[i].startIndex = curStartIndex;
      this->responsibilities[i].numElements = (i < numLeftOverElements ? blockSize + 1 : blockSize);
      curStartIndex += this->responsibilities[i].numElements;

      // if (Cluster::procId == 0) {
      //   cout << this->responsibilities[i].startIndex << " " << this->responsibilities[i].numElements << endl;
      // }
    }
  }

  void allocateSeqParts () {
    int curPart = 0;
    this->mySeqParts = new SeqPart<T>[this->numParts];
    for (int i = 0; i < this->numResponsibilities; i++) {
      if (this->responsibilities[i].procId != Cluster::procId) continue;
      this->mySeqParts[curPart].startIndex = this->responsibilities[i].startIndex;
      this->mySeqParts[curPart].numElements = this->responsibilities[i].numElements;
      this->mySeqParts[curPart].data = new T[this->mySeqParts[curPart].numElements];
      curPart++;
    }
  }

  void initialize (int n) {
    this->size = n;
    computeResponsibilities();
    allocateSeqParts();

    int equalSplit = this->size / Cluster::procs;
    int numLeftOverElements = this->size % Cluster::procs;
    int myLeftOver = Cluster::procId < numLeftOverElements;
    numElements = equalSplit + myLeftOver;
    if (myLeftOver) {
      startIndex = Cluster::procId * (equalSplit + 1);
    } else {
      startIndex = Cluster::procId * equalSplit + numLeftOverElements;
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
    MPI_Win_free(&data_window);
  }

  bool isMine (int index) {
    return startIndex <= index && index < startIndex + numElements;
  }

  int getNodeWithData (int index) {
    int equalSplit = this->size / Cluster::procs;
    int numLeftOverElements = this->size % Cluster::procs;
    int block = (equalSplit + 1) * numLeftOverElements;
    if (index < block) {
      return index / (equalSplit + 1);
    } else {
      return (index - block) / equalSplit + numLeftOverElements;
    }
  }

  int getDataDisp (int index) {
    int equalSplit = this->size / Cluster::procs;
    int numLeftOverElements = this->size % Cluster::procs;
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
  UberSequence (T *array, int n) {
    initialize(n);
    for (int i = 0; i < numElements; i++) {
      this->data[i] = array[startIndex + i];
    }
    endMethod();
  }

  UberSequence (function<T(int)> generator, int n) {
    initialize(n);
    for (int part = 0; part < this->numParts; part++) {
      int startIndex = this->mySeqParts[part].startIndex;
      int numElements = this->mySeqParts[part].numElements;
      for (int i = 0; i < numElements; i++) {
        this->mySeqParts[part].data[i] = generator(startIndex + i);
      }
    }
    endMethod();
  }

  ~UberSequence() {
    destroy();
  }

  void transform (function<T(T)> mapper) {
    for (int i = 0; i < numElements; i++) {
      this->data[i] = mapper(this->data[i]);
    }
    endMethod();
  }

  template<typename S>
  UberSequence<S> map(function<S(T)> mapper) {
    auto nop = [](int _) {
      return 42;
    };
    return UberSequence<S>(nop, 0);
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
    this->data[0] = combiner(scan, this->data[0]);
    for (int i = 1; i < numElements; i++) {
      this->data[i] = combiner(this->data[i-1], this->data[i]);
    }

    free(recvbuf);
  }

  T get (int index) {
    T value;
    MPI_Get(&value, sizeof(T), MPI_BYTE, getNodeWithData(index),
        getDataDisp(index), sizeof(T), MPI_BYTE, data_window);

    MPI_Win_fence(0, data_window);
    return value;
  }

  void set (int index, T value) {
    MPI_Put(&value, sizeof(T), MPI_BYTE, getNodeWithData(index),
        getDataDisp(index), sizeof(T), MPI_BYTE, data_window);

    MPI_Win_fence(0, data_window);
  }

  void print () {
    cout << "Node " << (Cluster::procId + 1)
         << "/"     << Cluster::procs << ":" << endl;
    for (int part = 0; part < this->numParts; part++) {
      int startIndex = this->mySeqParts[part].startIndex;
      int numElements = this->mySeqParts[part].numElements;
      cout << "Part " << part+1 << "/" << this->numParts <<  ": Elements "
           << startIndex << " to " << startIndex + numElements << endl;
      int i;
      for (i = 0; i < numElements; i++) {
        cout << this->mySeqParts[part].data[i] << " ";
        if (i % 10 == 9) cout << endl;
      }
      if (i % 10 != 9) cout << endl;
    }
    cout << endl;
    endMethod();
  }
};

#endif
