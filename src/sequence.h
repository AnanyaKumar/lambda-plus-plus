#ifndef _SEQUENCE_H_
#define _SEQUENCE_H_

#include <functional>

using namespace std;

/*
 * Abstract Sequence class
 *
 * Implemented by SerialSequence and ParallelSequence
 */
template<typename T>
class Sequence
{
public:
  // Data stored by the current node
  T* data;

  // Common information about the Sequence
  int size;

  virtual void transform(function<T(T)> mapper) = 0;

  template<typename S>
  Sequence<S> *map(function<S(T)> mapper);

  virtual T reduce (function<T(T,T)> combiner, T init) = 0;
  virtual void scan (function<T(T,T)> combiner, T init) = 0;

  virtual T get (int index) = 0;
  virtual void set (int index, T value) = 0;

  int length() {
    return size;
  }

  virtual void print () = 0;
};

#endif
