#include "seq.h"

template<typename A>
void
Sequence<A>::tabulate(std::function<A(unsigned int)> f, unsigned int n) {
  for(unsigned int i = 0; i < n; ++i) {
    elems_.push_back(f(i));
  }
}

template<typename A>
template<typename B>
void
Sequence<A>::map(std::function<B(A)> f) {
  for(A &elem : elems_) {
    elem = f(elem);
  }
}

template<typename A>
std::ostream& operator<<(std::ostream& os, const Sequence<A>& seq) {
  for(A x : seq.elems_) {
    std::cout << x << ", ";
  }

  return os;
}

// instantiations
template class Sequence<int>;

template void Sequence<int>::map(std::function<int(int)>);

template std::ostream& operator<<(std::ostream& os, const Sequence<int>& seq);
