# SystemMonitor

A comprehensive native system monitoring application for Haiku.

## Features

- **Performance Overview**:
  - Real-time interactive graphs for CPU, Memory, Network, Disk, and GPU usage.
  - Sidebar summary card views displaying live metrics and mini-graphs.
  - Configurable refresh intervals (0.5s, 1s, 2s, 5s) with automatic graph scaling.
- **Process Manager**:
  - Detailed list of active processes and teams with process ID, name, CPU usage, memory usage, state, priority, and user.
  - Fast real-time filtering by process name or PID.
  - Management actions via context menu or keyboard shortcuts (Kill, Suspend, Resume, Change Priority).
  - Permission validation preventing unauthorized process termination or priority modification.
  - Highly optimized thread scanning that skips expensive system calls for non-visible filtered items.
- **File Systems Tab**:
  - Detailed breakdown of mounted volumes, device paths, mount points, file system types, total space, free space, and usage progress bars.
  - Bidirectional column sorting with clickable header views.
- **Network Interfaces Tab**:
  - Real-time network interface metrics (bytes/packets sent and received, link speed, loopback detection).
- **System Details**:
  - Detailed hardware and software summary including Haiku OS version, kernel release, CPU brand & features, RAM utilization, swap usage, battery status, IP address, uptime, and installed package count.
- **Dark Mode & Native Haiku API Integration**:
  - Dynamic system color adaptation using `ui_color` for full dark mode support.
  - Built with Haiku Layout API, BListView, BColumnListView, and messaging architecture.

## Building

To build SystemMonitor:

```bash
./build.sh
```

Alternatively, compile directly with `make` inside the `SystemMonitor` directory:

```bash
cd SystemMonitor
make
```

## Running Tests & Benchmarks

SystemMonitor includes a test suite and performance benchmark suite running against Haiku API mocks:

```bash
./run_test.sh
```

This executes test targets covering:
- Unit tests: `CircularBuffer`, `DataHistory`, `Utils::FormatBytes`, `ProcessListItem`, `DiskListItem`, `NetworkInfo`, string safety, and bounds checks.
- Performance benchmarks: Thread scanning, visibility filtering, priority setting, disk polling, circular buffer access, and string filtering.

## License

Distributed under the MIT License.
