/**
 * @file config.cc
 * @brief 
 * @author wwk (1162431386@qq.com)
 * @version 1.0
 * @date 2025-03-28
 * 
 * @copyright Copyright (c) 2025  by  wwk
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2025-03-28     <td>1.0     <td>wwk   <td>修改?
 * </table>
 */

#include "app_modbus.h"
#include "log/logger.h"

#include <thread>
#include <filesystem>

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
                    config.rtu.slave_addr, type, 0, config.cmd_interval, config.rtu.port_name,
                    config.rtu.baudrate, config.rtu.parity == "NONE" ? Parity::NONE : Parity::ODD,
                    config.tcp.ip, config.tcp.port, config.max_retries, config.retry_interval);
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

    (void)thread_pool_.submit_task([this]() { runTask(); });
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
    if (start_index >= points.size()) {
        return {0, 0, start_index};
    }

    int startAddr = points[start_index].address;
    int currentAddr = startAddr;
    size_t j = start_index;

    while (j < points.size()
           && (currentAddr + ((points[j].data_type == "float32") ? 2 : 1))
                  <= (points[j].address + 1)) {
        int regCount = (points[j].data_type == "float32") ? 2 : 1;
        currentAddr = points[j].address + regCount;
        j++;
    }

    return {startAddr, currentAddr - startAddr, j};
}

void AppModbus::processContinuousRegisters(
    std::shared_ptr<ModbusApi> modbusApi, const ConfigDataModbus *config)
{
    auto points = config->data_points;
    if (points.empty())
        return;

    size_t i = 0;

    while (i < points.size()) {
        RegisterRange range = findContinuousRegisters(points, i);
        std::vector<uint16_t> buffer(range.count);

        // 批量读取
        bool success = modbusApi->readRegisters(range.start_addr, range.count, buffer.data());
        if (!success) {
            LOG(error) << "Failed to read " << " registers from " << range.start_addr << " count: " << range.count;
            i = range.end_index;
            continue;
        } 
        // 解析数据
        for (size_t k = i; k < range.end_index; k++) {
            const auto point = points[k];
            int offset = point.address - range.start_addr;

            float value = 0.0f;
            if (point.data_type == "float32") {
                uint32_t combined = (config->byte_order == "big")
                                        ? (buffer[offset] << 16) | buffer[offset + 1]
                                        : (buffer[offset + 1] << 16) | buffer[offset];
                value = *reinterpret_cast<float *>(&combined);
            } else {
                value = static_cast<int16_t>(buffer[offset]);
            }

            float finalValue = value * point.scale + point.offset;
            LOG(info) << "Device: " << config->device_id << " | " << point.name << " = "
                      << finalValue << " (raw: " << value << ")";
        }
        i = range.end_index;
    }
}

void AppModbus::runTask()
{
    // 定期执行的任务 读取数据
    for (const auto &device : devices_) {
        const auto &deviceId = device.first;
        (void)thread_pool_.submit_task([this, deviceId]() {
            auto modbusApi = getDeviceApi(deviceId);
            auto mConfig = Config::getInstance().getConfig()->getModbus(deviceId);
            if (!modbusApi || !mConfig) {
                LOG(error) << "Failed to get Modbus API or config for device: " << deviceId;
                return;
            }
            while (running_) {
                processContinuousRegisters(
                    modbusApi, mConfig);
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
    LOG(info) << "AppModbus::stop";
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