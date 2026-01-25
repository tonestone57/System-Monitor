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
#include <map>
#include <set>
#include <string>
#include <net/if.h>
#include "ActivityGraphView.h"
#include <Messenger.h>
#include <Catalog.h>
#include <ScrollView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NetworkView"

const float kNetNameWidth = 100;
const float kNetTypeWidth = 80;
const float kNetAddrWidth = 120;
const float kNetSentWidth = 90;
const float kNetRecvWidth = 90;
const float kNetTxSpeedWidth = 90;
const float kNetRxSpeedWidth = 90;

class InterfaceListItem : public BListItem {
public:
    InterfaceListItem(const BString& name, const BString& type, const BString& addr,
                      uint64 sent, uint64 recv, uint64 txSpeed, uint64 rxSpeed)
        : BListItem(),
          fName(name), fType(type), fAddr(addr),
          fSent(sent), fRecv(recv), fTxSpeed(txSpeed), fRxSpeed(rxSpeed)
    {
    }

    void Update(const BString& name, const BString& type, const BString& addr,
                      uint64 sent, uint64 recv, uint64 txSpeed, uint64 rxSpeed)
    {
        fName = name;
        fType = type;
        fAddr = addr;
        fSent = sent;
        fRecv = recv;
        fTxSpeed = txSpeed;
        fRxSpeed = rxSpeed;
    }

    virtual void DrawItem(BView* owner, BRect itemRect, bool complete = false) {
        if (IsSelected() || complete) {
            rgb_color color;
            if (IsSelected()) color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
            else color = ui_color(B_LIST_BACKGROUND_COLOR);
            owner->SetHighColor(color);
            owner->FillRect(itemRect);
        }

        rgb_color textColor;
        if (IsSelected()) textColor = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
        else textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);
        owner->SetHighColor(textColor);

        BFont font;
        owner->GetFont(&font);
        font_height fh;
        font.GetHeight(&fh);

        float x = itemRect.left + 5;
        float y = itemRect.bottom - fh.descent;

        auto drawTruncated = [&](const BString& str, float width) {
             BString out;
             font.TruncateString(&str, B_TRUNCATE_END, width - 10, &out);
             owner->DrawString(out.String(), BPoint(x, y));
             x += width;
        };

        auto drawRight = [&](const BString& str, float width) {
             float w = owner->StringWidth(str.String());
             owner->DrawString(str.String(), BPoint(x + width - w - 5, y));
             x += width;
        };

        drawTruncated(fName, kNetNameWidth);
        drawTruncated(fType, kNetTypeWidth);
        drawTruncated(fAddr, kNetAddrWidth);

        drawRight(FormatBytes(fSent), kNetSentWidth);
        drawRight(FormatBytes(fRecv), kNetRecvWidth);

        drawRight(FormatSpeed(fTxSpeed, 1000000), kNetTxSpeedWidth);
        drawRight(FormatSpeed(fRxSpeed, 1000000), kNetRxSpeedWidth);
    }

private:
    BString fName;
    BString fType;
    BString fAddr;
    uint64 fSent;
    uint64 fRecv;
    uint64 fTxSpeed;
    uint64 fRxSpeed;
};


NetworkView::NetworkView()
    : BView("NetworkView", B_WILL_DRAW | B_PULSE_NEEDED),
    fDownloadGraph(NULL),
    fUploadGraph(NULL),
    fUploadSpeed(0.0f),
    fDownloadSpeed(0.0f),
    fUpdateThread(-1),
    fScanSem(-1),
    fTerminated(false)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
    fScanSem = create_sem(0, "network scan sem");

    auto* netBox = new BBox("NetworkInterfacesBox");
    netBox->SetLabel(B_TRANSLATE("Network Interfaces"));

    // Header View
    BGroupView* headerView = new BGroupView(B_HORIZONTAL, 0);
    headerView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    auto addHeader = [&](const char* label, float width, alignment align = B_ALIGN_LEFT) {
        BStringView* sv = new BStringView(NULL, label);
        sv->SetExplicitMinSize(BSize(width, B_SIZE_UNSET));
        sv->SetExplicitMaxSize(BSize(width, B_SIZE_UNSET));
        sv->SetAlignment(align);
        sv->SetFont(be_bold_font);
        headerView->AddChild(sv);
    };

    addHeader(B_TRANSLATE("Name"), kNetNameWidth);
    addHeader(B_TRANSLATE("Type"), kNetTypeWidth);
    addHeader(B_TRANSLATE("Address"), kNetAddrWidth);
    addHeader(B_TRANSLATE("Sent"), kNetSentWidth, B_ALIGN_RIGHT);
    addHeader(B_TRANSLATE("Recv"), kNetRecvWidth, B_ALIGN_RIGHT);
    addHeader(B_TRANSLATE("TX Speed"), kNetTxSpeedWidth, B_ALIGN_RIGHT);
    addHeader(B_TRANSLATE("RX Speed"), kNetRxSpeedWidth, B_ALIGN_RIGHT);

    headerView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 20));

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

    for (int32 i = 0; i < fInterfaceListView->CountItems(); i++) {
        delete fInterfaceListView->ItemAt(i);
    }
    fInterfaceListView->MakeEmpty();
    fInterfaceItemMap.clear();
    fVisibleItems.clear();
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
    } else {
        BView::MessageReceived(message);
    }
}

void NetworkView::Pulse()
{
    // Trigger update in thread
    if (fScanSem >= 0) release_sem(fScanSem);
}

void NetworkView::UpdateData(BMessage* message)
{
    fLocker.Lock();
    
	std::set<BString, BStringLess> activeInterfaces;
    bigtime_t currentTime = system_time();
    uint64 totalSentDelta = 0;
    uint64 totalReceivedDelta = 0;

    int32 count = 0;
    type_code type;
    message->GetInfo("net_info", &type, &count);

    bool listChanged = false;

    for (int32 i = 0; i < count; i++) {
        const NetworkInfo* info;
        ssize_t size;
        if (message->FindData("net_info", B_RAW_TYPE, i, (const void**)&info, &size) == B_OK) {

            BString name(info->name);
            activeInterfaces.insert(name);

            if (!info->hasStats) continue;

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

                    if (typeStr != B_TRANSLATE("Loopback")) {
                        totalSentDelta += sentDelta;
                        totalReceivedDelta += recvDelta;
                    }
                }
            }

            rec.bytesSent = currentSent;
            rec.bytesReceived = currentReceived;
            rec.lastUpdateTime = currentTime;

            InterfaceListItem* item;
            auto rowIt = fInterfaceItemMap.find(name);
            if (rowIt == fInterfaceItemMap.end()) {
                item = new InterfaceListItem(name, typeStr, addressStr, currentSent, currentReceived, sendSpeedBytes, recvSpeedBytes);
                fInterfaceListView->AddItem(item);
                fInterfaceItemMap[name] = item;
                fVisibleItems.insert(item);
                listChanged = true;
            } else {
                item = rowIt->second;
                item->Update(name, typeStr, addressStr, currentSent, currentReceived, sendSpeedBytes, recvSpeedBytes);
            }
        }
    }

	// Prune dead interfaces from the map
	for (auto it = fPreviousStatsMap.begin(); it != fPreviousStatsMap.end();) {
		if (it->first != "__total__" && activeInterfaces.find(it->first) == activeInterfaces.end())
			it = fPreviousStatsMap.erase(it);
		else
			++it;
	}
	for (auto it = fInterfaceItemMap.begin(); it != fInterfaceItemMap.end();) {
		if (activeInterfaces.find(it->first) == activeInterfaces.end()) {
			InterfaceListItem* item = it->second;
            if (fVisibleItems.find(item) != fVisibleItems.end()) {
			    fInterfaceListView->RemoveItem(item);
                fVisibleItems.erase(item);
            }
			delete item;
			it = fInterfaceItemMap.erase(it);
            listChanged = true;
		} else {
			++it;
		}
	}
    
    fInterfaceListView->Invalidate();

    // Update graphs
    if (fUploadGraph && fDownloadGraph) {
        bigtime_t dt = currentTime - fPreviousStatsMap["__total__"].lastUpdateTime;
		if (dt <= 0)
			dt = 1000000;

        fUploadSpeed = totalSentDelta * 1000000.0 / dt;
        fDownloadSpeed = totalReceivedDelta * 1000000.0 / dt;

        fUploadGraph->AddValue(currentTime, fUploadSpeed);
        fDownloadGraph->AddValue(currentTime, fDownloadSpeed);
		fPreviousStatsMap["__total__"].lastUpdateTime = currentTime;
    }

    fLocker.Unlock();
}

int32 NetworkView::UpdateThread(void* data)
{
    NetworkView* view = static_cast<NetworkView*>(data);
    BMessenger target(view);

    while (!view->fTerminated) {
        status_t err = acquire_sem(view->fScanSem);
        if (err != B_OK) {
            if (view->fTerminated) break;
            if (err == B_INTERRUPTED) continue;
            break;
        }

        // Drain the semaphore
        int32 count;
        if (get_sem_count(view->fScanSem, &count) == B_OK && count > 0)
            acquire_sem_etc(view->fScanSem, count, B_RELATIVE_TIMEOUT, 0);

        BMessage updateMsg(kMsgNetworkDataUpdate);
        BNetworkRoster& roster = BNetworkRoster::Default();
        uint32 cookie = 0;
        BNetworkInterface interface;

        while (roster.GetNextInterface(&cookie, interface) == B_OK) {
            NetworkInfo info;
            strlcpy(info.name, interface.Name(), sizeof(info.name));

            // Determine Type
            BString typeStr = B_TRANSLATE("Ethernet");
            if (interface.Flags() & IFF_LOOPBACK) {
                typeStr = B_TRANSLATE("Loopback");
            } else if (interface.Flags() & IFF_POINTOPOINT) {
                typeStr = B_TRANSLATE("Point-to-Point");
            }
            strlcpy(info.typeStr, typeStr.String(), sizeof(info.typeStr));

            // Determine Address
            BString addressStr = "N/A";
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

void NetworkView::SetRefreshInterval(bigtime_t interval)
{
    BAutolock locker(fLocker);
    if (fUploadGraph)
        fUploadGraph->SetRefreshInterval(interval);
    if (fDownloadGraph)
        fDownloadGraph->SetRefreshInterval(interval);
}

BString NetworkView::FormatBytes(uint64 bytes)
{
    // Duplicate of Utils::FormatBytes?
    // Let's just use Utils::FormatBytes if available.
    // The previous code called FormatBytes(fValue) inside SizeField.
    // Utils.h has FormatBytes.
    return ::FormatBytes(bytes);
}
