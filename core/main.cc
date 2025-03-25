#include <unistd.h>

#include <iostream>
#include <thread>
#include <vector>

#include "app_modbus.h"
#include "log/logger.h"

int main() {
  Logger::Instance().setFlushOnLevel(Logger::info);
  Logger::Instance().Init("log/myapp.log", Logger::both, Logger::trace, 60, 5);
  AppModbus::ModbusType type = AppModbus::ModbusType::TCP;
  AppModbus modbus(1, type, 0, 100, "", 0, AppModbus::Parity::NONE, "192.168.1.100", 502);

  uint16_t regs[10];
  modbus.readRegisters(0, 10, regs);
  std::cout << "Read registers: ";
  for (auto reg : regs) {
      std::cout << reg << " ";
  }
  std::cout << std::endl;

  return 0;
}
