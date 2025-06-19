#include "iec104Service.h"
#include <iostream>
#include <iomanip>
#include <chrono>

IEC104Server::IEC104Server(const ServerConfig &config)
    : config_(config), slave_(nullptr), running_(false), periodicIntervalMs_(1000),
      periodicThread_(nullptr)
{
    slave_ = CS104_Slave_create(config_.maxConnections, config_.maxQueueSize);
    CS104_Slave_setLocalAddress(slave_, config_.localAddress.c_str());
    CS104_Slave_setServerMode(slave_, CS104_MODE_SINGLE_REDUNDANCY_GROUP);
    alParams_ = CS104_Slave_getAppLayerParameters(slave_);
}

IEC104Server::~IEC104Server()
{
    stop();
    CS104_Slave_destroy(slave_);
}

void IEC104Server::addDataPoint(int ioa, DataPointType type, int16_t scaledValue, uint8_t quality)
{
    std::lock_guard<std::mutex> lock(dataPointsMutex_);
    DataPoint dp;
    dp.ioa = ioa;
    dp.type = type;
    dp.value.scaledValue = scaledValue;
    dp.quality = quality;
    dp.timestamp = Hal_getTimeInMs();
    dataPoints_[ioa] = dp;
}

void IEC104Server::addDataPoint(int ioa, DataPointType type, bool singlePointValue, uint8_t quality)
{
    std::lock_guard<std::mutex> lock(dataPointsMutex_);
    DataPoint dp;
    dp.ioa = ioa;
    dp.type = type;
    dp.value.singlePointValue = singlePointValue;
    dp.quality = quality;
    dp.timestamp = Hal_getTimeInMs();
    dataPoints_[ioa] = dp;
}

void IEC104Server::updateDataPoint(int ioa, int16_t scaledValue)
{
    std::lock_guard<std::mutex> lock(dataPointsMutex_);
    auto it = dataPoints_.find(ioa);
    if (it != dataPoints_.end() && it->second.type == DataPointType::MEASURED_VALUE_SCALED) {
        it->second.value.scaledValue = scaledValue;
        it->second.timestamp = Hal_getTimeInMs();
    }
}

void IEC104Server::updateDataPoint(int ioa, bool singlePointValue)
{
    std::lock_guard<std::mutex> lock(dataPointsMutex_);
    auto it = dataPoints_.find(ioa);
    if (it != dataPoints_.end() && it->second.type == DataPointType::SINGLE_POINT) {
        it->second.value.singlePointValue = singlePointValue;
        it->second.timestamp = Hal_getTimeInMs();
    }
}

// 静态线程入口函数
void *IEC104Server::periodicThreadEntry(void *param)
{
    auto server = static_cast<IEC104Server *>(param);
    server->periodicSendTask();
    return nullptr;
}

bool IEC104Server::start()
{
    if (running_)
        return false;

    // 设置回调
    CS104_Slave_setClockSyncHandler(slave_, clockSyncHandler, this);
    CS104_Slave_setInterrogationHandler(slave_, interrogationHandler, this);
    CS104_Slave_setASDUHandler(slave_, asduHandler, this);
    CS104_Slave_setConnectionRequestHandler(slave_, connectionRequestHandler, this);
    CS104_Slave_setConnectionEventHandler(slave_, connectionEventHandler, this);
    CS104_Slave_setRawMessageHandler(slave_, rawMessageHandler, this);

    // 启动服务
    CS104_Slave_start(slave_);
    if (!CS104_Slave_isRunning(slave_)) {
        std::cerr << "Failed to start IEC104 server" << std::endl;
        return false;
    }

    running_ = true;
    periodicThread_ = Thread_create(periodicThreadEntry, this, true);
    if (!periodicThread_) {
        std::cerr << "Failed to create periodic thread" << std::endl;
        CS104_Slave_stop(slave_);
        running_ = false;
        return false;
    }
    Thread_start(periodicThread_);

    std::cout << "IEC104 server started on " << config_.localAddress << ":" << config_.port
              << std::endl;
    return true;
}

void IEC104Server::stop()
{
    if (!running_)
        return;

    running_ = false;
    if (periodicThread_) {
        Thread_destroy(periodicThread_); // 直接销毁线程
        periodicThread_ = nullptr;
    }

    CS104_Slave_stop(slave_);
    std::cout << "IEC104 server stopped" << std::endl;
}

void IEC104Server::periodicSendTask()
{
    while (running_) {
        std::vector<DataPoint> points;
        {
            std::lock_guard<std::mutex> lock(dataPointsMutex_);
            for (const auto &pair : dataPoints_) {
                points.push_back(pair.second);
            }
        }
        sendDataPoints(nullptr, points, CS101_COT_PERIODIC);
        Thread_sleep(periodicIntervalMs_);
    }
}

void IEC104Server::sendDataPoints(
    IMasterConnection connection, const std::vector<DataPoint> &points,
    CS101_CauseOfTransmission cot)
{
    if (points.empty())
        return;

    CS101_ASDU asdu = CS101_ASDU_create(alParams_, false, cot, 0, 1, false, false);
    if (!asdu) {
        std::cerr << "Failed to create ASDU" << std::endl;
        return;
    }

    for (const auto &dp : points) {
        InformationObject io = nullptr;
        if (dp.type == DataPointType::MEASURED_VALUE_SCALED) {
            io = (InformationObject)MeasuredValueScaled_create(
                nullptr, dp.ioa, dp.value.scaledValue, dp.quality);
        } else if (dp.type == DataPointType::SINGLE_POINT) {
            io = (InformationObject)SinglePointInformation_create(
                nullptr, dp.ioa, dp.value.singlePointValue, dp.quality);
        }
        if (io) {
            CS101_ASDU_addInformationObject(asdu, io);
            InformationObject_destroy(io);
        }
    }

    if (connection) {
        IMasterConnection_sendASDU(connection, asdu);
    } else {
        CS104_Slave_enqueueASDU(slave_, asdu);
    }
    CS101_ASDU_destroy(asdu);
}

bool IEC104Server::clockSyncHandler(
    void *parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime)
{
    auto server = static_cast<IEC104Server *>(parameter);
    std::cout << "Received clock sync command with time: " << std::setfill('0') << std::setw(2)
              << CP56Time2a_getHour(newTime) << ":" << std::setw(2) << CP56Time2a_getMinute(newTime)
              << ":" << std::setw(2) << CP56Time2a_getSecond(newTime) << " " << std::setw(2)
              << CP56Time2a_getDayOfMonth(newTime) << "/" << std::setw(2)
              << CP56Time2a_getMonth(newTime) << "/" << (CP56Time2a_getYear(newTime) + 2000)
              << std::endl;

    CP56Time2a_setFromMsTimestamp(newTime, Hal_getTimeInMs());
    return true;
}

bool IEC104Server::interrogationHandler(
    void *parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi)
{
    auto server = static_cast<IEC104Server *>(parameter);
    std::cout << "Received interrogation for group " << static_cast<int>(qoi) << std::endl;

    if (qoi == 20) { // 站总召唤
        IMasterConnection_sendACT_CON(connection, asdu, false);
        std::vector<DataPoint> points;
        {
            std::lock_guard<std::mutex> lock(server->dataPointsMutex_);
            for (const auto &pair : server->dataPoints_) {
                points.push_back(pair.second);
            }
        }
        server->sendDataPoints(connection, points, CS101_COT_INTERROGATED_BY_STATION);
        IMasterConnection_sendACT_TERM(connection, asdu);
    } else {
        IMasterConnection_sendACT_CON(connection, asdu, true);
    }
    return true;
}

bool IEC104Server::asduHandler(void *parameter, IMasterConnection connection, CS101_ASDU asdu)
{
    auto server = static_cast<IEC104Server *>(parameter);
    if (CS101_ASDU_getTypeID(asdu) == C_SC_NA_1) {
        std::cout << "Received single command" << std::endl;
        if (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION) {
            InformationObject io = CS101_ASDU_getElement(asdu, 0);
            if (io) {
                int ioa = InformationObject_getObjectAddress(io);
                SingleCommand sc = (SingleCommand)io;
                std::cout << "IOA: " << ioa << " switch to " << SingleCommand_getState(sc)
                          << std::endl;
                {
                    std::lock_guard<std::mutex> lock(server->dataPointsMutex_);
                    auto it = server->dataPoints_.find(ioa);
                    if (it != server->dataPoints_.end()
                        && it->second.type == DataPointType::SINGLE_POINT) {
                        it->second.value.singlePointValue = SingleCommand_getState(sc);
                        it->second.timestamp = Hal_getTimeInMs();
                    }
                }
                CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_CON);
                InformationObject_destroy(io);
            } else {
                CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_IOA);
            }
        } else {
            CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT);
        }
        IMasterConnection_sendASDU(connection, asdu);
        return true;
    }
    return false;
}

bool IEC104Server::connectionRequestHandler(void *parameter, const char *ipAddress)
{
    std::cout << "New connection request from " << ipAddress << std::endl;
    return true;
}

void IEC104Server::connectionEventHandler(
    void *parameter, IMasterConnection con, CS104_PeerConnectionEvent event)
{
    std::string eventStr;
    switch (event) {
        case CS104_CON_EVENT_CONNECTION_OPENED:
            eventStr = "opened";
            break;
        case CS104_CON_EVENT_CONNECTION_CLOSED:
            eventStr = "closed";
            break;
        case CS104_CON_EVENT_ACTIVATED:
            eventStr = "activated";
            break;
        case CS104_CON_EVENT_DEACTIVATED:
            eventStr = "deactivated";
            break;
    }
    std::cout << "Connection " << eventStr << " (" << con << ")" << std::endl;
}

void IEC104Server::rawMessageHandler(
    void *parameter, IMasterConnection connection, uint8_t *msg, int msgSize, bool sent)
{
    std::cout << (sent ? "SEND: " : "RCVD: ");
    for (int i = 0; i < msgSize; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(msg[i])
                  << " ";
    }
    std::cout << std::dec << std::endl;
}

/*std::atomic<bool> running(true);

void signalHandler(int signal)
{
    running = false;
}

int main()
{
    // 注册信号处理
    std::signal(SIGINT, signalHandler);

    // 配置104服务
    IEC104::ServerConfig config;
    config.localAddress = "0.0.0.0";
    config.port = 2404;
    config.maxConnections = 5;
    config.maxQueueSize = 5;

    // 创建服务实例
    IEC104::IEC104Server server(config);
    server.setPeriodicSendInterval(2000); // 每2秒发送一次

    // 添加测试数据点
    server.addDataPoint(100, IEC104::DataPointType::MEASURED_VALUE_SCALED, 1000, IEC60870_QUALITY_GOOD);
    server.addDataPoint(101, IEC104::DataPointType::MEASURED_VALUE_SCALED, 2000, IEC60870_QUALITY_GOOD);
    server.addDataPoint(200, IEC104::DataPointType::SINGLE_POINT, true, IEC60870_QUALITY_GOOD);
    server.addDataPoint(201, IEC104::DataPointType::SINGLE_POINT, false, IEC60870_QUALITY_GOOD);

    // 启动服务
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    // 模拟数据更新
    int16_t value = 1000;
    while (running) {
        server.updateDataPoint(100, value++);
        server.updateDataPoint(101, value + 1000);
        server.updateDataPoint(200, value % 2 == 0);
        Thread_sleep(1000);
    }

    // 停止服务
    server.stop();
    return 0;
}*/