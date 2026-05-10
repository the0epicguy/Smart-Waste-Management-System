# ♻️ Smart Waste Management System

> A GTK-based desktop application for real-time urban waste bin monitoring, priority-based alerting, and graph-routed truck dispatch optimization.

---

## Overview

The **Smart Waste Management System** is a C/GTK+ desktop application that helps municipal administrators monitor dustbin fill levels across city areas, receive urgency alerts, and dispatch collection trucks along the most efficient road routes.

The system models the city's road network as a **weighted graph** and uses **Dijkstra's shortest path algorithm** to determine the optimal truck route from the depot to each collection area — replacing a naive crow-fly distance sort with true road-network awareness.

---

## Problem Statement

Traditional waste management relies on fixed schedules and manual inspection, leading to:

- Overflowing bins and deteriorating public hygiene
- Fuel waste from suboptimal collection routes
- No real-time visibility into which bins are critical

---

## Solution

The system provides a live dashboard where administrators can:

- Monitor all bins by fill level, area, and urgency status
- Receive automatic URGENT alerts for bins at or above 90% capacity
- Sort and queue bins by **shortest road distance from the depot** using Dijkstra's algorithm
- Dispatch trucks along the road-optimal route, collecting all bins in a target area in a single trip
- Simulate time passage to watch fill levels rise and urgency change dynamically

---

## Architecture

The application is split into a GTK+ frontend and a pure-C core logic layer.

**Frontend (GTK+)**
- `gui.c` — Window creation, tabs, CSS theming, analytics chart
- `gui_callbacks.c` — Button and event handlers
- `gui_helpers.c` — Tree view refresh functions

**Core Logic (`main.c`)**
- Dustbin linked list
- AreaGraph (adjacency matrix, V ≤ 20)
- Dijkstra shortest path
- Priority queue for urgent bins (fill ≥ 90%)
- Normal queue for all other bins
- Truck dispatch simulation

The frontend calls into the core exclusively through the API declared in `core.h`. No core file imports any GTK header.

---

## Data Structures

| Structure | Type | Role |
|-----------|------|------|
| `Dustbin` | Linked list | Master store of all bins — ID, area, fill level, distance, priority score |
| `AreaGraph` | Adjacency matrix | City road network — area nodes and weighted edges in road km |
| `priorityqueue` | Sorted linked list | Urgent bins (fill ≥ 90%), ordered by priority score |
| `queue` | FIFO linked list | Normal bins, enqueued in Dijkstra road-distance order |

### Why a Graph instead of a BST?

The previous version built a **Binary Search Tree** on bin distance values, traversed it in-order, copied the result into queues, then freed the BST — every single time sorting was needed. This had two problems:

1. It sorted individual bins by **straight-line distance**, not by which area the truck should visit first on the actual road network.
2. The BST was a **temporary throwaway structure** — rebuilt and discarded every call, with no lasting value.

The graph replaces both the BST and the old `AreaDistance` linked list with a single persistent `AreaGraph`. Dijkstra runs once per dispatch to produce a road-optimal area visit order, which then fills the queues. The result is a more realistic and more efficient routing engine.

---

## Routing Algorithm

**Step 1 — Dijkstra from DEPOT**

Finds the shortest road path from the depot to every area node in the graph.

**Step 2 — Sort areas by road distance**

Areas that contain bins are sorted by their Dijkstra road distance (insertion sort, V ≤ 20).

**Step 3 — Fill queues**

Bins are enqueued area-by-area in road-optimal order. Bins with fill ≥ 90% go to the priority queue; all others go to the normal queue.

**Step 4 — Dispatch**

The truck pops the highest-priority target (priority queue first, then normal queue), re-runs Dijkstra to get the exact road distance, collects all bins in that area in one trip, then computes total route time as: travel out + loading + return.

---

## Pune Road Network

The system ships with an approximate Pune area road graph. Key connections:

| From | To | Road km |
|------|----|---------|
| DEPOT | Shivajinagar | 3 |
| DEPOT | Swargate | 4 |
| DEPOT | Kothrud | 6 |
| DEPOT | Camp | 6 |
| DEPOT | Koregaon Park | 5 |
| Shivajinagar | Koregaon Park | 4 |
| Shivajinagar | Swargate | 3 |
| Shivajinagar | Kothrud | 5 |
| Koregaon Park | Viman Nagar | 6 |
| Koregaon Park | Kharadi | 9 |
| Viman Nagar | Kharadi | 5 |
| Kharadi | Hadapsar | 7 |
| Hadapsar | Swargate | 8 |
| Swargate | Camp | 4 |
| Kothrud | Baner | 6 |
| Kothrud | Hinjewadi | 8 |
| Baner | Hinjewadi | 5 |

Dijkstra finds the true shortest path even when a direct depot edge is longer than a multi-hop route through intermediate areas.

---

## Tech Stack

| Category | Technology |
|----------|------------|
| Language | C (C11) |
| GUI Framework | GTK+ 3 |
| Build Tool | GCC / MinGW |
| Graph Algorithm | Dijkstra (O(V²), V ≤ 20) |
| Platform | Linux / Windows |

---

## Project Structure

| Path | Description |
|------|-------------|
| `build/smartwaste.exe` | Compiled binary |
| `include/core.h` | All structs, graph API, extern declarations |
| `include/gui.h` | GTK widget externs and callback prototypes |
| `src/main.c` | All core logic: graph, queues, simulation, entry point |
| `src/gui.c` | GTK window, tabs, CSS theming, analytics chart |
| `src/gui_callbacks.c` | Button and event handlers — calls core API |
| `src/gui_helpers.c` | Tree view refresh functions |

---

## Build & Run

### Prerequisites

- GCC or MinGW
- GTK+ 3 development libraries

**Linux:**
```bash
sudo apt install libgtk-3-dev
```

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-gtk3
```

### Compile

From the `src/` directory:

```bash
gcc main.c gui.c gui_callbacks.c gui_helpers.c -I../include -o ../build/smartwaste.exe `pkg-config --cflags --libs gtk+-3.0`
```

### Run

```bash
cd ../build
./smartwaste.exe
```

---

## Features

| Feature | Description |
|---------|-------------|
| **Live Dashboard** | System status, legend, and all action controls in one view |
| **Bins Overview** | Table of all bins with ID, area, distance, fill %, and status |
| **Collection Queues** | Separate views for urgent (priority) and normal queues |
| **Analytics Chart** | Interactive bar chart — click a bar to see per-category bin counts |
| **Truck Simulator** | Dispatches truck along Dijkstra-optimal route with animated progress |
| **Time Simulation** | Randomly increments fill levels to simulate passage of time |
| **Dark Mode** | Toggle between light and dark GTK themes |
| **Event Log** | Timestamped log of every truck dispatch in the Simulator tab |

---

## Bin Priority System

| Status | Fill Level | Queue |
|--------|------------|-------|
| URGENT | ≥ 90% | Priority queue — dispatched first |
| HIGH | 70 – 89% | Normal queue |
| MEDIUM | 50 – 69% | Normal queue |
| LOW | < 50% | Normal queue |

Priority score formula: `priority = (fillLevel × 2) − (distance × 5)`

---

## Future Scope

- Live IoT sensor integration via MQTT or serial interface
- Google Maps API overlay for visual route display
- Cloud sync for multi-depot fleet management
- Predictive fill-level forecasting using historical data
- Mobile companion app for field workers

---

## License

This project is licensed under the **MIT License** — free to use, modify, and distribute.

---

## Acknowledgements

Built as a DSA mini-project with a focus on applying graph theory to a real-world urban infrastructure problem. Thanks to our faculty mentors for their guidance throughout the development process.
