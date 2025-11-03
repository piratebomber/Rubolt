# Language Reference

This reference defines the Rubolt syntax and semantics.

## Lexical Structure

- Comments: ``//``, ``#``, and ``/* */``
- Strings: single and double quoted
- Numbers: integer and floating-point

## Types

- ``number``, ``string``, ``bool``, ``void``, ``any``, ``null``

## Control Flow

.. code-block:: rubolt

   if (cond) { ... } else { ... }
   while (cond) { ... }
   for (let i: number = 0; i < 10; i = i + 1) { ... }

## Functions

.. code-block:: rubolt

   def add(a: number, b: number) -> number {
       return a + b;
   }
