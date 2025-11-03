#!/usr/bin/env python3
import os, random, string, sys

# Simple case generator for Rubolt snippets

def gen_var_decl():
    name = 'v' + ''.join(random.choice(string.ascii_lowercase) for _ in range(5))
    val = random.randint(0, 100)
    return f"let {name}: number = {val};"

def gen_arith():
    a, b = random.randint(1, 10), random.randint(1, 10)
    return f"print({a} + {b});"

def gen_func():
    return (
        "def f(x: number) -> number {\n"
        "    return x * x;\n"
        "}\n"
        "print(f(5));"
    )

def gen_case():
    parts = [gen_var_decl(), gen_arith(), gen_func()]
    return '\n'.join(parts)

if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else 'generated_case.rbo'
    code = gen_case()
    with open(out, 'w', encoding='utf-8') as f:
        f.write(code)
    print(f'Wrote {out}')
