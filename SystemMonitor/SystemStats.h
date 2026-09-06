#ifndef SYSTEMSTATS_H
#define SYSTEMSTATS_H

struct SystemStats {
	SystemStats()
		: cpuUsage(0.0f),
		  memoryUsage(0.0f),
		  uploadSpeed(0.0f),
		  downloadSpeed(0.0f),
		  cpuFrequency(0),
		  memoryUsed(0),
		  memoryTotal(0) {}

	float cpuUsage;
	float memoryUsage;
	float uploadSpeed;
	float downloadSpeed;
	uint64 cpuFrequency;
	uint64 memoryUsed;
	uint64 memoryTotal;
};

#endif // SYSTEMSTATS_H
