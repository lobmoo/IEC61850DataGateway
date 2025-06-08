/**
 * @file app_modbus_client.h
 * @brief 
 * @author wwk (1162431386@qq.com)
 * @version 1.0
 * @date 2025-06-08
 * 
 * @copyright Copyright (c) 2025  by  wwk : wwk.lobmo@gmail.com
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2025-06-08     <td>1.0     <td>wwk   <td>修改?
 * </table>
 */

#ifndef APP_MODBUS_H
#define APP_MODBUS_H

#include "modbus_api.h"
#include <memory>
#include <atomic>
#include <map>
#include <BS_thread_pool.hpp>

#include "config/config.h"

struct RegisterRange {
    uint16_t start_addr;
    int count;
    size_t end_index; // 结束的数据点索引
};

class AppModbus
{

public:
    AppModbus();
    ~AppModbus();
    bool run();
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
    RegisterRange findContinuousRegisters(
        const std::vector<ConfigDataModbus::data_points_t> &points, size_t start_index);
    void processContinuousRegisters(
        std::shared_ptr<ModbusApi> modbusApi, const ConfigDataModbus *config);
    bool processUserData(
        const std::vector<uint16_t> &dataBuffer, std::vector<ConfigDataModbus::data_points_t> &dataPoints);
    void runTask();
};

#endif // app_modbus_h