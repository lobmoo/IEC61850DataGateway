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

#include "modbus_api.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "log/logger.h"

ModbusApi::ModbusApi(
    int slaveAddr, ModbusType type, int channel, int cmdInterval, const std::string &portName,
    int baudrate, Parity parity, const std::string &ip, int port, int maxRetries, int retryInterval)
    : slaveAddr_(slaveAddr), type_(type), channel_(channel), cmdInterval_(cmdInterval),
      portName_(portName), baudrate_(baudrate), parity_(parity), ip_(ip), port_(port),
      ctx_(nullptr), maxRetries_(maxRetries), retryInterval_(retryInterval), runing_(true)
{
    reconnectThread_ = std::thread(&ModbusApi::reconnectLoop, this);
}

ModbusApi::~ModbusApi()
{
    if (ctx_) {
        modbus_close(ctx_);
        modbus_free(ctx_);
    }
}

bool ModbusApi::readRegisters(uint16_t startAddr, int nbRegs, uint16_t *dest)
{
    std::lock_guard<std::mutex> lock(ctxMutex_);
    if (!ctx_) {
        LOG(error) << "No valid Modbus context available.";
        return false;
    }

    if (modbus_read_registers(ctx_, startAddr, nbRegs, dest) == -1) {
        return false;
        LOG(error) << "Failed to read registers: " + std::string(modbus_strerror(errno));
    }
    return true;
}

bool ModbusApi::writeRegister(uint16_t addr, uint16_t value)
{
    std::lock_guard<std::mutex> lock(ctxMutex_);
    if (!ctx_) {
        LOG(error) << "No valid Modbus context available.";
        return false;
    }

    if (modbus_write_register(ctx_, addr, value) == -1) {
        return false;
        LOG(error) << "Failed to write registers: " + std::string(modbus_strerror(errno));
    }
    return true;
}

void ModbusApi::reconnectLoop()
{
    int retries = 0;
    while (runing_) {
        if (!ctx_) {
            // 检查是否超过最大重连次数
            if (maxRetries_ > 0 && retries >= maxRetries_) {
                LOG(error) << "Exhausted all reconnection attempts. Modbus device is unavailable.";
                break; // 超过最大重连次数后退出循环
            }
            ctx_ = createModbusContext();
            if (ctx_) {
                LOG(info) << "Successfully reconnected to Modbus device.";
                retries = 0; // 重置重连计数器
            } else {
                LOG(warning) << "Reconnection attempt " << retries + 1 << "/" << maxRetries_
                             << " failed. Retrying in " << retryInterval_ << " ms...";
                retries++;
                std::this_thread::sleep_for(std::chrono::milliseconds(retryInterval_));
            }
        } else {
            // 如果连接正常，等待一段时间再检查
            std::this_thread::sleep_for(std::chrono::milliseconds(retryInterval_));
        }
    }
}

modbus_t *ModbusApi::createModbusContext()
{
    modbus_t *ctx = nullptr;

    if (type_ == ModbusType::TCP) {
        ctx = modbus_new_tcp(ip_.c_str(), port_);
        LOG(info) << "ModbusType::TCP" << "  " << "ip: " << ip_ << "  " << "port: " << port_;
    } else if (type_ == ModbusType::RTU) {
        ctx = modbus_new_rtu(portName_.c_str(), baudrate_, static_cast<char>(parity_), 8, 1);
        LOG(info) << "ModbusType::RTU" << "portName: " << portName_ << "baudrate: " << baudrate_
                  << "parity: " << static_cast<char>(parity_);
        if (modbus_set_slave(ctx, slaveAddr_) == -1) {
            LOG(error) << "Failed to set Modbus slave address: " << modbus_strerror(errno);
            modbus_free(ctx);
            return nullptr;
        }
    } else {
        LOG(error) << "ModbusType is not supported.";
    }

    if (!ctx) {
        return nullptr;
    }

    if (modbus_connect(ctx) == -1) {
        LOG(error) << "Failed to connect to Modbus device: " << modbus_strerror(errno);
        modbus_free(ctx);
        return nullptr;
    }
    return ctx;
}


void ModbusApi::stop()
{
    runing_ = false;
}