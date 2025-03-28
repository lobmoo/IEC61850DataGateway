#include <unistd.h>

#include <iostream>
#include <thread>
#include <vector>

#include "modbus/app_modbus.h"
#include "log/logger.h"
#include "config/config.h"

int main()
{
    Logger::Instance().setFlushOnLevel(Logger::info);
    Logger::Instance().Init("log/myapp.log", Logger::both, Logger::trace, 60, 5);
    auto &ptr = Config::getInstance();
    ptr.init("config");
    ptr.getConfig()->getModbus("test_1")->to_string();

    return 0;
}
