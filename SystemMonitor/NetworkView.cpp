#include "NetworkView.h"
#include "Utils.h"
#include <LayoutBuilder.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
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
    fDownloadSpeed(0.0f)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

    auto* netBox = new BBox("NetworkInterfacesBox");
    netBox->SetLabel("Network Interfaces");

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
    fInterfaceListView->AddColumn(new BStringColumn("Name", 100, 50, 200, B_TRUNCATE_END), kInterfaceNameColumn);
    fInterfaceListView->AddColumn(new BStringColumn("Type", 80, 40, 150, B_TRUNCATE_END), kInterfaceTypeColumn);
    fInterfaceListView->AddColumn(new BStringColumn("Address", 120, 50, 300, B_TRUNCATE_END), kInterfaceAddressColumn);
    fInterfaceListView->AddColumn(new BStringColumn("Sent", 90, 50, 200, B_TRUNCATE_END, B_ALIGN_RIGHT), kBytesSentColumn);
    fInterfaceListView->AddColumn(new BStringColumn("Recv", 90, 50, 200, B_TRUNCATE_END, B_ALIGN_RIGHT), kBytesRecvColumn);
    fInterfaceListView->AddColumn(new BStringColumn("TX Speed", 90, 50, 150, B_TRUNCATE_END, B_ALIGN_RIGHT), kSendSpeedColumn);
    fInterfaceListView->AddColumn(new BStringColumn("RX Speed", 90, 50, 150, B_TRUNCATE_END, B_ALIGN_RIGHT), kRecvSpeedColumn);

    fInterfaceListView->SetSortColumn(fInterfaceListView->ColumnAt(kInterfaceNameColumn), true, true);

    BLayoutBuilder::Group<>(netBox, B_VERTICAL, 0)
        .SetInsets(B_USE_DEFAULT_SPACING, fh.ascent + fh.descent + fh.leading + B_USE_DEFAULT_SPACING, 
                   B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
        .Add(fInterfaceListView);

    fDownloadGraph = new ActivityGraphView("download_graph", (rgb_color){80, 80, 255, 255});
    fUploadGraph = new ActivityGraphView("upload_graph", (rgb_color){255, 80, 80, 255});

    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
        .SetInsets(B_USE_DEFAULT_SPACING)
        .Add(netBox)
        .Add(fDownloadGraph)
        .Add(fUploadGraph)
    .End();
}

NetworkView::~NetworkView()
{
}

void NetworkView::AttachedToWindow()
{
    BView::AttachedToWindow();
    UpdateData();
}

void NetworkView::Pulse()
{
    UpdateData();
}

void NetworkView::UpdateData()
{
    fLocker.Lock();
    
	std::set<std::string> activeInterfaces;
    BNetworkRoster& roster = BNetworkRoster::Default();
    uint32 cookie = 0;
    BNetworkInterface interface;
    bigtime_t currentTime = system_time();
    uint64 totalSentDelta = 0;
    uint64 totalReceivedDelta = 0;

    while (roster.GetNextInterface(&cookie, interface) == B_OK) {
        BString name(interface.Name());
		activeInterfaces.insert(name.String());

        BString typeStr = "Ethernet";
        if (interface.Flags() & IFF_LOOPBACK) {
            typeStr = "Loopback";
        } else if (interface.Flags() & IFF_POINTOPOINT) {
            typeStr = "Point-to-Point";
        }

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

		ifreq_stats stats;
		status_t status = interface.GetStats(stats);
		uint64 currentSent = 0;
		uint64 currentReceived = 0;

		if (status == B_OK) {
			currentSent = stats.send.bytes;
			currentReceived = stats.receive.bytes;
		}

        BString sendSpeed = "N/A", recvSpeed = "N/A";

        InterfaceStatsRecord& rec = fPreviousStatsMap[name.String()];
        if (rec.lastUpdateTime > 0) {
            bigtime_t dt = currentTime - rec.lastUpdateTime;
            if (dt > 0) {
                uint64 sentDelta = (currentSent > rec.bytesSent) ? currentSent - rec.bytesSent : 0;
                uint64 recvDelta = (currentReceived > rec.bytesReceived) ? currentReceived - rec.bytesReceived : 0;
                sendSpeed = ::FormatSpeed(sentDelta, dt);
                recvSpeed = ::FormatSpeed(recvDelta, dt);
                if (!(interface.Flags() & IFF_LOOPBACK)) {
                    totalSentDelta += sentDelta;
                    totalReceivedDelta += recvDelta;
                }
            }
        }

        rec.bytesSent = currentSent;
        rec.bytesReceived = currentReceived;
        rec.lastUpdateTime = currentTime;

		BRow* row;
		if (fInterfaceRowMap.find(name.String()) == fInterfaceRowMap.end()) {
			row = new BRow();
			row->SetField(new BStringField(name), kInterfaceNameColumn);
			row->SetField(new BStringField(typeStr), kInterfaceTypeColumn);
			row->SetField(new BStringField(addressStr), kInterfaceAddressColumn);
			row->SetField(new BStringField(::FormatBytes(currentSent)), kBytesSentColumn);
			row->SetField(new BStringField(::FormatBytes(currentReceived)), kBytesRecvColumn);
			row->SetField(new BStringField(sendSpeed), kSendSpeedColumn);
			row->SetField(new BStringField(recvSpeed), kRecvSpeedColumn);
			fInterfaceListView->AddRow(row);
			fInterfaceRowMap[name.String()] = row;
		} else {
			row = fInterfaceRowMap[name.String()];
			row->SetField(new BStringField(typeStr), kInterfaceTypeColumn);
			row->SetField(new BStringField(addressStr), kInterfaceAddressColumn);
			row->SetField(new BStringField(::FormatBytes(currentSent)), kBytesSentColumn);
			row->SetField(new BStringField(::FormatBytes(currentReceived)), kBytesRecvColumn);
			row->SetField(new BStringField(sendSpeed), kSendSpeedColumn);
			row->SetField(new BStringField(recvSpeed), kRecvSpeedColumn);
			fInterfaceListView->UpdateRow(row);
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
