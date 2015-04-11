#ifndef _SEQ_H_
#define _SEQ_H_

#include <vector>
#include <iostream>
#include <functional>

template<typename A>
class Sequence {
  private:
    std::vector<A> elems_;

  public:
    void tabulate(std::function<A(unsigned int)> f, unsigned int n);

    template<typename B>
    void map(std::function<B(A)> f);

    template<typename U>
    friend std::ostream& operator<<(std::ostream& os, const Sequence<U>& seq);
};

#endif
