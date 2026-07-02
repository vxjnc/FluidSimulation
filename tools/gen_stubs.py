import json
import re
import subprocess
import sys

import clang.cindex as cl


def get_compile_args(compile_commands_path: str, filename: str) -> list[str]:
    with open(compile_commands_path) as f:
        db = json.load(f)
    for entry in db:
        if entry['file'].endswith(filename):
            args = entry['command'].split()[1:]
            args = [
                a
                for a in args
                if not a.endswith(('.cpp', '.o')) and a not in ('-c', '-o')
            ]
            return args
    return []


def collect_def_params(tu, src_path: str, fluidsim_line: int):
    results = {}

    def visit(cursor, enclosing_lambdas=0, parent_call=None):
        if (
            cursor.kind == cl.CursorKind.LAMBDA_EXPR
            and cursor.location.file
            and cursor.location.file.name.endswith(src_path)
        ):
            if (
                not enclosing_lambdas
                and cursor.location.line > fluidsim_line
                and (parent_call is None or parent_call.spelling != 'def_prop_rw')
            ):
                params = [
                    c.spelling
                    for c in cursor.get_children()
                    if c.kind == cl.CursorKind.PARM_DECL and c.spelling
                ]
                if params and params[0] == 'p':
                    params = params[1:]
                if params and parent_call:
                    results[cursor.location.line] = {
                        'params': params,
                        'end': (
                            parent_call.extent.end.line,
                            parent_call.extent.end.column,
                        ),
                    }
            for child in cursor.get_children():
                visit(child, enclosing_lambdas + 1, parent_call)
            return

        new_parent = cursor if cursor.kind == cl.CursorKind.CALL_EXPR else parent_call
        for child in cursor.get_children():
            visit(child, enclosing_lambdas, new_parent)

    visit(tu.cursor)
    return results


def generate(text, results):
    offsets = [0]
    for line in text.splitlines(keepends=True):
        offsets.append(offsets[-1] + len(line))

    insertions = []
    for info in results.values():
        end_line, end_col = info['end']
        pos = offsets[end_line - 1] + end_col - 2
        context = text[max(0, pos - 200) : pos]
        existing = set(re.findall(r'nb::arg\("(\w+)"\)', context))
        missing = [p for p in info['params'] if p not in existing]
        if missing:
            insertions.append((pos, ', '.join(f'nb::arg("{p}")' for p in missing)))

    for pos, args in sorted(insertions, reverse=True):
        text = text[:pos] + f', {args}' + text[pos:]

    return text


def process(compile_commands_path: str, src_path: str, out_path: str):
    args = get_compile_args(compile_commands_path, src_path)
    resource_dir = (
        subprocess.check_output(['clang-22', '-print-resource-dir']).decode().strip()
    )
    args += [f'-resource-dir={resource_dir}']

    tu = cl.Index.create().parse(src_path, args=args)
    for diag in tu.diagnostics:
        if diag.severity >= cl.Diagnostic.Error:
            print(f"Parse error: {diag.spelling}", file=sys.stderr)

    with open(src_path) as f:
        text = f.read()

    fluidsim_line = next(
        i + 1 for i, l in enumerate(text.splitlines()) if 'NB_MODULE(fluidsim' in l
    )
    results = collect_def_params(tu, src_path, fluidsim_line)

    with open(out_path, 'w') as f:
        f.write(generate(text, results))


if __name__ == '__main__':
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <compile_commands.json> <src.cpp> <out.cpp>")
        sys.exit(1)
    process(sys.argv[1], sys.argv[2], sys.argv[3])
