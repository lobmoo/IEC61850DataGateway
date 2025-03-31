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
    
}

ModbusApi::~ModbusApi()
{
    if (ctx_) {
        modbus_close(ctx_);
        modbus_free(ctx_);
    }
}

void ModbusApi::readRegisters(uint16_t startAddr, int nbRegs, uint16_t *dest)
{
    if (!ctx_) {
        reconnect();
    }

    if (modbus_read_registers(ctx_, startAddr, nbRegs, dest) == -1) {
        throw std::runtime_error(
            "Failed to read registers: " + std::string(modbus_strerror(errno)));
    }
    applyCommandInterval();
}

void ModbusApi::writeRegister(uint16_t addr, uint16_t value)
{
    if (!ctx_) {
        reconnect();
    }

    if (modbus_write_register(ctx_, addr, value) == -1) {
        throw std::runtime_error(
            "Failed to write register: " + std::string(modbus_strerror(errno)));
    }
    applyCommandInterval();
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

void ModbusApi::applyCommandInterval()
{
    if (cmdInterval_ > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cmdInterval_));
    }
}

void ModbusApi::reconnect()
{
    int retries = 0;
    while (retries <= maxRetries_ && runing_) {
        ctx_ = createModbusContext();
        if (ctx_) {
            LOG(info) << "Successfully reconnected to Modbus device.";
            return;
        }

        LOG(warning) << "Reconnection attempt " << retries + 1 << "/" << maxRetries_
                     << " failed. Retrying in " << retryInterval_ << " ms..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(retryInterval_));
        retries++;
    }

    LOG(error) << "Exhausted all reconnection attempts. Modbus device is unavailable." << std::endl;
    ctx_ = nullptr;
}

void ModbusApi::stop()
{
    runing_ = false;
}