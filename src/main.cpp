#include <algorithm>
#include <iostream>
#include <limits>
#include <cassert>
#include <omp.h>
#include <utility>

// test implementations
#include "paren_match.h"
#include "mandelbrot.h"
#include "uber_sequence.h"
#include "parallel_sequence.h"
#include "serial_sequence.h"
#include "cluster.h"

#include "CycleTimer.h"

int knapsack (UberSequence< pair<int, int> > *items, int weight) {
  auto intMax = [](int x, int y) { return max(x, y); };
  int money[weight+1];
  money[0] = 0;
  for (int i = 1; i <= weight; i++) {
    money[i] = 0;
    std::function<int(pair<int, int>)> bestUsing = [&](pair<int, int> item) {
      if (i - item.first < 0) return 0;
      return money[i - item.first] + item.second;
    };
    Sequence<int> *bests = items->map(bestUsing);
    money[i] = bests->reduce(intMax, 0);
    delete bests;
  }
  return money[weight];
}

int main (int argc, char **argv) {
  Cluster::init(&argc, &argv);

  // Some useful definitions
  auto identity = [](int i) { return i; };
  auto combiner = [](int x, int y) { return x + y; };
  auto doubleAdder = [](double x, double y) { return x + y; };

  // paren test
  test_paren_match(20000000);

  // mandelbrot test
  // test_mandelbrot();

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

  // Double Sequence test
  // auto toDouble = [](int i) { return (double)i; };
  // UberSequence<double> *s = new UberSequence<double>(toDouble, 100);
  // s->print();
  // std::function<int(double)> toInt = [](double d) -> int {return int(d);};
  // UberSequence<int> *s2 = s->map(toInt);
  // s2->print();

  // Pair Sequence test
  // auto pairer = [](int i) { return make_pair(i, 2.323); };
  // UberSequence< pair<int, double> > *s = new UberSequence< pair<int, double> >(pairer, 100);
  // s->print();
  // std::function<double(pair<int, double>)> extractSecond = [](pair<int, double> p) -> double { return p.second; };
  // UberSequence<double> *s2 = s->map(extractSecond);
  // double x = s2->reduce(doubleAdder, 0.0);
  // cout << x << endl;

  // Knapsack
  // double start_time = CycleTimer::currentSeconds();
  // std::function<pair<int, int>(int)> intToPair = [](int i) { return make_pair(i, i); };
  // int numItems = 50000;
  // UberSequence< pair<int, int> > *items = new UberSequence< pair<int, int> >(intToPair, numItems);
  // int weight = 50000;
  // int answer = knapsack(items, weight);
  // double total_time_parallel = CycleTimer::currentSeconds() - start_time;
  // cout << total_time_parallel << endl;
  // cout << answer << endl;;

  Cluster::close();
  return 0;
}
