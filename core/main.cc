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
    modbus.run();
    
    while (getchar() != 'q')
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    modbus.stop();
    
    
    return 0;
}
