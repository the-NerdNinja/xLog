# The Solo Leveling System: xLog

A command-line task tracker that transforms your daily life into a ***Solo-Leveling Game***.

## Table of Contents

- [Introduction](#introduction)
- [Features](#features)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Installation](#installation)
- [Usage](#usage)
  - [Command‑Line Options](#command-line-options)
  - [Examples](#examples)
- [Leveling System](#leveling-system)
  - [Ranks & XP Thresholds](#ranks--xp-thresholds)
  - [Task Types & Base XP](#task-types--base-xp)
  - [XP Modifiers](#xp-modifiers)
- [Contributing](#contributing)
- [License](#license)

---

## Introduction

**xLog** is a command-line–based task management tool that turns everyday responsibilities into an immersive, RPG-style progression system. Your life is divided into **four domains**, each comprised of **four distinct elements**, allowing precise tracking of personal growth. Completing tasks awards XP to both a **major** and a **minor** element, offering clear visibility into how you develop habits and skills over time.

Embodying the spirit of **solo leveling**, xLog emphasizes that the greatest challenge is overcoming your own limits—it’s finally **you vs. you**.

## Features

- **CLI-First**: Lightweight, dependency-free command-line interface.
- **Persistent Storage**: Robust SQLite backend for tasks and XP.
- **Progression Ranks**: Eight levels from **Rookie** to **Legend** with color-coded badges.
- **Task Categories**: Support for **Quick**, **Session**, and **Grind** tasks.
- **Element Focus**: Optionally mark an element as focus to earn a **10% XP bonus**.
- **Streak Mechanics**: Up to **+20% XP** for consecutive completions, plus **–40% XP** penalty for overdue tasks.
- **Daily Login Bonus**: Make sure to name a Element "Discipline" to get a daily login bonus.!

## Getting Started

### Prerequisites

- C++17–compatible compiler (e.g., GCC, Clang)
- SQLite3 development library and headers
- CMake or Make for build automation

### Installation

Clone the repository and run the installer script:

```bash
git clone https://github.com/yourusername/xLog.git
cd xLog
chmod +x install.sh      # if needed
./install.sh             # builds, installs binary to /usr/local/bin/xlog, and creates $HOME/xLog
```

## Usage

Invoke `xlog` without arguments to view your profile summary. Use the following commands to manage and complete tasks:

### Command-Line Options

| Command                         | Description                                    |
|---------------------------------|------------------------------------------------|
| `xlog`                          | Show profile overview                          |
| `xlog profile`                  | Display current rank, total XP, and progress   |
| `xlog today`                    | List tasks due today                            |
| `xlog create`                   | Interactive prompt to add a new task           |
| `xlog delete <task_name>`       | Remove a task by name                          |
| `xlog list`                     | View all tasks                                 |
| `xlog enable <task_name>`       | Enable a paused or disabled task               |
| `xlog pause <task_name>`        | Temporarily disable a recurring task           |
| `xlog done <names...>`          | Mark one or more tasks as complete             |
| `xlog info <domain>`            | Show detailed stats for a specific domain      |
| `xlog focus <element>`          | Toggle focus mode on an element                |
| `xlog quick <maj> <min>`        | Manually grant XP for a Quick task             |
| `xlog session <maj> <min>`      | Manually grant XP for a Session task           |
| `xlog grind <maj> <min>`        | Manually grant XP for a Grind task             |

### Examples

```bash
# Complete a task named "Morning Run"
xlog done "Morning Run"

# Create a new task
xlog create
# Follow prompts to set name, type, frequency, and elements.

# Toggle focus on element "strength"
xlog focus strength
```

## Leveling System

### Ranks & XP Thresholds

| Rank | Name        | Color  | Profile XP Needed | Approx. Time (@5h/day) |
|-----:|-------------|--------|-------------------|------------------------:|
|    0 | Rookie      | White  | 0                 | 0 days                  |
|    1 | Explorer    | Gray   | 1,712             | 1 month                 |
|    2 | Crafter     | Yellow | 6,847             | 3 months                |
|    3 | Strategist  | Orange | 15,408            | 7 months                |
|    4 | Expert      | Green  | 27,387            | 1 year 2 months         |
|    5 | Architect   | Blue   | 42,782            | 2 years                 |
|    6 | Elite       | Purple | 61,594            | 3 years                 |
|    7 | Master      | Red    | 83,823            | 4 years                 |
|    8 | Legend      | Black  | 109,500           | 5 years                 |

### Task Types & Base XP

| Type    | Major Base XP | Minor Base XP |
|:--------|--------------:|--------------:|
| Quick   | 10            | 5             |
| Session | 60            | 30            |
| Grind   | 125           | 75            |

### XP Modifiers

- **Focus Bonus:** +10% if the element is marked as focus.
- **Streak Bonus:** +1% per consecutive day (capped at +20%).
- **Overdue Penalty:** –40% if completing after the scheduled date.

*Example:* A `Session` task with base 60 XP, 15-day streak, in focus:
```
60 × (1 + 15/100) × 1.1 ≈ 75.9 → 76 XP awarded
```

## Contributing

Contributions are welcome! To propose changes:

1. Fork the repository.
2. Create a feature branch: `git checkout -b feature/YourFeature`
3. Commit your improvements: `git commit -m 'Add feature description'`
4. Push to your branch: `git push origin feature/YourFeature`
5. Open a pull request.

## License

Distributed under the MIT License. See [LICENSE.md](LICENSE.md) for details.

*Happy Questing!*


