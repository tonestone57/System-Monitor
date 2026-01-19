#ifndef SYSTEMSTATS_H
#define SYSTEMSTATS_H

struct SystemStats {
    SystemStats()
        : cpuUsage(0.0f),
          memoryUsage(0.0f),
          uploadSpeed(0.0f),
          downloadSpeed(0.0f) {}

    float cpuUsage;
    float memoryUsage;
    float uploadSpeed;
    float downloadSpeed;
};

#endif // SYSTEMSTATS_H
