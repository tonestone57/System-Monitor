#ifndef PERFORMANCEVIEW_H
#define PERFORMANCEVIEW_H

#include <View.h>
#include <SplitView.h>
#include <Message.h>

#include "SystemStats.h"

class CPUView;
class MemView;
class NetworkView;
class DiskView;
class GPUView;
class SummaryView;

class PerformanceView : public BView {
public:
						PerformanceView();
	virtual void		AttachedToWindow() override;
	virtual void		Pulse() override;
	virtual void		Hide() override;
	virtual void		Show() override;
	void				SetRefreshInterval(bigtime_t interval);

	void				SaveState(BMessage& state);
	void				LoadState(const BMessage& state);

private:
	BSplitView*			fSplitView;
	SummaryView*		fSummaryView;
	BView*				fRightPane;

	SystemStats			fStats;
	CPUView*			fCPUView;
	MemView*			fMemView;
	NetworkView*		fNetworkView;
	DiskView*			fDiskView;
	GPUView*			fGPUView;
};

#endif // PERFORMANCEVIEW_H
