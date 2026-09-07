# SystemMonitor

A comprehensive native system monitoring application for Haiku OS.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

## Overview

SystemMonitor provides real-time system performance tracking, process management, file system stats, network interface monitoring, and system details for Haiku OS.

## Key Features

- **Performance Dashboard**:
  - Real-time performance charts for CPU core utilization, Memory (RAM & Swap), Network activity, Disk I/O, and GPU usage.
  - Live summary sidebar with mini-charts and quick metrics.
  - Configurable update frequencies (0.5s, 1s, 2s, 5s).
- **Process Manager**:
  - Complete list of running processes with PID, Name, CPU %, Memory, State, Priority, and User ID.
  - Real-time search/filter bar for fast process lookup.
  - Process controls: Kill, Suspend, Resume, and Priority adjustment via context menu or key shortcuts (Delete key).
  - Permission checks enforcing root / user ID boundaries before attempting process actions.
  - Optimized update loop skipping thread/memory system calls for non-visible filtered items.
- **File Systems Tab**:
  - Information on mounted volumes, mount points, file system types, disk capacity, and free space progress bars.
  - Interactive bidirectional column sorting.
- **Network Tab**:
  - Monitor network interfaces, upload/download bandwidth, link status, and packet statistics.
- **System Info Tab**:
  - System hardware specifications, Haiku OS version & revision (`hrev`), CPU brand/features, RAM allocation, battery status, IP address, uptime, and package count.
- **Dark Mode & UI Integration**:
  - Dynamic adaptation to system theme colors (`ui_color`) supporting standard light and dark mode themes.

## Project Structure

```
.
├── SystemMonitor/          # Application source code
│   ├── ActivityGraphView.*  # Performance graph component
│   ├── CPUView.*            # CPU graph & core utilization
│   ├── MemView.*            # Memory & Swap usage
│   ├── DiskView.*           # File systems & disk tracking
│   ├── NetworkView.*        # Network interface stats
│   ├── ProcessView.*        # Process manager & filtering
│   ├── SystemDetailsView.*  # Hardware & OS information
│   ├── Utils.*              # Formatting, system info helpers
│   ├── tests/               # Unit tests & benchmark suite
│   └── Makefile             # Application Makefile
├── build.sh                 # Build script
├── run_test.sh              # Test & benchmark launcher
├── ANALYSIS_REPORT.md       # Codebase audit & architecture report
└── LICENSE                  # MIT License
```

## Building

To compile SystemMonitor, run the build script:

```bash
./build.sh
```

Or build directly using `make`:

```bash
cd SystemMonitor
make
```

## Running Tests & Benchmarks

To execute the test suite and performance benchmarks:

```bash
./run_test.sh
```

The test runner compiles and executes mock-based unit tests (`test_circular_buffer`, `test_data_history`, `test_utils`, `test_format_bytes`, `test_process_list_item`, `test_disk_list_item`, `test_network_info`, `test_strlcpy_bounds`, etc.) and performance benchmarks (`benchmark_thread_scanning`, `benchmark_process_view_visibility`, `benchmark_priority_set`, `benchmark_disk_poll`, `benchmark_battery_cache`, etc.).

## License

This project is licensed under the [MIT License](LICENSE).
