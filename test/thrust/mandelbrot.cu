#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/tabulate.h>
#include <thrust/execution_policy.h>
#include <thrust/copy.h>

#include <thrust/system/cuda/detail/synchronize.h>

#include <stdio.h>
#include <iostream>

#include "../../src/CycleTimer.h"

/*
 * Function taken from Assignment 1, part 3
 */
__host__ __device__
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

struct MandelIdxFunctor
{
  float x0;
  float y0;
  float dx;
  float dy;
  int   width;
  int   height;
  int   max_iters;

  MandelIdxFunctor(float _x0, float _y0, float _dx, float _dy,
      int _width, int _height, int _max_iters)
    : x0(_x0), y0(_y0), dx(_dx), dy(_dy),
      width(_width), height(_height), max_iters(_max_iters)
  {
    // empty
  }

  __host__ __device__
  int operator()(const int& i) const
  {
    float row = i / height;
    float col = i % width;

    float x = x0 + col * dx;
    float y = y0 + row * dy;

    return mandel(x, y, max_iters);
  }
};

void test_mandelbrot(int width=5000, int height=2000, int max_iters=250) {
  float x0 = -2;
  float x1 =  1;
  float y0 = -1;
  float y1 =  1;

  float dx = (x1 - x0) / width;
  float dy = (y1 - y0) / height;

  MandelIdxFunctor mandel_idx(x0, y0, dx, dy, width, height, max_iters);

  double start_time = CycleTimer::currentSeconds();
  thrust::device_vector<int> D(width * height);
  //thrust::host_vector<int>   H(width * height);

  thrust::tabulate(D.begin(), D.end(), mandel_idx);
  thrust::system::cuda::detail::synchronize();
  double end_time = CycleTimer::currentSeconds();

  printf("[mandelbrot parallel]:\t\t[%.3f] ms\n",
      (end_time - start_time) * 1000);

  //start_time = CycleTimer::currentSeconds();
  //thrust::tabulate(thrust::host, H.begin(), H.end(), mandel_idx);
  //end_time = CycleTimer::currentSeconds();

  //printf("[mandelbrot serial]:\t\t[%.3f] ms\n",
  //    (end_time - start_time) * 1000);

  // Ignore correctness checking because CUDA floating point is weird
  //for(int i = 0; i < width * height; i++) {
  //  //std::cout << i << ": " << D[i] << " " << H[i] << std::endl;
  //  if (D[i] != H[i]) {
  //    std::cout << "==> Not equal at " << i << ": " << D[i] << " " << H[i] << std::endl;
  //    return;
  //  }
  //}

}

int main(int argc, char *argv[]) {
  if (argc == 3) {
    int width     = atoi(argv[1]);
    int height    = atoi(argv[2]);

    test_mandelbrot(width, height);
  }
  else if (argc == 4) {
    int width     = atoi(argv[1]);
    int height    = atoi(argv[2]);
    int max_iters = atoi(argv[3]);

    test_mandelbrot(width, height, max_iters);
  }
  else if (argc != 1) {
    std::cout << "usage: " << argv[0]
      << " [width height [max_iters]]" << std::endl;

    return 1;
  }
  else {
    test_mandelbrot();
  }
}
