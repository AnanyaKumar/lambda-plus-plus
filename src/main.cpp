#include <iostream>
#include <functional>
#include "parallel_sequence.h"
using namespace std;

int main (int argc, char **argv) {
  Cluster::init(&argc, &argv);

  int A[] = {1,2,3, 4, 5, 6, 7, 8};

  // Square mapper
  int constant = 2;
  auto square = [=](int a) -> double {
    return constant * a * a; 
  };
  constant *= 2; // Doesn't affect the function square 

  // Sequence<int> s(A, 3);
  // s.transform(square);
  // s.print();

  // Identity mapper
  auto identity = [](int index) {
    return index;
  };

  // Sum combiner
  auto summer = [](int a, int b) {
    return a + b;
  };

  Sequence<int> s2(identity, 100);
  // s2.transform(square);
  s2.print();
  cout << "Get: " << s2.get(5) << endl;
  // int s2sum = s2.reduce(summer, 0);
  // cout << s2sum << endl;

  Cluster::close();
  return 0;
}
