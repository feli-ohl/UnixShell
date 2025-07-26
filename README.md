# 🐚 Custom Unix-like Shell with Job Control

**Student:** Felipe Oehler Guzmán  
**Course:** 2ºB Computer Science  
**Term:** Season 2024-2025  
**Department:** Architecture of Computers, University of Malaga (UMA)

---

## 📖 Overview

This project implements a custom Unix-like command-line shell with job control functionalities in C. The shell allows users to:

- Execute commands  
- Manage foreground and background jobs  
- Use built-in shell commands  
- Perform input/output redirection

It simulates core job control features found in modern Unix shells and provides a framework for understanding:

- Process management  
- Signal handling  
- Command execution

---

## ✨ Features

- ✅ **Basic Command Execution**: Run any system command like a standard Unix shell.
- 🔄 **Foreground/Background Jobs**: Use `&`, `fg`, and `bg` to control jobs.
- 🧠 **Job Control**: Monitor, resume, and terminate jobs using process groups.
- ⚠️ **Signal Handling**: Handles `SIGCHLD`, `SIGTSTP`, `SIGCONT`, `SIGINT`, etc.
- 🧰 **Built-in Commands**:
  - `cd [path]`: Change directory (defaults to `$HOME`).
  - `jobs`: List background or stopped jobs.
  - `fg [pos]`: Bring job to foreground (default: first job).
  - `bg [pos]`: Resume stopped job in background (default: first job).
  - `currjob`: Prints information about the current job in the job list.
  - `deljob`: Deletes the current job from the job list if it is running in background.
  - `zjobs`: Lists all zombie child processes.
  - `bgteam [N]`: Launches N background jobs running the specified command.
  - `fico`: Runs the filecount.sh cript.
  - `mask [sig]`: Allows running a command with the sig signal blocked.
  - `exit`: Exit the shell cleanly.
- 🔁 **I/O Redirection**:
  - Input: `< input.txt`
  - Output: `> output.txt`
  - Append: `>> output_append.txt`

---

## ⚙️ How to Build and Run

### Requirements

- Unix-like OS (Linux/macOS)
- GCC and GNU Make recommended
- Files required:
  - `shell.c`
  - `job_control.c`
  - `job_control.h`

### Compilation

```bash
gcc job_control.c shell.c -o MYSHELLOUTPUT
./MYSHELLOUTPUT
