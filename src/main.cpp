#include <iostream>
#include <limits>

#include <mpi.h>

#include "sequence.h"

// test implementations
#include "paren_match.h"
#include "mandelbrot.h"

#include "CycleTimer.h"

int main (int argc, char **argv) {
  MPI_Init(&argc, &argv);

  // paren test
  test_paren_match(1000);

  // mandelbrot test
  void test_mandelbrot();

  MPI_Finalize();
  return 0;
}
