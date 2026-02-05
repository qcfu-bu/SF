;;; sf-mode.el --- Major mode for the SF programming language -*- lexical-binding: t; -*-

;; Copyright (C) 2024

;; Author: SF Contributors
;; Version: 1.0.0
;; Package-Requires: ((emacs "25.1"))
;; Keywords: languages, sf
;; URL: https://github.com/sf-lang/sf

;; This file is not part of GNU Emacs.

;; This program is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <https://www.gnu.org/licenses/>.

;;; Commentary:

;; This package provides a major mode for editing SF programming language
;; source files.  It includes:
;;
;; - Rich syntax highlighting for keywords, types, operators, etc.
;; - Proper comment handling (line and block comments)
;; - Automatic indentation
;; - Imenu support for navigation
;;
;; To use this mode, add the following to your init file:
;;
;;   (add-to-list 'load-path "/path/to/sf-mode")
;;   (require 'sf-mode)
;;
;; SF files (.sf extension) will automatically use this mode.

;;; Code:

(require 'cl-lib)

;;; Customization

(defgroup sf nil
  "Major mode for editing SF source code."
  :prefix "sf-"
  :group 'languages
  :link '(url-link :tag "GitHub" "https://github.com/sf-lang/sf"))

(defcustom sf-indent-offset 4
  "Number of spaces for each indentation level in SF mode."
  :type 'integer
  :group 'sf
  :safe 'integerp)

(defcustom sf-tab-width 4
  "Width of a tab character in SF mode."
  :type 'integer
  :group 'sf
  :safe 'integerp)

;;; Faces

(defgroup sf-faces nil
  "Faces used in SF mode."
  :group 'sf
  :group 'faces)

(defface sf-keyword-face
  '((t :inherit font-lock-keyword-face))
  "Face for SF keywords."
  :group 'sf-faces)

(defface sf-builtin-face
  '((t :inherit font-lock-builtin-face))
  "Face for SF builtin functions and types."
  :group 'sf-faces)

(defface sf-type-face
  '((t :inherit font-lock-type-face))
  "Face for SF type names."
  :group 'sf-faces)

(defface sf-function-name-face
  '((t :inherit font-lock-function-name-face))
  "Face for SF function names."
  :group 'sf-faces)

(defface sf-variable-name-face
  '((t :inherit font-lock-variable-name-face))
  "Face for SF variable names."
  :group 'sf-faces)

(defface sf-constant-face
  '((t :inherit font-lock-constant-face))
  "Face for SF constants."
  :group 'sf-faces)

(defface sf-string-face
  '((t :inherit font-lock-string-face))
  "Face for SF strings."
  :group 'sf-faces)

(defface sf-comment-face
  '((t :inherit font-lock-comment-face))
  "Face for SF comments."
  :group 'sf-faces)

(defface sf-attribute-face
  '((t :inherit font-lock-preprocessor-face))
  "Face for SF attributes (@decorator)."
  :group 'sf-faces)

(defface sf-operator-face
  '((t :inherit font-lock-builtin-face))
  "Face for SF operators."
  :group 'sf-faces)

(defface sf-number-face
  '((t :inherit font-lock-constant-face))
  "Face for SF numbers."
  :group 'sf-faces)

(defface sf-module-face
  '((t :inherit font-lock-constant-face :weight bold))
  "Face for SF module names."
  :group 'sf-faces)

(defface sf-generic-face
  '((t :inherit font-lock-type-face :slant italic))
  "Face for SF generic type parameters."
  :group 'sf-faces)

;;; Syntax Table

(defvar sf-mode-syntax-table
  (let ((table (make-syntax-table)))
    ;; Comments: // and /* */
    (modify-syntax-entry ?/ ". 124b" table)
    (modify-syntax-entry ?* ". 23" table)
    (modify-syntax-entry ?\n "> b" table)

    ;; Strings
    (modify-syntax-entry ?\" "\"" table)
    (modify-syntax-entry ?\' "\"" table)

    ;; Escape character
    (modify-syntax-entry ?\\ "\\" table)

    ;; Symbol constituents
    (modify-syntax-entry ?_ "_" table)

    ;; Operators
    (modify-syntax-entry ?+ "." table)
    (modify-syntax-entry ?- "." table)
    (modify-syntax-entry ?= "." table)
    (modify-syntax-entry ?< "." table)
    (modify-syntax-entry ?> "." table)
    (modify-syntax-entry ?& "." table)
    (modify-syntax-entry ?| "." table)
    (modify-syntax-entry ?! "." table)
    (modify-syntax-entry ?% "." table)
    (modify-syntax-entry ?: "." table)

    ;; Parentheses and brackets
    (modify-syntax-entry ?\( "()" table)
    (modify-syntax-entry ?\) ")(" table)
    (modify-syntax-entry ?\[ "(]" table)
    (modify-syntax-entry ?\] ")[" table)
    (modify-syntax-entry ?\{ "(}" table)
    (modify-syntax-entry ?\} "){" table)

    table)
  "Syntax table for SF mode.")

;;; Keywords and Font Lock

(defconst sf-keywords
  '("if" "else" "switch" "case" "default"
    "for" "while" "in"
    "return" "break" "continue"
    "func" "init" "class" "enum" "interface" "extension"
    "type" "module" "package" "where"
    "let" "var" "mut"
    "private" "public" "static"
    "open" "import" "as")
  "SF language keywords.")

(defconst sf-builtin-types
  '("Int" "Float" "Double" "Bool" "String" "Char" "Void"
    "Array" "List" "Option" "Result" "Map" "Set" "Vec")
  "SF builtin types.")

(defconst sf-constants
  '("true" "false" "nil" "self" "Self")
  "SF language constants.")

(defconst sf-attributes-regexp
  "@\\([a-zA-Z_][a-zA-Z0-9_]*\\)"
  "Regexp matching SF attributes.")

(defconst sf-function-def-regexp
  "\\<\\(func\\|init\\)\\>\\s-+\\([a-zA-Z_][a-zA-Z0-9_]*\\)"
  "Regexp matching SF function definitions.")

(defconst sf-class-def-regexp
  "\\<\\(class\\|enum\\|interface\\|extension\\|module\\)\\>\\s-+\\([a-zA-Z_][a-zA-Z0-9_]*\\)"
  "Regexp matching SF class/enum/interface/extension/module definitions.")

(defconst sf-type-annotation-regexp
  ":\\s-*\\([A-Z][a-zA-Z0-9_]*\\)"
  "Regexp matching SF type annotations.")

(defconst sf-generic-type-regexp
  "\\<\\([A-Z][a-zA-Z0-9_]*\\)\\s-*<"
  "Regexp matching SF generic type usage.")

(defconst sf-type-parameter-regexp
  "<\\([A-Z][a-zA-Z0-9_,: ]*\\)>"
  "Regexp matching SF type parameters.")

(defconst sf-number-regexp
  (concat
   "\\<"
   "\\("
   "0[xX][0-9a-fA-F_]+"           ; Hexadecimal
   "\\|0[bB][01_]+"               ; Binary
   "\\|[0-9][0-9_]*\\.[0-9_]*\\([eE][+-]?[0-9_]+\\)?"  ; Float with decimal
   "\\|[0-9][0-9_]*[eE][+-]?[0-9_]+"  ; Float with exponent
   "\\|[0-9][0-9_]*"              ; Decimal integer
   "\\)"
   "\\>")
  "Regexp matching SF numbers.")

(defconst sf-operator-regexp
  "\\(->\\|=>\\|==\\|!=\\|<=\\|>=\\|&&\\|||\\|[+\\-*/%=<>!&|:]\\)"
  "Regexp matching SF operators.")

(defconst sf-case-regexp
  "\\<case\\>\\s-+\\([A-Z][a-zA-Z0-9_]*\\)"
  "Regexp matching SF enum case declarations.")

(defconst sf-import-regexp
  "\\<\\(import\\|open\\)\\>\\s-+\\([a-zA-Z_][a-zA-Z0-9_.]*\\)"
  "Regexp matching SF import statements.")

(defvar sf-font-lock-keywords
  `(
    ;; Attributes (@decorator)
    (,sf-attributes-regexp . (0 'sf-attribute-face))

    ;; Keywords
    (,(regexp-opt sf-keywords 'words) . 'sf-keyword-face)

    ;; Function/method definitions
    (,sf-function-def-regexp
     (1 'sf-keyword-face)
     (2 'sf-function-name-face))

    ;; Class/enum/interface/extension/module definitions
    (,sf-class-def-regexp
     (1 'sf-keyword-face)
     (2 'sf-type-face))

    ;; Enum case declarations
    (,sf-case-regexp
     (1 'sf-constant-face))

    ;; Import statements
    (,sf-import-regexp
     (1 'sf-keyword-face)
     (2 'sf-module-face))

    ;; Generic type usage (before < bracket)
    (,sf-generic-type-regexp
     (1 'sf-type-face))

    ;; Type annotations (: Type)
    (,sf-type-annotation-regexp
     (1 'sf-type-face))

    ;; Builtin types
    (,(regexp-opt sf-builtin-types 'words) . 'sf-builtin-face)

    ;; Constants
    (,(regexp-opt sf-constants 'words) . 'sf-constant-face)

    ;; Numbers
    (,sf-number-regexp . 'sf-number-face)

    ;; Single uppercase letter type parameters (generics like T, A, B)
    ("\\<\\([A-Z]\\)\\>" . 'sf-generic-face)

    ;; Arrow operators (special highlighting)
    ("\\(->\\|=>\\)" . 'sf-operator-face)

    ;; Variable declarations (let/var/mut followed by identifier)
    ("\\<\\(let\\|var\\|mut\\)\\>\\s-+\\([a-zA-Z_][a-zA-Z0-9_]*\\)"
     (1 'sf-keyword-face)
     (2 'sf-variable-name-face))

    ;; Function calls (identifier followed by parenthesis or generic)
    ("\\<\\([a-z_][a-zA-Z0-9_]*\\)\\s-*[(<]"
     (1 'sf-function-name-face))
    )
  "Font lock keywords for SF mode.")

;;; Indentation

(defun sf-indent-line ()
  "Indent current line as SF code."
  (interactive)
  (let ((indent (sf-calculate-indentation)))
    (when indent
      (let ((offset (- (current-column) (current-indentation))))
        (indent-line-to indent)
        (when (> offset 0)
          (forward-char offset))))))

(defun sf-calculate-indentation ()
  "Calculate the indentation for the current line."
  (save-excursion
    (beginning-of-line)
    (if (bobp)
        0
      (let ((not-indented t)
            (cur-indent 0)
            (closing-bracket (looking-at "^\\s-*[}\\])]")))
        ;; Check for closing bracket
        (if closing-bracket
            (progn
              (save-excursion
                (forward-line -1)
                (setq cur-indent (- (current-indentation) sf-indent-offset)))
              (when (< cur-indent 0)
                (setq cur-indent 0)))
          ;; Not a closing bracket
          (save-excursion
            (while not-indented
              (forward-line -1)
              (cond
               ;; Previous line ends with opening bracket
               ((looking-at ".*[{\\[(]\\s-*\\(//.*\\)?$")
                (setq cur-indent (+ (current-indentation) sf-indent-offset))
                (setq not-indented nil))
               ;; Previous line is a case/default statement
               ((looking-at "^\\s-*\\(case\\|default\\)\\>.*:\\s-*$")
                (setq cur-indent (+ (current-indentation) sf-indent-offset))
                (setq not-indented nil))
               ;; Start of buffer
               ((bobp)
                (setq cur-indent 0)
                (setq not-indented nil))
               ;; Previous non-empty line
               ((not (looking-at "^\\s-*$"))
                (setq cur-indent (current-indentation))
                (setq not-indented nil))))))
        cur-indent))))

;;; Imenu Support

(defvar sf-imenu-generic-expression
  `(("Functions" ,sf-function-def-regexp 2)
    ("Classes" "\\<class\\>\\s-+\\([a-zA-Z_][a-zA-Z0-9_]*\\)" 1)
    ("Enums" "\\<enum\\>\\s-+\\([a-zA-Z_][a-zA-Z0-9_]*\\)" 1)
    ("Interfaces" "\\<interface\\>\\s-+\\([a-zA-Z_][a-zA-Z0-9_]*\\)" 1)
    ("Extensions" "\\<extension\\>\\s-+\\([a-zA-Z_][a-zA-Z0-9_]*\\)" 1)
    ("Modules" "\\<module\\>\\s-+\\([a-zA-Z_][a-zA-Z0-9_]*\\)" 1)
    ("Types" "\\<type\\>\\s-+\\([a-zA-Z_][a-zA-Z0-9_]*\\)" 1))
  "Imenu generic expression for SF mode.")

;;; Comment Functions

(defun sf-comment-dwim (arg)
  "Comment or uncomment current line or region in SF style.
If the region is active, comment/uncomment the region.
Otherwise, comment/uncomment the current line.
With prefix ARG, use that many comment characters."
  (interactive "*P")
  (comment-dwim arg))

;;; Mode Definition

;;;###autoload
(define-derived-mode sf-mode prog-mode "SF"
  "Major mode for editing SF programming language source files.

\\{sf-mode-map}"
  :syntax-table sf-mode-syntax-table
  :group 'sf

  ;; Comments
  (setq-local comment-start "// ")
  (setq-local comment-end "")
  (setq-local comment-start-skip "\\(//+\\|/\\*+\\)\\s-*")
  (setq-local comment-multi-line t)
  (setq-local comment-use-syntax t)

  ;; Font lock
  (setq-local font-lock-defaults '(sf-font-lock-keywords nil nil nil nil))

  ;; Indentation
  (setq-local indent-line-function #'sf-indent-line)
  (setq-local tab-width sf-tab-width)
  (setq-local indent-tabs-mode nil)

  ;; Electric pair mode for brackets
  (setq-local electric-indent-chars
              (append "{}()[]:;" electric-indent-chars))

  ;; Imenu
  (setq-local imenu-generic-expression sf-imenu-generic-expression)

  ;; Paragraph filling
  (setq-local paragraph-start (concat "$\\|" page-delimiter))
  (setq-local paragraph-separate paragraph-start)
  (setq-local fill-paragraph-function #'sf-fill-paragraph)

  ;; Beginning/end of defun
  (setq-local beginning-of-defun-function #'sf-beginning-of-defun)
  (setq-local end-of-defun-function #'sf-end-of-defun))

(defun sf-fill-paragraph (&optional justify)
  "Fill paragraph for SF mode, handling comments properly.
JUSTIFY is passed to `fill-paragraph'."
  (interactive "P")
  (save-excursion
    (let ((ppss (syntax-ppss)))
      (cond
       ((nth 4 ppss)  ; Inside a comment
        (fill-comment-paragraph justify))
       (t
        ;; Not in a comment, don't fill
        t)))))

(defun sf-beginning-of-defun (&optional arg)
  "Move backward to the beginning of a function definition.
With ARG, do it that many times."
  (interactive "^p")
  (setq arg (or arg 1))
  (when (re-search-backward
         "^\\s-*\\(func\\|init\\|class\\|enum\\|interface\\|extension\\)\\>"
         nil t arg)
    (beginning-of-line)
    t))

(defun sf-end-of-defun (&optional arg)
  "Move forward to the end of a function definition.
With ARG, do it that many times."
  (interactive "^p")
  (setq arg (or arg 1))
  (when (sf-beginning-of-defun (- arg))
    (re-search-forward "{" nil t)
    (backward-char)
    (forward-sexp)
    (point)))

;;; Keymap

(defvar sf-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map (kbd "C-c C-c") #'sf-comment-dwim)
    map)
  "Keymap for SF mode.")

;;; Auto Mode

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.sf\\'" . sf-mode))

(provide 'sf-mode)

;;; sf-mode.el ends here
