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
  // MPI_Win data_window;
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
  int numThreadBlocks;

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
        // MPI_Win_create(NULL, 0, sizeof(T), MPI_INFO_NULL, MPI_COMM_WORLD, 
        //   &responsibilities[i].data_window);
        // MPI_Win_fence(0, responsibilities[i].data_window);
      } else {
        int numElements = this->responsibilities[i].numElements;
        this->mySeqParts[curPart].startIndex = this->responsibilities[i].startIndex;
        this->mySeqParts[curPart].numElements = numElements;
        this->mySeqParts[curPart].data = new T[numElements];
        curPart++;
        // MPI_Win_create(this->mySeqParts[curPart].data, numElements * sizeof(T), sizeof(T),
        //   MPI_INFO_NULL, MPI_COMM_WORLD, &responsibilities[i].data_window);
        // MPI_Win_fence(0, responsibilities[i].data_window);
      }
    }
  }

  void initialize (int n) {
    this->size = n;
    this->numThreadBlocks = Cluster::threadsPerProc;
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
      // MPI_Win_free(&(responsibilities[i].data_window));
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

  T *getSeqPartialReduces (SeqPart<T> *seqPart, function<T(T,T)> combiner) {
    int indexScaling = 64 / sizeof(T); // Scale all indices to prevent false sharing
    T *seqPartialReduces = new T[this->numThreadBlocks * indexScaling];
    #pragma omp parallel
    {
      // Find out which part of the seqPart I'm responsible for
      int numThreads = omp_get_num_threads();
      int threadId = omp_get_thread_num();
      int numElements = seqPart->numElements;
      int equalSplit = numElements / numThreads;
      int numLeftOverElements = numElements % numThreads;
      int myLeftOver = threadId < numLeftOverElements;
      int startIndex;
      if (myLeftOver) {
        startIndex = threadId * (equalSplit + 1);
      } else {
        startIndex = threadId * equalSplit + numLeftOverElements;
      }
      int myNumElements = equalSplit + myLeftOver;

      // Compute my partial reduce
      int threadIdx = threadId * indexScaling;
      seqPartialReduces[threadIdx] = seqPart->data[startIndex];
      for (int i = 1; i < myNumElements; i++) {
        seqPartialReduces[threadIdx] = combiner(seqPartialReduces[threadIdx], 
          seqPart->data[startIndex + i]);
      }
    }
    return seqPartialReduces;
  }

  void makeSeqPartialScans (T *seqPartialReduces, function<T(T,T)> combiner) {
    int indexScaling = 64 / sizeof(T); // Scale all indices to prevent false sharing
    for (int i = 1; i < this->numThreadBlocks; i++) {
      seqPartialReduces[i * indexScaling] = combiner(seqPartialReduces[(i - 1) * indexScaling], 
        seqPartialReduces[i * indexScaling]);
    }
  }

  void applySeqScans (SeqPart<T> *seqPart, function<T(T,T)> combiner, T init, T *seqPartialScans) {
    int indexScaling = 64 / sizeof(T); // Scale all indices to prevent false sharing
    #pragma omp parallel
    {
      // Find out which part of the seqPart I'm responsible for
      int numThreads = omp_get_num_threads();
      int threadId = omp_get_thread_num();
      int numElements = seqPart->numElements;
      int equalSplit = numElements / numThreads;
      int numLeftOverElements = numElements % numThreads;
      int myLeftOver = threadId < numLeftOverElements;
      int startIndex;
      if (myLeftOver) {
        startIndex = threadId * (equalSplit + 1);
      } else {
        startIndex = threadId * equalSplit + numLeftOverElements;
      }
      int myNumElements = equalSplit + myLeftOver;

      // Compute my scans
      int threadIdx = threadId * indexScaling;
      T scan;
      if (threadId == 0) {
        scan = init;
      } else {
        scan = combiner(init, seqPartialScans[threadIdx - indexScaling]);
      }
      for (int i = 0; i < myNumElements; i++) {
        scan = combiner(scan, seqPart->data[startIndex + i]);
        seqPart->data[startIndex + i] = scan;
      }
    }
  }

  T getSeqReduce (T *seqPartialReduces, function<T(T,T)> combiner) {
    int indexScaling = 64 / sizeof(T); // Scale all indices to prevent false sharing
    T reduce = seqPartialReduces[0];
    for (int i = 1; i < this->numThreadBlocks; i++) {
      reduce = combiner(reduce, seqPartialReduces[i * indexScaling]);
    }
    return reduce;
  }

  T *getPartialReduces (T *myPartialReduces) {
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
    free(myPartialReduces);
    free(recvbuf);
    free(recvcounts);
    free(displs);
    return partialReduces;
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

    return getPartialReduces(myPartialReduces);
  }

public:
  UberSequence (T *array, int n) {
    initialize(n);
    for (int part = 0; part < this->numParts; part++) {
      int startIndex = this->mySeqParts[part].startIndex;
      int numElements = this->mySeqParts[part].numElements;
      #pragma omp parallel for
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
      #pragma omp parallel for
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
    T *myPartialReduces = new T[this->numParts];
    for (int part = 0; part < this->numParts; part++) {
      T *seqPartialReduces = getSeqPartialReduces(&(this->mySeqParts[part]), combiner);
      myPartialReduces[part] = getSeqReduce(seqPartialReduces, combiner);
      delete[] seqPartialReduces;
    }

    T *partialReduces = getPartialReduces(myPartialReduces);

    // Compute the final answer
    T value = init;
    for (int i = 0; i < Cluster::procs * Cluster::blocksPerProc; i++) {
      value = combiner(value, partialReduces[i]);
    }

    delete[] partialReduces;
    endMethod();
    return value;
  }

  void scan (function<T(T,T)> combiner, T init) {
    T *myPartialReduces = new T[this->numParts];
    T **seqPartialReduces = new T*[this->numParts];
    for (int part = 0; part < this->numParts; part++) {
      seqPartialReduces[part] = getSeqPartialReduces(&(this->mySeqParts[part]), combiner);
      myPartialReduces[part] = getSeqReduce(seqPartialReduces[part], combiner);
      makeSeqPartialScans(seqPartialReduces[part], combiner);
    }

    T *partialReduces = getPartialReduces(myPartialReduces);

    // Get the combination of all values before values in current node
    T scan = init;
    int myBlocksScanned = 0;
    for (int i = 0; i < Cluster::procs * Cluster::blocksPerProc; i++) {
      // Check if the current block is mine, if so apply
      if (this->responsibilities[i].procId == Cluster::procId) {
        applySeqScans(&(this->mySeqParts[myBlocksScanned]), combiner, scan, 
          seqPartialReduces[myBlocksScanned]);
        myBlocksScanned++;
      }
      scan = combiner(scan, partialReduces[i]);
    }

    delete[] partialReduces;
    for (int part = 0; part < this->numParts; part++) {
      delete[] seqPartialReduces[part];
    }
    delete[] seqPartialReduces;
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
