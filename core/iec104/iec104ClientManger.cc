#include "iec104ClientManger.h"
#include "iec104Client.h"
#include "iostream"
#include "log/logger.h"


void testFunction()
{
    // 创建客户端，直接传递参数
    IEC104Client client("127.0.0.1", 2404, "", -1, 5, 3, 1000);

    // 设置连接事件回调
    client.setConnectionEventCallback([](CS104_Connection, CS104_ConnectionEvent event) {
        switch (event) {
        case CS104_CONNECTION_OPENED:
            LOG(info) << "Connection established";
            break;
        case CS104_CONNECTION_CLOSED:
            LOG(info) << "Connection closed";
            break;
        case CS104_CONNECTION_FAILED:
            LOG(error) << "Connection failed";
            break;
        default:
            LOG(warning) << "Connection event: " << event;
        }
    });

    // 设置ASDU接收回调
    client.setAsduReceivedCallback([](int address, CS101_ASDU asdu) -> bool {
        LOG(info) << "Received ASDU type: " << TypeID_toString(CS101_ASDU_getTypeID(asdu))
                  << " elements: " << CS101_ASDU_getNumberOfElements(asdu);
        if (CS101_ASDU_getTypeID(asdu) == M_SP_NA_1) {
            for (int i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                SinglePointInformation io = (SinglePointInformation)CS101_ASDU_getElement(asdu, i);
                LOG(info) << "  IOA: " << InformationObject_getObjectAddress((InformationObject)io)
                          << " value: " << SinglePointInformation_getValue(io);
                SinglePointInformation_destroy(io);
            }
        }
        return true;
    });

    // 启动客户端
    if (client.start()) {
        LOG(info) << "Client started, connecting to 127.0.0.1:2404";
    }

    // 模拟运行一段时间
    std::this_thread::sleep_for(std::chrono::seconds(3000));

    // 停止客户端
    client.stop();
    LOG(info) << "Client stopped";

    return;
}