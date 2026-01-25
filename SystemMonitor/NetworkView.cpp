#include "NetworkView.h"
#include "Utils.h"
#include "MonitorColumnTypes.h"
#include <LayoutBuilder.h>
#include <private/interface/ColumnListView.h>
#include "ColumnTypes.h"
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

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NetworkView"

// Column identifiers
enum {
    kInterfaceNameColumn,
    kInterfaceTypeColumn,
    kInterfaceAddressColumn,
    kBytesSentColumn,
    kBytesRecvColumn,
    kSendSpeedColumn,
    kRecvSpeedColumn
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

    font_height fh;
    netBox->GetFontHeight(&fh);

    BRect clvRect = netBox->Bounds();
    clvRect.top += fh.ascent + fh.descent + fh.leading + B_USE_DEFAULT_SPACING;
    clvRect.InsetBy(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

    fInterfaceListView = new BColumnListView(clvRect, "interface_clv",
                                             B_FOLLOW_ALL_SIDES,
                                             B_WILL_DRAW | B_NAVIGABLE,
                                             B_PLAIN_BORDER, true);

    // Setup columns
    fInterfaceListView->AddColumn(new SysMonStringColumn(B_TRANSLATE("Name"), 100, 50, 200, B_TRUNCATE_END), kInterfaceNameColumn);
    fInterfaceListView->AddColumn(new SysMonStringColumn(B_TRANSLATE("Type"), 80, 40, 150, B_TRUNCATE_END), kInterfaceTypeColumn);
    fInterfaceListView->AddColumn(new SysMonStringColumn(B_TRANSLATE("Address"), 120, 50, 300, B_TRUNCATE_END), kInterfaceAddressColumn);
    fInterfaceListView->AddColumn(new BSizeColumn(B_TRANSLATE("Sent"), 90, 50, 200, B_TRUNCATE_END, B_ALIGN_RIGHT), kBytesSentColumn);
    fInterfaceListView->AddColumn(new BSizeColumn(B_TRANSLATE("Recv"), 90, 50, 200, B_TRUNCATE_END, B_ALIGN_RIGHT), kBytesRecvColumn);
    fInterfaceListView->AddColumn(new BSpeedColumn(B_TRANSLATE("TX Speed"), 90, 50, 150, B_TRUNCATE_END, B_ALIGN_RIGHT), kSendSpeedColumn);
    fInterfaceListView->AddColumn(new BSpeedColumn(B_TRANSLATE("RX Speed"), 90, 50, 150, B_TRUNCATE_END, B_ALIGN_RIGHT), kRecvSpeedColumn);

    fInterfaceListView->SetSortColumn(fInterfaceListView->ColumnAt(kInterfaceNameColumn), true, true);

    BLayoutBuilder::Group<>(netBox, B_VERTICAL, 0)
        .SetInsets(B_USE_DEFAULT_SPACING, fh.ascent + fh.descent + fh.leading + B_USE_DEFAULT_SPACING, 
                   B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
        .Add(fInterfaceListView);

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

            // Helper for size fields
            auto setSizeField = [&](BRow* row, int index, uint64 val) {
                SizeField* field = static_cast<SizeField*>(row->GetField(index));
                if (field) {
                    if (field->Value() != val) {
                        field->SetValue(val);
                        return true;
                    }
                } else {
                    row->SetField(new SizeField(val), index);
                    return true;
                }
                return false;
            };

            // Helper for speed fields
            auto setSpeedField = [&](BRow* row, int index, uint64 val) {
                SpeedField* field = static_cast<SpeedField*>(row->GetField(index));
                if (field) {
                    if (field->Value() != val) {
                        field->SetValue(val);
                        return true;
                    }
                } else {
                    row->SetField(new SpeedField(val), index);
                    return true;
                }
                return false;
            };

            BRow* row;
            auto rowIt = fInterfaceRowMap.find(name);
            if (rowIt == fInterfaceRowMap.end()) {
                row = new BRow();
                row->SetField(new SysMonStringField(name), kInterfaceNameColumn);
                row->SetField(new SysMonStringField(typeStr), kInterfaceTypeColumn);
                row->SetField(new SysMonStringField(addressStr), kInterfaceAddressColumn);
                row->SetField(new SizeField(currentSent), kBytesSentColumn);
                row->SetField(new SizeField(currentReceived), kBytesRecvColumn);
                row->SetField(new SpeedField(sendSpeedBytes), kSendSpeedColumn);
                row->SetField(new SpeedField(recvSpeedBytes), kRecvSpeedColumn);
                fInterfaceListView->AddRow(row);
                fInterfaceRowMap[name] = row;
            } else {
                row = rowIt->second;
                bool changed = false;

                SysMonStringField* field = static_cast<SysMonStringField*>(row->GetField(kInterfaceTypeColumn));
                if (field != NULL) {
                    if (strcmp(field->String(), typeStr.String()) != 0) {
                        field->SetString(typeStr);
                        changed = true;
                    }
                } else {
                    row->SetField(new SysMonStringField(typeStr), kInterfaceTypeColumn);
                    changed = true;
                }

                field = static_cast<SysMonStringField*>(row->GetField(kInterfaceAddressColumn));
                if (field != NULL) {
                    if (strcmp(field->String(), addressStr.String()) != 0) {
                        field->SetString(addressStr);
                        changed = true;
                    }
                } else {
                    row->SetField(new SysMonStringField(addressStr), kInterfaceAddressColumn);
                    changed = true;
                }

                if (setSizeField(row, kBytesSentColumn, currentSent)) changed = true;
                if (setSizeField(row, kBytesRecvColumn, currentReceived)) changed = true;
                if (setSpeedField(row, kSendSpeedColumn, sendSpeedBytes)) changed = true;
                if (setSpeedField(row, kRecvSpeedColumn, recvSpeedBytes)) changed = true;

                if (changed)
                    fInterfaceListView->UpdateRow(row);
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
	for (auto it = fInterfaceRowMap.begin(); it != fInterfaceRowMap.end();) {
		if (activeInterfaces.find(it->first) == activeInterfaces.end()) {
			BRow* row = it->second;
			fInterfaceListView->RemoveRow(row);
			delete row;
			it = fInterfaceRowMap.erase(it);
		} else {
			++it;
		}
	}
    
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
