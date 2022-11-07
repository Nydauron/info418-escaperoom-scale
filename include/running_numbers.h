#pragma once

#include <stddef.h>

template<class T>
class RunningNumbers {
    private:
        T *avgs;
        size_t curr_idx;
        size_t valid_elements;
        size_t max_elements;
    public:
        RunningNumbers(size_t max_elements) : curr_idx(0), valid_elements(0), max_elements(max_elements) {
            avgs = new T[this->max_elements];

            for (size_t i = 0; i < this->max_elements; i++) {
                avgs[i] = 0;
            }
        }

        ~RunningNumbers() {
            delete[] avgs;
        }

        void add(T elem) {
            avgs[curr_idx] = elem;
            if (valid_elements < max_elements) valid_elements++;
            curr_idx = (curr_idx + 1) % max_elements;
        }

        T get_avg() {
            T total = 0;
            for (size_t i = 0; i < valid_elements; i++) {
                total += avgs[i];
            }

            return total / valid_elements;
        }

        bool is_within_tolerance(float expected, float tolerance) {
            float avg = get_avg();
            return abs(avg - expected) <= tolerance;
        }
};


class Test {
    private:
        int hi;
    public:
        Test() {
            hi = 1337;
        }
};

