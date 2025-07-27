#!/bin/bash
g++ SystemMonitor/SysMonTaskApp.cpp SystemMonitor/CPUView.cpp SystemMonitor/MemView.cpp SystemMonitor/DiskView.cpp SystemMonitor/NetworkView.cpp SystemMonitor/ProcessView.cpp SystemMonitor/GPUView.cpp SystemMonitor/SysInfoView.cpp -o SystemMonitor/SystemMonitor -lbe -ltracker -lcolumnlistview -lnetwork -lbnetapi
