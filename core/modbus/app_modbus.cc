#include "app_modbus.h"
#include "app_modbus.h"
#include "config/config.h"
#include "log/logger.h"


AppModbus::AppModbus()
{
    Config::getInstance().loadConfig("config/config.yaml");


}

AppModbus::~AppModbus()
{
}

