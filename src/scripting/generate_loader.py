import re


def generate_cpp_from_hpp(hpp_path, cpp_path):
    with open(hpp_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # extern type (*name)(args); // original
    fn_pattern = re.compile(
        r'extern\s+([\w\*\[\]\s\(\)\*]+)\s+\(\*([\w_]+)\)\s*\((.*?)\);\s*//\s*([\w_]+)'
    )

    # extern type name; // original
    var_pattern = re.compile(r'extern\s+([\w\*]+)\s+([\w_]+)\s*;\s*//\s*([\w_]+)')

    functions = []
    variables = []

    for line in content.splitlines():
        line = line.strip()
        if not line.startswith('extern'):
            continue

        fn_match = fn_pattern.match(line)
        if fn_match:
            ret_type, cpp_name, args, py_name = fn_match.groups()
            functions.append(
                {
                    'ret_type': ret_type.strip(),
                    'cpp_name': cpp_name.strip(),
                    'args': args.strip(),
                    'py_name': py_name.strip(),
                }
            )
            continue

        var_match = var_pattern.match(line)
        if var_match:
            var_type, cpp_name, py_name = var_match.groups()
            variables.append(
                {
                    'type': var_type.strip(),
                    'cpp_name': cpp_name.strip(),
                    'py_name': py_name.strip(),
                }
            )

    cpp_lines = []
    cpp_lines.append('#ifdef SCRIPTING_AVAILABLE\n')
    cpp_lines.append(f'#include "{hpp_path}"\n')
    cpp_lines.append('#include <dlfcn.h>')
    cpp_lines.append('#include <type_traits>\n')
    cpp_lines.append('namespace py {')

    for fn in functions:
        cpp_lines.append(
            f'    {fn["ret_type"]} (*{fn["cpp_name"]})({fn["args"]}) = nullptr;'
        )
    for var in variables:
        cpp_lines.append(f'    {var["type"]} {var["cpp_name"]} = nullptr;')

    cpp_lines.append('\n    bool resolve_all(void* lib) {')
    cpp_lines.append('        auto r = [lib](const char* name, auto& fn) -> bool {')
    cpp_lines.append(
        '            fn = reinterpret_cast<std::remove_reference_t<decltype(fn)>>(dlsym(lib, name));'
    )
    cpp_lines.append('            return fn != nullptr;')
    cpp_lines.append('        };\n')

    resolve_chain = []

    for var in variables:
        if var['cpp_name'] == 'none':
            resolve_chain.append(
                '[lib]() { none = static_cast<PyObject*>(dlsym(lib, "_Py_NoneStruct")); return none != nullptr; }()'
            )
        elif var['cpp_name'] == 'type_error':
            resolve_chain.append(
                '[lib]() { type_error = *reinterpret_cast<PyObject**>(dlsym(lib, "PyExc_TypeError")); return type_error != nullptr; }()'
            )

    for fn in functions:
        resolve_chain.append(f'r("{fn["py_name"]}", {fn["cpp_name"]})')

    cpp_lines.append(
        '        return ' + ' &&\n               '.join(resolve_chain) + ';'
    )
    cpp_lines.append('    }')
    cpp_lines.append('}')
    cpp_lines.append('\n#endif')

    with open(cpp_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(cpp_lines))


if __name__ == '__main__':
    import sys

    if len(sys.argv) == 3:
        hpp_path = sys.argv[1]
        cpp_path = sys.argv[2]
    else:
        hpp_path = 'python_api.hpp'
        cpp_path = 'python_api.cpp'

    generate_cpp_from_hpp(hpp_path, cpp_path)
