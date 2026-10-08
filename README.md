# Pascal to LLVM IR Compiler

This project is a high-level compiler that translates Pascal source code into **LLVM Intermediate Representation (IR)** developed in 2025. It uses a decoupled, multi-stage architecture where the **Abstract Syntax Tree (AST)** serves as the core bridge between language parsing and code generation.

The compiler is organized into the following main components:

1. **Frontend (Flex & Bison)** — Reads Pascal source code, validates its syntax, and constructs the AST.
2. **Abstract Syntax Tree (AST)** — Serves as the core intermediate structure representing the program's logic.
3. **Symbol Table** — Tracks scopes, variables, and function definitions during AST traversal.
4. **Backend (LLVM IR Generator)** — Traverses the AST and emits commented LLVM IR code.

---

## 2. The Abstract Syntax Tree (AST)

The **AST** is the central component of the compiler, completely separating language parsing from code generation.

- **Decoupled Design** — The frontend focuses on building the tree, while the backend focuses on translating the tree into target instructions.
- **Node Hierarchy** — Represents Pascal constructs such as conditional statements (`if/then/else`), loops (`while`), function calls, and arithmetic expressions through specialized node structures.
- **Tree Traversal** — The backend performs recursive depth-first traversals of the AST to perform type checking, manage memory allocations, and emit SSA-form LLVM IR.
- **Debugging Support** — Includes AST visualization utilities that print the formatted tree hierarchy to text files for easier inspection and debugging.

---

## 3. High-Level Pipeline Stages

| Component                              | Responsibility                                                           |
| :------------------------------------- | :----------------------------------------------------------------------- |
| **Scanner (`scanner.l`)**              | Converts Pascal source characters into tokens.                           |
| **Parser (`parser.y`)**                | Validates grammar rules and invokes AST construction routines.           |
| **AST Module (`frontend.c`, `ast.c`)** | Allocates and manages AST nodes and provides tree-printing capabilities. |
| **Symbol Table (`symbol_table.c`)**    | Manages scoped symbol resolution for global and local symbols.           |
| **Backend (`backend.c`)**              | Traverses the AST, resolves types, and emits annotated LLVM IR code.     |

---
