/**
 * @file app_modbus_client.cc
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

#include "app_modbus_client.h"
#include "log/logger.h"

#include <thread>
#include <filesystem>
#include "redis-api/app_redis.h"

// #define MODBUS_DEBUG

AppModbus::AppModbus() : running_(true), thread_pool_(1024)
{
}

AppModbus::~AppModbus()
{
}

bool AppModbus::run()
{

    auto mConfig = Config::getInstance().getConfig()->modbus;
    if (mConfig.empty()) {
        LOG(error) << "Modbus configuration is empty or invalid.";
        return false;
    }

    try {
        for (auto &devices : mConfig) {
            auto deviceId = devices.first;
            auto config = devices.second;

            if (config.Type.empty() || config.device_id.empty()) {
                LOG(error) << "Device " << deviceId << " has incomplete configuration, skipping.";
                continue;
            }

            ModbusType type = ModbusType::TCP;
            if (config.Type == "TCP") {
                type = ModbusType::TCP;
            } else if (config.Type == "RTU") {
                type = ModbusType::RTU;
            } else {
                LOG(error) << "Unknown Modbus type for device " << deviceId << ": " << config.Type;
                continue;
            }

            if (devices_.find(deviceId) != devices_.end()) {
                LOG(warning) << "Device " << deviceId << " already exists, skipping.";
                continue;
            }

            if (type == ModbusType::RTU
                && (config.rtu.port_name.empty() || config.rtu.baudrate <= 0)) {
                LOG(error) << "Invalid RTU configuration for device " << deviceId << ", skipping.";
                continue;
            }
            if (type == ModbusType::TCP && (config.tcp.ip.empty() || config.tcp.port <= 0)) {
                LOG(error) << "Invalid TCP configuration for device " << deviceId << ", skipping.";
                continue;
            }

            try {
                /*丢进去待命*/
                devices_[config.device_id] = std::make_shared<ModbusApi>(
                    config.slave_addr, type, 0, config.cmd_interval, config.rtu.port_name,
                    config.rtu.baudrate, config.rtu.parity == "NONE" ? Parity::NONE : Parity::ODD,
                    config.tcp.ip, config.tcp.port, config.max_retries, config.retry_interval);

                LOG(info) << "config.rtu.slave_addr " << config.slave_addr;
            } catch (const std::exception &e) {
                LOG(error) << "Failed to create ModbusApi for device " << deviceId << ": "
                           << e.what();
                continue;
            }
        }
    } catch (const std::exception &e) {
        LOG(error) << "Unexpected error during Modbus initialization: " << e.what();
        return false;
    }

    (void)thread_pool_.detach_task([this]() { runTask(); });
    LOG(info) << "AppModbus initialized successfully.";
    return true;
}

void AppModbus::readRegisters(
    const std::string &deviceId, uint16_t startAddr, int nbRegs, uint16_t *dest)
{
    auto modbusApi = getDeviceApi(deviceId);
    if (modbusApi) {
        modbusApi->readRegisters(startAddr, nbRegs, dest);
    } else {
        LOG(error) << "Device " << deviceId << " not found";
    }
}

void AppModbus::writeRegister(const std::string &deviceId, uint16_t addr, uint16_t value)
{
    auto modbusApi = getDeviceApi(deviceId);
    if (modbusApi) {
        modbusApi->writeRegister(addr, value);
    } else {
        LOG(error) << "Device " << deviceId << " not found";
    }
}

RegisterRange AppModbus::findContinuousRegisters(
    const std::vector<ConfigDataModbus::data_points_t> &points, size_t start_index)
{
    RegisterRange range = {0, 0, start_index};

    // 检查输入有效性
    if (start_index >= points.size()) {
        return range;
    }

    // 初始化起始点
    const auto &start_point = points[start_index];
    range.start_addr = start_point.address;
    range.end_index = start_index; // 初始化为起始索引

    // 计算起始点的寄存器数量
    switch (start_point.data_type) {
        case ConfigDataModbus::INT16:
        case ConfigDataModbus::UINT16:
            range.count = 1; // 2字节 = 1寄存器
            break;
        case ConfigDataModbus::INT32:
        case ConfigDataModbus::UINT32:
        case ConfigDataModbus::FLOAT32:
            range.count = 2; // 4字节 = 2寄存器
            break;
        default:
            range.count = 0;
            return range;
    }

    // 如果只有一个数据点
    if (start_index == points.size() - 1) {
        return range;
    }

    // 检查后续的连续寄存器
    int next_expected_addr = range.start_addr + range.count;
    size_t current_index = start_index + 1;

    while (current_index < points.size()) {
        const auto &current_point = points[current_index];
        int current_reg_count = 0;

        // 计算当前数据点占用的寄存器数量
        switch (current_point.data_type) {
            case ConfigDataModbus::INT16:
            case ConfigDataModbus::UINT16:
                current_reg_count = 1;
                break;
            case ConfigDataModbus::INT32:
            case ConfigDataModbus::UINT32:
            case ConfigDataModbus::FLOAT32:
                current_reg_count = 2;
                break;
            default:
                return range; // 未知类型，结束
        }

        // 检查地址是否连续
        if (current_point.address == next_expected_addr) {
            range.count += current_reg_count;
            range.end_index = current_index; // 更新为当前索引
            next_expected_addr = current_point.address + current_reg_count;
            current_index++;
        } else {
            return range; // 如果地址不连续，返回当前范围
        }
    }
    return range;
}

bool AppModbus::processUserData(
    const std::vector<uint16_t> &dataBuffer,
    std::vector<ConfigDataModbus::data_points_t> &dataPoints)
{
    uint32_t   uOffset = 0;
    DRDSDataRedis redis;
    for (auto point : dataPoints) {
        switch (point.data_type) {
            case ConfigDataModbus::INT16: {
                int16_t value = static_cast<int16_t>(dataBuffer[uOffset]);
                redis.storeInt(point.name, value);
                uOffset += 2;   //todo 这里不确定传几进制  二进制可以用pipline模式
                printf("INT16: %d\n", value);
            } break;
            case ConfigDataModbus::UINT16: {
                uint16_t value = static_cast<uint16_t>(dataBuffer[uOffset]);
                uOffset += 2;
                redis.storeUInt(point.name, value);
                printf("UINT16: %u\n", value);
            } break;
            case ConfigDataModbus::INT32: {
                int32_t value = static_cast<int32_t>(dataBuffer[uOffset]);
                uOffset += 4;
                redis.storeDInt(point.name, value);
                printf("INT32: %d\n", value);
            } break;
            case ConfigDataModbus::UINT32: {
                uint32_t value = static_cast<uint32_t>(dataBuffer[uOffset]);
                uOffset += 4;
                redis.storeUDInt(point.name, value);
                printf("UINT32: %u\n", value);
            } break;
            case ConfigDataModbus::FLOAT32: {
                float value = *reinterpret_cast<const float *>(&dataBuffer[uOffset]);
                uOffset += 4;
                redis.storeLReal(point.name, value);
                printf("FLOAT32: %f\n", value);
            } break;
            default:
                break;
        }
    }
    return true;
}

void AppModbus::processContinuousRegisters(
    std::shared_ptr<ModbusApi> modbusApi, const ConfigDataModbus *config)
{
    auto points = config->data_points;
    if (points.empty())
        return;

   

    /*地址按照顺序排列，方便拿数据*/
    std::sort(
        points.begin(), points.end(),
        [&points](
            const ConfigDataModbus::data_points_t &a, const ConfigDataModbus::data_points_t &b) {
            return a.address < b.address;
        });

    LOG(debug) << "device : " << config->device_id << "  +++++size: " << points.size() << "+++++";
    size_t i = 0;
    while (i < points.size()) {
        RegisterRange range = findContinuousRegisters(points, i);
        LOG(debug) << "Processing range >> start_addr : " << range.start_addr
                   << " count: " << range.count << " index: " << range.end_index;
        std::vector<uint16_t> buffer(range.count);
        // 批量读取
        bool success = modbusApi->readRegisters(range.start_addr, range.count, buffer.data());
        if (!success) {
            LOG(error) << "Failed to read " << " registers from " << range.start_addr
                       << " count: " << range.count;
            std::this_thread::sleep_for(std::chrono::milliseconds(config->cmd_interval));
            i = range.end_index + 1; // 确保跳过当前范围
            continue;
        }

        for (auto data : buffer) {
            printf("%d  ", data);
        }
        printf("\n");

        /*数据处理*/
        if(!processUserData(buffer, points))
        {
            LOG(error) << "Failed to process user data";
            i = range.end_index + 1;
            continue;
        }
        i = range.end_index + 1;
    }
}

void AppModbus::runTask()
{
    // 定期执行的任务 读取数据
    for (const auto &device : devices_) {
        const auto &deviceId = device.first;
        (void)thread_pool_.detach_task([this, deviceId]() {
            auto modbusApi = getDeviceApi(deviceId);
#ifdef MODBUS_DEBUG
            /*打开调试 */
            modbusApi->set_debug();
#endif
            auto mConfig = Config::getInstance().getConfig()->getModbus(deviceId);
            if (!modbusApi || !mConfig) {
                LOG(error) << "Failed to get Modbus API or config for device: " << deviceId;
                return;
            }
            while (running_) {
                processContinuousRegisters(modbusApi, mConfig);
                std::this_thread::sleep_for(std::chrono::milliseconds(mConfig->cmd_interval));
            }
        });
    }
}

void AppModbus::stop()
{
    for (const auto &device : devices_) {
        auto modbusApi = getDeviceApi(device.first);
        if (modbusApi) {
            modbusApi->stop();
        }
    }

    running_ = false;
    thread_pool_.wait();
    LOG(info) << "AppModbus  stop !";
}

std::shared_ptr<ModbusApi> AppModbus::getDeviceApi(const std::string &deviceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(deviceId);
    if (it != devices_.end()) {
        return it->second;
    }
    return nullptr;
}