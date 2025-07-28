#include "NetworkView.h"
#include <LayoutBuilder.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <NetworkRoster.h>
#include <NetworkInterface.h>
#include <Alert.h>
#include <map>
#include <string>

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
    : BView(frame, "NetworkView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED)
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

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .SetInsets(0)
        .Add(netBox)
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

        // Use dummy traffic stats (real stats would require platform-specific code)
        uint64 dummyTx = rand() % 10000000;
        uint64 dummyRx = rand() % 10000000;
        BString sendSpeed = "N/A", recvSpeed = "N/A";

        InterfaceStatsRecord& rec = fPreviousStatsMap[name.String()];
        if (rec.lastUpdateTime > 0) {
            bigtime_t dt = currentTime - rec.lastUpdateTime;
            if (dt > 0) {
                sendSpeed = FormatSpeed(dummyTx > rec.bytesSent ? dummyTx - rec.bytesSent : 0, dt);
                recvSpeed = FormatSpeed(dummyRx > rec.bytesReceived ? dummyRx - rec.bytesReceived : 0, dt);
            }
        }

        rec.bytesSent = dummyTx;
        rec.bytesReceived = dummyRx;
        rec.lastUpdateTime = currentTime;

        row->SetField(new BStringField(FormatBytes(dummyTx)), kBytesSentColumn);
        row->SetField(new BStringField(FormatBytes(dummyRx)), kBytesRecvColumn);
        row->SetField(new BStringField(sendSpeed), kSendSpeedColumn);
        row->SetField(new BStringField(recvSpeed), kRecvSpeedColumn);

        fInterfaceListView->AddRow(row);
    }
    
    fLocker.Unlock();
}
