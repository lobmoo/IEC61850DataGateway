#ifndef APP_MODBUS_H
#define APP_MODBUS_H

#include "modbus_api.h"
#include <memory>
#include <atomic>

class AppModbus
{

public:
    AppModbus(const std::string &configPath);

    ~AppModbus();

    bool init();
    void run();
    void stop();
    void readRegisters(uint16_t startAddr, int nbRegs, uint16_t *dest);
    void writeRegister(uint16_t addr, uint16_t value);

private:
    std::shared_ptr<ModbusApi> modbusApi_;
    std::string configPath_;
    std::atomic<bool> running_;
};

#endif // app_modbus_h