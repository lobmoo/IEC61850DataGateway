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

#ifndef APP_MODBUS_H
#define APP_MODBUS_H

#include "modbus_api.h"
#include <memory>
#include <atomic>
#include <map>
#include <BS_thread_pool.hpp>

class AppModbus
{

public:
    AppModbus(const std::string &configPath);

    ~AppModbus();

    bool init();
    void run();
    void stop();
   
private:
    std::string configPath_;
    std::atomic<bool> running_;
    BS::thread_pool<> thread_pool_;
    std::map<std::string, std::shared_ptr<ModbusApi>> devices_;
    std::mutex mutex_;

private:
    std::shared_ptr<ModbusApi> getDeviceApi(const std::string &deviceId);
    void readRegisters(const std::string &deviceId, uint16_t startAddr, int nbRegs, uint16_t *dest);
    void writeRegister(const std::string &deviceId, uint16_t addr, uint16_t value);

};

#endif // app_modbus_h