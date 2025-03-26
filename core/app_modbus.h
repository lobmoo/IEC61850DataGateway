#ifndef APP_MODBUS_H
#define APP_MODBUS_H

#include <string>
#include <modbus/modbus.h>


enum class ModbusType {
    TCP,
    RTU
};

enum class Parity {
    NONE,
    EVEN,
    ODD
};

class AppModbus {
public:
    AppModbus(int slaveAddr, ModbusType type, int channel, int cmdInterval,
              const std::string& portName, int baudrate, Parity parity,
              const std::string& ip, int port, int maxRetries = 3, int retryInterval = 1000);
    ~AppModbus();

    void readRegisters(uint16_t startAddr, int nbRegs, uint16_t* dest);
    void writeRegister(uint16_t addr, uint16_t value);

private:
    modbus_t* createModbusContext();
    void applyCommandInterval();
    void reconnect();

    int slaveAddr;
    ModbusType type;
    int channel;
    int cmdInterval;
    std::string portName;
    int baudrate;
    Parity parity;
    std::string ip;
    int port;
    modbus_t* ctx;
    int maxRetries;         // 最大重试次数
    int retryInterval;      // 重试间隔（毫秒）
};

#endif // APP_MODBUS_H