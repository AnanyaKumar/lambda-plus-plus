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

  // paren test
  // test_paren_match(100000000);

  // mandelbrot test
  // test_mandelbrot();

  // {
    // auto identity = [](int i) { return i; };
    // UberSequence<int> *s = new UberSequence<int>(identity, 100);
    // auto combiner = [](int x, int y) { return x + y; };
    // s->scan(combiner, 0);
    // s->print();
    // cout << s->get(8) << endl;
    // int x = s->reduce(combiner, 0);
    // cout << x << endl;
  //   // s->set(5, 2);
  //   // cout << s->get(99) << endl;
  //   // cout << s->get(5) << endl;
    // delete s;
  // }

  //auto combiner = [](int x, int y) { return x + y; };
  //s->scan(combiner, 0);
  //s->print();
  //delete s;

  //ParallelSequence<int> s2(identity, 1000);
  //s2.print();

  // int iam, np;

  // #pragma omp parallel
  // {
  //   np = omp_get_num_threads();
  //   iam = omp_get_thread_num();
  //   printf("Hello from thread %d out of %d, in node %d out of %d\n",
  //          iam, np, Cluster::procId, Cluster::procs);
  // }

  // Toy example test
  double start_time = CycleTimer::currentSeconds();

  // Hard work function
  auto work = [](int i) {
    int x = 0;
    for (int j = 0; j < 20000; j++) {
      x += i * j;
    }
    return x;
  };

  // Addition combiner
  // auto combiner = [](int x, int y) { return x + y; };

  // Work Test
  UberSequence<int> *s3 = new UberSequence<int>(work, 500000);
  // s3->print();

  // Reduce Test
  // int x;
  // for (int i = 0; i < 5000; i++) {
  //   x += s3->reduce(combiner, 0);
  // }

  double total_time_parallel = CycleTimer::currentSeconds() - start_time;

  // cout << "Answer: " << x << endl;
  cout << total_time_parallel << endl;

  Cluster::close();
  return 0;
}
