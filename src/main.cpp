#include <iostream>
#include <limits>
#include <cassert>

// test implementations
#include "paren_match.h"
// #include "mandelbrot.h"
#include "parallel_sequence.h"
#include "serial_sequence.h"
#include "cluster.h"

#include "CycleTimer.h"

int main (int argc, char **argv) {
  Cluster::init(&argc, &argv);

  // paren test
  //test_paren_match(100000000);

  // mandelbrot test
  //test_mandelbrot();

  {
    auto identity = [](int i) { return i; };

    ParallelSequence<int> *s = new ParallelSequence<int>(identity, 100);
    s->set(5, 2);
    cout << s->get(99) << endl;
    cout << s->get(5) << endl;
  }

  //auto combiner = [](int x, int y) { return x + y; };
  //s->scan(combiner, 0);
  //s->print();
  //delete s;

  //ParallelSequence<int> s2(identity, 1000);
  //s2.print();

  Cluster::close();
  return 0;
}
