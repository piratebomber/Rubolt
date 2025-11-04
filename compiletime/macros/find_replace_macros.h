/*
 * find_replace_macros.h - Compile-time Find/Replace Helpers
 * Macros and X-macro patterns to generate replacement code at compile-time.
 */

#ifndef FIND_REPLACE_MACROS_H
#define FIND_REPLACE_MACROS_H

/* Basic preprocessor utilities */
#define RB_PP_CAT(a,b) RB_PP_CAT_I(a,b)
#define RB_PP_CAT_I(a,b) a##b
#define RB_PP_CAT3(a,b,c) RB_PP_CAT(RB_PP_CAT(a,b),c)
#define RB_PP_STRINGIZE(x) RB_PP_STRINGIZE_I(x)
#define RB_PP_STRINGIZE_I(x) #x
#define RB_PP_EXPAND(x) x
#define RB_PP_EMPTY()
#define RB_PP_DEFER(id) id RB_PP_EMPTY()

/* X-macro application helper
   Define a table like:
       #define RB_REPLACE_TABLE(X) \
           X(print, rb_print) \
           X(len, rb_len)

   Then use generators below to make data/funcs.
*/

/* Generate string pairs array initializer */
#define RB_GEN_STR_PAIRS(name) \
    static const struct { const char* from; const char* to; } name[] = {\
        RB_REPLACE_TABLE(RB_GEN_STR_PAIR_ELEM) \
    };

#define RB_GEN_STR_PAIR_ELEM(from, to) { RB_PP_STRINGIZE(from), RB_PP_STRINGIZE(to) },

/* Generate a simple C function that replaces an exact symbol string.
   NOTE: This is generated at compile-time, executed at runtime. */
#define RB_GEN_REPLACER(func_name) \
    static const char* func_name(const char* s) { \
        RB_GEN_STR_PAIRS(_rb_pairs_) \
        for (unsigned i = 0; i < sizeof(_rb_pairs_) / sizeof(_rb_pairs_[0]); ++i) { \
            if (strcmp(s, _rb_pairs_[i].from) == 0) return _rb_pairs_[i].to; \
        } \
        return s; \
    }


#endif /* FIND_REPLACE_MACROS_H */
