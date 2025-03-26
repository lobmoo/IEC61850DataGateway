#include "app_modbus.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "log/logger.h"
AppModbus::AppModbus(
    int slaveAddr, ModbusType type, int channel, int cmdInterval, const std::string& portName, int baudrate,
    Parity parity, const std::string& ip, int port, int maxRetries, int retryInterval)
    : slaveAddr(slaveAddr),
      type(type),
      channel(channel),
      cmdInterval(cmdInterval),
      portName(portName),
      baudrate(baudrate),
      parity(parity),
      ip(ip),
      port(port),
      ctx(nullptr),
      maxRetries(maxRetries),
      retryInterval(retryInterval) {
  reconnect();
}

AppModbus::~AppModbus() {
  if (ctx) {
    modbus_close(ctx);
    modbus_free(ctx);
  }
}

void AppModbus::readRegisters(uint16_t startAddr, int nbRegs, uint16_t* dest) {
  if (!ctx) {
    reconnect();
  }

  if (modbus_read_registers(ctx, startAddr, nbRegs, dest) == -1) {
    throw std::runtime_error("Failed to read registers: " + std::string(modbus_strerror(errno)));
  }
  applyCommandInterval();
}

void AppModbus::writeRegister(uint16_t addr, uint16_t value) {
  if (!ctx) {
    reconnect();
  }

  if (modbus_write_register(ctx, addr, value) == -1) {
    throw std::runtime_error("Failed to write register: " + std::string(modbus_strerror(errno)));
  }
  applyCommandInterval();
}

modbus_t* AppModbus::createModbusContext() {
  modbus_t* ctx = nullptr;
  
  if (type == ModbusType::TCP) {
    ctx = modbus_new_tcp(ip.c_str(), port);
    LOG(info) << "ModbusType::TCP" << "  " << "ip: " << ip << "  " << "port: " << port;
  } else if (type == ModbusType::RTU) {
    ctx = modbus_new_rtu(portName.c_str(), baudrate, static_cast<char>(parity), 8, 1);
    LOG(info) << "ModbusType::RTU" << "portName: " << portName << "baudrate: " << baudrate
              << "parity: " << static_cast<char>(parity);
    if (modbus_set_slave(ctx, slaveAddr) == -1) {
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

void AppModbus::applyCommandInterval() {
  if (cmdInterval > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(cmdInterval));
  }
}

void AppModbus::reconnect() {
  int retries = 0;
  while (retries <= maxRetries) {
    ctx = createModbusContext();
    if (ctx) {
      LOG(info) << "Successfully reconnected to Modbus device.";
      return;
    }

    LOG(warning) << "Reconnection attempt " << retries + 1 << "/" << maxRetries << " failed. Retrying in "
                 << retryInterval << " ms..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(retryInterval));
    retries++;
  }

  LOG(error) << "Exhausted all reconnection attempts. Modbus device is unavailable." << std::endl;
  ctx = nullptr;
}