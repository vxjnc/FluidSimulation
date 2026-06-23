#pragma once

#ifdef SCRIPTING_AVAILABLE

#include <concepts>
#include <span>
#include <tuple>
#include <type_traits>

#include "src/scripting/python_api.hpp"

namespace py_util {
    template <typename T>
    concept BindableType = requires {
        requires std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, double> ||
                     std::is_same_v<T, const char*> || std::is_same_v<T, PyObject*> ||
                     std::is_same_v<T, bool>;
    };

    template <BindableType T> struct PyFormat;
    template <> struct PyFormat<bool> {
        static constexpr char value = 'p';
    };
    template <> struct PyFormat<int> {
        static constexpr char value = 'i';
    };
    template <> struct PyFormat<float> {
        static constexpr char value = 'f';
    };
    template <> struct PyFormat<double> {
        static constexpr char value = 'd';
    };
    template <> struct PyFormat<const char*> {
        static constexpr char value = 's';
    };
    template <> struct PyFormat<PyObject*> {
        static constexpr char value = 'O';
    };

    template <BindableType... Args> constexpr auto make_format_string() {
        return std::array<char, sizeof...(Args) + 1>{PyFormat<Args>::value..., '\0'};
    }

    inline PyObject* to_py(PyObject* obj) { return obj; }

    inline PyObject* to_py(std::integral auto val) { return py::long_from_size_t(static_cast<size_t>(val)); }

    inline PyObject* to_py(std::floating_point auto val) {
        return py::float_from_double(static_cast<double>(val));
    }

    namespace impl {
        template <typename T> struct function_traits : function_traits<decltype(&T::operator())> {};
        template <typename R, typename... Args> struct function_traits<R (*)(Args...)> {
            using ReturnType = R;
            using ArgsTuple = std::tuple<std::decay_t<Args>...>;
        };
        template <typename C, typename R, typename... Args> struct function_traits<R (C::*)(Args...) const> {
            using ReturnType = R;
            using ArgsTuple = std::tuple<std::decay_t<Args>...>;
        };
    }

    struct BoundMethod {
        PyCFunction func;
        int flags;
    };

    struct Method {
        const char* name;
        BoundMethod bound;
        const char* doc = nullptr;

        template <typename F>
        constexpr Method(const char* method_name, F&& func, const char* method_doc = nullptr)
            : name(method_name), doc(method_doc) {
            static std::decay_t<F> storage_func = std::forward<F>(func);
            using Traits = impl::function_traits<std::decay_t<F>>;
            using ArgsTuple = typename Traits::ArgsTuple;

            constexpr int method_flags = (std::tuple_size_v<ArgsTuple> == 0) ? METH_NOARGS : METH_VARARGS;

            auto c_func = [](PyObject*, PyObject* args) -> PyObject* {
                if constexpr (std::tuple_size_v<ArgsTuple> == 0) {
                    if constexpr (std::is_void_v<typename Traits::ReturnType>) {
                        storage_func();
                        py::incref(py::none);
                        return py::none;
                    }
                    else {
                        return to_py(storage_func());
                    }
                }
                else {
                    ArgsTuple unpacked_args;
                    constexpr auto fmt = std::apply(
                        [](auto... dummy) { return make_format_string<std::decay_t<decltype(dummy)>...>(); },
                        ArgsTuple{});

                    bool parse_success = std::apply(
                        [&](auto&... tuple_args) {
                            return py::ArgParseTuple(args, fmt.data(), &tuple_args...);
                        },
                        unpacked_args);

                    if (!parse_success) {
                        return nullptr;
                    }

                    if constexpr (std::is_void_v<typename Traits::ReturnType>) {
                        std::apply(storage_func, unpacked_args);
                        py::incref(py::none);
                        return py::none;
                    }
                    else {
                        return to_py(std::apply(storage_func, unpacked_args));
                    }
                }
            };

            bound = BoundMethod{(PyCFunction) + c_func, method_flags};
        }
    };

    template <size_t N> struct MethodTable {
        std::array<PyMethodDef, N + 1> methods{};

        constexpr MethodTable(std::span<const Method, N> input_methods) {
            for (size_t i = 0; i < N; ++i) {
                methods[i] = PyMethodDef{input_methods[i].name, input_methods[i].bound.func,
                                         input_methods[i].bound.flags, input_methods[i].doc};
            }
        }
        PyMethodDef* data() { return methods.data(); }
        const PyMethodDef* data() const { return methods.data(); }
    };

    template <size_t N> constexpr auto make_table(std::span<const Method, N> input_methods) {
        return MethodTable<N>(input_methods);
    }

    template <size_t N> constexpr auto make_table(const Method (&input_methods)[N]) {
        return make_table(std::span<const Method, N>(input_methods));
    }
}

#endif
