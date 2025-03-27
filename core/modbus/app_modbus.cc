#include "app_modbus.h"
#include "app_modbus.h"
#include "config/config.h"
#include "log/logger.h"

#include <thread>



AppModbus::AppModbus(const std::string &configPath) : configPath_(configPath), running_(true)
{
   

}

AppModbus::~AppModbus()
{
}

bool AppModbus::init()
{
    Config::getInstance().loadConfig(configPath_);
    const ConfigData *config = Config::getInstance().getConfig();

    ModbusType type = ModbusType::TCP;
    if(config->modbus.Type == "TCP")
    {
        type = ModbusType::TCP;
    }
    else if (config->modbus.Type == "RTU")
    {
        type = ModbusType::RTU;
    }
    config->modbus.to_string();
    modbusApi_ = std::make_shared<ModbusApi>(
        config->modbus.rtu.slave_addr, type, 0, config->modbus.cmd_interval,
        config->modbus.rtu.port_name, config->modbus.rtu.baudrate,
        config->modbus.rtu.parity == "NONE" ? Parity::NONE : Parity::ODD, config->modbus.tcp.ip, config->modbus.tcp.port,
        config->modbus.max_retries, config->modbus.retry_interval);

    return true;
}


void AppModbus::readRegisters(uint16_t startAddr, int nbRegs, uint16_t *dest)
{
    modbusApi_->readRegisters(startAddr, nbRegs, dest);
}


void AppModbus::writeRegister(uint16_t addr, uint16_t value)
{
    modbusApi_->writeRegister(addr, value);
}

void AppModbus::run()
{
    while (running_) {
        uint16_t dest[10];
        readRegisters(0, 10, dest);
        for (int i = 0; i < 10; i++) {
            LOG(info) << "dest[" << i << "] = " << dest[i];
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    LOG(info) << "AppModbus::stop";
}

void AppModbus::stop()
{
    running_ = false;
}   
