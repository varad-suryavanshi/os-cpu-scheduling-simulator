# 🧠 CPU Scheduling Simulator - C++

This project simulates various **CPU scheduling algorithms** as part of an Operating Systems Lab assignment. It models how processes are scheduled under different strategies like FCFS, LCFS, SRTF, RR, PRIO, and PREPRIO.

The simulation reads process information and a random number file, and outputs metrics such as turnaround time, wait time, I/O and CPU utilization.

---

## 📌 Features

- Simulates multiple scheduling algorithms:
  - FCFS (First Come First Serve)
  - LCFS (Last Come First Serve)
  - SRTF (Shortest Remaining Time First)
  - RR (Round Robin)
  - PRIO (Priority Scheduling)
  - PREPRIO (Preemptive Priority Scheduling)
- Calculates:
  - CPU and I/O utilization
  - Average turnaround and wait time
  - Process state transitions
- Supports testing automation and grading scripts

---

## 📁 Directory Structure

```
.
├── Lab2.cpp           # Main simulation source code
├── Makefile           # Build script
├── runit.sh           # Runs simulations on all test cases
├── gradeit.sh         # Compares outputs with reference outputs
├── Inputs/            # Contains input files (e.g., input0, input1, ...)
├── refout/            # Reference output files
├── .gitignore         # Git ignore rules
```

---

## ⚙️ Build Instructions

To compile the simulator:

```bash
make
```

This will generate the simulator binary based on your Makefile rules.

---

## ▶️ How to Run the Simulator

You can run the simulation manually or use provided scripts:

---

### 🔁 `runit.sh` – Run all test cases

**Usage:**

```bash
./runit.sh <output-directory> <scheduler-spec>
```

**Example:**

```bash
mkdir -p Outputs
./runit.sh Outputs ./sched -v
```

The script will run all combinations of input files and schedulers and save the outputs in the specified directory.

> You can control schedulers with flags like `-sF`, `-sR5`, `-sP2:4`, `-sE2:5`, etc.

---

### 📊 `gradeit.sh` – Grade your outputs

This script compares your simulator’s outputs to reference outputs using `diff`.

**Usage:**

```bash
./gradeit.sh refout Outputs
```

This will print a summary of how many outputs matched the reference and log mismatches in `Outputs/LOG.txt`.

---

## 🧪 Example Test Run

```bash
make
mkdir -p Outputs
./runit.sh Outputs ./sched -v
./gradeit.sh refout Outputs
```

---

## 📝 Notes

- Make sure the simulator binary (e.g., `sched`) is executable.
- Input files and reference outputs must match expected naming conventions (e.g., `input0`, `out_0_F`, etc.).

---
