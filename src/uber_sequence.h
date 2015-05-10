#ifndef _UBER_SEQUENCE_H_
#define _UBER_SEQUENCE_H_

#include <iostream>
#include <cassert>
#include <mpi.h>
#include <omp.h>

#include "sequence.h"
#include "cluster.h"

using namespace std;

/** Used to store which parts of the sequence each node is responsible for **/
struct Responsibility
{
  int procId;
  int startIndex;
  int numElements;
  MPI_Win data_window;
};

/** Used to store the parts of the sequence the node is responsible for **/
template<typename T>
struct SeqPart
{
  int startIndex;
  int numElements;
  T* data;
};

/** This is an uber sequence **/
template<typename T>
class UberSequence : public Sequence<T>
{
  int numResponsibilities;
  Responsibility *responsibilities;
  int numParts;
  SeqPart<T> *mySeqParts;

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
      if (this->responsibilities[i].numElements < 1) {
        cout << "Warning: Sequence library not verified for small sequences." << endl;
      }
    }
  }

  void allocateSeqParts () {
    int curPart = 0;
    this->mySeqParts = new SeqPart<T>[this->numParts];
    for (int i = 0; i < this->numResponsibilities; i++) {
      if (this->responsibilities[i].procId != Cluster::procId) {
        MPI_Win_create(NULL, 0, sizeof(T), MPI_INFO_NULL, MPI_COMM_WORLD, 
          &responsibilities[i].data_window);
        MPI_Win_fence(0, responsibilities[i].data_window);
      } else {
        int numElements = this->responsibilities[i].numElements;
        this->mySeqParts[curPart].startIndex = this->responsibilities[i].startIndex;
        this->mySeqParts[curPart].numElements = numElements;
        this->mySeqParts[curPart].data = new T[numElements];
        curPart++;
        MPI_Win_create(this->mySeqParts[curPart].data, numElements * sizeof(T), sizeof(T),
          MPI_INFO_NULL, MPI_COMM_WORLD, &responsibilities[i].data_window);
        MPI_Win_fence(0, responsibilities[i].data_window);
      }
    }
  }

  void initialize (int n) {
    this->size = n;
    computeResponsibilities();
    allocateSeqParts();
  }

  void destroy () {
    for (int i = 0; i < this->numParts; i++) {
      delete[] this->mySeqParts[i].data;
    }
    delete[] this->mySeqParts;
    int totalBlocks = Cluster::procs * Cluster::blocksPerProc;
    for (int i = 0; i < totalBlocks; i++) {
      MPI_Win_free(&(responsibilities[i].data_window));
    }
  }

  int getNodeWithData (int index) {
    // Get the node for the index
    int nodeWithIndex = 0;
    int totalBlocks = Cluster::procs * Cluster::blocksPerProc;
    for (int block = 0; block < totalBlocks; block++) {
      int startIndex = this->responsibilities[block].startIndex;
      int numElements = this->responsibilities[block].numElements;
      if (startIndex <= index && index < startIndex + numElements) {
        nodeWithIndex = this->responsibilities[block].procId;
      }
    }
    return nodeWithIndex;
  }

  /** Assumes the current node has the index **/
  T getData (int index) {
    for (int part = 0; part < this->numParts; part++) {
      int startIndex = this->mySeqParts[part].startIndex;
      int numElements = this->mySeqParts[part].numElements;
      if (startIndex <= index && index < startIndex + numElements) {
        return this->mySeqParts[part].data[index - startIndex];
      }
    }
    assert(false);
    T x;
    return x;
  }

  void endMethod () {
    MPI_Barrier(MPI_COMM_WORLD);
  }

  T *getPartialReduces (function<T(T,T)> combiner) {
    // Get all my partial results (sendbuf). Note assumes each part has >= 1 element.
    T *myPartialReduces = new T[this->numParts];
    for (int part = 0; part < this->numParts; part++) {
      int numElements = this->mySeqParts[part].numElements;
      T *curData = this->mySeqParts[part].data;
      myPartialReduces[part] = curData[0];
      for (int i = 1; i < numElements; i++) {
        myPartialReduces[part] = combiner(myPartialReduces[part], curData[i]);
      }
    }

    // Compute receive counts, displacements for AllGatherV
    int totalBlocks = Cluster::blocksPerProc * Cluster::procs;
    T *recvbuf = new T[totalBlocks];
    T *recvcounts = new T[Cluster::procs]; // Note, this is in BYTES
    T *displs = new T[Cluster::procs]; // Note, this is in BYTES
    for (int i = 0; i < Cluster::procs; i++) {
      recvcounts[i] = Cluster::blocksPerProc * sizeof(T);
      displs[i] = i * Cluster::blocksPerProc * sizeof(T);
    }

    // MPI all gatherv
    MPI_Barrier(MPI_COMM_WORLD); // Is the barrier necessary?
    MPI_Allgatherv(myPartialReduces, this->numParts * sizeof(T), MPI_BYTE, 
      recvbuf, recvcounts, displs, MPI_BYTE, MPI_COMM_WORLD);

    // Sort the receive buffer into the correct order (hack, assumes that nodes' data is interleaved)
    T *partialReduces = new T[totalBlocks];
    for (int i = 0; i < totalBlocks; i++) {
      int procNum = i / Cluster::blocksPerProc;
      int procDisp = i % Cluster::blocksPerProc;
      int index = procDisp * Cluster::procs + procNum;
      partialReduces[index] = recvbuf[i];
    }

    // Free everything & Return
    free(recvbuf);
    free(recvcounts);
    free(displs);
    return partialReduces;
  }

public:
  UberSequence (T *array, int n) {
    initialize(n);
    for (int part = 0; part < this->numParts; part++) {
      int startIndex = this->mySeqParts[part].startIndex;
      int numElements = this->mySeqParts[part].numElements;
      for (int i = 0; i < numElements; i++) {
        this->mySeqParts[part].data[i] = array[startIndex + i];
      }
    }
    endMethod();
  }

  UberSequence (function<T(int)> generator, int n) {
    initialize(n);
    for (int part = 0; part < this->numParts; part++) {
      int startIndex = this->mySeqParts[part].startIndex;
      int numElements = this->mySeqParts[part].numElements;
      #pragma omp parallel for
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
    for (int part = 0; part < this->numParts; part++) {
      int numElements = this->mySeqParts[part].numElements;
      for (int i = 0; i < numElements; i++) {
        this->mySeqParts[part].data[i] = mapper(this->mySeqParts[part].data[i]);
      }
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
    T *partialReduces = getPartialReduces(combiner);

    // Compute the final answer
    T value = init;
    for (int i = 0; i < Cluster::procs * Cluster::blocksPerProc; i++) {
      value = combiner(value, partialReduces[i]);
    }

    free(partialReduces);
    endMethod();
    return value;
  }

  void scan (function<T(T,T)> combiner, T init) {
    T *partialReduces = getPartialReduces(combiner);

    // Get the combination of all values before values in current node
    T scan = init;
    int myBlocksScanned = 0;
    for (int i = 0; i < Cluster::procs * Cluster::blocksPerProc; i++) {
      // Check if the current block is mine, if so apply
      if (this->responsibilities[i].procId == Cluster::procId) {
        int numElements = this->mySeqParts[myBlocksScanned].numElements;
        T *curData = this->mySeqParts[myBlocksScanned].data;
        for (int j = 0; j < numElements; j++) {
          scan = combiner(scan, curData[j]);
          curData[j] = scan;
        }
        myBlocksScanned++;
      } else {
        scan = combiner(scan, partialReduces[i]);
      }
    }

    free(partialReduces);
    endMethod();
  }

  T get (int index) {
    int nodeWithIndex = getNodeWithData(index);
    T value;
    if (Cluster::procId == nodeWithIndex) {
      value = getData(index);
    }

    // Hack, only works if you call get from outside the sequence library
    MPI_Bcast(&value, sizeof(T), MPI_BYTE, nodeWithIndex, MPI_COMM_WORLD);

    // T value = 5;
    // if (Cluster::procId != 0) {
    //   MPI_Get(&value, sizeof(T), MPI_BYTE, 0,
    //       0, sizeof(T), MPI_BYTE, this->responsibilities[0].data_window);
    // } else {
    //   value = this->mySeqParts[0].data[1];
    // }
    // MPI_Win_fence(0, this->responsibilities[0].data_window);
    // // MPI_Win_fence(0, this->responsibilities[0].data_window);
    return value;
  }

  void set (int index, T value) {
    // MPI_Put(&value, sizeof(T), MPI_BYTE, getNodeWithData(index),
    //     getDataDisp(index), sizeof(T), MPI_BYTE, data_window);
    // MPI_Win_fence(0, data_window);
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
