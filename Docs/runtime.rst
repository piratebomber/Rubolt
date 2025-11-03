# Runtime

The Rubolt runtime preloads standard utilities and prepares search paths.

## Features

- Preloads ``StdLib/prelude.rbo`` before your program
- Adds ``StdLib/`` to search paths for libraries
- Hooks for future import resolution

## Usage

The CLI and interpreter use the runtime automatically.

.. code-block:: bash

   src/rubolt examples/complete_demo.rbo

