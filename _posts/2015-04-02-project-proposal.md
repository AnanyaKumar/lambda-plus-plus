---
layout: post
title: "Project Proposal"
date: "Thu Apr  2 13:01:12 EDT 2015"
---


For our final project, we will build a parallel sequence library for C++ with
support for operations like `map`, `reduce`, and `scan`, which take higher order
functions. Our library will automatically detect hardware configurations and
adapt its performance.


## Background

Writing parallel programs is difficult. Challenges such as intelligently
assigning work to nodes and minimizing communication costs makes it impossible
for the average programmer to write good parallel applications.  Programmers
need libraries that enable easy development of parallel applications--libraries
that are general and fast. The problem with most libraries is that they don't
adapt to hardware configurations, which leads to poor performance.

We will build a parallel sequence library for C++ to solve this problem. Our
library will support operations such as creating new sequences, mapping a
function onto sequences, reducing combiners over sequences (eg. adding
elements). Support for higher order functions using C++11 lambda functions is
key to the versatility of this library. The functions in this library are very
general--they can be used to solve problems from sorting to writing a breadth
first search.

This platform will be implemented in MPI, because MPI gives us control over
communication between nodes. We focus on large scale parallel applications--
programs that run across multiple computers. We will build configuration scripts
to estimate the topology of the network (so we don't send data across nodes that
are far away), and communication costs (larger communication costs means that
you'd want less of a dynamic assignment). This enables high performance across
hardware configurations.


## Challenges

The first challenge is designing a good API. Exposing cluster-level parallelism
to a sequential program requires a strong understanding of the types of parallel
programs people would use this API for. We will also need to learn about C++11
features such as lambda functions, and how to create a library that seamlessly
works on different platforms with minimal setup costs.

Ensuring our application performs well, especially across different hardware
configurations, is a huge challenge. For example, a higher order function passed
to a `map` call might not take the same amount of time on all inputs. A naive
static assignment can be particularly bad in certain use cases (like in
computing the Mandelbrot set). The chunk size in a dynamic scheme will need to
be varied based on the type of cluster used (large chunk sizes would work better
if communication costs are high). Effectively characterizing cluster properties
requires an intimate understand of parallel architecture.


## Resources

We would like access to Amazon EC2 clusters if possible. Besides that, we will
use Blacklight for testing.


## Goals & Deliverables

We've broken up our goals and deliverables into three sections: implementing a
library of functions on sequences that take functions as arguments, using these
library functions to write performant code, and finally analyzing how well we
did compared with other parallel implementations.

### Implement Functional Sequence Library

The first feature we plan to achieve is to implement a subset of the [`SEQUENCE`
signature][seq] in C++ that supports higher-order functions. Users of this library
will be able to pass lambda functions to library functions such as `map`,
`reduce`, `tabulate`, `scan`, and `collect`, as they are specified in
the `SEQUENCE` signature.

A feature we hope to achieve is to add more functions to this interface (like
`rev`, `append`, `flatten`, etc.), in the hopes of making it easy and practical
to write functional programs in C++.

### Write Performant Code Using Lambda++

Having implemented the library functions themselves, the next feature we plan to
achieve is to actually implement a number of algorithms using this set of
library functions. These include:

- a sorting function
- at least one of
  - a graph algorithm, like BFS or Bellman-Ford, or
  - an image manipulation algorithm, like Mandelbrot fractal image generation

We also want to implement a feature that lets us analyze and tune our algorithms
based on the network where the program will be running. This will allow us to
eke out eke out even more performance from our system.

One feature we hope to achieve is to make our code (both these example
applications as well as the base library code) performant by determining
effective ways of __dynamically assigning work__. This problem applies
especially to our project because we'll be taking and running arbitrary
functions on sequences of data. Since the programmer will be limited in
optimization potential by the library functions given, the burden of optimizing
code performance falls to the implementation of the library itself.

### Analyze Performance

The final feature we plan to achieve is to provide an analysis of the
performance of our code compared with the performance of widely-established
parallel algorithm implementations, for example those in the Problem-Based
Benchmark Suite or the implementations we've seen in class.


## Platform Choice

For the task of implementing a functional, parallel sequence library, we had a
couple of options. The first was to choose a language that heavily supports the
functional programming paradigm, like SML, OCaml, or Haskell. While these
languages support a very deep level of functional programming in the language,
they don't expose powerful primitives for implementing parallelism.

Luckily, thanks to C++11 (which is now supported on GPUs as well, thanks to CUDA
7[^1]), we can have access to functions as values as well as expressive parallel
programming primitives. Though C++ might not support functional programming as
deeply as other languages, it offers nice balance for our intentions.

Since we're targeting large scale parallel applications with many nodes, we'll
be using MPI to parallelize our code. Using a message-passing interface for
parallelism will allow us to fine tune our communication patterns to adapt to
different network topologies and communication costs.


## Schedule

Project milestones are listed by intended completion date.

- __Sunday, April 12__
    - Implement a parallel, minimum working product of the Sequence interface
        - This will allow us to begin implementing algorithms that use the library.
- __Sunday, April 19__
    - Implement basic versions of the example applications we'd like to
      experiment on.
        - Progress on optimizing our code can't happen until we have a basic
          working implementation for both the library and application.
- __Sunday, April 26__
  - Refactor the implementations of both the library and application code, using
    improvements and tweaks in one to bolster performance in the other.
- __Sunday, March 3__
  - Write scripts to analyze the network topology of our system and minimize
    communication costs.
- __Sunday, March 10__
  - Collect and analyze the results of our application code with respect to
    popular alternative implementations.



[^1]: http://devblogs.nvidia.com/parallelforall/cuda-7-release-candidate-feature-overview/
[seq]: http://www.cs.cmu.edu/~15210/docs/sig/SEQUENCE.html
