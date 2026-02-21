#ifndef SYSTEM_SUMMARY_VIEW_H
#define SYSTEM_SUMMARY_VIEW_H

#include <View.h>
#include <StringView.h>
#include <String.h>
#include <kernel/OS.h>
#include <Messenger.h>
#include <atomic>

class BBox;
class BTextView;
class BScrollView;

class SystemSummaryView : public BView {
public:
	SystemSummaryView();
	virtual ~SystemSummaryView();

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);
	virtual void Show();
	virtual void Pulse();

private:
	void CreateLayout();
	void _StartLoadThread();
	static int32 _LoadDataThread(void* data);

	BTextView* fLogoTextView;
	BTextView* fInfoTextView;
	thread_id  fLoadThread;
	std::atomic<bool> fThreadRunning;
};

#endif // SYSTEM_SUMMARY_VIEW_H
