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
    // AppModbus modbus(1, ModbusType::TCP, 0, 100, "", 0, Parity::NONE, "127.0.0.1", 1502, 5, 2000);
    // uint16_t registers[5];
    // while (1) {
    //     std::this_thread::sleep_for(std::chrono::seconds(1));

    //     modbus.readRegisters(0, 5, registers);

    //     for (int i = 0; i < 5; ++i) {
    //         std::cout << "Register " << i << ": " << registers[i] << std::endl;
    //     }
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    // }

    return 0;
}
