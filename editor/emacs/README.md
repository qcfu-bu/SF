# SF Mode for Emacs

A major mode for editing SF programming language source files in Emacs.

## Features

- **Rich syntax highlighting** for:
  - Keywords (`func`, `class`, `if`, `for`, etc.)
  - Types and builtin types (`Int`, `String`, `Option`, etc.)
  - Function and method definitions
  - Class, enum, interface, and extension definitions
  - Attributes/decorators (`@abstract`, `@extern`, etc.)
  - Constants (`true`, `false`, `nil`, `self`)
  - Numbers (decimal, hex, binary, floating-point)
  - Strings (single and double-quoted)
  - Comments (line `//` and block `/* */`)
  - Generic type parameters
  - Operators and arrows (`->`, `=>`)

- **Automatic indentation** based on braces and keywords
- **Comment support** with `C-c C-c` for toggling comments
- **Imenu integration** for navigating functions, classes, enums, etc.
- **Electric pair mode** support for automatic bracket matching

## Installation

### Manual Installation

1. Copy `sf-mode.el` to a directory in your Emacs `load-path`
2. Add the following to your Emacs init file:

```elisp
(add-to-list 'load-path "/path/to/sf-mode")
(require 'sf-mode)
```

### Using use-package

```elisp
(use-package sf-mode
  :load-path "/path/to/sf-mode"
  :mode "\\.sf\\'")
```

### Using straight.el

```elisp
(straight-use-package
 '(sf-mode :type git :host github :repo "sf-lang/sf"
           :files ("editor/emacs/*.el")))
```

## Usage

SF mode is automatically activated for files with the `.sf` extension.

### Key Bindings

| Key       | Command           | Description              |
|-----------|-------------------|--------------------------|
| `C-c C-c` | `sf-comment-dwim` | Toggle comment on region |

### Customization

You can customize SF mode with the following options:

```elisp
;; Set indentation width (default: 4)
(setq sf-indent-offset 2)

;; Set tab width (default: 4)
(setq sf-tab-width 2)
```

### Faces

SF mode defines custom faces that inherit from standard font-lock faces.
You can customize them in your theme or init file:

- `sf-keyword-face` - Keywords
- `sf-builtin-face` - Builtin types
- `sf-type-face` - Type names
- `sf-function-name-face` - Function names
- `sf-variable-name-face` - Variable names
- `sf-constant-face` - Constants
- `sf-string-face` - Strings
- `sf-comment-face` - Comments
- `sf-attribute-face` - Attributes (@decorators)
- `sf-operator-face` - Operators
- `sf-number-face` - Numbers
- `sf-module-face` - Module names
- `sf-generic-face` - Generic type parameters

## Requirements

- Emacs 25.1 or later

## License

GNU General Public License v3.0
