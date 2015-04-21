#include <stdio.h>
#include <limits>
#include <algorithm>

#include "mandelbrot.h"

#include "serial_sequence.h"
#include "parellel_sequence.h"

#include "CycleTimer.h"

/*
 * Function taken from Assignment 1, part 3
 */
int mandel(float c_re, float c_im, int count) {
  float z_re = c_re, z_im = c_im;
  int i;
  for (i = 0; i < count; ++i) {

    if (z_re * z_re + z_im * z_im > 4.f)
      break;

    float new_re = z_re*z_re - z_im*z_im;
    float new_im = 2.f * z_re * z_im;
    z_re = c_re + new_re;
    z_im = c_im + new_im;
  }

  return i;
}

// Do not call this function, except from within mandelbrot_serial or
// mandelbrot_parallel. You should probably use one of these functions in your
// code.
Sequence<int> mandelbrot_helper(float x0, float y0,
                                float x1, float y1,
                                int width, int height,
                                int max_iters, bool parallelize) {
  float dx = (x1 - x0) / width;
  float dy = (y1 - y0) / height;

  // Given an index i from [0, width * height - 1], compute the
  // mandelbrot set for the pixel at x = i % width, y = i / height
  auto mandel_idx = [=](int i) {
    float row = i / height;
    float col = i % width;

    float x = x0 + col * dx;
    float y = y0 + row * dy;

    return mandel(x, y, max_iters);
  };

  if (parallelize) {
    return ParallelSequence(mandel_idx, width * height);
  }
  else {
    return SerialSequence(mandel_idx, width * height);
  }
}

/*
 * Compute mandelbrot fractal in serial and in parallel
 *
 * We've created extra API functions wrapping a boolean parameter (like
 * parallelize) for maintainability down the road.
 */
Sequence<int> mandelbrot_serial(float x0, float y0,
    float x1, float y1, int width, int height, int max_iters) {
  bool parallelize = false;

  return mandelbrot_helper(x0, y0, x1, y1, width, height,
      max_iters, parallelize);
}
Sequence<int> mandelbrot_parallel(float x0, float y0,
    float x1, float y1, int width, int height, int max_iters) {
  bool parallelize = true;

  return mandelbrot_helper(x0, y0, x1, y1, width, height,
      max_iters, parallelize);
}

/*
 * Sequence length to test on, then creates some sequences, runs the tests on
 * those sequences, and reports results
 */
void test_mandelbrot() {
  float x0 = -2;
  float x1 = 1;
  float y0 = -1;
  float y1 = 1;

  int width = 1200;
  int height = 800;
  int max_iters = 256;

  // Run the serial implementation. Report the minimum time of three
  // runs for robust timing.
  double min_serial = 1e30;
  for (int i = 0; i < 3; ++i) {
    double start_time = CycleTimer::currentSeconds();
    Sequence<int> seq = mandelbrot_serial(x0, y0, x1, y1, width,
        height, max_iters);
    double end_time = CycleTimer::currentSeconds();
    min_serial = std::min(min_serial, end_time - start_time);
  }

  printf("[mandelbrot serial]:\t\t[%.3f] ms\n", min_serial * 1000);

  // Run the parallel implementation. Report the minimum time of three
  // runs for robust timing.
  double min_parallel = 1e30;
  for (int i = 0; i < 3; ++i) {
    double start_time = CycleTimer::currentSeconds();
    Sequence<int> seq = mandelbrot_parallel(x0, y0, x1, y1, width,
        height, max_iters);
    double end_time = CycleTimer::currentSeconds();
    min_parallel = std::min(min_parallel, end_time - start_time);
  }

  printf("[mandelbrot parallel]:\t\t[%.3f] ms\n", min_parallel * 1000);

  printf("\t\t\t\t(%.2fx speedup)\n", min_serial/min_parallel);
}


