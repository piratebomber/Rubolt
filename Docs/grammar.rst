# Grammar

This section describes grammar testing and utilities.

## Files

- ``Grammar/tests/`` — Small programs exercising syntax
- ``Grammar/CUtils/`` — C tools for lexing/parsing traces

## Token Dump Utility

Build the token dumper and run it on a file:

.. code-block:: bash

   # Windows (from repo root)
   cl /nologo /I src Grammar\CUtils\token_dump.c src\lexer.c /Fe:token_dump.exe
   token_dump.exe examples\hello.rbo

   # MSYS/MinGW or Linux
   gcc -I src Grammar/CUtils/token_dump.c src/lexer.c -o token_dump
   ./token_dump examples/hello.rbo
