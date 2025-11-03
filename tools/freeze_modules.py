#!/usr/bin/env python3
import os, subprocess, sys, shutil

ROOT = os.path.dirname(os.path.dirname(__file__))
FROZEN_C = os.path.join(ROOT, 'src', 'frozen.c')

MODULES = [
    ('StdLib/prelude.rbo', 'prelude'),
]

def escape(s: str) -> str:
    return s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')

def main():
    lines = [
        '#include "frozen.h"',
        '#include <string.h>',
        'typedef struct { const char* name; const char* data; } FrozenEntry;',
    ]
    entries = []
    for path, alias in MODULES:
        full = os.path.join(ROOT, path)
        if not os.path.exists(full):
            continue
        with open(full, 'r', encoding='utf-8') as f:
            data = f.read()
        varname = alias
        lines.append(f'static const char* {varname} = "{escape(data)}";')
        entries.append((path, varname))
        entries.append((alias, varname))
    lines.append('static FrozenEntry g_frozen[] = {')
    for name, var in entries:
        lines.append(f'    {{"{name}", {var}}},')
    lines.append('};')
    lines.append('const char* frozen_get(const char* name) {')
    lines.append('    for (size_t i = 0; i < sizeof(g_frozen)/sizeof(g_frozen[0]); ++i) {')
    lines.append('        if (strcmp(g_frozen[i].name, name) == 0) return g_frozen[i].data;')
    lines.append('    }')
    lines.append('    return NULL;')
    lines.append('}')

    with open(FROZEN_C, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))
    print(f'Wrote {FROZEN_C}')

if __name__ == '__main__':
    main()
