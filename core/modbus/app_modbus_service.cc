// /**
//  * @file app_modbus_service.cc
//  * @brief 
//  * @author wwk (1162431386@qq.com)
//  * @version 1.0
//  * @date 2025-06-08
//  * 
//  * @copyright Copyright (c) 2025  by  wwk : wwk.lobmo@gmail.com
//  * 
//  * @par 修改日志:
//  * <table>
//  * <tr><th>Date       <th>Version <th>Author  <th>Description
//  * <tr><td>2025-06-08     <td>1.0     <td>wwk   <td>修改?
//  * </table>
//  */

// #include "app_modbus_service.h"
// #include "log/logger.h"

// #include <thread>
// #include <filesystem>
// #include "redis-api/app_redis.h"

// AppModBusService::AppModBusService() : running_(true), thread_pool_(1024)
// {
// }

// AppModBusService::~AppModBusService()
// {
//     stop();
// }

// bool AppModBusService::run()
// {
//     auto mConfig = Config::getInstance().getConfig()->modbus;
//     if (mConfig.empty()) {
//         LOG(error) << "Modbus configuration is empty or invalid.";
//         return false;
//     }

//     try{
//         for(auto &devices : mConfig)
//         {
//             // 对于所有device，判断其配置是否合理，并启动modbus连接 在初始化ModbusApi的时候就创建modbus连接了
//             auto deviceId = devices.first;
//             auto deviceConfig = devices.second;

//             if (deviceConfig.Type.empty() || deviceConfig.device_id.empty()) {
//                 LOG(error) << "Device " << deviceId << " has incomplete configuration, skipping.";
//                 continue;
//             }
//         }

//     }   catch (const std::exception &e) {
//         LOG(error) << "Unexpected error during Modbus service initialization: " << e.what();
//         return false;
//     }
// }
