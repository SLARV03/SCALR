<div align="center">

```
 ███████╗ ██████╗ █████╗ ██╗     ██████╗
 ██╔════╝██╔════╝██╔══██╗██║     ██╔══██╗
 ███████╗██║     ███████║██║     ██████╔╝
 ╚════██║██║     ██╔══██║██║     ██╔══██╗
 ███████║╚██████╗██║  ██║███████╗██║  ██║
 ╚══════╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝
```

**LR Parser Table Generator**

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue?style=flat-square&logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![Python](https://img.shields.io/badge/Python-3.8+-yellow?style=flat-square&logo=python)](https://www.python.org/)
[![CMake](https://img.shields.io/badge/CMake-3.15+-red?style=flat-square&logo=cmake)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green?style=flat-square)](LICENSE)

*Given a context-free grammar, SCALR produces ACTION/GOTO parsing tables using LR(0), SLR(1), CLR(1), or LALR(1) — with a full GUI, parse tree visualiser, and conflict analyser.*

</div>

---

## 👥 Collaborators

| | Name | Role |
|---|---|---|
| 🧑‍💻 | **Raj** | Core C++ Backend & LR Table Generation |
| 🧑‍💻 | **Pratyush** | Parser Algorithms & Set Computation |
| 👩‍💻 | **Palak** | Parse Tree Visualiser & Heatmap |
| 👩‍💻 | **Diya** | GUI Design & Frontend Integration |

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Pipeline](#-pipeline)
- [Features](#-features)
- [Running the App](#-running-the-app)
- [Grammar Format](#-grammar-format)
- [Architecture](#-architecture)
- [The Four Methods](#-the-four-methods)
- [GUI Walkthrough](#-gui-walkthrough)
- [JSON Output Format](#-json-output-format)

---

## 🔍 Overview

SCALR is a **C++ + Python** tool that automates the construction of LR parsing tables from a context-free grammar. It supports all four standard LR parsing strategies and exports results as structured JSON consumed by an interactive Python GUI.

**Example grammar input:**
```
E -> E + T
E -> T
T -> T * F
T -> F
F -> ( E )
F -> id
```

SCALR will augment the grammar, compute FIRST/FOLLOW sets, build the canonical item-set collection, and produce a complete ACTION/GOTO table — all in milliseconds.

---

## 🔄 Pipeline

```
 ┌─────────────────────────────────────────────────────────────────────┐
 │                        SCALR PIPELINE                               │
 └─────────────────────────────────────────────────────────────────────┘

  Raw Grammar Text
  (stdin / GUI)
        │
        ▼
  ┌─────────────┐     parse()          ┌──────────────────────────────┐
  │             │──────────────────────▶  1. Line-by-line rule parsing │
  │  MetaParser │     classify()       │  2. Terminal classification   │
  │             │──────────────────────▶  3. Grammar augmentation S'→S │
  └─────────────┘                      └──────────────────────────────┘
        │
        ▼
  ┌──────────────┐   computeFirst()    ┌──────────────────────────────┐
  │              │──────────────────────▶  FIRST(X) for all symbols   │
  │ SetGenerator │   computeFollow()   │  FOLLOW(A) for non-terminals │
  │              │──────────────────────▶  Fixed-point iteration        │
  └──────────────┘                      └──────────────────────────────┘
        │
        ▼
  ┌─────────────────────────────────────────────────────────┐
  │                   Method Selection                       │
  └──────┬──────────┬──────────────┬───────────────────────┘
         │          │              │              │
         ▼          ▼              ▼              ▼
    ┌────────┐ ┌────────┐   ┌──────────┐  ┌──────────┐
    │  LR(0) │ │ SLR(1) │   │  CLR(1)  │  │ LALR(1)  │
    │        │ │        │   │          │  │          │
    │ LR(0)  │ │ LR(0)  │   │  LR(1)   │  │ LR(1) →  │
    │ items  │ │ items  │   │  items   │  │  merge   │
    │        │ │FOLLOW  │   │ exact LA │  │ union LA │
    └────┬───┘ └────┬───┘   └────┬─────┘  └────┬─────┘
         └──────────┴────────────┴──────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │   JsonExporter   │
                    │                  │
                    │  ACTION + GOTO   │
                    │  table as JSON   │
                    │  → stdout        │
                    └────────┬─────────┘
                             │
                             ▼
                    ┌──────────────────┐
                    │  scalr_gui2.py   │
                    │                  │
                    │  • Results tab   │
                    │  • Analytics     │
                    │  • Parse Tree    │
                    │  • Heatmap       │
                    └──────────────────┘
```

---

## ✨ Features

- **4 LR parsing strategies** — LR(0), SLR(1), CLR(1), LALR(1) — run all at once or individually
- **Automatic grammar augmentation** — inserts S' → S as rule 0
- **FIRST & FOLLOW computation** — fixed-point iteration with epsilon support
- **Conflict detection & classification** — S/R and R/R conflicts reported per state/symbol
- **Parser recommendation** — automatically suggests the simplest conflict-free strategy
- **Interactive GUI** — built with CustomTkinter on a dark GitHub-style theme
- **Parse trace** — step-by-step stack/input/action table for any input string
- **Parsing Heatmap** — colour-coded reduction count per token
- **Parse Tree visualiser** — scrollable canvas tree reconstructed from shift/reduce trace
- **JSON export** — structured output for tooling integration

---

## 🚀 Running the App

### Prerequisites

- **C++ compiler** supporting C++20 (GCC 10+, Clang 12+, MSVC 2019+)
- **CMake** 3.15+
- **Python** 3.8+ with `customtkinter` installed:
  ```bash
  pip install customtkinter
  ```

### Build & Run

**1. Generate the build system using CMake:**

```bash
# Default (Linux / macOS / Windows with MSVC)
cmake -S . -B build

# Windows with MSYS2 / MinGW
cmake -G "MinGW Makefiles" -S . -B build
```

**2. Build the project:**

```bash
cmake --build build --config Release
```

**3. Start the GUI:**

```bash
python scalr_gui2.py
```

**4.** Enter your grammar via the **Editor** tab, or use the pre-filled example grammar.

### CLI Usage (without GUI)

```bash
echo "E -> E + T
E -> T
T -> T * F
T -> F
F -> ( E )
F -> id" | ./scalr CLR1
```

Replace `CLR1` with `LR0`, `SLR1`, or `LALR1` as needed. Output is JSON on stdout.

---

## 📝 Grammar Format

| Rule | Description |
|------|-------------|
| `A -> B C` | Production with symbols separated by spaces |
| `A -> B \| C` | Multiple alternatives on one line |
| `A -> epsilon` | Explicit epsilon production |
| `A -> ε` | Unicode epsilon also accepted |

**Important:** Every symbol is whitespace-delimited. `(`, `)`, `+`, `*`, `id` are each one symbol.

```
# Example: Classic expression grammar
E -> E + T | T
T -> T * F | F
F -> ( E ) | id
```

The first rule's LHS becomes the **start symbol**. SCALR automatically adds the augmented rule `E' -> E` as rule 0.

---

## 🏗️ Architecture

```
SCALR/
├── src/
│   ├── main.cpp            ← Entry point & pipeline orchestration
│   ├── MetaParser.cpp      ← Grammar text → Grammar object
│   ├── SetGenerator.cpp    ← FIRST & FOLLOW set computation
│   ├── LRCommon.cpp        ← Closure, GOTO, canonical collection
│   ├── LR0.cpp             ← LR(0) table generator
│   ├── SLR1.cpp            ← SLR(1) table generator
│   ├── CLR1.cpp            ← CLR(1) table generator
│   ├── LALR1.cpp           ← LALR(1) table generator
│   └── JsonExporter.cpp    ← Parsing table → JSON serialiser
├── include/
│   ├── Shared_Structs.hpp  ← Rule, Item, Grammar, Action, ParsingTable
│   ├── MetaParser.hpp
│   ├── SetGenerator.hpp
│   ├── LRCommon.hpp
│   ├── LR0.hpp / SLR1.hpp / CLR1.hpp / LALR1.hpp
│   └── JsonExporter.hpp
├── scalr_gui2.py           ← Python GUI (CustomTkinter)
├── grammar_validator.py    ← Grammar & input string validation
├── CMakeLists.txt
└── README.md
```

### Core Data Structures (`Shared_Structs.hpp`)

| Struct | Fields | Purpose |
|--------|--------|---------|
| `Rule` | `id, lhs, rhs[], sourceLine` | One production rule |
| `Item` | `ruleId, dotPosition, lookahead` | LR(0)/LR(1) item |
| `Grammar` | `rules, terminals, nonTerminals, firstSets, followSets` | Complete grammar state |
| `Action` | `type (SHIFT/REDUCE/ACCEPT/ERROR), target` | Parsing table cell |
| `ParsingTable` | `actionTable, gotoTable, conflicts` | Final output structure |

---

## ⚙️ The Four Methods

```
┌─────────────────────────────────────────────────────────────────────┐
│                   COMPARISON OF LR METHODS                          │
├──────────┬──────────────┬──────────────┬────────────┬──────────────┤
│ Aspect   │    LR(0)     │   SLR(1)     │   CLR(1)   │  LALR(1)     │
├──────────┼──────────────┼──────────────┼────────────┼──────────────┤
│ Items    │ LR(0)        │ LR(0)        │ LR(1)      │ LR(1)→merge  │
│ Reduce   │ ALL terms+$  │ FOLLOW(A)    │ Exact LA   │ Merged LA    │
│ States   │ Fewest       │ = LR(0)      │ Most       │ = LR(0)      │
│ Power    │ Weakest      │ Better       │ Strongest  │ Near-CLR(1)  │
│ Conflicts│ Most         │ Some         │ Fewest     │ Rare         │
└──────────┴──────────────┴──────────────┴────────────┴──────────────┘

  LR(0) ──────────────────────────────────────────────► LALR(1)
  (weakest, most conflicts)              (practical sweet spot — used by Yacc/Bison)
```

### Why LALR(1) is the sweet spot

LALR(1) merges CLR(1) states that share the same **core** (same rules + dot positions, ignoring lookaheads), taking the union of lookahead sets. This gives it:
- The **same state count** as LR(0)/SLR(1) → small, efficient tables
- **CLR-quality lookaheads** → resolves almost as many conflicts as CLR(1)

The only downside: merging can occasionally introduce R/R conflicts that CLR(1) wouldn't have.

---

## 🖥️ GUI Walkthrough

The GUI has five tabs:

| Tab | Description |
|-----|-------------|
| **Editor** | Grammar input, input string, parser selector, run button, live log terminal |
| **Results** | Compatibility summary table across all 4 parsers + recommendation |
| **Analytics** | Side-by-side metrics: states, conflicts, timing with bar charts |
| **Detailed View** | Full parsing table, FIRST/FOLLOW sets, conflicts, parse trace for a selected parser |
| **Parse Tree** | Token heatmap + interactive scrollable parse tree canvas |

### Parsing Heatmap

Each input token is displayed as a coloured tile. The colour intensity encodes how many **reduction steps** occurred while that token was the lookahead:

```
  Cool green = few reductions        Warm red = many reductions
  ██ id      ██ +      ██ id      ██ *      ██ id
  1 reduc.   3 reduc.  1 reduc.   5 reduc.  1 reduc.
```

Tokens that triggered parse errors appear in **dark burgundy** with a red border.

---

## 📦 JSON Output Format

```json
{
  "status": "success",
  "meta": {
    "states": 12,
    "conflicts": 0,
    "time_ms": 1.23
  },
  "grammar_map": [
    { "id": 1, "rule": "E -> E + T", "line": 1 }
  ],
  "first_sets":  { "E": ["(", "id"], "T": ["(", "id"], "F": ["(", "id"] },
  "follow_sets": { "E": ["$", "+", ")"], "T": ["$", "+", "*", ")"] },
  "table": {
    "state_0": { "id": ["s5"], "(": ["s4"], "E": ["1"], "T": ["2"] },
    "state_1": { "$":  ["acc"], "+": ["s6"] }
  },
  "conflicts": []
}
```

**Action encoding:**

| String | Meaning |
|--------|---------|
| `"s5"` | SHIFT to state 5 |
| `"r3"` | REDUCE by rule 3 |
| `"acc"` | ACCEPT |
| `"1"` | GOTO state 1 (non-terminal transition) |

---

<div align="center">

**Built by Raj · Pratyush · Palak · Diya**

</div>
