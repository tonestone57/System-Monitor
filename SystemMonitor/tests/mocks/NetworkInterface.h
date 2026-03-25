#ifndef MOCK_NETWORK_INTERFACE_H
#define MOCK_NETWORK_INTERFACE_H
#include "../HaikuMocks.h"
class BNetworkInterfaceAddress {
public:
    BNetworkAddress Address() const { return BNetworkAddress(); }
};
class BNetworkInterface {
public:
    int Flags() const { return IFF_UP; }
    int CountAddresses() const { return 1; }
    int GetAddressAt(int, BNetworkInterfaceAddress&) { return 0; }
};
#endif
