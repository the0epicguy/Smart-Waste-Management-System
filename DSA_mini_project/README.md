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
- Sort and queue bins by **shortest road distance from the depot** (Dijkstra)
- Dispatch trucks along the road-optimal route, collecting all bins in a target area in a single trip
- Simulate time passage to watch fill levels rise and urgency change dynamically

---

## Architecture
┌─────────────────────────────────────────────┐
│               GTK+ Frontend                  │
│                                             │
│  Dashboard │ Bins Overview │ Queues         │
│  Analytics │ Simulator                      │
│                                             │
│  gui.c  │  gui_callbacks.c  │  gui_helpers.c│
└──────────────────┬──────────────────────────┘
│  calls
┌──────────────────▼──────────────────────────┐
│               Core Logic  (main.c)           │
│                                             │
│  Dustbin linked list                        │
│  AreaGraph (adjacency matrix, V ≤ 20)       │
│  Dijkstra shortest path                     │
│  Priority queue  (urgent bins, fill ≥ 90%)  │
│  Normal queue    (all other bins)           │
│  Truck dispatch simulation                  │
└─────────────────────────────────────────────┘

---

## Data Structures

| Structure | Role |
|-----------|------|
| `Dustbin` (linked list) | Master store of all bins — ID, area, fill level, distance, priority score |
| `AreaGraph` (adjacency matrix) | City road network — area nodes + weighted edges (road km) |
| `priorityqueue` (sorted linked list) | Urgent bins (fill ≥ 90%), ordered by priority score |
| `queue` (FIFO linked list) | Normal bins, enqueued in Dijkstra road-distance order |

### Why a Graph instead of a BST?

The previous version built a **Binary Search Tree** on bin distance values, traversed it in-order to get a sorted list, copied that list into queues, then freed the BST — every single time sorting was needed. This had two problems:

1. **It sorted individual bins by straight-line distance**, not by which *area* the truck should visit first on the actual road network.
2. **The BST was a temporary structure** rebuilt and discarded on every call, wasting allocation cycles with no structural benefit over a simple sort.

The graph replaces both the BST and the old `AreaDistance` linked list with a single persistent `AreaGraph`. Dijkstra runs once per dispatch to produce a road-optimal area visit order, which is then used to fill the queues. The result is a more realistic and more efficient routing engine.

---

## Routing Algorithm
graph_dijkstra(src=DEPOT, dist[], prev[])
│
├─ Finds shortest road path from depot to every area node
├─ Areas sorted by road distance (insertion sort, V ≤ 20)
└─ Bins enqueued area-by-area in that order
├─ fill ≥ 90% → priority queue
└─ fill  < 90% → normal queue
simulateTruckCollection()
│
├─ Pop highest-priority target (priority queue first, else normal)
├─ Re-run Dijkstra → get road km to target area
├─ Collect ALL bins in that area in one trip
└─ Compute: travel out + loading + return = total route time

---

## Pune Road Network (built-in graph)

The system ships with an approximate Pune area road graph used during `initializeRandomBins()`:
DEPOT ──3km── Shivajinagar ──4km── Koregaon Park ──6km── Viman Nagar
│                │                     │                    │
4km              3km                   9km                  5km
│                │                     │                    │
Swargate ──4km── Camp          Kharadi ──7km── Hadapsar ──8km── Swargate
Shivajinagar ──5km── Kothrud ──6km── Baner ──5km── Hinjewadi

Edge weights are approximate road kilometres. Dijkstra finds the true shortest path even when a direct depot edge is longer than a multi-hop route.

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
DSA_mini_project/
├── build/
│   └── smartwaste.exe          # Compiled binary
├── include/
│   ├── core.h                  # All structs, graph API, extern declarations
│   └── gui.h                   # GTK widget externs and callback prototypes
└── src/
├── main.c                  # All core logic: graph, queues, simulation, main()
├── gui.c                   # GTK window, tabs, CSS theming, analytics chart
├── gui_callbacks.c         # Button/event handlers — calls core API
└── gui_helpers.c           # Tree view refresh functions

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
gcc main.c gui.c gui_callbacks.c gui_helpers.c \
    -I../include \
    -o ../build/smartwaste.exe \
    `pkg-config --cflags --libs gtk+-3.0`
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
| **Bins Overview** | Sortable table of all bins with ID, area, distance, fill %, and status |
| **Collection Queues** | Separate views for urgent (priority) and normal queues |
| **Analytics Chart** | Interactive bar chart — click a bar to see per-category bin counts |
| **Truck Simulator** | Dispatches truck along Dijkstra-optimal route; animated progress bar |
| **Time Simulation** | Randomly increments fill levels to simulate real-world passage of time |
| **Dark Mode** | Toggle between light and dark GTK themes |
| **Event Log** | Timestamped log of every truck dispatch in the Simulator tab |

---

## Bin Priority System

| Status | Fill Level | Queue |
|--------|-----------|-------|
| URGENT | ≥ 90% | Priority queue — dispatched first |
| HIGH | 70–89% | Normal queue |
| MEDIUM | 50–69% | Normal queue |
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
