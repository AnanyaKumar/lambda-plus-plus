#include <iostream>
#include <limits>
#include <cassert>
#include <omp.h>

// test implementations
#include "paren_match.h"
#include "mandelbrot.h"
#include "parallel_sequence.h"
#include "serial_sequence.h"
#include "cluster.h"

#include "CycleTimer.h"

int main (int argc, char **argv) {
  Cluster::init(&argc, &argv);

  // paren test
  // test_paren_match(100000000);

  // mandelbrot test
  // test_mandelbrot();

  // {
  //   auto identity = [](int i) { return i; };

  //   ParallelSequence<int> *s = new ParallelSequence<int>(identity, 100);
  //   s->set(5, 2);
  //   cout << s->get(99) << endl;
  //   cout << s->get(5) << endl;
  // }

  //auto combiner = [](int x, int y) { return x + y; };
  //s->scan(combiner, 0);
  //s->print();
  //delete s;

  //ParallelSequence<int> s2(identity, 1000);
  //s2.print();

  // Toy example test
  double start_time = CycleTimer::currentSeconds();

  // Hard work function
  auto work = [](int i) {
    int x = 0;
    for (int j = 0; j < 200; j++) {
      x += i * j;
    }
    return x;
  };
  
  // Addition combiner
  auto combiner = [](int x, int y) { return x + y; };

  // Work Test
  ParallelSequence<int> *s3 = new ParallelSequence<int>(work, 100000000);

  // Reduce Test
  // int x;
  // for (int i = 0; i < 5000; i++) {
  //   x += s3->reduce(combiner, 0);
  // }

  double total_time_parallel = CycleTimer::currentSeconds() - start_time;

  // cout << "Answer: " << x << endl;
  cout << total_time_parallel << endl;
  delete s3;

  Cluster::close();
  return 0;
}
