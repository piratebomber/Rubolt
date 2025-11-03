# Standard Library

Rubolt ships with native and pure Rubolt libraries.

## Native Modules

- ``math``: sqrt, pow, abs, floor, ceil, sin, cos
- ``os``: getcwd, getenv, system
- ``file``: read, write, exists
- ``time``: now, sleep
- ``sys``: version, exit
- ``string``: len, upper, lower, concat
- ``random``: int, float
- ``atomics``: create, inc, get, cas

## Pure Rubolt Helpers

- ``Objects/string.rbo``: upper, lower, len, concat
- ``Objects/number.rbo``: abs, clamp, lerp

Usage:

.. code-block:: rubolt

   import math
   let x: number = math.sqrt(16);
