#pragma once


#include <string>
#include <stdexcept>
#include  "modbus/modbus.h"

class AppModbus {
public:
    enum class ModbusType {
        TCP,
        RTU
    };

    enum class Parity {
        NONE,
        EVEN,
        ODD
    };

    AppModbus(int slaveAddr, ModbusType type, int channel, int cmdInterval,
              const std::string& portName = "", int baudrate = 9600, Parity parity = Parity::NONE,
              const std::string& ip = "", int port = 502);
    ~AppModbus();

    void readRegisters(uint16_t startAddr, int nbRegs, uint16_t* dest);
    void writeRegister(uint16_t addr, uint16_t value);

private:
    modbus_t* ctx;
    int slaveAddr;
    ModbusType type;
    int channel;
    int cmdInterval;
    std::string portName;
    int baudrate;
    Parity parity;
    std::string ip;
    int port;

    modbus_t* createModbusContext();
};

