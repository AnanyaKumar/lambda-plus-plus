---
layout: landing
---

# Designed for Monkeys

Most programmers don't know the intricacies of parallel programming. That's why we designed a
libary even a Monkey could use.

Example code for an algorithm that computes whether a sequence of parentheses is well matched:

```cpp
/*
 * Test if a sequence of 1's or -1's is "matched", treating
 * 1's as open parens and -1's as close parens.
 * (String parsing omitted)
 */
bool paren_match(Sequence<int> &seq) {
  // C++11 lambda functions
  auto plus = [](int a, int b) {
    return a + b;
  };
  auto min = [](int a, int b) {
    return a < b ? a : b;
  };

  // Compute a prefix sum
  seq.scan(plus, 0);

  int int_max = std::numeric_limits<int>::max();

  // A well matched sequence has a net sum of 0 and
  // never drops under zero
  return seq.get(seq.length() - 1) == 0 &&
         seq.reduce(min, int_max) >= 0;
}
```

A more compact version using functions (plus, min_elem) supplied by our libraries:

```cpp
bool paren_match(Sequence<int> &seq) {
  seq.scan(Lambda::plus, 0);
  return seq.get(seq.length() - 1) == 0 &&
         seq.reduce(Lambda:min_elem, INT_MAX) >= 0;
}
```


# Lightning Fast Cluster Computing

Lambda++ uses the Message Passing Interface (MPI) to minimize communication accross nodes in the
cluster and to explicitly handle memory allocation. This means that your programs aren't blocked by
high network latencies or by memory addresses jumping back and forth accross nodes.

Within a node, Lambda++ uses OpenMP to optimize for high speed shared memory architectures. The
library uses external devices (like the 40-core Xeon Phi) when possible to squeeze every ounce of
power from the cluster.

Lambda++ uses randomized algorithms to balance load accross the cluster. This ensures that one
processor doesn't hold up the compute task, but avoids the overhead of synchronization in
traditional work stealing approaches.

To sweeten the deal, Lambda++ adapts to those pesky architectures where some compute resources dwarf 
others in performance! We do this by quickly profiling available compute resources before the work 
is distributed.


# Versatile: from Bracket Matching to DP



# Customizable for Power Users



# The Power of Lambda

Those coming from the realm of Haskell or ML will surely know the beauty of
functions. Being able to describe an algorithm in terms of the operations it's
composed of, building on top of a toolkit of "higher-order" functions like
`map`, `reduce`, `filter`, and `scan` is incredibly powerful. On top of their
value to a programmer's efficiency, these primitives lend themselves nicely to
parallelization.

With Lambda++, we bring these benefits to C++. Using OpemMPI on top
of C++11, we have been able to implement a library of these functions, called
the `Sequence` library, which borrows heavily in it's API design from the class
15-210. It aims to be familiar and useful, enabling the user to achieve great
performance while retaining expressiveness.

As an example of what it looks like in practice, here's a snippet from a simple
algorithm that computes whether a sequence of parentheses is well matched:

```cpp
/*
 * Test if a sequence of 1's or -1's is "matched", treating
 * 1's as open parens and -1's as close parens.
 * (String parsing omitted)
 */
bool paren_match(Sequence<int> &seq) {
  // C++11 lambda functions
  auto plus = [](int a, int b) {
    return a + b;
  };
  auto min = [](int a, int b) {
    return a < b ? a : b;
  };

  // Compute a prefix sum
  seq.scan(plus, 0);

  int int_max = std::numeric_limits<int>::max();

  // A well matched sequence has a net sum of 0 and
  // never drops under zero
  return seq.get(seq.length() - 1) == 0 &&
         seq.reduce(min, int_max) >= 0;
}
```


# Some (very preliminary) Results

We know that a library for parallel program lives or dies by it's performance,
so we've taken care to optimize for many types of usage patterns.

[![][speedup]][speedup]


# Where to Go Now

This has been just a brief introduction to our project. For more information,
there are a number of other resources you can check out:

- [Our original project proposal][proposal]
- [Our project's final writeup][writeup]
- [The source and documentation on GitHub][lpp]

We're pretty excited about our project, and we hope we've manged to share that
with you.

This site is still a work in progress! We'll be revising it with more up-to-date
information soon, so be sure to check back!

[speedup]: {{ "/img/speedup.png" | prepend: site.baseurl }}

[proposal]: project-proposal/
[writeup]: final-writeup/
[lpp]: https://github.com/ananyakumar/lambda-plus-plus







