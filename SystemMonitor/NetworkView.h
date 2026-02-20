#ifndef NETWORKVIEW_H
#define NETWORKVIEW_H

#include <View.h>
#include <Locker.h>
#include <String.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <set>
#include <atomic>
#include <Font.h>
#include "ActivityGraphView.h"

class BListView;
class BListItem;
class ClickableHeaderView;
class InterfaceListItem; // Forward declaration
class ActivityGraphView;

struct InterfaceStatsRecord {
	uint64 bytesSent = 0;
	uint64 bytesReceived = 0;
	bigtime_t lastUpdateTime = 0;
	int32 generation = 0;
};

struct NetworkInfo {
	char name[B_OS_NAME_LENGTH];
	char typeStr[64];
	char addressStr[128];
	uint64 bytesSent;
	uint64 bytesReceived;
	bool hasStats;
	bool isLoopback;
};

const uint32 kMsgNetworkDataUpdate = 'netd';

enum NetworkSortMode {
	SORT_NET_BY_NAME,
	SORT_NET_BY_TYPE,
	SORT_NET_BY_ADDR,
	SORT_NET_BY_SENT,
	SORT_NET_BY_RECV,
	SORT_NET_BY_TX_SPEED,
	SORT_NET_BY_RX_SPEED
};

class NetworkView : public BView {
public:
	NetworkView();
	virtual ~NetworkView();

	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void MessageReceived(BMessage* message);
	virtual void Pulse();

	float GetUploadSpeed();
	float GetDownloadSpeed();
	void SetRefreshInterval(bigtime_t interval);
	void SetPerformanceViewVisible(bool visible) { fPerformanceViewVisible = visible; }

	float NameWidth() const { return fNameWidth; }
	float TypeWidth() const { return fTypeWidth; }
	float AddrWidth() const { return fAddrWidth; }
	float SentWidth() const { return fSentWidth; }
	float RecvWidth() const { return fRecvWidth; }
	float TxSpeedWidth() const { return fTxSpeedWidth; }
	float RxSpeedWidth() const { return fRxSpeedWidth; }

private:
	static int32 UpdateThread(void* data);
	void UpdateData(BMessage* message);

	BListView* fInterfaceListView;
	std::vector<ClickableHeaderView*> fHeaders;
	ActivityGraphView* fDownloadGraph;
	ActivityGraphView* fUploadGraph;

	BLocker fLocker;

	struct BStringHash {
		size_t operator()(const BString& s) const {
			size_t hash = 5381;
			const char* str = s.String();
			int c;
			while ((c = *str++))
				hash = ((hash << 5) + hash) + c;
			return hash;
		}
	};

	std::unordered_map<BString, InterfaceStatsRecord, BStringHash> fPreviousStatsMap;
	std::unordered_map<BString, InterfaceListItem*, BStringHash> fInterfaceItemMap;
	float fUploadSpeed;
	float fDownloadSpeed;

	BFont fCachedFont;

	thread_id fUpdateThread;
	sem_id fScanSem;
	std::atomic<bool> fTerminated;
	std::atomic<bool> fPerformanceViewVisible;
	std::atomic<bigtime_t> fRefreshInterval;
	int32 fListGeneration;

	float fNameWidth;
	float fTypeWidth;
	float fAddrWidth;
	float fSentWidth;
	float fRecvWidth;
	float fTxSpeedWidth;
	float fRxSpeedWidth;

	NetworkSortMode fSortMode;

	void _SortItems();
	void _RestoreSelection(const BString& selectedName);
};

#endif // NETWORKVIEW_H
