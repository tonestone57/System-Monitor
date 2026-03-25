#ifndef MOCK_NETWORK_ROSTER_H
#define MOCK_NETWORK_ROSTER_H
#include "../HaikuMocks.h"
class BNetworkInterface;
class BNetworkRoster {
public:
    static BNetworkRoster& Default() { static BNetworkRoster r; return r; }
    int GetNextInterface(uint32*, BNetworkInterface&) { return -1; }
};
#endif
