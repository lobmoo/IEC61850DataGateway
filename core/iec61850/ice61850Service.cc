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
#include "iec61850/model/static_model.h"
#include "log/logger.h"

extern IedModel iedModel; // 静态模型，定义在 static_model.c 中


static bool automaticOperationMode = true;
static ClientConnection controllingClient = NULL;


// 构造函数
ice61850Service::ice61850Service()
    : server_(nullptr), model_(nullptr), running_(false), svPublisher_(nullptr)
{
}

// 析构函数
ice61850Service::~ice61850Service()
{
    stopServer();
}

static void connectionIndicationHandler(
    IedServer server, ClientConnection connection, bool connected, void *parameter)
{
    const char *clientAddress = ClientConnection_getPeerAddress(connection);

    if (connected) {
        LOG(info) << "BeagleDemoServer: new client connection from %s\n", clientAddress;
    } else {
        LOG(info) << "BeagleDemoServer: client connection from %s closed\n", clientAddress;

        if (controllingClient == connection) {
            LOG(info) << "Controlling client has closed connection -> switch to automatic operation mode";
            controllingClient = NULL;
            automaticOperationMode = true;
        }
    }
}

// 初始化服务端，使用静态模型
bool ice61850Service::init(
    const std::string &interface, const std::string &specificGooseInterface,
    const std::string &specificGcbName)
{
    try {
        // 使用静态模型
        model_ = &iedModel;
        if (!model_) {
            LOG(error) << "Static model not available";
            return false;
        }

        // 初始化MMS服务端
        server_ = IedServer_create(model_);
        if (!server_) {
            LOG(error) << "Failed to create IEC 61850 server";
            return false;
        }
        IedServer_setConnectionIndicationHandler(
            server_, (IedConnectionIndicationHandler)connectionIndicationHandler, NULL);

        // 初始化GOOSE发布者
        initializeGoose(interface, specificGooseInterface, specificGcbName);

        // 初始化SV发布者
        initializeSvPublisher(interface);

        // 初始化本地数据存储
        initializeLocalData();
            
        LOG(info) << "IEC 61850 server initialized successfully";
        return true;
    } catch (const std::exception &e) {
        LOG(error) << "Initialization error: " << e.what();
        return false;
    }
}

// 初始化本地数据（示例数据）
void ice61850Service::initializeLocalData()
{
    // 模拟本地数据，实际应用中可从数据库或文件读取
}

// 初始化GOOSE发布者
void ice61850Service::initializeGoose(
    const std::string &interface, const std::string &specificGooseInterface,
    const std::string &specificGcbName)
{
    // 设置全局 GOOSE 接口
    if (!interface.empty()) {
        LOG(info) << "Using GOOSE interface: " + interface;
        IedServer_setGooseInterfaceId(server_, interface.c_str());
    }
    // 设置特定 GOOSE 控制块接口
    if (!specificGooseInterface.empty()) {
        LOG(info) << "Using GOOSE interface for GenericIO/LLN0." + specificGcbName + ": "
                         + specificGooseInterface;
        IedServer_setGooseInterfaceIdEx(
            server_, IEDMODEL_GenericIO_LLN0, specificGcbName.c_str(),
            specificGooseInterface.c_str());
    }
}

// 初始化SV发布者
void ice61850Service::initializeSvPublisher(const std::string &interface)
{
    // 创建SV发布者
    svPublisher_ = SVPublisher_create(NULL, interface.c_str());
    if (svPublisher_) {
        // SVPublisher_ASDU asdu1 = SVPublisher_addASDU(svPublisher_, "svpub1", NULL, 1);
        // SVPublisher_setupComplete(svPublisher_);
        LOG(info) << "SV publisher initialized" << std::endl;
    } else {
        LOG(error) << "Failed to create SV publisher" << std::endl;
    }
}

// 启动服务端
bool ice61850Service::startServer(uint16_t port)
{
    if (!server_ || running_) {
        LOG(error) << "Server not initialized or already running" << std::endl;
        return false;
    }

    try {
        // 启动MMS服务端
        IedServer_start(server_, port);
        if (!IedServer_isRunning(server_)) {
            LOG(error) << "Failed to start IEC 61850 server" << std::endl;
            return false;
        }

        IedServer_enableGoosePublishing(server_);
        running_ = true;
        LOG(info) << "IEC 61850 server started" << std::endl;

        // 启动数据更新线程
        dataUpdateThread_ = std::thread(&ice61850Service::updateDataLoop, this); // 启动SV发布线程
        svThread_ = std::thread(&ice61850Service::publishSvLoop, this);
        return true;
    } catch (const std::exception &e) {
        LOG(error) << "Start server error: " << e.what() << std::endl;
        return false;
    }
}

// 停止服务端
void ice61850Service::stopServer()
{
    if (server_ && running_) {
        running_ = false;
        if (dataUpdateThread_.joinable()) {
            dataUpdateThread_.join();
        }
        if (svThread_.joinable()) {
            svThread_.join();
        }
        IedServer_stop(server_);
        IedServer_destroy(server_);
        server_ = nullptr;
    }
    if (svPublisher_) {
        SVPublisher_destroy(svPublisher_);
        svPublisher_ = nullptr;
    }
    // 静态模型无需手动释放
    model_ = nullptr;
    LOG(info) << "IEC 61850 server stopped" << std::endl;
}

// 数据更新循环（MMS）
void ice61850Service::updateDataLoop()
{
    while (running_) {
        updateServerData();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 每秒更新
    }
}

// 更新MMS服务端数据
void ice61850Service::updateServerData()
{
    IedServer_lockDataModel(server_);

    IedServer_unlockDataModel(server_);
}

// GOOSE发布循环
void ice61850Service::publishGooseLoop()
{
    while (running_) {
    }
}

// SV发布循环
void ice61850Service::publishSvLoop()
{
    while (running_) {

        std::this_thread::sleep_for(std::chrono::microseconds(125)); // 8kHz采样率
    }
}
