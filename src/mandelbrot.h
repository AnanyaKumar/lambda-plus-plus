#ifndef _MANDELBROT_H_
#define _MANDELBROT_H_

#include "sequence.h"

Sequence<int> *mandelbrot_serial(float x0, float y0,
                                float x1, float y1,
                                int width, int height,
                                int max_iters);

Sequence<int> *mandelbrot_parallel(float x0, float y0,
                                  float x1, float y1,
                                  int width, int height,
                                  int max_iters);

void test_mandelbrot();

#endif
