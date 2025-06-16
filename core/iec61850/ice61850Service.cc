/**
 * @file ice61850Service.cc
 * @brief 
 * @author wwk (1162431386@qq.com)
 * @version 1.0
 * @date 2025-06-08
 * 
 * @copyright Copyright (c) 2025  by  wwk : wwk.lobmo@gmail.com
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2025-06-08     <td>1.0     <td>wwk   <td>修改?
 * </table>
 */
#include "ice61850Service.h"
#include "static_model.h" // 由 model_generator 生成
#include <libiec61850/iec61850_server.h>
#include <libiec61850/iec61850_model.h>
#include <libiec61850/iec61850_common.h>
#include <libiec61850/goose_publisher.h>
#include <libiec61850/sv_publisher.h>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>

extern IedModel staticIedModel; // 静态模型，定义在 static_model.c 中

// 构造函数
ice61850Service::ice61850Service() : server(nullptr), model(nullptr), running(false), goosePublisher(nullptr), svPublisher(nullptr) {
}

// 析构函数
ice61850Service::~ice61850Service() {
    stopServer();
}

// 初始化服务端，使用静态模型
bool ice61850Service::initialize(const std::string& ipAddress, int port, const std::string& interface) {
    try {
        // 使用静态模型
        model = &staticIedModel;
        if (!model) {
            std::cerr << "Static model not available" << std::endl;
            return false;
        }

        // 初始化MMS服务端
        server = IedServer_create(model);
        if (!server) {
            std::cerr << "Failed to create IEC 61850 server" << std::endl;
            return false;
        }

        // 配置MMS服务端网络参数
        IedServer_setLocalIpAddress(server, ipAddress.c_str());
        IedServer_setTcpPort(server, port);

        // 初始化GOOSE发布者
        initializeGoosePublisher(interface);

        // 初始化SV发布者
        initializeSvPublisher(interface);

        // 初始化本地数据存储
        initializeLocalData();

        std::cout << "IEC 61850 server initialized successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
        return false;
    }
}

// 初始化本地数据（示例数据）
void ice61850Service::initializeLocalData() {
    // 模拟本地数据，实际应用中可从数据库或文件读取
    localDataStore["LD0/LLN0$ST$Mod$stVal"] = {1.0f, QUALITY_GOOD, Hal_getTimeInMs()};
    localDataStore["LD0/LLN0$MX$AnIn1$mag$f"] = {42.5f, QUALITY_GOOD, Hal_getTimeInMs()};
    // 用于GOOSE的示例数据
    localDataStore["LD0/LLN0$GO$Status$stVal"] = {0.0f, QUALITY_GOOD, Hal_getTimeInMs()};
    // 用于SV的示例数据
    localDataStore["LD0/LLN0$SV$Amp$instMag$f"] = {1000.0f, QUALITY_GOOD, Hal_getTimeInMs()};
}

// 初始化GOOSE发布者
void ice61850Service::initializeGoosePublisher(const std::string& interface) {
    // 获取GOOSE控制块
    LogicalNode* lln0 = (LogicalNode*)IedModel_getModelNodeByShortName(model, "LD0/LLN0");
    if (lln0) {
        DataObject* gcb = (DataObject*)IedModel_getModelNodeByShortName(model, "LD0/LLN0$GooseCB");
        if (gcb) {
            goosePublisher = GoosePublisher_create(gcb, interface.c_str());
            if (goosePublisher) {
                GoosePublisher_setGoCbRef(goosePublisher, "LD0/LLN0$GooseCB");
                GoosePublisher_setAppId(goosePublisher, 0x0001);
                GoosePublisher_setConfRev(goosePublisher, 1);
                std::cout << "GOOSE publisher initialized" << std::endl;
            } else {
                std::cerr << "Failed to create GOOSE publisher" << std::endl;
            }
        } else {
            std::cerr << "GOOSE control block not found in model" << std::endl;
        }
    } else {
        std::cerr << "Logical node LD0/LLN0 not found" << std::endl;
    }
}

// 初始化SV发布者
void ice61850Service::initializeSvPublisher(const std::string& interface) {
    // 创建SV发布者
    svPublisher = SvPublisher_create(interface.c_str(), 0x4000); // 示例AppID
    if (svPublisher) {
        SvPublisherSetup svSetup = SvPublisherSetup_create();
        SvPublisherSetup_setSvId(svSetup, "SVID001");
        SvPublisherSetup_setDataSetRef(svSetup, "LD0/LLN0$DataSet1");
        SvPublisherSetup_addASDUs(svSetup, 1, 4, 80); // 1 ASDU, 4 samples per cycle, 80 cycles/sec
        SvPublisher_configure(svPublisher, svSetup);
        SvPublisherSetup_destroy(svSetup);
        std::cout << "SV publisher initialized" << std::endl;
    } else {
        std::cerr << "Failed to create SV publisher" << std::endl;
    }
}

// 启动服务端
bool ice61850Service::startServer() {
    if (!server || running) {
        std::cerr << "Server not initialized or already running" << std::endl;
        return false;
    }

    try {
        // 启动MMS服务端
        IedServer_start(server);
        if (!IedServer_isRunning(server)) {
            std::cerr << "Failed to start IEC 61850 server" << std::endl;
            return false;
        }

        running = true;
        std::cout << "IEC 61850 server started" << std::endl;

        // 启动数据更新线程
        dataUpdateThread = std::thread(&ice61850Service::updateDataLoop, this);

        // 启动GOOSE发布线程
        gooseThread = std::thread(&ice61850Service::publishGooseLoop, this);

        // 启动SV发布线程
        svThread = std::thread(&ice61850Service::publishSvLoop, this);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Start server error: " << e.what() << std::endl;
        return false;
    }
}

// 停止服务端
void ice61850Service::stopServer() {
    if (server && running) {
        running = false;
        if (dataUpdateThread.joinable()) {
            dataUpdateThread.join();
        }
        if (gooseThread.joinable()) {
            gooseThread.join();
        }
        if (svThread.joinable()) {
            svThread.join();
        }
        IedServer_stop(server);
        IedServer_destroy(server);
        server = nullptr;
    }
    if (goosePublisher) {
        GoosePublisher_destroy(goosePublisher);
        goosePublisher = nullptr;
    }
    if (svPublisher) {
        SvPublisher_destroy(svPublisher);
        svPublisher = nullptr;
    }
    // 静态模型无需手动释放
    model = nullptr;
    std::cout << "IEC 61850 server stopped" << std::endl;
}

// 数据更新循环（MMS）
void ice61850Service::updateDataLoop() {
    while (running) {
        updateServerData();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 每秒更新
    }
}

// 更新MMS服务端数据
void ice61850Service::updateServerData() {
    IedServer_lockDataModel(server);

    for (const auto& [dataRef, data] : localDataStore) {
        DataAttribute* da = IedServer_getAttributeByReference(server, dataRef.c_str());
        if (da) {
            if (da->type == IEC61850_FLOAT32) {
                IedServer_updateFloatValue(server, da, data.value);
            } else if (da->type == IEC61850_INT32) {
                IedServer_updateInt32Value(server, da, static_cast<int32_t>(data.value));
            }
            IedServer_updateQuality(server, da, data.quality);
            IedServer_updateTimestamp(server, da, data.timestamp);
        }
    }

    IedServer_unlockDataModel(server);
}

// GOOSE发布循环
void ice61850Service::publishGooseLoop() {
    while (running) {
        if (goosePublisher) {
            // 示例：发布GOOSE状态数据
            LinkedList dataSetValues = LinkedList_create();
            LinkedList_add(dataSetValues, MmsValue_newIntegerFromInt32(static_cast<int32_t>(localDataStore["LD0/LLN0$GO$Status$stVal"].value)));
            GoosePublisher_publish(goosePublisher, dataSetValues);
            LinkedList_destroyDeep(dataSetValues, (void (*)(void*))MmsValue_delete);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 每100ms发布
    }
}

// SV发布循环
void ice61850Service::publishSvLoop() {
    while (running) {
        if (svPublisher) {
            // 示例：发布SV采样值
            float amp = localDataStore["LD0/LLN0$SV$Amp$instMag$f"].value;
            SvPublisher_ASDU asdu = SvPublisher_addASDU(svPublisher);
            SvPublisher_ASDU_setFLOAT32(asdu, 0, amp); // 设置采样值
            SvPublisher_ASDU_setQuality(asdu, 0, localDataStore["LD0/LLN0$SV$Amp$instMag$f"].quality);
            SvPublisher_publish(svPublisher);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(125)); // 8kHz采样率
    }
}

// 设置本地数据（外部接口）
void ice61850Service::setLocalData(const std::string& dataRef, float value, Quality quality) {
    localDataStore[dataRef] = {value, quality, Hal_getTimeInMs()};
}



