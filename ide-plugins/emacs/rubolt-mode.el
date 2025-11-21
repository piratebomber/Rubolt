;;; rubolt-mode.el --- Major mode for Rubolt programming language -*- lexical-binding: t; -*-

;; Copyright (C) 2024 Rubolt Team

;; Author: Rubolt Team <team@rubolt.dev>
;; Version: 1.0.0
;; Package-Requires: ((emacs "25.1"))
;; Keywords: languages, rubolt
;; URL: https://github.com/piratebomber/Rubolt

;;; Commentary:

;; This package provides a major mode for editing Rubolt source code.
;; It includes syntax highlighting, indentation, and integration with
;; the Rubolt toolchain.

;;; Code:

(require 'font-lock)
(require 'syntax)
(require 'comint)

(defgroup rubolt nil
  "Support for the Rubolt programming language."
  :group 'languages
  :prefix "rubolt-")

(defcustom rubolt-executable "rbcli"
  "Path to the Rubolt CLI executable."
  :type 'string
  :group 'rubolt)

(defcustom rubolt-indent-offset 4
  "Number of spaces for each indentation level."
  :type 'integer
  :group 'rubolt)

(defcustom rubolt-auto-format-on-save nil
  "Automatically format code when saving."
  :type 'boolean
  :group 'rubolt)

(defcustom rubolt-show-errors t
  "Show syntax errors in the buffer."
  :type 'boolean
  :group 'rubolt)

;; Syntax table
(defvar rubolt-mode-syntax-table
  (let ((table (make-syntax-table)))
    ;; Comments
    (modify-syntax-entry ?/ ". 124b" table)
    (modify-syntax-entry ?* ". 23" table)
    (modify-syntax-entry ?\n "> b" table)
    (modify-syntax-entry ?# "< b" table)
    
    ;; Strings
    (modify-syntax-entry ?\" "\"" table)
    (modify-syntax-entry ?\' "\"" table)
    (modify-syntax-entry ?\\ "\\" table)
    
    ;; Operators
    (modify-syntax-entry ?+ "." table)
    (modify-syntax-entry ?- "." table)
    (modify-syntax-entry ?* "." table)
    (modify-syntax-entry ?% "." table)
    (modify-syntax-entry ?< "." table)
    (modify-syntax-entry ?> "." table)
    (modify-syntax-entry ?& "." table)
    (modify-syntax-entry ?| "." table)
    (modify-syntax-entry ?^ "." table)
    (modify-syntax-entry ?! "." table)
    (modify-syntax-entry ?= "." table)
    (modify-syntax-entry ?? "." table)
    
    ;; Delimiters
    (modify-syntax-entry ?\( "()" table)
    (modify-syntax-entry ?\) ")(" table)
    (modify-syntax-entry ?\[ "(]" table)
    (modify-syntax-entry ?\] ")[" table)
    (modify-syntax-entry ?\{ "(}" table)
    (modify-syntax-entry ?\} "){" table)
    
    ;; Punctuation
    (modify-syntax-entry ?, "." table)
    (modify-syntax-entry ?\; "." table)
    (modify-syntax-entry ?: "." table)
    
    table)
  "Syntax table for Rubolt mode.")

;; Keywords
(defconst rubolt-keywords
  '("def" "class" "enum" "type" "import" "export" "let" "const"
    "if" "else" "match" "case" "when" "while" "for" "in"
    "break" "continue" "return" "throw" "try" "catch" "finally"
    "async" "await" "yield" "and" "or" "not" "null" "true" "false"
    "this" "super" "public" "private" "protected" "static" "abstract"))

(defconst rubolt-types
  '("number" "string" "bool" "void" "any" "null" "function"
    "Array" "List" "Dict" "Set" "Tuple" "Optional" "Result" "Promise"))

(defconst rubolt-builtins
  '("print" "len" "type" "str" "int" "float" "bool"
    "assert" "panic" "unreachable" "min" "max" "abs" "round"
    "floor" "ceil" "map" "filter" "reduce" "forEach" "find" "some" "every"))

(defconst rubolt-constants
  '("true" "false" "null"))

;; Font lock keywords
(defvar rubolt-font-lock-keywords
  `(
    ;; Keywords
    (,(regexp-opt rubolt-keywords 'words) . font-lock-keyword-face)
    
    ;; Types
    (,(regexp-opt rubolt-types 'words) . font-lock-type-face)
    
    ;; Built-in functions
    (,(regexp-opt rubolt-builtins 'words) . font-lock-builtin-face)
    
    ;; Constants
    (,(regexp-opt rubolt-constants 'words) . font-lock-constant-face)
    
    ;; Function definitions
    ("\\<def\\s-+\\(\\w+\\)" 1 font-lock-function-name-face)
    
    ;; Class definitions
    ("\\<class\\s-+\\(\\w+\\)" 1 font-lock-type-face)
    
    ;; Type annotations
    (":\\s-*\\(\\w+\\)" 1 font-lock-type-face)
    
    ;; Attributes/decorators
    ("@\\w+" . font-lock-preprocessor-face)
    
    ;; Numbers
    ("\\<[0-9]+\\(\\.[0-9]+\\)?\\([eE][+-]?[0-9]+\\)?\\>" . font-lock-constant-face)
    ("\\<0x[0-9a-fA-F]+\\>" . font-lock-constant-face)
    ("\\<0b[01]+\\>" . font-lock-constant-face)
    ("\\<0o[0-7]+\\>" . font-lock-constant-face)
    
    ;; Strings
    ("\"[^\"]*\"" . font-lock-string-face)
    ("'[^']*'" . font-lock-string-face)
    
    ;; String interpolation
    ("\\${[^}]*}" . font-lock-variable-name-face)
    
    ;; Variables
    ("\\<\\([a-zA-Z_][a-zA-Z0-9_]*\\)\\s-*=" 1 font-lock-variable-name-face)
    
    ;; Pattern matching
    ("\\<_\\>" . font-lock-keyword-face)
    )
  "Font lock keywords for Rubolt mode.")

;; Indentation
(defun rubolt-indent-line ()
  "Indent current line as Rubolt code."
  (interactive)
  (let ((indent-col 0)
        (cur-indent (current-indentation)))
    (save-excursion
      (beginning-of-line)
      (if (bobp)
          (setq indent-col 0)
        (let ((prev-line (rubolt-previous-non-empty-line)))
          (if prev-line
              (progn
                (goto-line prev-line)
                (setq indent-col (current-indentation))
                
                ;; Increase indent after opening constructs
                (when (looking-at ".*\\(def\\|class\\|if\\|else\\|while\\|for\\|match\\|try\\|catch\\|finally\\).*[:{]\\s-*$")
                  (setq indent-col (+ indent-col rubolt-indent-offset)))
                
                ;; Increase indent after opening braces
                (when (looking-at ".*{\\s-*$")
                  (setq indent-col (+ indent-col rubolt-indent-offset)))
                
                ;; Decrease indent for closing braces
                (beginning-of-line)
                (when (looking-at "\\s-*}")
                  (setq indent-col (max 0 (- indent-col rubolt-indent-offset))))
                
                ;; Decrease indent for case statements
                (when (looking-at "\\s-*\\(case\\|default\\)")
                  (setq indent-col (max 0 (- indent-col rubolt-indent-offset))))
                )))))
    
    (if (= cur-indent indent-col)
        (if (< (current-column) (current-indentation))
            (back-to-indentation))
      (beginning-of-line)
      (delete-horizontal-space)
      (indent-to indent-col)
      (if (< (current-column) (current-indentation))
          (back-to-indentation)))))

(defun rubolt-previous-non-empty-line ()
  "Find the previous non-empty line number."
  (save-excursion
    (forward-line -1)
    (while (and (not (bobp)) (looking-at "^\\s-*$"))
      (forward-line -1))
    (if (bobp) nil (line-number-at-pos))))

;; Completion
(defun rubolt-completion-at-point ()
  "Completion function for Rubolt mode."
  (let* ((bounds (bounds-of-thing-at-point 'symbol))
         (start (or (car bounds) (point)))
         (end (or (cdr bounds) (point)))
         (candidates (append rubolt-keywords rubolt-types rubolt-builtins)))
    (list start end candidates)))

;; REPL integration
(defvar rubolt-repl-buffer-name "*Rubolt REPL*")

(defun rubolt-run-repl ()
  "Start a Rubolt REPL."
  (interactive)
  (let ((buffer (get-buffer-create rubolt-repl-buffer-name)))
    (with-current-buffer buffer
      (unless (comint-check-proc buffer)
        (make-comint-in-buffer "rubolt-repl" buffer rubolt-executable "repl")
        (rubolt-repl-mode)))
    (pop-to-buffer buffer)))

(define-derived-mode rubolt-repl-mode comint-mode "Rubolt REPL"
  "Major mode for Rubolt REPL."
  (setq comint-prompt-regexp "^rubolt> ")
  (setq comint-use-prompt-regexp t))

;; File operations
(defun rubolt-run-file ()
  "Run the current Rubolt file."
  (interactive)
  (if (buffer-file-name)
      (let ((command (format "%s run %s" rubolt-executable (buffer-file-name))))
        (compile command))
    (message "Buffer is not visiting a file")))

(defun rubolt-debug-file ()
  "Debug the current Rubolt file."
  (interactive)
  (if (buffer-file-name)
      (let ((command (format "%s debug %s" rubolt-executable (buffer-file-name))))
        (compile command))
    (message "Buffer is not visiting a file")))

(defun rubolt-format-buffer ()
  "Format the current buffer."
  (interactive)
  (if (buffer-file-name)
      (let ((command (format "%s format %s" rubolt-executable (buffer-file-name))))
        (shell-command command))
    (message "Buffer is not visiting a file")))

(defun rubolt-lint-buffer ()
  "Lint the current buffer."
  (interactive)
  (if (buffer-file-name)
      (let ((command (format "%s lint %s" rubolt-executable (buffer-file-name))))
        (compile command))
    (message "Buffer is not visiting a file")))

(defun rubolt-run-tests ()
  "Run tests for the current project."
  (interactive)
  (let ((command (format "%s test" rubolt-executable)))
    (compile command)))

(defun rubolt-build-project ()
  "Build the current project."
  (interactive)
  (let ((command (format "%s build" rubolt-executable)))
    (compile command)))

(defun rubolt-send-region-to-repl (start end)
  "Send the current region to the REPL."
  (interactive "r")
  (let ((text (buffer-substring-no-properties start end)))
    (rubolt-run-repl)
    (with-current-buffer rubolt-repl-buffer-name
      (goto-char (point-max))
      (insert text)
      (comint-send-input))))

;; Syntax checking
(defun rubolt-check-syntax ()
  "Check syntax of the current buffer."
  (when (and rubolt-show-errors (buffer-file-name))
    (let ((command (format "%s compile --check %s" rubolt-executable (buffer-file-name))))
      (shell-command command))))

;; Keymap
(defvar rubolt-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map (kbd "C-c C-r") 'rubolt-run-file)
    (define-key map (kbd "C-c C-d") 'rubolt-debug-file)
    (define-key map (kbd "C-c C-i") 'rubolt-run-repl)
    (define-key map (kbd "C-c C-f") 'rubolt-format-buffer)
    (define-key map (kbd "C-c C-l") 'rubolt-lint-buffer)
    (define-key map (kbd "C-c C-t") 'rubolt-run-tests)
    (define-key map (kbd "C-c C-b") 'rubolt-build-project)
    (define-key map (kbd "C-c C-e") 'rubolt-send-region-to-repl)
    map)
  "Keymap for Rubolt mode.")

;; Menu
(easy-menu-define rubolt-mode-menu rubolt-mode-map
  "Menu for Rubolt mode."
  '("Rubolt"
    ["Run File" rubolt-run-file t]
    ["Debug File" rubolt-debug-file t]
    ["Start REPL" rubolt-run-repl t]
    "---"
    ["Format Buffer" rubolt-format-buffer t]
    ["Lint Buffer" rubolt-lint-buffer t]
    "---"
    ["Run Tests" rubolt-run-tests t]
    ["Build Project" rubolt-build-project t]
    "---"
    ["Send Region to REPL" rubolt-send-region-to-repl t]))

;; Imenu support
(defvar rubolt-imenu-generic-expression
  '(("Functions" "^\\s-*def\\s-+\\(\\w+\\)" 1)
    ("Classes" "^\\s-*class\\s-+\\(\\w+\\)" 1)
    ("Types" "^\\s-*type\\s-+\\(\\w+\\)" 1)
    ("Enums" "^\\s-*enum\\s-+\\(\\w+\\)" 1))
  "Imenu generic expression for Rubolt mode.")

;; Auto-mode-alist
;;;###autoload
(add-to-list 'auto-mode-alist '("\\.rbo\\'" . rubolt-mode))

;; Define the major mode
;;;###autoload
(define-derived-mode rubolt-mode prog-mode "Rubolt"
  "Major mode for editing Rubolt source code."
  :syntax-table rubolt-mode-syntax-table
  
  ;; Font lock
  (setq font-lock-defaults '(rubolt-font-lock-keywords))
  
  ;; Indentation
  (setq indent-line-function 'rubolt-indent-line)
  (setq tab-width rubolt-indent-offset)
  (setq indent-tabs-mode nil)
  
  ;; Comments
  (setq comment-start "// ")
  (setq comment-end "")
  (setq comment-start-skip "//+\\s-*")
  
  ;; Completion
  (add-hook 'completion-at-point-functions 'rubolt-completion-at-point nil t)
  
  ;; Imenu
  (setq imenu-generic-expression rubolt-imenu-generic-expression)
  
  ;; Auto-format on save
  (when rubolt-auto-format-on-save
    (add-hook 'before-save-hook 'rubolt-format-buffer nil t))
  
  ;; Syntax checking
  (when rubolt-show-errors
    (add-hook 'after-save-hook 'rubolt-check-syntax nil t))
  
  ;; Electric pairs
  (electric-pair-local-mode 1)
  
  ;; Show matching parens
  (show-paren-mode 1))

;; Flycheck integration (if available)
(eval-after-load 'flycheck
  '(progn
     (flycheck-define-checker rubolt
       "A Rubolt syntax checker using rbcli."
       :command ("rbcli" "compile" "--check" source)
       :error-patterns
       ((error line-start (file-name) ":" line ":" column ": error: " (message) line-end)
        (warning line-start (file-name) ":" line ":" column ": warning: " (message) line-end))
       :modes rubolt-mode)
     
     (add-to-list 'flycheck-checkers 'rubolt)))

;; Company integration (if available)
(eval-after-load 'company
  '(progn
     (defun company-rubolt (command &optional arg &rest ignored)
       "Company backend for Rubolt."
       (interactive (list 'interactive))
       (case command
         (interactive (company-begin-backend 'company-rubolt))
         (prefix (and (eq major-mode 'rubolt-mode)
                      (company-grab-symbol)))
         (candidates (all-completions arg (append rubolt-keywords rubolt-types rubolt-builtins)))
         (meta (format "Rubolt keyword: %s" arg))))
     
     (add-to-list 'company-backends 'company-rubolt)))

;; LSP integration (if available)
(eval-after-load 'lsp-mode
  '(progn
     (add-to-list 'lsp-language-id-configuration '(rubolt-mode . "rubolt"))
     
     (lsp-register-client
      (make-lsp-client :new-connection (lsp-stdio-connection "rbcli lsp")
                       :major-modes '(rubolt-mode)
                       :server-id 'rubolt-lsp))))

(provide 'rubolt-mode)

;;; rubolt-mode.el ends here