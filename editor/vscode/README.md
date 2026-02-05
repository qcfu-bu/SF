# SF Language Support for Visual Studio Code

Rich syntax highlighting and language support for the SF programming language.

## Features

- **Syntax Highlighting**: Comprehensive syntax highlighting for SF code including:
  - Keywords (class, enum, module, func, if, for, switch, etc.)
  - Type annotations and generic parameters
  - Functions and function calls
  - Attributes (@abstract, @extern, etc.)
  - Comments (line and block)
  - Strings and numbers
  - Operators and punctuation

- **Language Features**:
  - Auto-closing brackets, quotes, and angle brackets
  - Comment toggling (line and block comments)
  - Bracket matching
  - Automatic indentation
  - Code folding

## Installation

1. Copy the extension folder to your VSCode extensions directory:
   - **Windows**: `%USERPROFILE%\.vscode\extensions`
   - **macOS/Linux**: `~/.vscode/extensions`

2. Reload VSCode

Alternatively, you can package and install the extension using `vsce`:

```bash
npm install -g @vscode/vsce
cd editor/vscode
vsce package
code --install-extension sf-lang-0.1.0.vsix
```

## Usage

Once installed, the extension will automatically activate for files with the `.sf` extension.

## Example Code

```sf
import math;

module Core {
    @abstract
    class Functor {
        type Self<A>;
        func map<A,B>(x: Self<A>, f: A -> B) -> Self<B>;
    }

    enum Option<T> {
        case None
        case Some(T)
    }

    class Vec<T> private (data, capacity, len) {
        private let mut data: Array<T> = data;

        func push(self: Vec<T>, value: T) {
            if self.len >= self.capacity {
                // resize logic
            }
            self.data[self.len] = value;
            self.len += 1;
        }
    }
}
```

## License

MIT
