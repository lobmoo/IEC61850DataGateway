#include "app_modbus.h"
#include <chrono>
#include <thread>

AppModbus::AppModbus(int slaveAddr, ModbusType type, int channel, int cmdInterval,
                     const std::string& portName, int baudrate, Parity parity,
                     const std::string& ip, int port)
    : slaveAddr(slaveAddr), type(type), channel(channel), cmdInterval(cmdInterval),
      portName(portName), baudrate(baudrate), parity(parity), ip(ip), port(port) {
    ctx = createModbusContext();
    if (!ctx) {
        throw std::runtime_error("Failed to create Modbus context");
    }
}

AppModbus::~AppModbus() {
    if (ctx) {
        modbus_close(ctx);
        modbus_free(ctx);
    }
}

void AppModbus::readRegisters(uint16_t startAddr, int nbRegs, uint16_t* dest) {
    if (modbus_read_registers(ctx, startAddr, nbRegs, dest) == -1) {
        throw std::runtime_error("Failed to read registers: " + std::string(modbus_strerror(errno)));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(cmdInterval));
}

void AppModbus::writeRegister(uint16_t addr, uint16_t value) {
    if (modbus_write_register(ctx, addr, value) == -1) {
        throw std::runtime_error("Failed to write register: " + std::string(modbus_strerror(errno)));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(cmdInterval));
}

modbus_t* AppModbus::createModbusContext() {
    modbus_t* ctx = nullptr;
    if (type == ModbusType::TCP) {
        ctx = modbus_new_tcp(ip.c_str(), port);
    } else if (type == ModbusType::RTU) {
        ctx = modbus_new_rtu(portName.c_str(), baudrate, static_cast<char>(parity), 8, 1);
    }

    if (!ctx) {
        return nullptr;
    }

    modbus_set_slave(ctx, slaveAddr);

    if (modbus_connect(ctx) == -1) {
        modbus_free(ctx);
        return nullptr;
    }

    return ctx;
}