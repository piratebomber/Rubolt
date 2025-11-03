#!/usr/bin/env python3
import os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(__file__))
TOKEN_DUMPER = os.path.join(ROOT, 'Grammar', 'CUtils', 'token_dump' + ('.exe' if os.name=='nt' else ''))

if not os.path.exists(TOKEN_DUMPER):
    # Try to build
    if os.name == 'nt':
        subprocess.call(['gcc', '-I', os.path.join(ROOT,'src'), os.path.join(ROOT,'Grammar','CUtils','token_dump.c'), os.path.join(ROOT,'src','lexer.c'), '-o', TOKEN_DUMPER])
    else:
        subprocess.call(['gcc', '-I', os.path.join(ROOT,'src'), os.path.join(ROOT,'Grammar','CUtils','token_dump.c'), os.path.join(ROOT,'src','lexer.c'), '-o', TOKEN_DUMPER])

if len(sys.argv) < 2:
    print('Usage: tokenize_file.py <file.rbo>')
    sys.exit(1)

subprocess.call([TOKEN_DUMPER, sys.argv[1]])
