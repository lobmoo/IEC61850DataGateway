#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

#include "lib60870/cs104_slave.h"
#include "hal_thread.h"
#include "hal_time.h"

enum class DataPointType { MEASURED_VALUE_SCALED, SINGLE_POINT };

struct DataPoint {
    int ioa;
    DataPointType type;
    union {
        int16_t scaledValue;
        bool singlePointValue;
    } value;
    uint8_t quality;
    uint64_t timestamp;
};

struct ServerConfig {
    std::string localAddress = "0.0.0.0";
    int port = 2404;
    int maxConnections = 10;
    int maxQueueSize = 10;
};

int server104Test();
class IEC104Server
{
public:
    
    explicit IEC104Server(const ServerConfig &config);
    ~IEC104Server();

    void addDataPoint(
        int ioa, DataPointType type, int16_t scaledValue, uint8_t quality = IEC60870_QUALITY_GOOD);
    void addDataPoint(
        int ioa, DataPointType type, bool singlePointValue,
        uint8_t quality = IEC60870_QUALITY_GOOD);
    void updateDataPoint(int ioa, int16_t scaledValue);
    void updateDataPoint(int ioa, bool singlePointValue);
    bool start();
    void stop();
    void setPeriodicSendInterval(uint32_t intervalMs) { periodicIntervalMs_ = intervalMs; }

private:
    static void *periodicThreadEntry(void *param); // 新增静态线程入口
    static bool clockSyncHandler(
        void *parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime);
    static bool interrogationHandler(
        void *parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi);
    static bool asduHandler(void *parameter, IMasterConnection connection, CS101_ASDU asdu);
    static bool connectionRequestHandler(void *parameter, const char *ipAddress);
    static void
    connectionEventHandler(void *parameter, IMasterConnection con, CS104_PeerConnectionEvent event);
    static void rawMessageHandler(
        void *parameter, IMasterConnection connection, uint8_t *msg, int msgSize, bool sent);

    void periodicSendTask();
    void sendDataPoints(
        IMasterConnection connection, const std::vector<DataPoint> &points,
        CS101_CauseOfTransmission cot);

    ServerConfig config_;
    CS104_Slave slave_;
    CS101_AppLayerParameters alParams_;
    std::map<int, DataPoint> dataPoints_;
    std::mutex dataPointsMutex_;
    std::atomic<bool> running_;
    uint32_t periodicIntervalMs_;
    Thread periodicThread_; // 修正为 Thread
};
