" Vim syntax file for Rubolt
" Language: Rubolt
" Maintainer: Rubolt Team
" Latest Revision: 2024

if exists("b:current_syntax")
  finish
endif

" Keywords
syn keyword ruboltKeyword def class enum type import export let const
syn keyword ruboltKeyword if else match case when while for in break continue
syn keyword ruboltKeyword return throw try catch finally async await yield
syn keyword ruboltKeyword and or not null true false this super
syn keyword ruboltKeyword public private protected static abstract

" Types
syn keyword ruboltType number string bool void any null function
syn keyword ruboltType Array List Dict Set Tuple Optional Result Promise

" Built-in functions
syn keyword ruboltBuiltin print len type str int float bool
syn keyword ruboltBuiltin assert panic unreachable
syn keyword ruboltBuiltin min max abs round floor ceil
syn keyword ruboltBuiltin map filter reduce forEach find some every

" Operators
syn match ruboltOperator "\v\+|\-|\*|\/|\%|\*\*"
syn match ruboltOperator "\v\=\=|\!\=|\<|\>|\<\=|\>\="
syn match ruboltOperator "\v\&\&|\|\||\!"
syn match ruboltOperator "\v\&|\||\^|\~|\<\<|\>\>"
syn match ruboltOperator "\v\=|\+\=|\-\=|\*\=|\/\=|\%\="
syn match ruboltOperator "\v\-\>|\=\>|\?\?"

" Numbers
syn match ruboltNumber "\v<\d+>"
syn match ruboltNumber "\v<\d+\.\d+>"
syn match ruboltNumber "\v<\d*\.?\d+([eE][+-]?\d+)?>"
syn match ruboltNumber "\v<0x[0-9a-fA-F]+>"
syn match ruboltNumber "\v<0b[01]+>"
syn match ruboltNumber "\v<0o[0-7]+>"

" Strings
syn region ruboltString start='"' end='"' contains=ruboltEscape
syn region ruboltString start="'" end="'" contains=ruboltEscape
syn match ruboltEscape contained "\\[nrtbf\\\"']"
syn match ruboltEscape contained "\\u[0-9a-fA-F]\{4\}"

" String interpolation
syn region ruboltInterpolation contained start="${" end="}" contains=ALL

" Comments
syn match ruboltComment "//.*$"
syn match ruboltComment "#.*$"
syn region ruboltComment start="/\*" end="\*/"

" Attributes/Decorators
syn match ruboltAttribute "@\w\+"

" Function definitions
syn match ruboltFunction "\v<def\s+\zs\w+\ze\s*\("

" Class definitions
syn match ruboltClass "\v<class\s+\zs\w+\ze"

" Type annotations
syn match ruboltTypeAnnotation "\v:\s*\zs\w+\ze"

" Generics
syn region ruboltGeneric start="<" end=">" contains=ruboltType,ruboltTypeAnnotation

" Pattern matching
syn keyword ruboltPattern match case when guard
syn match ruboltWildcard "_"

" Async/await
syn keyword ruboltAsync async await Promise

" Error handling
syn keyword ruboltError try catch finally throw Result Ok Error

" Memory management
syn keyword ruboltMemory gc rc weak strong

" JIT annotations
syn match ruboltJIT "@jit_compile\|@hot_path\|@inline_always\|@no_gc"

" Delimiters
syn match ruboltDelimiter "[(){}[\],;]"

" Define highlighting groups
hi def link ruboltKeyword Keyword
hi def link ruboltType Type
hi def link ruboltBuiltin Function
hi def link ruboltOperator Operator
hi def link ruboltNumber Number
hi def link ruboltString String
hi def link ruboltEscape SpecialChar
hi def link ruboltInterpolation Special
hi def link ruboltComment Comment
hi def link ruboltAttribute PreProc
hi def link ruboltFunction Function
hi def link ruboltClass Type
hi def link ruboltTypeAnnotation Type
hi def link ruboltGeneric Special
hi def link ruboltPattern Conditional
hi def link ruboltWildcard Special
hi def link ruboltAsync Keyword
hi def link ruboltError Exception
hi def link ruboltMemory Keyword
hi def link ruboltJIT PreProc
hi def link ruboltDelimiter Delimiter

let b:current_syntax = "rubolt"

" Vim plugin for Rubolt language support
" File: ~/.vim/plugin/rubolt.vim

if exists('g:loaded_rubolt')
    finish
endif
let g:loaded_rubolt = 1

" Configuration
if !exists('g:rubolt_executable')
    let g:rubolt_executable = 'rbcli'
endif

if !exists('g:rubolt_auto_format')
    let g:rubolt_auto_format = 0
endif

if !exists('g:rubolt_show_errors')
    let g:rubolt_show_errors = 1
endif

" Commands
command! -nargs=0 RuboltRun call rubolt#run_file()
command! -nargs=0 RuboltDebug call rubolt#debug_file()
command! -nargs=0 RuboltRepl call rubolt#open_repl()
command! -nargs=0 RuboltFormat call rubolt#format_file()
command! -nargs=0 RuboltLint call rubolt#lint_file()
command! -nargs=0 RuboltTest call rubolt#run_tests()
command! -nargs=0 RuboltBuild call rubolt#build_project()
command! -range RuboltRunSelection call rubolt#run_selection(<line1>, <line2>)

" Key mappings
nnoremap <leader>rr :RuboltRun<CR>
nnoremap <leader>rd :RuboltDebug<CR>
nnoremap <leader>ri :RuboltRepl<CR>
nnoremap <leader>rf :RuboltFormat<CR>
nnoremap <leader>rl :RuboltLint<CR>
nnoremap <leader>rt :RuboltTest<CR>
nnoremap <leader>rb :RuboltBuild<CR>
vnoremap <leader>rs :RuboltRunSelection<CR>

" Auto commands
augroup rubolt
    autocmd!
    autocmd FileType rubolt setlocal commentstring=//\ %s
    autocmd FileType rubolt setlocal suffixesadd=.rbo
    autocmd FileType rubolt setlocal includeexpr=substitute(v:fname,'\\.','/','g')
    
    if g:rubolt_auto_format
        autocmd BufWritePre *.rbo call rubolt#format_file()
    endif
    
    if g:rubolt_show_errors
        autocmd BufWritePost *.rbo call rubolt#check_syntax()
    endif
augroup END

" Folding
function! RuboltFoldExpr(lnum)
    let line = getline(a:lnum)
    
    " Fold functions
    if line =~ '^\s*def\s'
        return '>1'
    endif
    
    " Fold classes
    if line =~ '^\s*class\s'
        return '>1'
    endif
    
    " Fold blocks
    if line =~ '{\s*$'
        return 'a1'
    endif
    
    if line =~ '^\s*}\s*$'
        return 's1'
    endif
    
    return '='
endfunction

function! RuboltFoldText()
    let line = getline(v:foldstart)
    let sub = substitute(line, '/\*\|\*/\|{{{\d\=', '', 'g')
    return v:folddashes . sub
endfunction

" Set folding for Rubolt files
autocmd FileType rubolt setlocal foldmethod=expr
autocmd FileType rubolt setlocal foldexpr=RuboltFoldExpr(v:lnum)
autocmd FileType rubolt setlocal foldtext=RuboltFoldText()

" Indentation
function! GetRuboltIndent(lnum)
    let prevlnum = prevnonblank(a:lnum - 1)
    
    if prevlnum == 0
        return 0
    endif
    
    let prevline = getline(prevlnum)
    let previndent = indent(prevlnum)
    let line = getline(a:lnum)
    
    " Increase indent after opening braces, function definitions, etc.
    if prevline =~ '{\s*$\|def\s.*:\s*$\|class\s.*:\s*$\|if\s.*:\s*$\|else:\s*$\|while\s.*:\s*$\|for\s.*:\s*$\|match\s.*{\s*$'
        return previndent + &shiftwidth
    endif
    
    " Decrease indent for closing braces
    if line =~ '^\s*}'
        return previndent - &shiftwidth
    endif
    
    " Decrease indent for case statements
    if line =~ '^\s*case\s\|^\s*default:'
        return previndent - &shiftwidth
    endif
    
    return previndent
endfunction

autocmd FileType rubolt setlocal indentexpr=GetRuboltIndent(v:lnum)
autocmd FileType rubolt setlocal indentkeys=0{,0},0),0],!^F,o,O,e,0=case,0=default

" Completion
function! RuboltComplete(findstart, base)
    if a:findstart
        let line = getline('.')
        let start = col('.') - 1
        while start > 0 && line[start - 1] =~ '\w'
            let start -= 1
        endwhile
        return start
    else
        let completions = []
        
        " Keywords
        let keywords = ['def', 'class', 'enum', 'type', 'import', 'export', 
                       \ 'let', 'const', 'if', 'else', 'match', 'case', 'when',
                       \ 'while', 'for', 'in', 'break', 'continue', 'return',
                       \ 'throw', 'try', 'catch', 'finally', 'async', 'await']
        
        " Types
        let types = ['number', 'string', 'bool', 'void', 'any', 'null', 
                    \ 'function', 'Array', 'List', 'Dict', 'Set', 'Tuple']
        
        " Built-ins
        let builtins = ['print', 'len', 'type', 'str', 'int', 'float', 'bool',
                       \ 'assert', 'panic', 'min', 'max', 'abs', 'round']
        
        for word in keywords + types + builtins
            if word =~ '^' . a:base
                call add(completions, word)
            endif
        endfor
        
        return completions
    endif
endfunction

autocmd FileType rubolt setlocal completefunc=RuboltComplete

" File detection
autocmd BufNewFile,BufRead *.rbo setfiletype rubolt