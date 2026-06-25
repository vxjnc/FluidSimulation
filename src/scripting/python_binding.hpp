#pragma once
#ifdef SCRIPTING_AVAILABLE

#include <array>
#include <concepts>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>

#include <pfr.hpp>

#include "src/scripting/python_api.hpp"

namespace py_util {
    template <typename T>
    concept BindableType = requires {
        requires std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, double> ||
                     std::is_same_v<T, const char*> || std::is_same_v<T, PyObject*> ||
                     std::is_same_v<T, bool>;
    };

    template <typename T> inline PyObject* PyNamedTupleClass = nullptr;

    inline PyObject* to_py(PyObject* obj) { return obj; }
    inline PyObject* to_py(std::integral auto val) { return py::long_from_size_t(static_cast<size_t>(val)); }
    inline PyObject* to_py(std::floating_point auto val) {
        return py::float_from_double(static_cast<double>(val));
    }
    inline PyObject* to_py(bool value) { return py::bool_from_long(value ? 1 : 0); }

    template <typename T>
        requires requires(T c) {
            c.begin();
            c.end();
        }
    inline PyObject* to_py(const T& container) {
        PyObject* py_list = py::list_new(container.size());
        size_t idx = 0;
        for (const auto& item : container) {
            py::list_set_item(py_list, idx++, to_py(item));
        }
        return py_list;
    }

    template <typename T>
        requires std::is_aggregate_v<T> && (!requires(T c) { c.begin(); })
    inline PyObject* to_py(const T& obj) {
        if (PyNamedTupleClass<T> != nullptr) {
            PyObject* args = py::tuple_new(pfr::tuple_size_v<T>);
            pfr::for_each_field(
                obj, [&](const auto& field, auto idx) { py::tuple_set_item(args, idx, to_py(field)); });
            PyObject* instance = py::object_call(PyNamedTupleClass<T>, args, nullptr);
            py::decref(args);
            return instance;
        }
        py::incref(py::none);
        return py::none;
    }

    template <typename T> inline T from_py(PyObject* obj);

    template <> inline int from_py<int>(PyObject* obj) { return static_cast<int>(py::long_as_long(obj)); }
    template <> inline float from_py<float>(PyObject* obj) {
        return static_cast<float>(py::float_as_double(obj));
    }
    template <> inline double from_py<double>(PyObject* obj) { return py::float_as_double(obj); }
    template <> inline const char* from_py<const char*>(PyObject* obj) { return py::unicode_as_utf8(obj); }
    template <> inline PyObject* from_py<PyObject*>(PyObject* obj) { return obj; }
    template <> inline bool from_py<bool>(PyObject* obj) { return py::object_istrue(obj) > 0; }

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

        template <typename Tuple, size_t... Is>
        void unpack_fastcall_args(PyObject* const* args, Tuple& tuple, std::index_sequence<Is...>) {
            ((std::get<Is>(tuple) = from_py<std::tuple_element_t<Is, Tuple>>(args[Is])), ...);
        }
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

            constexpr size_t nargs_expected = std::tuple_size_v<ArgsTuple>;

            constexpr int method_flags = (nargs_expected == 0) ? METH_NOARGS : METH_FASTCALL;

            auto c_func = [](PyObject*, PyObject* const* args, Py_ssize_t nargs) -> PyObject* {
                if constexpr (nargs_expected == 0) {
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
                    if (nargs != static_cast<Py_ssize_t>(nargs_expected)) {
                        py::Err_SetString(py::type_error, "Invalid number of arguments");
                        return nullptr;
                    }

                    ArgsTuple unpacked_args;
                    impl::unpack_fastcall_args(args, unpacked_args,
                                               std::make_index_sequence<nargs_expected>{});

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

            bound = BoundMethod{(PyCFunction)(void*)+c_func, method_flags};
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

    template <typename T>
        requires std::is_aggregate_v<T>
    void register_namedtuple(PyObject* module, const char* python_class_name) {
        std::string fields;
        pfr::for_each_field(T{}, [&](const auto&, auto idx) {
            if (!fields.empty()) {
                fields += " ";
            }
            fields += pfr::get_name<idx, T>();
        });

        PyObject* collections = py::import_import_module("collections");
        PyObject* namedtuple_fn = py::object_get_attr_string(collections, "namedtuple");
        PyObject* args = py::tuple_pack(2, py::unicode_from_string(python_class_name),
                                        py::unicode_from_string(fields.c_str()));

        PyNamedTupleClass<T> = py::object_call(namedtuple_fn, args, nullptr);

        py::decref(args);
        py::decref(namedtuple_fn);
        py::decref(collections);

        py::module_add_object(module, python_class_name, PyNamedTupleClass<T>);
    }
}

#endif
