#ifndef _SERIAL_SEQUENCE_H_
#define _SERIAL_SEQUENCE_H_

#include <functional>

#include "sequence.h"

using namespace std;


template<typename T>
class SerialSequence : public Sequence<T>
{
public:
  SerialSequence (T *array, int n) {
    size = n;
    data = new T[size];
    for (int i = 0; i < size; i++) {
      data[i] = array[i];
    }
  }

  SerialSequence (function<T(int)> generator, int n) {
    size = n;
    data = new T[size];
    for (int i = 0; i < size; i++) {
      data[i] = generator(i);
    }
  }

  ~SerialSequence() {
    delete [] data;
  }

  void transform (function<T(T)> mapper) {
    for (int i = 0; i < size; i++) {
      data[i] = mapper(data[i]);
    }
  }

  template<typename S>
  SerialSequence<S> map(function<S(T)> mapper) {
    auto tabulateFunction = [&](int index) {
      return mapper(this.get(index));
    };
    SerialSequence<S> seq = new SerialSequence<S>(tabulateFunction, size);
    return S;
  }

  T reduce (function<T(T,T)> combiner, T init) {
    T value = init;
    for (int i = 0; i < size; i++) {
      value = combiner(value, data[i]);
    }
    return value;
  }

  void scan (function<T(T,T)> combiner, T init) {
    if (size > 0) {
      data[0] = combiner(init, data[0]);
    }
    for (int i = 1; i < size; i++) {
      data[i] = combiner(data[i-1], data[i]);
    }
  }

  T get (int index) {
    return data[index];
  }

  void set (int index, T value) {
    data[index] = value;
  }

  void print () {
    cout << "Node " << (Cluster::procId+1) << "/" << Cluster::procs << ":" << endl;
    int i;
    for (i = 0; i < size; i++) {
      cout << data[i] <<  " ";
      if (i % 10 == 9) cout << endl;
    }
    if (i % 10 != 9) cout << endl;
  }
};

#endif
