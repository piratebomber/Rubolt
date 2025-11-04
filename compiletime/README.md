# Compiletime Directory

Contains compile-time macros and preprocessor directives for the Rubolt interpreter.

## Structure

- `macros/` — C preprocessor macros for token replacement and code generation

## Purpose

These macros are processed by the C preprocessor before compilation, enabling:
- Token find/replace through alias maps
- Conditional compilation
- Small code generators (via X-macros)