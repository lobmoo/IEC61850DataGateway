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
#include "config/config.h"
#include "log/logger.h"

#include <thread>
#include <filesystem>

AppModbus::AppModbus(const std::string &configPath)
    : configPath_(configPath), running_(true), thread_pool_(1024)
{
}

AppModbus::~AppModbus()
{
}

bool AppModbus::init()
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

    LOG(info) << "AppModbus initialized.";
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

void AppModbus::run()
{
    while (running_) {
    }
    LOG(info) << "AppModbus::stop";
}

void AppModbus::stop()
{
    running_ = false;
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