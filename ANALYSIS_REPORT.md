# Code Analysis Report: SystemMonitor

This report details the findings from a static analysis of the `SystemMonitor` codebase.

## Executive Summary

The `SystemMonitor` application is well-structured, utilizing the Haiku API effectively (Layout API, Messaging, Threading). The audit revealed that several previously suspected issues (semaphore accumulation, missing resize logic) were already correctly implemented. New optimizations and functional fixes were applied to enhance performance and usability.

## 1. Audit Findings & Verification

### 1.1 Verified Existing Fixes
The following issues were investigated and found to be **already addressed** in the current codebase, contrary to earlier reports:
- **Semaphore Accumulation**: `DiskView` and `NetworkView` correctly drain the semaphore in their update threads (using `get_sem_count` and `acquire_sem_etc`), preventing redundant scanning loops under load.
- **DataHistory Resizing**: The `SetRefreshInterval` method is fully implemented and correctly adjusts buffer sizes dynamically when the refresh rate changes.

### 1.2 Issues Fixed in This Audit
- **DataHistory Stability**: Added a check in `DataHistory::ValueAt` to prevent a division-by-zero crash when interpolating between identical timestamps.
- **Keyboard Support**: Implemented `KeyDown` in `ProcessView` (specifically `ProcessListView`) to support the `Delete` key for killing processes even when the list has focus.

### 1.3 Optimizations Implemented
- **NetworkView String Handling**: Replaced `std::string` with `BString` keys in internal maps to reduce allocation overhead and avoid unnecessary conversions.
- **ProcessView Search**: Implemented local filtering (`FilterRows`) to provide immediate UI feedback when typing in the search box, without waiting for the background thread.
- **Memory Allocation**: Optimized `ProcessView::UpdateThread` by moving vector allocations (`procList`, `activeThreads`) outside the main loop and reusing their capacity (using `.clear()` and `.reserve()`) to reduce heap fragmentation.

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
