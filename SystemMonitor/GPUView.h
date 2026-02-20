#ifndef GPUVIEW_H
#define GPUVIEW_H

#include <View.h>
#include <StringView.h>
#include <String.h>
#include <vector>
#include "ActivityGraphView.h"

class GPUView : public BView {
public:
	GPUView();
	virtual ~GPUView();

	virtual void AttachedToWindow();
	virtual void Pulse();

	void SetRefreshInterval(bigtime_t interval);

private:
	void CreateLayout();
	void UpdateData();
	void _UpdateStaticInfo();

	BStringView* fCardNameValue;
	BStringView* fDriverVersionValue;
	BStringView* fMemorySizeValue;
	BStringView* fResolutionValue;
	BStringView* fUtilizationValue;

	std::vector<ActivityGraphView*> fGpuGraphs;

	int32 fCachedWidth;
	int32 fCachedHeight;
	BString fCachedResolution;
};

#endif // GPUVIEW_H