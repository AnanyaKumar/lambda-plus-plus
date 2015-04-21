#include <iostream>
#include <limits>

#include "sequence.h"

// test implementations
#include "paren_match.h"
#include "mandelbrot.h"

#include "CycleTimer.h"

int main (int argc, char **argv) {
  Cluster::init(&argc, &argv);

  // paren test
  test_paren_match(1000);

  // mandelbrot test
  void test_mandelbrot();

  Cluster::close();
  return 0;
}
