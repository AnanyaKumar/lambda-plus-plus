---
layout: post
title: "Final Writeup"
short-title: "Writeup"
date: "Wed May  6 18:37:13 EDT 2015"
---


Lambda++ is a C++ parallel programming library designed to run across a cluster
that achieves near-optimal speedups while enabling programmers to write concise,
functional code. We achieve these speedups using a variety of techniques,
including static cluster profiling, work balancing, and multi-level parallelism
within the cluster.


## Abstraction

Lambda++ presents two abstractions. The Cluster abstraction is used to initialize
and tear down a cluster.

The core of the library, the `Sequence` abstraction, is basically an ordered list
of elements that supports operations like map, reduce, and scan.

More precisely, the `Sequence` abstraction is inspired by 15-210's `SEQUENCE` 
signature for SML. The `Sequence` class declares a number of higher order functions 
which allow users of the library to express their algorithms in a way that is both 
expressive for the algorithm designer as well as easily parallelizable by the library 
authors. For the purposes of our analyses, we implemented a subset of the `SEQUENCE` 
functions, including

- `map`
- `reduce`
- `scan`
- `tabulate`, implemented as a C++ constructor

as well as 3 additional functions not in the 15-210 `SEQUENCE` signature:

- `transform`, an in-place `map`
- `get` and `set`, which load and modify (respectively) the value at an index

All of these operations take in arbitrary functions (thanks to C++11's new
syntax for lambda functions) and work with arbitrary types (due to C++'s
template system).

Operations like `map`, `tabulate`, and `transform` are completely data parallel.
However since they take in arbitrary functions, it's possible for the workers to
diverge in the amount of work done per input, so we put in extra effort to
balance the load across workers to equalize these divergences. We took a similar
approach with `reduce` and `scan`, though these functions have a larger
proportion of serial work load (i.e., they have logarithmic span).


## Implementation

Arbitrary Lambda Functions: All code outside of calls to the Sequence library is executed 
identically by every node in the MPI cluster. 
This allows the Sequence library to operate on generic 
functions (even those that require "variable captures"), since every node has access 
to the same data. 
When a Sequence library method is called, the nodes operate on different parts of 
the sequence, and so the execution paths (of nodes) diverge. 
At the end of the Sequence library call, 
we ensure that all data outside of calls to the Sequence library is the same across nodes. 
The nodes resume executing code outside of the Sequence library identically (the 
execution paths converge).

Advantages: The above 'symmetric' architecture allows 
for arbitrary lambda functions. 
Most other architectures require communicating
lambda functions across nodes. 
Since C++ is a compiled language, it's almost impossible
to send arbitrary lambda functions from one node to another. 
One possible 'hack' is to
bitwise copy the lambda function, but the resulting code would not be portable (since
the way the lambda function is stored is implementation defined). 

Disadvantages: Since code outside of the Sequence library is duplicated, the setup could be
energy inefficient. 
However, this issue can be resolved if the user uses our abstractions only in the parts 
of the workload he wishes to parallelize.
Another issue is that code outside of the Sequence library must be deterministic (e.g. the
user can't ge the system time, or call random number generators). 
This can be solved by
providing the user with libraries for such use cases.
A bigger issue is that we were not able to implement Sequences of sequences using this
architecture, and we are not completely sure if it is feasible.
This is left as future work. We think most workloads can survive without
having to create multi-level sequences.

Work Balancing: The naive way to distribute the sequence across the cluster is to
partition the data evenly.

[![][simple-load-distro]][simple-load-distro]

<!--
  TODO

  Ananya, you can use this section to describe
  - the MPI + OpenMP heterogenous parallelism,
  - work balancing and randomization
  - cluster profiling
  -->

TODO! Check back later :D


## Results

We have two categories of results to outline. The first of these deals with the
performance of our parallel code with respect to our serial code. These results
mostly serve to highlight the effectiveness of our approach (i.e., work
balancing, etc.). The second category compares our parallel implementation to
other parallel implementations, in particular Thrust for GPU-level parallelism
using a similar, functional programming style.

### Effectiveness of Approach

<!--
  TODO

  Ananya, feel free to rearrange these images and add analysis of the approach.
  You might be able to copy a bit from `index.md` with respect to the analysis
  of these graphs.
  -->

[![][ghc-speedup]][ghc-speedup]

[![][ghc-speedup-wb]][ghc-speedup-wb]

[![][latedays-speedup-mandelbrot]][latedays-speedup-mandelbrot]

[![][latedays-speedup-paren]][latedays-speedup-paren]


### Comparison to Thrust

After determining that our approach was effective at load balancing the work to
achieve the optimal speedups on a cluster, we turned our attention to seeing how
well it compared to alternative parallel frameworks, in particular CUDA and
Thrust.

For all of our comparisons, we used 4 nodes on Latedays with the `UberSequence`
implementation, and the NVIDIA GTX 780 on ghc41.

Here's a chart comparing the results of both the `paren_match` and `mandelbrot`
tests for each platform:

[![][thrust-speedup]][thrust-speedup]

In this graph, taller bars are better, as we're comparing the time a given
implementation took to run vs. the time the `UberSequence` implementation took
to run (explaining why `UberSequence` is always 1).

Notice that the Lambda++ implementation of `paren_match` was much faster than on
the GPU, whereas the `mandelbrot` was close but Thrust won by a slight margin.
We can attribute this to the fact that GPUs are really good at running SIMD,
which is a pretty accurate description of what's going on in Mandelbrot
generation. However, when it comes to reductions, Lambda++ is faster because it
can move work around nodes as well as parallelize the reduction within a node
for decreased communication overall.

All in all, these results indicate that Lambda++ is a competitive choice for
certain types of applications, though it's worthwhile to note that this is a
cluster of 4 machines vs a single machine with a GPU. It will heavily depend on
circumstances to determine whether this tradeoff is worth the potential
performance benefit.


## References

We referenced a number of resources while writing Lambda++, including

- 15-210's [`SEQUENCE` docs][seq], which describe the semantics of the various
  higher order functions like `map`, `reduce`, `scan`, etc.
- The Thrust documentation, available at
  - <https://thrust.github.io/doc/> and
  - <https://github.com/thrust/thrust/wiki>


<!--
  TODO

  Ananya, you can add any references you used here. I don't have that much
  Internet access right now, so I can't look up many references.
  -->


## Effort Distribution

We feel that equal effort was demonstrated by project members throughout. We
both had a great time working on Lambda++ and are proud of how it turned out!


<!-- References -->
[seq]: http://www.cs.cmu.edu/~15210/docs/sig/SEQUENCE.html

<!-- Images -->
[simple-load-distro]: {{ "/img/simple-load-distro.png" | prepend: site.baseurl }}
[ghc-speedup]: {{ "/img/ghc-speedup.png" | prepend: site.baseurl }}
[ghc-speedup-wb]: {{ "/img/ghc-speedup-wb.png" | prepend: site.baseurl }}
[latedays-speedup-mandelbrot]: {{ "/img/latedays-speedup-mandelbrot.png" | prepend: site.baseurl }}
[latedays-speedup-paren]: {{ "/img/latedays-speedup-paren.png" | prepend: site.baseurl }}
[thrust-speedup]: {{ "/img/thrust-speedup.png" | prepend: site.baseurl }}
