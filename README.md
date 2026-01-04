# Kamish Shell 🐚


**Kamish** is a fully functional, memory-safe Unix shell implemented in C++. It features a recursive descent parser, optimized process execution, and robust signal handling, mimicking the behavior of modern shells like Bash or Zsh.

## 🚀 Key Features

* **Command Chaining:** Support for logical `&&` (AND), `||` (OR), and sequential `;` operators.
* **Piping:** Infinite pipe depth (e.g., `cmd1 | cmd2 | ... | cmdN`).
* **Redirections:** Input (`<`), Output (`>`), and Append (`>>`) support.
* **Built-in Commands:** Native implementation of `cd` and `exit`.
* **Smart Execution:** Optimized forking model to reduce process overhead.
* **User Experience:** Integrated **GNU Readline** for command history (Up/Down arrows) and line editing.
* **Memory Safe:** Verified 0 memory leaks using Valgrind.

## 🛠️ Architecture


Kamish is built on a modular architecture designed for extensibility and performance.


### 1. Abstract Syntax Tree (AST)
Commands are not executed linearly but parsed into a polymorphic tree structure.
* **Base Class:** `Command` (Virtual interface).
* **Derived Classes:** `SimpleCommand`, `PipeCommand`, `RedirectCommand`, `AndCommand`, `OrCommand`.

This allows complex nesting like:
`ls -la | grep "cpp" > out.txt && echo "Success" || echo "Failure"`

### 2. Recursive Descent Parser
The shell uses a custom recursive descent parser that respects standard Unix operator precedence:
1.  **Sequence** (`;`) - *Lowest Binding*
2.  **Logic** (`&&`, `||`)
3.  **Pipes** (`|`)
4.  **Redirection** (`>`, `>>`, `<`) - *Highest Binding*

### 3. "Manager vs. Worker" Optimization
Unlike basic shell implementations that fork blindly, Kamish uses context-aware execution to save resources.
* **Manager Mode:** Logic and Pipe nodes act as managers, waiting for children.
* **Worker Mode:** When a command is the last link in a chain (e.g., inside a pipe), it executes directly (`execve`) without forking a second time, effectively replacing the intermediate process.

## 📦 Installation

### Prerequisites
Kamish requires `g++` and the `readline` development library.

**Ubuntu/Debian:**
```bash
sudo apt-get install libreadline-dev
```

**MacOS (Homebrew):**
```bash

brew install readline

```

### Compilation
Clone the repository and compile using `g++`:

```bash
g++ -std=c++11 main.cpp shell.cpp command.cpp -o kamish -lreadline
```

## 💻 Usage

Run the shell:
```bash
./kamish
```

### Examples


**Simple Commands & Built-ins:**
```bash
kamish$: ls -la

kamish$: cd ..
kamish$: pwd
```

**Pipes & Redirection:**
```bash

kamish$: cat main.cpp | grep "include" > headers.txt
```

**Complex Logic:**
```bash
kamish$: mkdir test_folder && cd test_folder || echo "Directory creation failed"
```

## 🧠 Memory Safety

The project is verified using **Valgrind** to ensure zero memory leaks.

```bash
valgrind --leak-check=full --suppressions=readline.supp ./kamish
```

*Note: A suppression file (`readline.supp`) is used to filter out internal one-time allocations from the GNU Readline library.*

## 📄 License
This project is open source and available under the [GNU GPL v3 License](LICENSE).
