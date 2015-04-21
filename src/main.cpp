#include <iostream>
#include <limits>

#include "parallel_sequence.h"

#include "CycleTimer.h"

int main (int argc, char **argv) {
  Cluster::init(&argc, &argv);

  test_paren_match(1000);

  Cluster::close();
  return 0;
}
