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

    Config::getInstance().init(configPath_);
    // const ConfigData *config = Config::getInstance().getConfig();
    // ModbusType type = ModbusType::TCP;
    // if (config->modbus.Type == "TCP") {
    //     type = ModbusType::TCP;
    // } else if (config->modbus.Type == "RTU") {
    //     type = ModbusType::RTU;
    // }

    // config->modbus.to_string();
    // devices_[config->modbus.device_id] = std::make_shared<ModbusApi>(
    //     config->modbus.rtu.slave_addr, type, 0, config->modbus.cmd_interval,
    //     config->modbus.rtu.port_name, config->modbus.rtu.baudrate,
    //     config->modbus.rtu.parity == "NONE" ? Parity::NONE : Parity::ODD, config->modbus.tcp.ip,
    //     config->modbus.tcp.port, config->modbus.max_retries, config->modbus.retry_interval);

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