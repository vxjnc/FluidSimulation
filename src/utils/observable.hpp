#pragma once

#include <type_traits>

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

template <typename T> struct is_observable : std::false_type {};
template <typename T> struct is_observable<Observable<T>> : std::true_type {
    using inner_type = T;
};
template <typename T> constexpr bool is_observable_v = is_observable<T>::value;
