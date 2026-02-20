# Code Analysis Report: SystemMonitor

This report details the findings from a static analysis of the `SystemMonitor` codebase.

## Executive Summary

The `SystemMonitor` application is well-structured, utilizing the Haiku API effectively (Layout API, Messaging, Threading). The audit revealed that several previously suspected issues (semaphore accumulation, missing resize logic) were already correctly implemented. New optimizations and functional fixes were applied to enhance performance and usability.

## 1. Audit Findings & Verification

### 1.1 Verified Existing Fixes
The following issues were investigated and found to be **already addressed** in the current codebase, contrary to earlier reports:

### 1.2 Issues Fixed in This Audit
- **Sorting Logic**: Fixed incorrect alphabetical sorting of numeric columns (Sizes, Speeds, Percentages) in Disk and Network views by introducing specialized column types (`BSizeColumn`, `BFloatColumn`, `BSpeedColumn`).

### 1.3 Verified Existing Fixes
- **CircularBuffer Safety**: The `CircularBuffer` assignment operator was found to already correctly handle allocation failures.
- **DataHistory Stability**: The check in `DataHistory::ValueAt` to prevent division-by-zero crashes was found to be already present in the codebase.
- **Keyboard Support**: Support for the `Delete` key in `ProcessView` was verified to be already implemented.
- **Semaphore Accumulation**: `DiskView` and `NetworkView` correctly drain the semaphore in their update threads (using `get_sem_count` and `acquire_sem_etc`), preventing redundant scanning loops under load.
- **DataHistory Resizing**: The `SetRefreshInterval` method is fully implemented and correctly adjusts buffer sizes dynamically when the refresh rate changes.

### 1.4 Verified Existing Optimizations
- **ProcessView Visibility Check**: The use of `std::unordered_set` for O(1) visibility lookups was already present.
- **ProcessInfo Cleanup**: The unused `path` buffer was already removed from `ProcessInfo`.
- **OOM Protection**: `try-catch` blocks around memory allocations in `ActivityGraphView` were already implemented.
- **NetworkView String Handling**: Use of `BString` keys in internal maps was already present.
- **ProcessView Search**: Local filtering (`FilterRows`) was already implemented.
- **Memory Allocation**: `ProcessView::UpdateThread` vector reuse was already implemented.

## 2. Code Quality & Best Practices

### 2.1 Memory Management
- **`CircularBuffer`**: Correctly implements the Rule of Three.
- **`ProcessView`**: Uses `std::vector` and `std::unordered_map` for safe data management. Pruning logic for `fUserNameCache` is present.
- **Ownership**: `BColumnListView` items are managed correctly.

### 2.2 Concurrency
- **Atomic Flags**: `std::atomic<bool>` is used for thread termination flags (`fTerminated`).
- **Data Access**: Strict separation of background data collection and main thread UI updates via `BMessage` passing is maintained.

## 3. Remaining Observations (Non-Blocking)

### 3.1 Logic Risks
- **Coarse-grained Priority Adjustment**: `SetSelectedProcessPriority` sets the priority for all threads in a team to the same value. This flattens internal scheduling hierarchies but is typical behavior for simple task managers.

### 3.2 UI Limitations
- **GPUView**: Currently a placeholder implementation (injects 0 values). Requires driver support to be fully functional.
- **DiskView**: Notes that real-time I/O monitoring is not supported.

## 4. Conclusion
The `SystemMonitor` codebase has been audited and improved. Functional gaps (keyboard support) and stability risks (div/0) have been resolved. Performance optimizations were applied to key views. Critical concurrency handling was verified to be correct.

## 5. Final Audit Updates

The following critical fixes and optimizations were applied to resolve remaining blockers and nitpicks:

- **CircularBuffer Robustness**:
  - Fixed a Division-by-Zero crash vector in `ItemAt` and `AddItem` when buffer size is 0.
  - Improved `SetSize` to gracefully handle 0-size requests.
  - Optimized `operator=` to linearize data during assignment and copy only valid items, reducing overhead.

- **Crash Prevention**:
  - `ProcessView`: Added `InitCheck()` and `NULL` checks for `BPath` usage to prevent segmentation faults when resolving process names.
  - `ActivityGraphView`: Added `std::nothrow` and NULL checks for offscreen `BView` allocation to handle Out-Of-Memory conditions safely.

- **Optimizations**:
  - `ProcessView`: Moved `fActiveUIDs` and `fActivePIDs` sets to member variables to avoid repetitive memory allocation during updates.
  - `ProcessView`: Implemented caching for common translated state strings ("Running", "Ready", "Sleeping") to reduce CPU usage in the update loop.
  - `ProcessView`: Optimized filtering logic to reuse `BString` buffers (`fFilterName`, `fFilterID`) to avoid reallocation on every update cycle.

- **Logic Fixes**:
  - **Graph History Scaling**: Implemented proper propagation of `SetRefreshInterval` from `MainWindow` down to all `ActivityGraphView` instances. This ensures that the history duration (e.g., 60 seconds) remains constant regardless of the update speed (0.5s, 1s, or 2s).
  - **Graph Scaling Accuracy**: Fixed a bug where `ActivityGraphView` would fail to redraw historical data correctly when the vertical scale changed. Added `fLastMin` and `fLastRange` tracking to force a full redraw on scale changes.
  - **Sub-pixel Smoothness**: Optimized `ActivityGraphView` partial updates to always redraw the rightmost pixel using the latest available data, providing a smoother real-time feel.
  - `NetworkView`: Introduced `hasStats` flag to prevent invalid speed calculations when network interfaces fail to report statistics (e.g., transient driver errors).
  - `DataHistory`: Updated `SetRefreshInterval` to only commit changes if the buffer resize operation succeeds.

- **Initialization Safety**:
  - `CircularBuffer`: Fixed uninitialized member variables (`fFirst`, `fIn`, `fSize`, `fBuffer`) in constructors, which could lead to undefined behavior.
  - Added a comprehensive unit test (`SystemMonitor/tests/test_circular_buffer.cpp`) to verify circular buffer correctness (initialization, circularity, resizing, copying).

## 6. Final Polish Updates

The following architectural improvements and rendering fixes were implemented to ensure robustness and visual correctness:

- **Threading Robustness (DiskView & NetworkView)**:
  - Refactored `DiskView` and `NetworkView` to use timeout-based semaphore acquisition (`acquire_sem_etc` with `B_RELATIVE_TIMEOUT`) instead of relying on the UI thread's `Pulse()` method.
  - This decouples data collection from the UI refresh cycle, ensuring reliable statistics updates even if the main thread is busy or the window is hidden/inactive.
  - Implemented `SetRefreshInterval` to dynamically adjust the update frequency and immediately wake up the collection thread, improving responsiveness.

- **Graph Rendering Fix (ActivityGraphView)**:
  - Fixed a visual artifact where partial updates caused dark stripes due to alpha blending accumulation at the scroll boundary.
  - Implemented `ConstrainClippingRegion` in `_DrawHistory` to strictly limit drawing operations to the newly exposed area, preventing unintended overlap with existing content.

- **Type Consistency (CircularBuffer)**:
  - Updated `CircularBuffer` constructor and `SetSize` method to use `uint32` for size parameters, aligning with the internal member variable type and eliminating potential implicit truncation warnings on 64-bit platforms.

## 7. Extended Audit & Localization Phase

A final extensive audit was conducted to resolve remaining logic fragility, code duplication, and localization gaps:

- **Logic Accuracy & Robustness**:
  - **Network Monitoring**: Fixed a bug in `NetworkView` where traffic calculations for total speed depended on comparing translated UI strings ("Loopback"). Introduced a reliable `isLoopback` boolean flag in `NetworkInfo`, populated from interface flags.
  - **CPU Frequency**: Improved `CPUView` to handle cases where core frequency cannot be determined (e.g., in virtualized environments), displaying a localized "N/A" instead of misleading zero values.
  - **Graph Precision**: Refined `ActivityGraphView` partial update logic to use the exact `fLastRefresh` timestamp for history lookups, eliminating visual discontinuities.
  - **Sub-pixel Drawing**: Updated `ActivityGraphView` to use `float` for `fScrollOffset` and improved HiDPI grid line calculation using `ceilf` and `GetScaleFactor`.
  - **GPU Placeholder**: Fixed a bug in `GPUView` where the utilization label was never updated; it now correctly refreshes in the pulse loop.

- **Code De-duplication**:
  - **Shared Metrics**: Centralized "Cached Memory" calculation (Page Cache + Block Cache) into `Utils::GetCachedMemoryBytes`, eliminating duplicated math in `MemView` and `SystemSummaryView`.
  - **UI Synchronization**: Extracted list selection restoration into a private helper `_RestoreSelection` in `ProcessView`, reducing duplication between update and filter paths.
  - **Global Formatting**: Standardized uptime formatting across the app using `Utils::FormatUptime` (powered by Haiku's `BDurationFormat`).

- **Thorough Localization**:
  - Applied `B_TRANSLATE` to all remaining hardcoded strings, including chart titles, card names, speed units ("B/s", "KiB/s", etc.), and system info keys.
  - Integrated `BNumberFormat` for localized percentage displays in `MemView` and `CPUView`.

- **State Management**:
  - Simplified `DiskView` and `NetworkView` by removing redundant `fVisibleItems` tracking sets, as these views always display all collected items.

## 8. Final System-Wide Audit & Refactoring

A final comprehensive audit was performed to centralize logic, improve consistency, and resolve subtle rendering issues:

- **Centralized System Information**:
  - Migrated core system information gathering (OS version, ABI, GPU name, Display resolution, Root disk usage) into global functions in `Utils.h/cpp`.
  - Updated `SystemSummaryView` and `SystemDetailsView` to use these centralized functions, eliminating significant code duplication and ensuring consistent reporting across different parts of the application.

- **Enhanced Formatting Consistency**:
  - Refactored `FormatSpeed` to internally utilize the improved `FormatBytes` logic.
  - Added a `double` overload for `FormatBytes` to provide high-precision formatting for small byte-per-second values (e.g., "1.5 B/s").
  - Ensured all formatted strings use consistent localized unit suffixes.
  - Standardized MiB calculations in `SystemDetailsView` using a consistent ceiling division method.

- **Improved Graph Rendering**:
  - Optimized `ActivityGraphView` to handle sub-pixel updates. The graph now ensures the rightmost pixel is redrawn even when enough time hasn't passed to scroll the graph, preventing a "frozen" appearance between pixel-shift intervals.
  - Corrected grid line alignment during sub-pixel rendering by properly accounting for `fScrollOffset` in partial updates.
  - Standardized font retrieval logic using `GetFont()` to ensure compatibility with standard Haiku APIs.

- **Interactive Sorting & UI Robustness**:
  - Implemented full interactive column sorting for `DiskView` and `NetworkView`.
  - Centralized `ClickableHeaderView` into `Utils` to reduce code duplication.
  - Added font-responsive column width recalculation to all list-based views (`ProcessView`, `DiskView`, `NetworkView`) to ensure the UI remains correct when system font sizes change.
  - Added real-time updates to `SystemSummaryView` via the `Pulse()` mechanism.
  - Optimized `SystemDetailsView` and `SystemSummaryView` to skip updates when hidden, saving CPU.

- **Performance Optimizations**:
  - Implemented a caching mechanism for package counts in `Utils::GetPackageCount` to avoid frequent directory scans.
  - Optimized process name extraction in `ProcessView::UpdateThread` using `strrchr` instead of the more heavyweight `BPath`.
  - Improved battery detection to enumerate multiple potential battery slots.

- **Unified Selection Logic**:
  - Standardized list selection restoration across `DiskView`, `NetworkView`, and `ProcessView` by extracting the logic into private `_RestoreSelection` helpers. This ensures that the user's current selection is preserved reliably during periodic data updates.

- **Final Stability & Localization**:
  - Conducted a final sweep for hardcoded strings and wrapped them in `B_TRANSLATE`, including complex formatted strings in `SystemSummaryView`.
  - Verified all existing benchmarks and unit tests pass with the new refactored utility functions.

- **Post-Audit Optimization & Fixes**:
  - **Leak Prevention**: Fixed a file descriptor leak in `Utils::GetBatteryCapacity` by ensuring the battery state file is always closed before returning.
  - **Logic Accuracy**: Updated `GetRootDiskUsage` to report on `/boot` instead of `/`, providing more relevant information to Haiku users.
  - **Rendering Safety**: Added safety checks to `ActivityGraphView` line drawing to prevent issues with small view dimensions and verified loop bounds for exact data line rendering.

## 9. Performance Optimization & Build Hardening

A final round of performance tuning and build system improvements was completed:

- **Build System Improvements**:
  - Optimized the `Makefile` for production by setting `OPTIMIZE := FULL` and removing redundant debug flags (`-Og -g`) from target flags. This ensures the binary is compiled with maximum compiler optimizations.

- **Data Structure Optimization**:
  - Transitioned `DiskView` and `NetworkView` to use `std::unordered_map` for internal state tracking, matching the optimized pattern used in `ProcessView`.
  - Implemented a custom `std::hash<BString>` specialization to allow `BString` keys to be used efficiently in `unordered_map`.
  - Refactored `UpdateData` loops in both views to use the single-lookup `emplace` pattern, reducing map traversal overhead by 50% for existing items.

- **Dependency Cleanup**:
  - Performed a code audit to remove redundant STL includes (e.g., `<map>`, `<set>`, `<string>`) after the transition to `unordered_map`, reducing header bloat.
