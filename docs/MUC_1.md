# FXL Mục 1 - Compiler Implementation

## Overview

Mục 1 implements the FXL compiler (`fxlc`) that transpiles `.fxl` source files to C++ through a complete pipeline:

```
FXL Source → Lexer → Tokens → Parser → AST → Semantic Analysis → AutoSet Resolution → C++ Code Generator
```

## Architecture

### Components

| Component | Files | Description |
|-----------|-------|-------------|
| **Lexer** | `lexer.hpp/cpp` | Tokenizes FXL source with line/column tracking |
| **Parser** | `parser.hpp/cpp` | Recursive descent parser building AST |
| **AST** | `ast.hpp/cpp` | Node types: Program, ObjectDecl, MethodCall, ShorthandCall, AutoDirective, etc. |
| **Semantic** | `semantic.hpp/cpp` | Scope management, variable resolution, validation |
| **AutoResolver** | `auto_resolver.hpp/cpp` | Resolves #autoSet shorthands (Size→setSize, Color→setColor, etc.) |
| **CodeGen** | `codegen.hpp/cpp` | Generates C++ with main() function |
| **Runtime** | `runtime/include/fxl/runtime.hpp` | Header-only UI backend (Window, Button, Object, Panel, Rectangle) |

### Diagnostics

- Severity: Error, Warning, Info
- Location: file, line, column
- Format: `error [CODE]: message at line L, column C`
- Never crashes on bad source

## Language Features (Mục 1)

### Directives
- `#autoSet { ... }` - Block form with scope
- `#autoPick { ... }` - Parsed, not fully implemented (TODO diagnostic)
- `main #autoSet` - Single statement form (per §11)
- Combined directives rejected: `#autoSet #autoPick { }` → error

### AutoSet Mapping
| Shorthand | Resolves To |
|-----------|-------------|
| `Size` | `setSize` |
| `Color` | `setColor` |
| `Opacity` | `setOpacity` |
| `Position` | `setPosition` |
| `Text` | `setText` |

### Scope Rules
- `#autoSet` scope only affects its own block
- Nested auto directives create new auto scope
- Variables declared in `#autoSet` visible outside (no variable scope isolation)
- Unknown shorthand (e.g., `Rotate(30)`) → `E_UNKNOWN_AUTOSET_OP` error

### Literals
- Numbers: `42`, `3.14`
- Strings: `"hello"`
- Colors: `#RGB`, `#RRGGBB` (normalized to 6-digit uppercase)
- Percentages: `15%` → `15.0`

## CLI Usage

```bash
# Compile FXL to C++
fxlc compile input.fxl [-o output.cpp] [--ast] [--tokens]

# Compile and build executable
fxlc build input.fxl [-o output.exe] [--debug|--release] [--keep-cpp]

# Build and run preview
fxlc preview input.fxl [--debug|--release]
```

### Examples

```bash
# Basic compilation
fxlc examples/basic.fxl
# → generates examples/basic.cpp

# Debug build (default)
fxlc build examples/basic.fxl
# → builds examples/basic.exe with debug symbols

# Release build
fxlc build examples/autoset.fxl --release
# → builds examples/autoset.exe with -O3 -DNDEBUG

# Preview (build debug + run)
fxlc preview examples/nested.fxl
# → builds temp executable and runs it
```

## Build System

- **CMake 3.20+**, C++20
- Targets: `fxlc` (compiler), `fxl_compiler` (static lib), `fxl_runtime` (interface lib)
- Tests: `test_lexer`, `test_parser`, `test_semantic`, `test_autoset`, `test_codegen`
- Strict warnings: `/W4 /WX /permissive-` (MSVC) or `-Wall -Wextra -Wpedantic -Werror ...` (GCC/Clang)

## Tests

| Test Suite | Tests | Status |
|------------|-------|--------|
| Lexer | 13 | ✅ PASS |
| Parser | 10 | ✅ PASS |
| Semantic | 6 | ✅ PASS |
| AutoSet | 6 | ✅ PASS |
| CodeGen | 8 | ✅ PASS |
| **Total** | **43** | **✅ ALL PASS** |

Run all: `ctest --output-on-failure` or run individual test executables.

## Examples

### basic.fxl
```fxl
import ui

window = Window()
window.setSize(800, 600)

button = Button()
button.setText("Click me")

window.add(button)
```

### autoset.fxl
```fxl
import ui

#autoSet {
    window = Window(
        Size(800, 600)
        Position(100, 100)

        rectangle = Object(
            Color(#000000)
            Opacity(15%)
            Size(13, 45)
        )
    )
}

window.setTitle("FXL App")
```

### nested.fxl
```fxl
import ui

window = Window(
    window.setSize(800, 600)

    panel = Panel(
        panel.setPosition(10, 10)

        rectangle = Rectangle(
            rectangle.setColor(#FF0000)
            rectangle.setSize(100, 100)

            button = Button(
                button.setText("Click")
                button.setColor(#00FF00)
            )
        )
    )
)
```

## Runtime

Header-only library at `runtime/include/fxl/runtime.hpp` providing:
- `Window`, `Button`, `Panel`, `Rectangle`, `Object` classes
- All setter methods for #autoSet compatibility
- `add(child)` for hierarchy construction
- `using namespace fxl::runtime;` in generated code

## Known Limitations (Mục 1)

1. `#autoPick` - parsed but not semantically implemented (emits TODO diagnostic)
2. No actual UI rendering - runtime is simulation only
3. No import resolution - imports are comments in generated C++
4. No standard library / built-in types beyond examples
5. Preview runs executable but no actual UI window (no event loop)

## Future Work (Mục 2+)

- Actual UI backend (Win32, X11, Wayland, Web)
- Event system and reactivity
- Import/module system
- More built-in types and widgets
- Hot reload for preview
- Language server / LSP