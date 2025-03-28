/**
 * @file config.cc
 * @brief 
 * @author wwk (1162431386@qq.com)
 * @version 1.0
 * @date 2025-03-28
 * 
 * @copyright Copyright (c) 2025  by  wwk
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2025-03-28     <td>1.0     <td>wwk   <td>修改?
 * </table>
 */

#ifndef MODBUS_API_H
#define MODBUS_API_H

#include <string>
#include <modbus/modbus.h>

enum class ModbusType { TCP, RTU };

enum class Parity { NONE, EVEN, ODD };

class ModbusApi
{
public:
    ModbusApi(
        int slaveAddr, ModbusType type, int channel, int cmdInterval, const std::string &portName,
        int baudrate, Parity parity, const std::string &ip, int port, int maxRetries = 3,
        int retryInterval = 1000);
    ~ModbusApi();

    void readRegisters(uint16_t startAddr, int nbRegs, uint16_t *dest);
    void writeRegister(uint16_t addr, uint16_t value);

private:
    modbus_t *createModbusContext();
    void applyCommandInterval();
    void reconnect();

    int slaveAddr_;
    ModbusType type_;
    int channel_;
    int cmdInterval_;
    std::string portName_;
    int baudrate_;
    Parity parity_;
    std::string ip_;
    int port_;
    modbus_t *ctx_;
    int maxRetries_;    // 最大重试次数
    int retryInterval_; // 重试间隔（毫秒）
};

#endif // APP_MODBUS_H