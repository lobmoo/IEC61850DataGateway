#include "iec104ClientManger.h"

#include "iostream"

#include "config/config.h"
#include "log/logger.h"

IEC104ClientManger::IEC104ClientManger(/* args */)
{
}

IEC104ClientManger::~IEC104ClientManger()
{
    for (const auto &client : clients_) {
        if (client) {
            client->stop();
        }
    }
    clients_.clear();
    LOG(info) << "IEC 104 Client Manager destroyed, all clients stopped.";
}

void IEC104ClientManger::initClients()
{
    auto config = Config::getInstance().getConfig();
    if (!config) {
        LOG(error) << "Configuration data is not initialized.";
        return;
    }

    for (const auto &clientConfig : config->iec104) {
        auto client = std::make_shared<IEC104Client>(
            clientConfig.second.tcp.ip, clientConfig.second.tcp.port,
            clientConfig.second.local_address.ip, clientConfig.second.local_address.port,
            clientConfig.second.protocol.apci.t0,
            clientConfig.second.protocol.application_layer.originator_address,
            clientConfig.second.communication.gi_interval_ms);
        LOG(info) << "Initializing IEC 104 client for device: " << clientConfig.second.device_id
                  << " at " << clientConfig.second.tcp.ip << ":" << clientConfig.second.tcp.port
                  << " with local IP: " << clientConfig.second.local_address.ip
                  << " and local port: " << clientConfig.second.local_address.port;

        client->setConnectionEventCallback([](CS104_Connection, CS104_ConnectionEvent event) {
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
        client->setAsduReceivedCallback([](int address, CS101_ASDU asdu) -> bool {
            LOG(info) << "Received ASDU type: " << TypeID_toString(CS101_ASDU_getTypeID(asdu))
                      << " elements: " << CS101_ASDU_getNumberOfElements(asdu);
            if (CS101_ASDU_getTypeID(asdu) == M_ME_NB_1) {
                for (int i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {
                    SinglePointInformation io =
                        (SinglePointInformation)CS101_ASDU_getElement(asdu, i);
                    LOG(info) << "  IOA: "
                              << InformationObject_getObjectAddress((InformationObject)io)
                              << " value: " << SinglePointInformation_getValue(io);
                    SinglePointInformation_destroy(io);
                }
            }
            return true;
        });

        if (client->start()) {
            LOG(info) << "Connected to IEC 104 server at " << clientConfig.second.tcp.ip << ":"
                      << clientConfig.second.tcp.port;
            clients_.push_back(client);
        } else {
            LOG(error) << "Failed to connect to IEC 104 server at " << clientConfig.second.tcp.ip
                       << ":" << clientConfig.second.tcp.port;
        }
    }
}

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
        if (CS101_ASDU_getTypeID(asdu) == M_ME_NB_1) {
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