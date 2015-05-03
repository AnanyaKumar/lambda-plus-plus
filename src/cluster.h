#ifndef _CLUSTER_H_
#define _CLUSTER_H_

namespace Cluster {
  // Information about this node
  extern int procs;
  extern int procId;
  void init (int *argc, char ***argv);
  void close ();
};

#endif
