#include <iostream>
#include <limits>
#include "parallel_sequence.h"
#include "CycleTimer.h"

int main (int argc, char **argv) {
  Cluster::init(&argc, &argv);


  // Miscellaneous tests
  auto square = [](int a) -> double {
    return a * a; 
  };
  auto identity = [](int index) {
    return index;
  };
  Sequence<int> s2(identity, 100);
  // s2.transform(square);
  s2.print();
  cout << "Get: " << s2.get(5) << endl;
  // int s2sum = s2.reduce(summer, 0);
  // cout << s2sum << endl;

  // Paren test
  // test_paren_match(1000);

  Cluster::close();
  return 0;
}
