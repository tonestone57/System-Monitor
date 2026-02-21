#include "NetworkView.h"
#include "Utils.h"
#include <LayoutBuilder.h>
#include <ListView.h>
#include <ListItem.h>
#include <StringView.h>
#include <Box.h>
#include <Font.h>
#include <Autolock.h>
#include <NetworkRoster.h>
#include <NetworkInterface.h>
#include <Alert.h>
#include <cstring>
#include <net/if.h>
#include "ActivityGraphView.h"
#include "InterfaceListItem.h"
#include <Messenger.h>
#include <Catalog.h>
#include <ScrollView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NetworkView"

const float kBaseNetNameWidth = 100;
const float kBaseNetTypeWidth = 80;
const float kBaseNetAddrWidth = 120;
const float kBaseNetSentWidth = 90;
const float kBaseNetRecvWidth = 90;
const float kBaseNetTxSpeedWidth = 90;
const float kBaseNetRxSpeedWidth = 90;


NetworkView::NetworkView()
	: BView("NetworkView", B_WILL_DRAW),
	fDownloadGraph(NULL),
	fUploadGraph(NULL),
	fUploadSpeed(0.0f),
	fDownloadSpeed(0.0f),
	fLastTotalUpdateTime(0),
	fUpdateThread(-1),
	fScanSem(-1),
	fTerminated(false),
	fPerformanceViewVisible(true),
	fRefreshInterval(1000000),
	fSortMode(SORT_NET_BY_TX_SPEED),
	fListGeneration(0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fScanSem = create_sem(0, "network scan sem");

	auto* netBox = new BBox("NetworkInterfacesBox");
	netBox->SetLabel(B_TRANSLATE("Network Interfaces"));

	// Calculate scaling
	BFont font;
	GetFont(&font);
	float scale = GetScaleFactor(&font);

	fNameWidth = kBaseNetNameWidth * scale;
	fTypeWidth = kBaseNetTypeWidth * scale;
	fAddrWidth = kBaseNetAddrWidth * scale;
	fSentWidth = kBaseNetSentWidth * scale;
	fRecvWidth = kBaseNetRecvWidth * scale;
	fTxSpeedWidth = kBaseNetTxSpeedWidth * scale;
	fRxSpeedWidth = kBaseNetRxSpeedWidth * scale;

	// Header View
	BGroupView* headerView = new BGroupView(B_HORIZONTAL, 0);
	headerView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	auto addHeader = [&](const char* label, float width, int32 mode, alignment align = B_ALIGN_LEFT) {
		ClickableHeaderView* sv = new ClickableHeaderView(label, width, mode, this);
		sv->SetAlignment(align);
		headerView->AddChild(sv);
		fHeaders.push_back(sv);
	};

	addHeader(B_TRANSLATE("Name"), fNameWidth, SORT_NET_BY_NAME);
	addHeader(B_TRANSLATE("Type"), fTypeWidth, SORT_NET_BY_TYPE);
	addHeader(B_TRANSLATE("Address"), fAddrWidth, SORT_NET_BY_ADDR);
	addHeader(B_TRANSLATE("Sent"), fSentWidth, SORT_NET_BY_SENT, B_ALIGN_RIGHT);
	addHeader(B_TRANSLATE("Recv"), fRecvWidth, SORT_NET_BY_RECV, B_ALIGN_RIGHT);
	addHeader(B_TRANSLATE("TX Speed"), fTxSpeedWidth, SORT_NET_BY_TX_SPEED, B_ALIGN_RIGHT);
	addHeader(B_TRANSLATE("RX Speed"), fRxSpeedWidth, SORT_NET_BY_RX_SPEED, B_ALIGN_RIGHT);

	headerView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 20 * scale));

	fInterfaceListView = new BListView("interface_list", B_SINGLE_SELECTION_LIST, B_WILL_DRAW | B_NAVIGABLE);
	BScrollView* netScrollView = new BScrollView("net_scroll", fInterfaceListView, 0, false, true, true);

	BLayoutBuilder::Group<>(netBox, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING + 15,
				   B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(headerView)
		.Add(netScrollView);

	fDownloadGraph = new ActivityGraphView("download_graph", {0, 0, 0, 0}, B_MENU_SELECTION_BACKGROUND_COLOR);
	fUploadGraph = new ActivityGraphView("upload_graph", {0, 0, 0, 0}, B_FAILURE_COLOR);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(netBox)
		.Add(fDownloadGraph)
		.Add(fUploadGraph)
	.End();
}

NetworkView::~NetworkView()
{
	fTerminated = true;
	if (fScanSem >= 0) delete_sem(fScanSem);
	if (fUpdateThread >= 0) {
		status_t dummy;
		wait_for_thread(fUpdateThread, &dummy);
	}

	fInterfaceListView->MakeEmpty();
	for (auto& pair : fInterfaceItemMap) {
		delete pair.second;
	}
	fInterfaceItemMap.clear();
}

void NetworkView::AttachedToWindow()
{
	BView::AttachedToWindow();
	fTerminated = false;

	if (fScanSem < 0)
		fScanSem = create_sem(0, "network scan sem");

	fUpdateThread = spawn_thread(UpdateThread, "NetworkView Update", B_NORMAL_PRIORITY, this);
	if (fUpdateThread >= 0)
		resume_thread(fUpdateThread);
}

void NetworkView::DetachedFromWindow()
{
	fTerminated = true;
	if (fScanSem >= 0) {
		delete_sem(fScanSem);
		fScanSem = -1;
	}
	if (fUpdateThread >= 0) {
		status_t dummy;
		wait_for_thread(fUpdateThread, &dummy);
		fUpdateThread = -1;
	}
	BView::DetachedFromWindow();
}

void NetworkView::MessageReceived(BMessage* message)
{
	if (message->what == kMsgNetworkDataUpdate) {
		UpdateData(message);
	} else if (message->what == MSG_HEADER_CLICKED) {
		int32 mode;
		if (message->FindInt32("mode", &mode) == B_OK) {
			fSortMode = (NetworkSortMode)mode;
			_SortItems();
			fInterfaceListView->Invalidate();
		}
	} else {
		BView::MessageReceived(message);
	}
}

void NetworkView::UpdateData(BMessage* message)
{
	fLocker.Lock();

	// Preserve selection
	int32 selection = fInterfaceListView->CurrentSelection();
	BString selectedName = "";
	if (selection >= 0) {
		InterfaceListItem* item = static_cast<InterfaceListItem*>(fInterfaceListView->ItemAt(selection));
		if (item) selectedName = item->Name();
	}

	fListGeneration++;
	bigtime_t currentTime = system_time();
	uint64 totalSentDelta = 0;
	uint64 totalReceivedDelta = 0;

	int32 count = 0;
	type_code type;
	message->GetInfo("net_info", &type, &count);

	// Get Font once
	BFont font;
	fInterfaceListView->GetFont(&font);

	bool fontChanged = (font != fCachedFont);
	if (fontChanged) {
		fCachedFont = font;
		float scale = GetScaleFactor(&font);
		fNameWidth = kBaseNetNameWidth * scale;
		fTypeWidth = kBaseNetTypeWidth * scale;
		fAddrWidth = kBaseNetAddrWidth * scale;
		fSentWidth = kBaseNetSentWidth * scale;
		fRecvWidth = kBaseNetRecvWidth * scale;
		fTxSpeedWidth = kBaseNetTxSpeedWidth * scale;
		fRxSpeedWidth = kBaseNetRxSpeedWidth * scale;

		UpdateHeaderWidths(fHeaders, { fNameWidth, fTypeWidth, fAddrWidth, fSentWidth, fRecvWidth, fTxSpeedWidth, fRxSpeedWidth });
	}

	for (int32 i = 0; i < count; i++) {
		const NetworkInfo* info;
		ssize_t size;
		if (message->FindData("net_info", B_RAW_TYPE, i, reinterpret_cast<const void**>(&info), &size) == B_OK) {

			BString name(info->name);

			if (!info->hasStats) {
				// Preserve existing items by updating their generation
				if (fPreviousStatsMap.count(name)) {
					 fPreviousStatsMap[name].generation = fListGeneration;
				}
				if (fInterfaceItemMap.count(name)) {
					 fInterfaceItemMap[name]->SetGeneration(fListGeneration);
				}
				continue;
			}

			BString typeStr(info->typeStr);
			BString addressStr(info->addressStr);
			uint64 currentSent = info->bytesSent;
			uint64 currentReceived = info->bytesReceived;

			uint64 sendSpeedBytes = 0;
			uint64 recvSpeedBytes = 0;

			InterfaceStatsRecord& rec = fPreviousStatsMap[name];
			if (rec.lastUpdateTime > 0) {
				bigtime_t dt = currentTime - rec.lastUpdateTime;
				if (dt > 0) {
					uint64 sentDelta = (currentSent > rec.bytesSent) ? currentSent - rec.bytesSent : 0;
					uint64 recvDelta = (currentReceived > rec.bytesReceived) ? currentReceived - rec.bytesReceived : 0;

					// Convert to Bytes/sec for SpeedField
					sendSpeedBytes = sentDelta * 1000000 / dt;
					recvSpeedBytes = recvDelta * 1000000 / dt;

					if (!info->isLoopback) {
						totalSentDelta += sentDelta;
						totalReceivedDelta += recvDelta;
					}
				}
			}

			rec.bytesSent = currentSent;
			rec.bytesReceived = currentReceived;
			rec.lastUpdateTime = currentTime;
			rec.generation = fListGeneration;

			InterfaceListItem* item;
			auto result = fInterfaceItemMap.emplace(name, nullptr);
			if (result.second) {
				item = new InterfaceListItem(name, typeStr, addressStr, currentSent, currentReceived, sendSpeedBytes, recvSpeedBytes, &font, this);
				fInterfaceListView->AddItem(item);
				result.first->second = item;
			} else {
				item = result.first->second;
				item->Update(name, typeStr, addressStr, currentSent, currentReceived, sendSpeedBytes, recvSpeedBytes, &font, fontChanged);
			}
			item->SetGeneration(fListGeneration);
		}
	}

	// Prune dead interfaces from the map
	for (auto it = fPreviousStatsMap.begin(); it != fPreviousStatsMap.end();) {
		if (it->second.generation != fListGeneration)
			it = fPreviousStatsMap.erase(it);
		else
			++it;
	}
	for (auto it = fInterfaceItemMap.begin(); it != fInterfaceItemMap.end();) {
		if (it->second->Generation() != fListGeneration) {
			InterfaceListItem* item = it->second;
			fInterfaceListView->RemoveItem(item);
			delete item;
			it = fInterfaceItemMap.erase(it);
		} else {
			++it;
		}
	}

	_SortItems();

	_RestoreSelection(selectedName);

	fInterfaceListView->Invalidate();

	// Update graphs
	if (fUploadGraph && fDownloadGraph) {
		bigtime_t dt = currentTime - fLastTotalUpdateTime;
		if (dt <= 0)
			dt = 1000000;

		fUploadSpeed = totalSentDelta * 1000000.0 / dt;
		fDownloadSpeed = totalReceivedDelta * 1000000.0 / dt;

		fUploadGraph->AddValue(currentTime, fUploadSpeed);
		fDownloadGraph->AddValue(currentTime, fDownloadSpeed);
		fLastTotalUpdateTime = currentTime;
	}

	fLocker.Unlock();
}

int32 NetworkView::UpdateThread(void* data)
{
	NetworkView* view = static_cast<NetworkView*>(data);
	BMessenger target(view);

	while (!view->fTerminated) {
		status_t err = acquire_sem_etc(view->fScanSem, 1, B_RELATIVE_TIMEOUT, view->fRefreshInterval);
		if (err != B_OK && err != B_TIMED_OUT && err != B_INTERRUPTED)
			break;

		if (view->fTerminated) break;

		if (!view->fPerformanceViewVisible)
			continue;

		// Drain the semaphore if we were woken up explicitly (e.g. interval change)
		if (err == B_OK) {
			int32 count;
			if (get_sem_count(view->fScanSem, &count) == B_OK && count > 0)
				acquire_sem_etc(view->fScanSem, count, B_RELATIVE_TIMEOUT, 0);
		}

		BMessage updateMsg(kMsgNetworkDataUpdate);
		BNetworkRoster& roster = BNetworkRoster::Default();
		uint32 cookie = 0;
		BNetworkInterface interface;

		while (roster.GetNextInterface(&cookie, interface) == B_OK) {
			NetworkInfo info;
			strlcpy(info.name, interface.Name(), sizeof(info.name));

			// Determine Type
			BString typeStr = B_TRANSLATE("Ethernet");
			info.isLoopback = (interface.Flags() & IFF_LOOPBACK) != 0;
			if (info.isLoopback) {
				typeStr = B_TRANSLATE("Loopback");
			} else if (interface.Flags() & IFF_POINTOPOINT) {
				typeStr = B_TRANSLATE("Point-to-Point");
			}
			strlcpy(info.typeStr, typeStr.String(), sizeof(info.typeStr));

			// Determine Address
			BString addressStr = B_TRANSLATE("N/A");
			for (int32 i = 0; i < interface.CountAddresses(); ++i) {
				BNetworkInterfaceAddress ifaceAddr;
				if (interface.GetAddressAt(i, ifaceAddr) == B_OK) {
					BNetworkAddress addr = ifaceAddr.Address();
					if (addr.Family() == AF_INET || addr.Family() == AF_INET6) {
						addressStr = addr.ToString();
						break;
					}
				}
			}
			strlcpy(info.addressStr, addressStr.String(), sizeof(info.addressStr));

			// Get Stats
			ifreq_stats stats;
			status_t status = interface.GetStats(stats);
			if (status == B_OK) {
				info.bytesSent = stats.send.bytes;
				info.bytesReceived = stats.receive.bytes;
				info.hasStats = true;
			} else {
				info.bytesSent = 0;
				info.bytesReceived = 0;
				info.hasStats = false;
			}

			updateMsg.AddData("net_info", B_RAW_TYPE, &info, sizeof(NetworkInfo));
		}

		target.SendMessage(&updateMsg);
	}
	return B_OK;
}

float NetworkView::GetUploadSpeed()
{
	BAutolock locker(fLocker);
	return fUploadSpeed;
}

float NetworkView::GetDownloadSpeed()
{
	BAutolock locker(fLocker);
	return fDownloadSpeed;
}

void NetworkView::_SortItems()
{
	switch (fSortMode) {
		case SORT_NET_BY_NAME: fInterfaceListView->SortItems(InterfaceListItem::CompareName); break;
		case SORT_NET_BY_TYPE: fInterfaceListView->SortItems(InterfaceListItem::CompareType); break;
		case SORT_NET_BY_ADDR: fInterfaceListView->SortItems(InterfaceListItem::CompareAddr); break;
		case SORT_NET_BY_SENT: fInterfaceListView->SortItems(InterfaceListItem::CompareSent); break;
		case SORT_NET_BY_RECV: fInterfaceListView->SortItems(InterfaceListItem::CompareRecv); break;
		case SORT_NET_BY_TX_SPEED: fInterfaceListView->SortItems(InterfaceListItem::CompareTxSpeed); break;
		case SORT_NET_BY_RX_SPEED: fInterfaceListView->SortItems(InterfaceListItem::CompareRxSpeed); break;
		default: fInterfaceListView->SortItems(InterfaceListItem::CompareSpeed); break;
	}
}

void NetworkView::SetRefreshInterval(bigtime_t interval)
{
	fRefreshInterval = interval;
	if (fScanSem >= 0)
		release_sem(fScanSem);

	BAutolock locker(fLocker);
	if (fUploadGraph)
		fUploadGraph->SetRefreshInterval(interval);
	if (fDownloadGraph)
		fDownloadGraph->SetRefreshInterval(interval);
}

void NetworkView::_RestoreSelection(const BString& selectedName)
{
	if (selectedName.IsEmpty())
		return;

	for (int32 i = 0; i < fInterfaceListView->CountItems(); i++) {
		InterfaceListItem* item = static_cast<InterfaceListItem*>(fInterfaceListView->ItemAt(i));
		if (item && item->Name() == selectedName) {
			fInterfaceListView->Select(i);
			break;
		}
	}
}
