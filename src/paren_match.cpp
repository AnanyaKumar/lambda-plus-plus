#include <iostream>
#include <limits>
#include <functional>
#include <vector>

#include "paren_match.h"

#include "serial_sequence.h"
#include "parallel_sequence.h"

#include "CycleTimer.h"

/*
 * Test if a sequence of 1's or -1's is "matched", treating 1's as open parens
 * and -1's as close parens.
 */
bool paren_match(Sequence<int> &seq) {
  auto plus = [](int a, int b) {
    return a + b;
  };

  auto min = [](int a, int b) {
    return a < b ? a : b;
  };

  // in memory scan
  seq.scan(plus, 0);

  int int_max = std::numeric_limits<int>::max();
  // TODO: Need length function
  return seq.get(seq.length() - 1) == 0 && seq.reduce(min, int_max) >= 0;
}

/*
 * Sequence length to test on, then creates some sequences, runs the tests on
 * those sequences, and reports results
 */
void test_paren_match(int n) {
  std::vector<std::function<int(int)>> generators;
  bool expecteds[4];

  // ()()()()()()...
  generators.push_back([](int i) { return i % 2 == 0 ? 1 : -1; });
  expecteds[0]  = true;
  // (((((...)))))
  generators.push_back([=](int i) { return i <= n / 2 ? 1 : -1; });
  expecteds[1]  = true;
  // )()()()()()(...
  generators.push_back([](int i) { return i % 2 == 0 ? -1 : 1; });
  expecteds[2]  = false;
  // )))))...(((((
  generators.push_back([=](int i) { return i <= n / 2 ? -1 : 1; });
  expecteds[3]  = false;


  // ----- Run tests -----
  double start_time, total_time_serial, total_time_parallel;
  std::string result;
  bool rc;
  for (int i = 0; i < 4; i++) {
    // ----- Serial test -----
    start_time = CycleTimer::currentSeconds();
    SerialSequence<int> seq1 = SerialSequence<int>(generators[i], n);
    rc = paren_match(seq1);
    total_time_serial = CycleTimer::currentSeconds() - start_time;

    result = rc == expecteds[i] ? "PASS" : "FAIL";
    std::cout << "[" << result << "] Test " << i << " (parallel): "
      << total_time_serial << std::endl;

    // ----- Parallel test -----
    start_time = CycleTimer::currentSeconds();
    ParallelSequence<int> seq2 = ParallelSequence<int>(generators[i], n);
    rc = paren_match(seq2);
    total_time_parallel = CycleTimer::currentSeconds() - start_time;

    result = rc == expecteds[i] ? "PASS" : "FAIL";
    std::cout << "[" << result << "] Test " << i << " (parallel): "
      << total_time_parallel << std::endl;

    // ----- Speedup -----
    std::cout << "Speedup: "
      << total_time_serial / total_time_parallel << std::endl;
  }
}

void hello() {
  std::cout << "Hello, world!" << std::endl;
}