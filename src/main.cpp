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
  auto summer = [](int a, int b) {
    return a + b;
  };

  Sequence<int> s2(identity, 100);
  // s2.print();

  int s2sum = s2.reduce(summer, 1);
  cout << "Sum: " << s2sum << endl;

  s2.scan(summer, 1);
  s2.print();


  // Paren test
  // test_paren_match(1000);

  Cluster::close();
  return 0;
}
