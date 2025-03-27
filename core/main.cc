#include <unistd.h>

#include <iostream>
#include <thread>
#include <vector>

#include "modbus/app_modbus.h"
#include "log/logger.h"

int main()
{
    Logger::Instance().setFlushOnLevel(Logger::info);
    Logger::Instance().Init("log/myapp.log", Logger::both, Logger::trace, 60, 5);
    AppModbus modbus("config/config.yaml");
    modbus.init();
    std::thread t1([&modbus]() {
            modbus.run();
        });
    t1.detach();
    while (std::cin.get() != '\n') {
    }
    modbus.stop(); 
    return 0;
}
