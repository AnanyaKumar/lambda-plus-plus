# Lambda++: Hardware sensitive sequence library

## Description

We are going to build a parallel sequence library with support for operations
like map, reduce, and scan. Our library should automatically detect hardware
configurations and adapt its performance. For a sample program see main.cpp.

## Restrictions:

`Cluster::init(&argc, &argv)` must be called before executing any other code.
This sets up your cluster.

`Cluster::close()` must be called at the end of all other code (before returning
from main).

Standard input/output functions should only be used for debugging purposes.

You may not use non-deterministic functions (e.g. rand) outside of sequence
code. Use equivalent functions from our library (to come in the future).

## Architecture notes

The sequence library is an abstraction, and is not restricted to any particular
architecture. However, understanding the implementation can help you understand
performance details.

The current platform is implemented in MPI. All code outside of calls to the
sequence library is executed identically by every node in the MPI cluster (which
is why only deterministic code is allowed). The restriction allows the Sequence
library to operate on generic functions (even those that require "variable
captures"), since every node has access to the same data.

The Sequence class stores data distributed across the nodes in the cluster. When
executing a function (like map) the nodes operate on their data, and if
necessary communicate information (for functions like reduce).
