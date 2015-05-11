#ifndef _CLUSTER_H_
#define _CLUSTER_H_

namespace Cluster {
  // Information about the cluster
  extern int procs;
  extern int blocksPerProc;
  extern int threadsPerProc;
  extern int systemTime;
  extern int *procTimes;
  // Information about this node
  extern int procId;
  void init (int *argc, char ***argv);
  void close ();
};

#endif
