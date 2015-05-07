---
layout: post
title: "Project Checkpoint"
short-title: "Checkpoint"
date: "Mon Apr 20 23:56:28 EDT 2015"
---


After two weeks of work, we have made a good deal of progress on our final
project. We have reached the point where we have a working reference, sequential
implementation as well as a preliminary parallel implementation. If you recall,
our project is

> {{ site.description }}

So far, we have managed to implement the following higher order functions in
parallel using OpenMPI:

- `tabulate`
- `transform` (an in-place `map`)
- `reduce`
- `scan`
- `get` (read from `nth`)
- `set` (write to `nth`)
- `length`

Using these primitives, we have implemented the Mandelbrot fractal generation
algorithm and the parallel algorithm for determining if strings of parentheses
are well matched.


## Goals & Deliverables Revisited

We'll break down our goals and deliverables into the same three categories as
before: implementing a library of functions on sequences that take functions as
arguments, using these library functions to write performant code, and finally
analyzing how well we did compared with other parallel implementations.

### Implement Functional Sequence Library

The first thing we wanted to deliver was a functional sequence library, and we
are well on our way to doing this. We mentioned that we hoped to be able to add
more library functions, to make our Sequence library into something practically
useful in the wild, including operations like `rev`, `append`, `flatten`, etc.

While we still might add these, I suspect it will be largely determined by the
difficulty of implementing our selected algorithms without them. That is, if we
find it hard to implement a particular algorithm without one of these primitives
or we find ourselves re-inventing the wheel, we will push that functionality
into the library.

### Write Performant Code Using Lambda++

We said that the algorithms we wanted to implement using our Sequence library
included:

- a sorting function
- at least one of
  - a graph algorithm, like BFS or Bellman-Ford, or
  - an image manipulation algorithm, like Mandelbrot fractal image generation

We have implemented the Mandelbrot fractal image generation algorithm. We have
yet to implement a sorting function, but this will be coming soon.

We still might implement a graph algorithm to see how our Sequence library fares
on non-linear data structures that have more complicated data dependencies. This
will help us with one of the features we hoped to achieve which was to fine tune
our algorithms based on the costs of network communication between any two
nodes, dynamically assigning work in such a way as to minimize this overhead. A
graph algorithm would be a better measuring stick towards this goal.

### Analyze Performance

We have yet to doing any real analysis of our performance, as our
implementations are still in the preliminary stage. As we optimize our
algorithms in the next few steps (see the schedule below), this will change.


## Schedule Revisited

Below we have reproduced our original schedule, with annotations in _italics_
describing what we have accomplished.

- __Sunday, April 12__
    - Implement a parallel, minimum working product of the Sequence interface
        - This will allow us to begin implementing algorithms that use the library.
    - _This milestone is complete. We have a working, preliminary implementation
       of the Sequence library in C++ using OpenMPI._
- __Sunday, April 19__
    - Implement basic versions of the example applications we'd like to
      experiment on.
        - Progress on optimizing our code can't happen until we have a basic
          working implementation for both the library and application.
    - _This milestone is also complete. We have a working implementation of the
      Mandelbrot fractal generation algorithm as well as an algorithm that
      simulates determining whether sequences of parenthesis are well-matched._
- __Sunday, April 26__
  - Refactor the implementations of both the library and application code, using
    improvements and tweaks in one to bolster performance in the other.
  - Jake will be responsible for implementing a graph algorithm or a sorting
    algorithm to meet one of the features we planned to achieve.
  - Ananya will focus on making the library code performant with respect to the
    OpenMPI implementation of the Sequence library functions (above having an
    initial, working implementation).
- __Sunday, March 3__
  - Write scripts to analyze the network topology of our system and minimize
    communication costs.
  - Ananya and Jake will investigate and implement a way to optimize the amount
    of work distribution that needs to happen.
  - Jake will adjust the reference algorithms in accordance with the new
    changes.
  - Ananya will use the optimized work distributions to refine the way the
    Sequence library functions are implemented.
- __Sunday, March 10__
  - Collect and analyze the results of our application code with respect to
    popular alternative implementations.
  - Ananya and Jake will profile the code and compile a number of metrics
    comparing performance of the parallel sequence library compared to our
    reference sequential implementation of the sequence library.

As you can see, we're almost exactly where we anticipated we'd be at this point
in the project. We doing plan on stopping now! We're super excited and looking
forward to seeing this project through to completion.
