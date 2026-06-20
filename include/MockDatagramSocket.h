//
// Created by Obikwelu on 2025-03-22.
//

#ifndef SYSC_3303_ELEVATOR_PROJECT_MOCKDATAGRAMSOCKET_H
#define SYSC_3303_ELEVATOR_PROJECT_MOCKDATAGRAMSOCKET_H

#include <gmock/gmock.h>
#include "Datagram.h"

class MockDatagramSocket : public DatagramSocket {
public:
    MOCK_METHOD(void, send, (const DatagramPacket& packet));
};

#endif //SYSC_3303_ELEVATOR_PROJECT_MOCKDATAGRAMSOCKET_H
