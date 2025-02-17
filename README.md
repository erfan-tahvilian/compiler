# Compiler with Lexer and Parser - C++

## Overview

This project is a comprehensive implementation of various programming language parsers and a lexical analyzer (**Lexer**) using **C++**. The primary objective is to analyze and compare the efficiency and performance of different types of parsers in processing input grammars and string analysis. Additionally, the project includes tools for grammar preprocessing, such as **left recursion elimination**, **left factoring**, and **LL(1) grammar verification**.

## Features

- **Lexer (Lexical Analyzer):** Tokenizes the input string, identifies symbols, and manages the symbol table.
- **Recursive Descent Parser:** A top-down parser that is manually managed and designed for simple languages.
- **Non-Recursive LL(1) Predictive Parser:** A table-driven, top-down parser that processes LL(1) grammars efficiently.
- **Canonical LR(1) Parser:** A bottom-up parser capable of handling complex LR(1) grammars.
- **Look-Ahead LR(1) Parser:** A bottom-up parser optimized for reduced memory and execution time, suitable for common programming language grammars.

## Capabilities

1. **Support for Various Grammars** - Allows users to analyze diverse grammar structures via text file input.
2. **Performance Analysis** - Measures and reports execution time for different parsers.
3. **User-Friendly Interaction** - Provides an interactive menu for testing different grammars and input strings.
4. **Parsing Table Generation** - Displays tables such as **FIRST**, **FOLLOW**, **Symbol Table**, and parsing tables for each parser.
5. **Grammar Preprocessing:**
   - **Left Recursion Elimination** - Converts left-recursive grammars for compatibility with top-down parsers.
   - **Left Factoring** - Refactors grammars into a format suitable for LL(1) parsing.
   - **LL(1) Compatibility Check** - Evaluates whether a given grammar conforms to LL(1) rules.
6. **Error Handling:**
   - **LL(1) Parser Error Recovery** - Dynamically processes syntax and lexical errors to allow continued parsing.
   - **Error Reporting** - Displays detailed syntax and lexical error messages, including column numbers for user reference.

## Installation

### Prerequisites

- **C++ Compiler** (e.g., g++, clang++)
- **Windows Operating System** (required due to Windows-specific library used)

### Build and Run

```sh
# Clone the repository
git clone https://github.com/erfan-tahvilian/compiler-lexer-parser.git
cd compiler-lexer-parser

# Compile the project
g++ -o compiler.exe main.cpp -std=c++17

# Run the program
compiler.exe
```

## Usage

1. **Provide an input grammar file** (e.g., `Grammars/grammar.txt`).
2. **Choose a parser type** from the interactive menu.
3. **Enter test strings** to analyze their parsing results.
4. **View generated parsing tables and error reports.**

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for more details.

## Support & Donations

If you find this project useful and would like to support further development, you can donate via:

- **Tron (TRX) Address:** `TL8WyZLLGu8UraHAT8dCoeMsnzGpX2oAYn`
- **Binance Coin (BNB - BEP20) Address:** `0xeC4F61F21238685cC842bC236D86684e5fc2Bc57`
- **Bitcoin (BTC) Address:** `bc1q2y9m8y02fqgsr4c6533duwmqtmrhadc8k8mkt4`

Your support is greatly appreciated! 🚀