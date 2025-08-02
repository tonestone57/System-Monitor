#include "NetworkView.h"
#include <LayoutBuilder.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <Box.h>
#include <Font.h>
#include <NetworkRoster.h>
#include <NetworkInterface.h>
#include <Alert.h>
#include <map>
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

NetworkView::NetworkView(BRect frame)
    : BView(frame, "NetworkView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED),
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
    UpdateData();
    BView::AttachedToWindow();
}

void NetworkView::Pulse()
{
    UpdateData();
}

BString NetworkView::FormatBytes(uint64 bytes)
{
    double kb = bytes / 1024.0, mb = kb / 1024.0, gb = mb / 1024.0;
    BString str;
    if (gb >= 1.0) str.SetToFormat("%.2f GiB", gb);
    else if (mb >= 1.0) str.SetToFormat("%.2f MiB", mb);
    else if (kb >= 1.0) str.SetToFormat("%.1f KiB", kb);
    else str.SetToFormat("%llu B", (unsigned long long)bytes);
    return str;
}

BString NetworkView::FormatSpeed(uint64 bytesDelta, bigtime_t microSecondsDelta)
{
    if (microSecondsDelta <= 0) return "0 B/s";
    double speed = bytesDelta / (microSecondsDelta / 1000000.0);
    double kbs = speed / 1024.0, mbs = kbs / 1024.0;
    BString str;
    if (mbs >= 1.0) str.SetToFormat("%.2f MiB/s", mbs);
    else if (kbs >= 1.0) str.SetToFormat("%.2f KiB/s", kbs);
    else str.SetToFormat("%.0f B/s", speed);
    return str;
}

void NetworkView::UpdateData()
{
    fLocker.Lock();
    
    // Clear existing rows
    while (BRow* row = fInterfaceListView->RowAt(0)) {
        fInterfaceListView->RemoveRow(row);
        delete row;
    }

    BNetworkRoster& roster = BNetworkRoster::Default();
    uint32 cookie = 0;
    BNetworkInterface interface;
    bigtime_t currentTime = system_time();
    uint64 totalSentDelta = 0;
    uint64 totalReceivedDelta = 0;

    while (roster.GetNextInterface(&cookie, interface) == B_OK) {
        BRow* row = new BRow();
        BString name(interface.Name());
        row->SetField(new BStringField(name), kInterfaceNameColumn);

        BString typeStr = "Ethernet";
        if (interface.Flags() & IFF_LOOPBACK) {
            typeStr = "Loopback";
        } else if (interface.Flags() & IFF_POINTOPOINT) {
            typeStr = "Point-to-Point";
        }

        row->SetField(new BStringField(typeStr), kInterfaceTypeColumn);

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
        row->SetField(new BStringField(addressStr), kInterfaceAddressColumn);

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
                sendSpeed = FormatSpeed(sentDelta, dt);
                recvSpeed = FormatSpeed(recvDelta, dt);
                if (!(interface.Flags() & IFF_LOOPBACK)) {
                    totalSentDelta += sentDelta;
                    totalReceivedDelta += recvDelta;
                }
            }
        }

        rec.bytesSent = currentSent;
        rec.bytesReceived = currentReceived;
        rec.lastUpdateTime = currentTime;

        row->SetField(new BStringField(FormatBytes(currentSent)), kBytesSentColumn);
        row->SetField(new BStringField(FormatBytes(currentReceived)), kBytesRecvColumn);
        row->SetField(new BStringField(sendSpeed), kSendSpeedColumn);
        row->SetField(new BStringField(recvSpeed), kRecvSpeedColumn);

        fInterfaceListView->AddRow(row);
    }
    
    // Update graphs
    if (fUploadGraph && fDownloadGraph) {
        bigtime_t dt = 1000000; // 1 second, as Pulse is set to 1s
        double maxSpeed = 12.5 * 1024 * 1024; // 100 Mbit/s

        fUploadSpeed = totalSentDelta / (dt / 1000000.0);
        fDownloadSpeed = totalReceivedDelta / (dt / 1000000.0);

        fUploadGraph->AddValue(system_time(), fUploadSpeed);
        fDownloadGraph->AddValue(system_time(), fDownloadSpeed);
    }

    fLocker.Unlock();
}

float NetworkView::GetUploadSpeed()
{
    return fUploadSpeed;
}

float NetworkView::GetDownloadSpeed()
{
    return fDownloadSpeed;
}
