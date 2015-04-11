#include "seq.h"

int main() {
  Sequence<int> seq;

  std::function<int(unsigned int)> identity = [](unsigned int x) -> int {
    return (int) x;
  };

  std::function<int(int)> addOne = [](int x) -> int {
    return x + 1;
  };

  seq.tabulate(identity, 10);

  std::cout << seq << std::endl;

  seq.map(addOne);

  std::cout << seq << std::endl;

  return 0;
}
