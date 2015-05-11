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


## Background

To implement Lambda++, we wrap a vector in an abstraction we call the `Sequence`
class, directly inspired by 15-210's `SEQUENCE` signature for SML. The
`Sequence` class (as the `SEQUENCE` signature before it) declares a number of
higher order functions which allow users of the library to express their
algorithms in a way that is both expressive for the algorithm designer as well
as easily parallelizable by the library author. For the purposes of our
analyses, we implemented a subset of the `SEQUENCE` functions, including

- `map`
- `reduce`
- `scan`
- `tabulate`, implemented as a C++ constructor

all of which operate on arbitrary functions of arbitrary types, as well as

- `transform`, an in-place `map`
- `get` and `set`, which load and modify (respectively) the value at an index

These operations are all computationally expensive while being well-suited to
parallelization. Operations like `map`, `tabulate`, and `transform` are
completely data parallel. However since they take in arbitrary functions, it's
possible for the workers to diverge in the amount of work done per input, so we
put in extra effort to balance the load across workers to equalize these
divergences. We took a similar approach with `reduce` and `scan`, though these
functions have a larger proportion of serial work load (i.e., they have
logarithmic span).


## Approach

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

To our dismay, a very simple port of our `paren_match` and `mandelbrot`
implementations to Thrust achieved much better results than our cluster-wide
implementation.

<!--
  TODO

  Jake, create graphs for comparing the Thrust speedups vs Lambda++ speedups
  once you get back online.
  -->


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
[ghc-speedup]: {{ "/img/ghc-speedup.png" | prepend: site.baseurl }}
[ghc-speedup-wb]: {{ "/img/ghc-speedup-wb.png" | prepend: site.baseurl }}
[latedays-speedup-mandelbrot]: {{ "/img/latedays-speedup-mandelbrot.png" | prepend: site.baseurl }}
[latedays-speedup-paren]: {{ "/img/latedays-speedup-paren.png" | prepend: site.baseurl }}
