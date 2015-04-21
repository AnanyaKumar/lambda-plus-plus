#ifndef _SERIAL_SEQUENCE_H_
#define _SERIAL_SEQUENCE_H_

#include <iostream>

#include "sequence.h"

using namespace std;


template<typename T>
class SerialSequence : public Sequence<T>
{
public:
  SerialSequence (T *array, int n) {
    this->size = n;
    this->data = new T[this->size];
    for (int i = 0; i < this->size; i++) {
      this->data[i] = array[i];
    }
  }

  SerialSequence (function<T(int)> generator, int n) {
    this->size = n;
    this->data = new T[this->size];
    for (int i = 0; i < this->size; i++) {
      this->data[i] = generator(i);
    }
  }

  ~SerialSequence() {
    delete [] this->data;
  }

  void transform (function<T(T)> mapper) {
    for (int i = 0; i < this->size; i++) {
      this->data[i] = mapper(this->data[i]);
    }
  }

  template<typename S>
  SerialSequence<S> map(function<S(T)> mapper) {
    auto tabulateFunction = [&](int index) {
      return mapper(this.get(index));
    };
    SerialSequence<S> seq = new SerialSequence<S>(tabulateFunction, this->size);
    return seq;
  }

  T reduce (function<T(T,T)> combiner, T init) {
    T value = init;
    for (int i = 0; i < this->size; i++) {
      value = combiner(value, this->data[i]);
    }
    return value;
  }

  void scan (function<T(T,T)> combiner, T init) {
    if (this->size > 0) {
      this->data[0] = combiner(init, this->data[0]);
    }
    for (int i = 1; i < this->size; i++) {
      this->data[i] = combiner(this->data[i-1], this->data[i]);
    }
  }

  T get (int index) {
    return this->data[index];
  }

  void set (int index, T value) {
    this->data[index] = value;
  }

  void print () {
    int i;
    for (i = 0; i < this->size; i++) {
      std::cout << this->data[i] <<  " ";
      if (i % 10 == 9) std::cout << std::endl;
    }
    if (i % 10 != 9) std::cout << std::endl;
  }
};

#endif
