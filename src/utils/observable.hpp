#pragma once

#include <sigslot/signal.hpp>

template <typename T> struct Observable {
    sigslot::signal<T> onChange;

    Observable(T initial) : value(initial) {}
    Observable() = default;

    Observable& operator=(const T& val) {
        if (val != value) {
            value = val;
            onChange(value);
        }
        return *this;
    }

    operator T() const { return value; }

    T* ptr() { return &value; }
    const T* ptr() const { return &value; }

private:
    T value{};
};