#include <iostream>
#include <limits>
#include <cassert>
#include <omp.h>

// test implementations
#include "paren_match.h"
#include "mandelbrot.h"
#include "uber_sequence.h"
#include "parallel_sequence.h"
#include "serial_sequence.h"
#include "cluster.h"

#include "CycleTimer.h"

int main (int argc, char **argv) {
  Cluster::init(&argc, &argv);

  // Some useful definitions
  auto identity = [](int i) { return i; };
  auto combiner = [](int x, int y) { return x + y; };

  // paren test
  // test_paren_match(20000000);

  // mandelbrot test
  test_mandelbrot();

  // Simple sequence tests
  // UberSequence<int> *s = new UberSequence<int>(identity, 100);
  // s->scan(combiner, 0);
  // s->print();
  // s->printResponsibilities();
  // cout << s->get(8) << endl;
  // int x = s->reduce(combiner, 0);
  // cout << x << endl;
  // delete s;

  // Basic timed test
  // double start_time = CycleTimer::currentSeconds();
  // auto work = [](int i) {
  //   int x = 0;
  //   for (int j = 0; j < 20000; j++) {
  //     x += i * j + j % 4;
  //   }
  //   return x;
  // };
  // UberSequence<int> *s3 = new UberSequence<int>(work, 50000);
  // double total_time_parallel = CycleTimer::currentSeconds() - start_time;
  // cout << total_time_parallel << endl;
  // delete s3;

  Cluster::close();
  return 0;
}
