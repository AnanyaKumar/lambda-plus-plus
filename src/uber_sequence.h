#ifndef _UBER_SEQUENCE_H_
#define _UBER_SEQUENCE_H_

#include <algorithm>
#include <iostream>
#include <cassert>
#include <ctime>
#include <mpi.h>
#include <omp.h>

#include "sequence.h"
#include "cluster.h"

using namespace std;

#define RANDOMIZE_WORK true // Randomly allocated blocks to nodes (instead of interleaving)
#define ADJUST_WORK true // Gives faster nodes more work

/** Used to store which parts of the sequence each node in the cluster is responsible for **/
struct Responsibility
{
  int procId;
  int startIndex;
  int numElements;
};

/** Used to store the parts of the sequence the current node is responsible for **/
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
public:
  int numResponsibilities;
  Responsibility *responsibilities;
  int numParts;
  SeqPart<T> *mySeqParts;
  int numThreadBlocks;

  /** Figure out which nodes are responsible for which parts of the sequence **/
  void computeResponsibilities () {
    int totalBlocks = Cluster::blocksPerProc * Cluster::procs;
    this->numResponsibilities = totalBlocks;
    this->numParts = Cluster::blocksPerProc;
    this->responsibilities = new Responsibility[totalBlocks];

    // Interleave blocks amongst nodes
    int *partToNodeMap = new int[totalBlocks];
    for (int part = 0; part < totalBlocks; part++) {
      partToNodeMap[part] = part % Cluster::procs;
    }

    if (RANDOMIZE_WORK) {
      // Ensure that all nodes have the same seed for the subsequent random shuffle
      time_t seed;
      if (Cluster::procId == 0) {
        seed = time(NULL);
      }
      MPI_Bcast(&seed, sizeof(time_t), MPI_BYTE, 0, MPI_COMM_WORLD);
      srand(seed);

      // Randomly shuffle chunks representing who's responsible for what    
      auto myRandom = [](int i) { 
        return rand() % i;
      };
      random_shuffle(partToNodeMap, partToNodeMap + totalBlocks, myRandom);
    }

    // Determine the sizes of the responsibilities
    if (ADJUST_WORK) {
      int elementsCovered = 0;
      for (int block = 0; block < totalBlocks; block++) {
        int procId = partToNodeMap[block];
        this->responsibilities[block].numElements = (Cluster::procTimes[procId] * this->size) / 
          (Cluster::blocksPerProc * Cluster::systemTime);
        // For correctness, some functions require that every block has one element
        if (this->responsibilities[block].numElements < 1) {
          this->responsibilities[block].numElements = 1;
        }
        elementsCovered += this->responsibilities[block].numElements;
      }
      int elementsLeft = this->size - elementsCovered;

      // Make sure the blocks sum up to the total size
      int block = 0;
      while (elementsLeft > 0) {
        this->responsibilities[block].numElements++;
        block = (block + 1) % totalBlocks;
        elementsLeft--;
      }
      while (elementsLeft < 0) {
        if (this->responsibilities[block].numElements > 1) {
          this->responsibilities[block].numElements--;
          elementsLeft++;
        }
        block = (block + 1) % totalBlocks;
      }
    } else {
      int blockSize = this->size / totalBlocks;
      int numLeftOverElements = this->size % totalBlocks;
      for (int block = 0; block < totalBlocks; block++) {
        int procId = partToNodeMap[block];
        this->responsibilities[block].numElements = (block < numLeftOverElements ? 
          blockSize + 1 : blockSize);
      }
    }

    // Assign responsibilities
    int curStartIndex = 0;
    for (int block = 0; block < totalBlocks; block++) {
      this->responsibilities[block].procId = partToNodeMap[block];
      this->responsibilities[block].startIndex = curStartIndex;
      curStartIndex += this->responsibilities[block].numElements;
      if (this->responsibilities[block].numElements < 1) {
        cout << "Warning: Sequence library not verified for small sequences." << endl;
      }
    }

    // Clean up
    delete[] partToNodeMap;
  }

  /** Allocate sequence parts based on the work that has been assigned to the current node **/
  void allocateSeqParts () {
    int curPart = 0;
    this->mySeqParts = new SeqPart<T>[this->numParts];
    for (int i = 0; i < this->numResponsibilities; i++) {
      if (this->responsibilities[i].procId != Cluster::procId) {
      } else {
        int numElements = this->responsibilities[i].numElements;
        this->mySeqParts[curPart].startIndex = this->responsibilities[i].startIndex;
        this->mySeqParts[curPart].numElements = numElements;
        this->mySeqParts[curPart].data = new T[numElements];
        curPart++;
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
    delete[] this->responsibilities;
  }

  /** Find which node has the element indexed by 'index' **/
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

  /** Assumes the current node has the element index by 'index'
      Otherwise, kills itself to prevent programming screwups **/
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

  /** Call this at the end of every method **/
  void endMethod () {
    MPI_Barrier(MPI_COMM_WORLD);
  }

  /** Gets reduces for each thread block in the sequence part
      E.g. if the sequence part is (1, 3, 5, 2, 8, 1) and there are 2 thread blocks
           then the sequence reduces are 9 and 11 for each block.
      Warning: seqPartialReduces is indexed abnormally to avoid false sharing **/
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
      if (startIndex < seqPart->numElements) {
        int myNumElements = equalSplit + myLeftOver;

        // Compute my partial reduce
        int threadIdx = threadId * indexScaling;
        seqPartialReduces[threadIdx] = seqPart->data[startIndex];
        for (int i = 1; i < myNumElements; i++) {
          seqPartialReduces[threadIdx] = combiner(seqPartialReduces[threadIdx], 
            seqPart->data[startIndex + i]);
        }
      }
    }
    return seqPartialReduces;
  }

  /** Makes the partial reduces into partial scans
      E.g. (0, 1, 3, 6) becomes (0, 1, 4, 10)
      Warning: seqPartialReduces is indexed abnormally to avoid false sharing **/
  void makeSeqPartialScans (T *seqPartialReduces, function<T(T,T)> combiner) {
    int indexScaling = 64 / sizeof(T); // Scale all indices to prevent false sharing
    for (int i = 1; i < this->numThreadBlocks; i++) {
      seqPartialReduces[i * indexScaling] = combiner(seqPartialReduces[(i - 1) * indexScaling], 
        seqPartialReduces[i * indexScaling]);
    }
  }

  /** Let's say seqPart is (1, 4, 2, 8, 9, 11), there are 3 thread blocks, and combiner is +
      init is the scan to the start of the sequence part (let's say it's 5)
      seqPartialScans is (5, 10, 20)
      Then seqPart will be transformed to (5+1, 5+1+4, 5+1+4+2, 5+1+4+2+8, ...)
      In effect 'applying' the scan to the sequence part **/
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

      if (startIndex < seqPart->numElements) {
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
  }

  /** Returns combiner(seqPartialReduces[0], combiner(seqPartialReduces[0], ...)) **/
  T getSeqReduce (SeqPart<T> *seqPart, T *seqPartialReduces, function<T(T,T)> combiner) {
    int indexScaling = 64 / sizeof(T); // Scale all indices to prevent false sharing
    T reduce = seqPartialReduces[0];
    for (int i = 1; i < min(this->numThreadBlocks, seqPart->numElements); i++) {
      reduce = combiner(reduce, seqPartialReduces[i * indexScaling]);
    }
    return reduce;
  }

  /** Given reduced values for each sequence part in the current node, in order
      Returns an ordered list of reduced values for each entry in responsibilities
        (from accross the cluster) **/
  T *getPartialReduces (T *myPartialReduces) {
    // Compute receive counts, displacements for AllGatherV
    int totalBlocks = Cluster::blocksPerProc * Cluster::procs;
    T *recvbuf = new T[totalBlocks];
    int *recvcounts = new int[Cluster::procs]; // Note, this is in BYTES
    int *displs = new int[Cluster::procs]; // Note, this is in BYTES
    for (int i = 0; i < Cluster::procs; i++) {
      recvcounts[i] = Cluster::blocksPerProc * sizeof(T);
      displs[i] = i * Cluster::blocksPerProc * sizeof(T);
    }

    // MPI all gatherv
    MPI_Barrier(MPI_COMM_WORLD); // Is the barrier necessary?
    MPI_Allgatherv(myPartialReduces, this->numParts * sizeof(T), MPI_BYTE, 
      recvbuf, recvcounts, displs, MPI_BYTE, MPI_COMM_WORLD);

    // Sort the receive buffer into the correct order to get partialReduces
    T *partialReduces = new T[totalBlocks];
    int reduceCounts[Cluster::procs];
    fill(reduceCounts, reduceCounts + Cluster::procs, 0);
    for (int i = 0; i < totalBlocks; i++) {
      int procId = responsibilities[i].procId;
      int recvIndex = Cluster::blocksPerProc * procId + reduceCounts[procId];
      reduceCounts[procId]++;
      partialReduces[i] = recvbuf[recvIndex];
    }

    // Free everything & Return
    delete[] recvbuf;
    delete[] recvcounts;
    delete[] displs;
    return partialReduces;
  }

  /** Returns an ordered list of reduced values for each entry in responsibilities
        (from accross the cluster) **/
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

  /** API Functions **/

  UberSequence () {

  }

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

  template<typename S>
  UberSequence<S> *map(function<S(T)> mapper) {
    UberSequence<S> *newSeq = new UberSequence<S>;
    newSeq->size = this->size;
    newSeq->numResponsibilities = this->numResponsibilities;
    int totalBlocks = Cluster::blocksPerProc * Cluster::procs;
    newSeq->responsibilities = new Responsibility[totalBlocks];
    for (int block = 0; block < numResponsibilities; block++) {
      newSeq->responsibilities[block].procId = this->responsibilities[block].procId;
      newSeq->responsibilities[block].startIndex = this->responsibilities[block].startIndex;
      newSeq->responsibilities[block].numElements = this->responsibilities[block].numElements;
    }
    newSeq->numParts = this->numParts;
    newSeq->allocateSeqParts();
    newSeq->numThreadBlocks = this->numThreadBlocks;
    for (int part = 0; part < this->numParts; part++) {
      int numElements = this->mySeqParts[part].numElements;
      for (int i = 0; i < numElements; i++) {
        newSeq->mySeqParts[part].data[i] = mapper(this->mySeqParts[part].data[i]);
      }
    }
    endMethod();
    return newSeq;
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

  T reduce (function<T(T,T)> combiner, T init) {
    T *myPartialReduces = new T[this->numParts];
    for (int part = 0; part < this->numParts; part++) {
      T *seqPartialReduces = getSeqPartialReduces(&(this->mySeqParts[part]), combiner);
      myPartialReduces[part] = getSeqReduce(&(this->mySeqParts[part]), seqPartialReduces, combiner);
      delete[] seqPartialReduces;
    }

    T *partialReduces = getPartialReduces(myPartialReduces);

    // Compute the final answer
    T value = init;
    for (int i = 0; i < Cluster::procs * Cluster::blocksPerProc; i++) {
      value = combiner(value, partialReduces[i]);
    }

    delete[] myPartialReduces;
    delete[] partialReduces;
    endMethod();
    return value;
  }

  void scan (function<T(T,T)> combiner, T init) {
    T *myPartialReduces = new T[this->numParts];
    T **seqPartialReduces = new T*[this->numParts];
    for (int part = 0; part < this->numParts; part++) {
      seqPartialReduces[part] = getSeqPartialReduces(&(this->mySeqParts[part]), combiner);
      myPartialReduces[part] = getSeqReduce(&(this->mySeqParts[part]), seqPartialReduces[part], combiner);
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

    delete[] myPartialReduces;
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
    return value;
  }

  void set (int index, T value) {

  }

  /** For debugging purposes **/
  void print () {
    // Issue: doesn't work for some data types
    // cout << "Node " << (Cluster::procId + 1)
    //      << "/"     << Cluster::procs << ":" << endl;
    // for (int part = 0; part < this->numParts; part++) {
    //   int startIndex = this->mySeqParts[part].startIndex;
    //   int numElements = this->mySeqParts[part].numElements;
    //   cout << "Part " << part+1 << "/" << this->numParts <<  ": Elements "
    //        << startIndex << " to " << startIndex + numElements << endl;
    //   int i;
    //   for (i = 0; i < numElements; i++) {
    //     cout << this->mySeqParts[part].data[i] << " ";
    //     if (i % 10 == 9) cout << endl;
    //   }
    //   if (i % 10 != 0) cout << endl;
    // }
    // cout << endl;
    endMethod();
  }

  /** For debugging purposes only **/
  void printResponsibilities () {
    if (Cluster::procId == 0) {
      for (int i = 0; i < this->numResponsibilities; i++) {
        cout << responsibilities[i].startIndex << ":" << 
          responsibilities[i].startIndex + responsibilities[i].numElements - 1 <<
          " by Node " << responsibilities[i].procId << endl;
      }
    }
  }
};

#endif
