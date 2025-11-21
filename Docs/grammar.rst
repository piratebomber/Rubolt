=======
Grammar
=======

This document describes the formal grammar of the Rubolt programming language, including syntax rules, parsing techniques, and grammar testing utilities.

.. contents:: Table of Contents
   :local:
   :depth: 3

Overview
========

The Rubolt grammar is designed to be:

* **Unambiguous**: Clear parsing rules with no ambiguity
* **Extensible**: Easy to add new language features
* **Error-tolerant**: Graceful handling of syntax errors
* **Performance-oriented**: Efficient parsing with minimal backtracking

The grammar is implemented as a recursive descent parser with operator precedence parsing for expressions.

Lexical Grammar
===============

Tokens
------

The lexical analyzer recognizes the following token types:

**Literals:**

.. code-block:: text

   NUMBER      ::= INTEGER | FLOAT
   INTEGER     ::= DECIMAL | HEXADECIMAL | BINARY | OCTAL
   DECIMAL     ::= [0-9]+ ('_' [0-9]+)*
   HEXADECIMAL ::= '0x' [0-9a-fA-F]+ ('_' [0-9a-fA-F]+)*
   BINARY      ::= '0b' [01]+ ('_' [01]+)*
   OCTAL       ::= '0o' [0-7]+ ('_' [0-7]+)*
   FLOAT       ::= [0-9]+ '.' [0-9]+ ([eE] [+-]? [0-9]+)?
   
   STRING      ::= '"' STRING_CHAR* '"' | "'" STRING_CHAR* "'"
   STRING_CHAR ::= [^"\\] | ESCAPE_SEQUENCE
   ESCAPE_SEQUENCE ::= '\\' [nrtbf\\'"0] | '\\u' HEX_DIGIT{4}
   
   BOOLEAN     ::= 'true' | 'false'
   NULL        ::= 'null'

**Identifiers:**

.. code-block:: text

   IDENTIFIER  ::= [a-zA-Z_] [a-zA-Z0-9_]*

**Keywords:**

.. code-block:: text

   and       as        async     await     break     case      catch
   class     const     continue  def       else      enum      false
   finally   for       function  if        import    in        let
   match     null      or        return    super     this      throw
   true      try       type      void      while     yield

**Operators:**

.. code-block:: text

   // Arithmetic
   +  -  *  /  %  **
   
   // Assignment
   =  +=  -=  *=  /=  %=
   
   // Comparison
   ==  !=  <  <=  >  >=
   
   // Logical
   &&  ||  !
   
   // Bitwise
   &  |  ^  ~  <<  >>
   
   // Other
   .  ,  ;  :  ?  =>  ->  ...

**Delimiters:**

.. code-block:: text

   (  )  [  ]  {  }

**Comments:**

.. code-block:: text

   LINE_COMMENT  ::= '//' [^\n]* '\n'
   LINE_COMMENT  ::= '#' [^\n]* '\n'
   BLOCK_COMMENT ::= '/*' (. | '\n')* '*/'

Syntactic Grammar
=================

Program Structure
-----------------

.. code-block:: ebnf

   program         ::= statement*
   
   statement       ::= declaration
                    |  expression_statement
                    |  control_statement
                    |  import_statement
   
   declaration     ::= function_declaration
                    |  class_declaration
                    |  variable_declaration
                    |  type_declaration
                    |  enum_declaration

Declarations
------------

**Function Declaration:**

.. code-block:: ebnf

   function_declaration ::= attribute* 'def' IDENTIFIER type_parameters?
                           '(' parameter_list? ')' ('->' type)? block_statement
   
   type_parameters     ::= '<' type_parameter (',' type_parameter)* '>'
   type_parameter      ::= IDENTIFIER (':' type_constraint)?
   type_constraint     ::= type ('+' type)*
   
   parameter_list      ::= parameter (',' parameter)*
   parameter           ::= IDENTIFIER ':' type ('=' expression)?
                        |  '...' IDENTIFIER ':' type

**Class Declaration:**

.. code-block:: ebnf

   class_declaration   ::= attribute* 'class' IDENTIFIER type_parameters?
                          ('extends' type)? ('implements' type_list)?
                          '{' class_member* '}'
   
   class_member        ::= field_declaration
                        |  method_declaration
                        |  constructor_declaration
   
   field_declaration   ::= visibility? IDENTIFIER ':' type ('=' expression)? ';'
   method_declaration  ::= visibility? function_declaration
   constructor_declaration ::= visibility? 'def' 'init' '(' parameter_list? ')' block_statement
   
   visibility          ::= 'private' | 'protected' | 'public'

**Variable Declaration:**

.. code-block:: ebnf

   variable_declaration ::= ('let' | 'const') IDENTIFIER (':' type)? '=' expression ';'

**Type Declaration:**

.. code-block:: ebnf

   type_declaration    ::= 'type' IDENTIFIER type_parameters? '=' type ';'

**Enum Declaration:**

.. code-block:: ebnf

   enum_declaration    ::= 'enum' IDENTIFIER '{' enum_member (',' enum_member)* '}'
   enum_member         ::= IDENTIFIER ('=' expression)?

Types
-----

.. code-block:: ebnf

   type                ::= primary_type
                        |  union_type
                        |  function_type
                        |  generic_type
   
   primary_type        ::= 'number' | 'string' | 'bool' | 'void' | 'any' | 'null'
                        |  IDENTIFIER
                        |  array_type
                        |  tuple_type
                        |  object_type
   
   union_type          ::= type ('|' type)+
   function_type       ::= 'function' '(' type_list? ')' '->' type
   generic_type        ::= IDENTIFIER '<' type_list '>'
   array_type          ::= type '[' ']'
   tuple_type          ::= '(' type (',' type)* ')'
   object_type         ::= '{' object_type_member (',' object_type_member)* '}'
   
   object_type_member  ::= IDENTIFIER ':' type
                        |  '[' type ']' ':' type
   
   type_list           ::= type (',' type)*

Statements
----------

**Control Statements:**

.. code-block:: ebnf

   control_statement   ::= if_statement
                        |  while_statement
                        |  for_statement
                        |  match_statement
                        |  try_statement
                        |  break_statement
                        |  continue_statement
                        |  return_statement
                        |  throw_statement
   
   if_statement        ::= 'if' '(' expression ')' statement ('else' statement)?
   
   while_statement     ::= 'while' '(' expression ')' statement
   
   for_statement       ::= 'for' '(' for_init 'in' expression ')' statement
   for_init            ::= IDENTIFIER | '(' IDENTIFIER (',' IDENTIFIER)* ')'
   
   match_statement     ::= 'match' expression '{' match_case* '}'
   match_case          ::= pattern (guard_clause)? '=>' (expression | statement) ';'
   
   try_statement       ::= 'try' block_statement catch_clause* finally_clause?
   catch_clause        ::= 'catch' '(' IDENTIFIER ':' type IDENTIFIER ')' block_statement
   finally_clause      ::= 'finally' block_statement
   
   break_statement     ::= 'break' ';'
   continue_statement  ::= 'continue' ';'
   return_statement    ::= 'return' expression? ';'
   throw_statement     ::= 'throw' expression ';'

**Block Statement:**

.. code-block:: ebnf

   block_statement     ::= '{' statement* '}'
   expression_statement ::= expression ';'

**Import Statement:**

.. code-block:: ebnf

   import_statement    ::= 'import' import_specifier ';'
   import_specifier    ::= STRING ('as' IDENTIFIER)?
                        |  '{' import_list '}' 'from' STRING
                        |  IDENTIFIER ('as' IDENTIFIER)?
   
   import_list         ::= import_item (',' import_item)*
   import_item         ::= IDENTIFIER ('as' IDENTIFIER)?

Expressions
-----------

**Expression Hierarchy (by precedence, highest to lowest):**

.. code-block:: ebnf

   expression          ::= assignment_expression
   
   assignment_expression ::= conditional_expression
                          |  conditional_expression assignment_operator assignment_expression
   
   assignment_operator ::= '=' | '+=' | '-=' | '*=' | '/=' | '%='
   
   conditional_expression ::= logical_or_expression
                           |  logical_or_expression '?' expression ':' conditional_expression
   
   logical_or_expression ::= logical_and_expression
                          |  logical_or_expression '||' logical_and_expression
   
   logical_and_expression ::= equality_expression
                           |  logical_and_expression '&&' equality_expression
   
   equality_expression ::= relational_expression
                        |  equality_expression ('==' | '!=') relational_expression
   
   relational_expression ::= additive_expression
                          |  relational_expression ('<' | '<=' | '>' | '>=') additive_expression
   
   additive_expression ::= multiplicative_expression
                        |  additive_expression ('+' | '-') multiplicative_expression
   
   multiplicative_expression ::= exponential_expression
                              |  multiplicative_expression ('*' | '/' | '%') exponential_expression
   
   exponential_expression ::= unary_expression
                           |  unary_expression '**' exponential_expression
   
   unary_expression    ::= postfix_expression
                        |  ('!' | '-' | '+' | '~') unary_expression
   
   postfix_expression  ::= primary_expression
                        |  postfix_expression '[' expression ']'
                        |  postfix_expression '.' IDENTIFIER
                        |  postfix_expression '(' argument_list? ')'
   
   primary_expression  ::= IDENTIFIER
                        |  literal
                        |  '(' expression ')'
                        |  array_literal
                        |  object_literal
                        |  function_literal
                        |  'this'
                        |  'super'

**Literals:**

.. code-block:: ebnf

   literal             ::= NUMBER | STRING | BOOLEAN | NULL
   
   array_literal       ::= '[' expression_list? ']'
   
   object_literal      ::= '{' object_property_list? '}'
   object_property_list ::= object_property (',' object_property)*
   object_property     ::= (IDENTIFIER | STRING) ':' expression
                        |  '...' expression
   
   function_literal    ::= '(' parameter_list? ')' ('->' type)? '=>' (expression | block_statement)
   
   expression_list     ::= expression (',' expression)*
   argument_list       ::= argument (',' argument)*
   argument            ::= expression | IDENTIFIER '=' expression

Patterns
--------

.. code-block:: ebnf

   pattern             ::= literal_pattern
                        |  identifier_pattern
                        |  wildcard_pattern
                        |  tuple_pattern
                        |  array_pattern
                        |  object_pattern
                        |  type_pattern
   
   literal_pattern     ::= NUMBER | STRING | BOOLEAN | NULL
   
   identifier_pattern  ::= IDENTIFIER (':' type)?
   
   wildcard_pattern    ::= '_'
   
   tuple_pattern       ::= '(' pattern (',' pattern)* ')'
   
   array_pattern       ::= '[' array_pattern_element* ']'
   array_pattern_element ::= pattern | '...' IDENTIFIER
   
   object_pattern      ::= '{' object_pattern_element (',' object_pattern_element)* '}'
   object_pattern_element ::= IDENTIFIER (':' pattern)?
                           |  IDENTIFIER ':' IDENTIFIER
                           |  '...' IDENTIFIER
   
   type_pattern        ::= type
   
   guard_clause        ::= 'if' expression

Attributes
----------

.. code-block:: ebnf

   attribute           ::= '@' IDENTIFIER ('(' argument_list? ')')?

Grammar Implementation
======================

Parser Structure
----------------

The Rubolt parser is implemented as a recursive descent parser with the following key components:

**Lexer (``src/lexer.c``):**

.. code-block:: c

   typedef struct Lexer {
       const char *source;
       const char *current;
       size_t line;
       size_t column;
       Token current_token;
       Token peek_token;
   } Lexer;

   // Main tokenization function
   Token next_token(Lexer *lexer);
   
   // Token type checking
   bool match_token(Lexer *lexer, TokenType type);
   bool check_token(Lexer *lexer, TokenType type);

**Parser (``src/parser.c``):**

.. code-block:: c

   typedef struct Parser {
       Lexer *lexer;
       bool had_error;
       bool panic_mode;
   } Parser;

   // Parsing functions for each grammar rule
   Expr *parse_expression(Parser *parser);
   Stmt *parse_statement(Parser *parser);
   Decl *parse_declaration(Parser *parser);

**AST Nodes (``src/ast.h``):**

.. code-block:: c

   typedef enum ExprType {
       EXPR_LITERAL,
       EXPR_IDENTIFIER,
       EXPR_BINARY,
       EXPR_UNARY,
       EXPR_CALL,
       EXPR_MEMBER,
       EXPR_INDEX,
       // ... more expression types
   } ExprType;

   typedef struct Expr {
       ExprType type;
       union {
           LiteralExpr literal;
           IdentifierExpr identifier;
           BinaryExpr binary;
           UnaryExpr unary;
           CallExpr call;
           // ... more expression variants
       } as;
   } Expr;

Error Recovery
--------------

The parser implements sophisticated error recovery:

.. code-block:: c

   // Synchronization points for error recovery
   static TokenType sync_tokens[] = {
       TOKEN_CLASS, TOKEN_DEF, TOKEN_LET, TOKEN_CONST,
       TOKEN_IF, TOKEN_WHILE, TOKEN_FOR, TOKEN_RETURN,
       TOKEN_EOF
   };

   void synchronize(Parser *parser) {
       parser->panic_mode = false;
       
       while (parser->lexer->current_token.type != TOKEN_EOF) {
           if (parser->lexer->current_token.type == TOKEN_SEMICOLON) {
               advance(parser);
               return;
           }
           
           for (size_t i = 0; i < ARRAY_SIZE(sync_tokens); i++) {
               if (parser->lexer->current_token.type == sync_tokens[i]) {
                   return;
               }
           }
           
           advance(parser);
       }
   }

Operator Precedence
-------------------

Expression parsing uses precedence climbing:

.. code-block:: c

   typedef enum Precedence {
       PREC_NONE,
       PREC_ASSIGNMENT,    // =
       PREC_CONDITIONAL,   // ?:
       PREC_OR,           // ||
       PREC_AND,          // &&
       PREC_EQUALITY,     // == !=
       PREC_COMPARISON,   // < > <= >=
       PREC_TERM,         // + -
       PREC_FACTOR,       // * / %
       PREC_EXPONENT,     // **
       PREC_UNARY,        // ! - +
       PREC_CALL,         // . () []
       PREC_PRIMARY
   } Precedence;

   Expr *parse_precedence(Parser *parser, Precedence precedence) {
       // Parse prefix expression
       Expr *left = parse_prefix(parser);
       
       // Parse infix expressions with higher precedence
       while (precedence <= get_precedence(parser->lexer->current_token.type)) {
           left = parse_infix(parser, left);
       }
       
       return left;
   }

Grammar Testing
===============

Test Files
----------

The ``Grammar/tests/`` directory contains test files for various language constructs:

**Expression Tests (``expressions.rbo``):**

.. code-block:: rubolt

   // Arithmetic expressions
   let a = 1 + 2 * 3;
   let b = (4 + 5) * 6;
   let c = 2 ** 3 ** 2;  // Right associative
   
   // Logical expressions
   let d = true && false || true;
   let e = !false && (true || false);
   
   // Comparison expressions
   let f = 1 < 2 <= 3;
   let g = "hello" == "world" != "foo";

**Statement Tests (``statements.rbo``):**

.. code-block:: rubolt

   // Control flow
   if (condition) {
       do_something();
   } else if (other_condition) {
       do_other_thing();
   } else {
       do_default();
   }
   
   // Loops
   for (i in 0..10) {
       print(i);
   }
   
   while (running) {
       process_events();
   }
   
   // Pattern matching
   match value {
       0 => "zero";
       x if x > 0 => "positive";
       _ => "negative";
   }

**Token Tests (``tokens.rbo``):**

.. code-block:: rubolt

   // Number literals
   let decimal = 123_456;
   let hex = 0xFF_AA_BB;
   let binary = 0b1010_1111;
   let octal = 0o755;
   let float = 3.14159;
   let scientific = 1.23e-4;
   
   // String literals
   let simple = "hello";
   let escaped = "line1\nline2\ttab";
   let unicode = "Greek: \u03B1\u03B2\u03B3";
   
   // Identifiers
   let valid_identifier = 42;
   let _private_var = "secret";
   let CamelCase = "style";

Grammar Utilities
=================

Token Dump Utility
-------------------

The ``Grammar/CUtils/token_dump.c`` utility helps analyze tokenization:

.. code-block:: c

   // Build and run token dumper
   // Windows:
   // cl /nologo /I src Grammar\CUtils\token_dump.c src\lexer.c /Fe:token_dump.exe
   // token_dump.exe examples\hello.rbo
   
   // Unix/Linux/macOS:
   // gcc -I src Grammar/CUtils/token_dump.c src/lexer.c -o token_dump
   // ./token_dump examples/hello.rbo

   int main(int argc, char *argv[]) {
       if (argc != 2) {
           fprintf(stderr, "Usage: %s <file.rbo>\n", argv[0]);
           return 1;
       }
       
       char *source = read_file(argv[1]);
       if (!source) {
           fprintf(stderr, "Could not read file: %s\n", argv[1]);
           return 1;
       }
       
       Lexer lexer;
       lexer_init(&lexer, source);
       
       Token token;
       do {
           token = next_token(&lexer);
           print_token(&token);
       } while (token.type != TOKEN_EOF);
       
       free(source);
       return 0;
   }

**Example Output:**

.. code-block:: text

   TOKEN_DEF        "def"           line 1, col 1
   TOKEN_IDENTIFIER "hello"        line 1, col 5
   TOKEN_LEFT_PAREN "("            line 1, col 10
   TOKEN_RIGHT_PAREN ")"           line 1, col 11
   TOKEN_ARROW      "->"           line 1, col 13
   TOKEN_IDENTIFIER "void"         line 1, col 16
   TOKEN_LEFT_BRACE "{"            line 1, col 21
   TOKEN_IDENTIFIER "print"        line 2, col 5
   TOKEN_LEFT_PAREN "("            line 2, col 10
   TOKEN_STRING     "Hello!"       line 2, col 11
   TOKEN_RIGHT_PAREN ")"           line 2, col 19
   TOKEN_SEMICOLON  ";"            line 2, col 20
   TOKEN_RIGHT_BRACE "}"           line 3, col 1
   TOKEN_EOF        ""             line 3, col 2

AST Dump Utility
-----------------

For debugging parser output:

.. code-block:: c

   void print_ast(Expr *expr, int indent) {
       for (int i = 0; i < indent; i++) printf("  ");
       
       switch (expr->type) {
           case EXPR_LITERAL:
               printf("Literal: %s\n", expr->as.literal.value);
               break;
               
           case EXPR_BINARY:
               printf("Binary: %s\n", expr->as.binary.operator);
               print_ast(expr->as.binary.left, indent + 1);
               print_ast(expr->as.binary.right, indent + 1);
               break;
               
           case EXPR_CALL:
               printf("Call:\n");
               print_ast(expr->as.call.callee, indent + 1);
               for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                   print_ast(expr->as.call.args[i], indent + 1);
               }
               break;
               
           // ... handle other expression types
       }
   }

Grammar Extensions
==================

Adding New Syntax
------------------

To add new language features:

1. **Update Lexer**: Add new token types in ``src/lexer.h`` and recognition in ``src/lexer.c``

2. **Update Parser**: Add parsing rules in ``src/parser.c``

3. **Update AST**: Add new node types in ``src/ast.h``

4. **Update Interpreter**: Add execution logic in ``src/interpreter.c``

5. **Add Tests**: Create test cases in ``Grammar/tests/``

**Example: Adding a new operator**

.. code-block:: c

   // 1. Add token type
   typedef enum TokenType {
       // ... existing tokens ...
       TOKEN_POWER,  // ** operator
   } TokenType;

   // 2. Update lexer
   Token next_token(Lexer *lexer) {
       // ... existing tokenization ...
       if (*lexer->current == '*' && *(lexer->current + 1) == '*') {
           lexer->current += 2;
           return make_token(lexer, TOKEN_POWER);
       }
   }

   // 3. Update parser precedence
   Precedence get_precedence(TokenType type) {
       switch (type) {
           // ... existing cases ...
           case TOKEN_POWER: return PREC_EXPONENT;
       }
   }

   // 4. Add parsing rule
   Expr *parse_infix(Parser *parser, Expr *left) {
       switch (parser->lexer->current_token.type) {
           // ... existing cases ...
           case TOKEN_POWER: {
               advance(parser);
               Expr *right = parse_precedence(parser, PREC_EXPONENT + 1); // Right associative
               return make_binary_expr("**", left, right);
           }
       }
   }

Grammar Validation
==================

Automated Testing
-----------------

The grammar is validated through automated tests:

.. code-block:: bash

   # Run grammar tests
   python tools/test_grammar.py Grammar/tests/

   # Test specific constructs
   python tools/test_grammar.py Grammar/tests/expressions.rbo

**Test Script Example:**

.. code-block:: python

   import subprocess
   import sys
   import os

   def test_file(filename):
       """Test parsing of a single file."""
       try:
           result = subprocess.run([
               './src/rubolt', '--parse-only', filename
           ], capture_output=True, text=True)
           
           if result.returncode == 0:
               print(f"✓ {filename}: Parse successful")
               return True
           else:
               print(f"✗ {filename}: Parse failed")
               print(f"  Error: {result.stderr}")
               return False
       except Exception as e:
           print(f"✗ {filename}: Exception: {e}")
           return False

   def main():
       test_dir = sys.argv[1] if len(sys.argv) > 1 else "Grammar/tests"
       
       passed = 0
       failed = 0
       
       for filename in os.listdir(test_dir):
           if filename.endswith('.rbo'):
               filepath = os.path.join(test_dir, filename)
               if test_file(filepath):
                   passed += 1
               else:
                   failed += 1
       
       print(f"\nResults: {passed} passed, {failed} failed")
       return 0 if failed == 0 else 1

   if __name__ == "__main__":
       sys.exit(main())

Performance Testing
-------------------

Grammar performance is tested with large files:

.. code-block:: python

   import time
   import subprocess

   def benchmark_parser(filename, iterations=10):
       """Benchmark parser performance."""
       times = []
       
       for _ in range(iterations):
           start = time.time()
           subprocess.run([
               './src/rubolt', '--parse-only', filename
           ], capture_output=True)
           end = time.time()
           times.append(end - start)
       
       avg_time = sum(times) / len(times)
       print(f"Average parse time for {filename}: {avg_time:.4f}s")
       return avg_time

Error Message Quality
---------------------

The parser provides helpful error messages:

.. code-block:: c

   void parser_error(Parser *parser, const char *message) {
       Token *token = &parser->lexer->current_token;
       
       fprintf(stderr, "[line %zu] Error", token->line);
       
       if (token->type == TOKEN_EOF) {
           fprintf(stderr, " at end");
       } else if (token->type == TOKEN_ERROR) {
           // Nothing
       } else {
           fprintf(stderr, " at '%.*s'", (int)token->length, token->start);
       }
       
       fprintf(stderr, ": %s\n", message);
       parser->had_error = true;
   }

**Example Error Messages:**

.. code-block:: text

   [line 5] Error at 'def': Expected ';' after variable declaration.
   [line 12] Error at ')': Expected expression.
   [line 18] Error at end: Expected '}' after block.

Future Grammar Extensions
=========================

Planned Features
----------------

The grammar is designed to accommodate future language features:

* **Async/await syntax**: ``async def``, ``await expr``
* **Destructuring assignment**: ``let [a, b] = tuple``
* **Spread operator**: ``...args`` in function calls
* **Optional chaining**: ``obj?.prop?.method?()``
* **Null coalescing**: ``value ?? default``
* **Pipeline operator**: ``value |> transform |> process``

Grammar Documentation
---------------------

The formal grammar specification is maintained in EBNF format and automatically generates:

* **Railroad diagrams** for visual grammar representation
* **Parser generator input** for alternative parser implementations
* **Language reference documentation** with syntax examples

This comprehensive grammar system ensures that Rubolt has a well-defined, extensible syntax that can evolve with the language while maintaining backward compatibility and providing excellent developer experience through clear error messages and robust parsing.