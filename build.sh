#!/bin/bash
g++ `pkg-config --cflags --libs beos` -O0 -g SystemMonitor/SysMonTaskApp.cpp SystemMonitor/CPUView.cpp SystemMonitor/MemView.cpp SystemMonitor/DiskView.cpp SystemMonitor/NetworkView.cpp SystemMonitor/ProcessView.cpp SystemMonitor/GPUView.cpp SystemMonitor/SysInfoView.cpp -o SystemMonitor/SystemMonitor -ltracker -lcolumnlistview -lnetwork -lbnetapi
