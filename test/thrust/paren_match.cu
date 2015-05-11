#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/scan.h>
#include <thrust/tabulate.h>

#include <thrust/system/cuda/detail/synchronize.h>

#include <limits>
#include <stdio.h>
#include <iostream>

#include "../../src/CycleTimer.h"

struct Generator1 {
  __host__ __device__
  int operator()(const int& i) const {
    return i % 2 == 0 ? 1 : -1;
  }
};
struct Generator2 {
  const int n;
  Generator2(int length) : n(length) {}

  __host__ __device__
  int operator()(const int& i) const {
    return i < n / 2 ? 1 : -1;
  }
};
struct Generator3 {
  __host__ __device__
  int operator()(const int& i) const {
    return i % 2 == 0 ? -1 : 1;
  }
};
struct Generator4 {
  int n;
  Generator4(int length) : n(length) {}

  __host__ __device__
  int operator()(const int& i) const {
    return i < n / 2 ? -1 : 1;
  }
};

bool paren_match(thrust::device_vector<int> &D) {
  double start_time = CycleTimer::currentSeconds();
  thrust::exclusive_scan(D.begin(), D.end(), D.begin());

  int int_max = std::numeric_limits<int>::max();
  int min = thrust::reduce(D.begin(), D.end(), int_max, thrust::minimum<int>());

  bool result = D[D.size() - 1] == 0 && min >= 0;
  double end_time = CycleTimer::currentSeconds();

  printf("[paren_match parallel]:\t\t[%.3f] ms\n",
      (end_time - start_time) * 1000);

  return result;
}

void test_paren_match(int length=10000) {
  thrust::device_vector<int> D1(length);
  thrust::device_vector<int> D2(length);
  thrust::device_vector<int> D3(length);
  thrust::device_vector<int> D4(length);

  thrust::tabulate(D1.begin(), D1.end(), Generator1());
  thrust::tabulate(D2.begin(), D2.end(), Generator2(length));
  thrust::tabulate(D3.begin(), D3.end(), Generator3());
  thrust::tabulate(D4.begin(), D4.end(), Generator4(length));

  paren_match(D1);
  thrust::system::cuda::detail::synchronize();

  paren_match(D2);
  thrust::system::cuda::detail::synchronize();

  paren_match(D3);
  thrust::system::cuda::detail::synchronize();

  paren_match(D4);
  thrust::system::cuda::detail::synchronize();

}

int main(int argc, char *argv[]) {
  if (argc == 2) {
    int length = atoi(argv[1]);

    test_paren_match(length);
  }
  else if (argc != 1) {
    std::cout << "usage: " << argv[0] << " [length]" << std::endl;

    return 1;
  }
  else {
    test_paren_match();
  }
}
