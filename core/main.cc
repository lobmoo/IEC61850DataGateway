/**
 * @file main.cc
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
 
#include <unistd.h>

#include <iostream>
#include <thread>
#include <vector>

#include "modbus/app_modbus_client.h"
#include "log/logger.h"
#include "config/config.h"
#include "redis-api/app_redis.h"
#include "iec61850/iec61850ClientManger.h"
#include "iec61850/ice61850Service.h"

const std::string daemon_ = R"(
                +++++++IEC61850+++++++
                       _oo0oo_
                      o8888888o
                      88" . "88
                      (| -_- |)
                      0\  =  /0
                    ___/`---'\___
                  .' \\|     |// '.
                 / \\|||  :  |||// \
                / _||||| -:- |||||- \
               |   | \\\  - /// |   |
               | \_|  ''\---/''  |_/ |
               \  .-\__  '-'  ___/-. /
             ___'. .'  /--.--\  `. .'___
          ."" '<  `.___\_<|>_/___.' >' "".
         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
         \  \ `_.   \_ __\ /__ _/   .-` /  /
     =====`-.____`.___ \_____/___.-`___.-'=====
                       `=---='


     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    )";

int main()
{
    std::cout << daemon_ << std::endl;
    Logger::Instance().setFlushOnLevel(Logger::info);
    Logger::Instance().Init("log/myapp.log", Logger::console, Logger::info, 60, 5);

    // /*初始化配置*/
    // auto &ptr = Config::getInstance();
    // if (!ptr.init("/home/wwk/work/IEC61850DataGateway/config/")) {
    //     LOG(error) << "Failed to load configuration.";
    //     return -1;
    // }

    // ptr.getConfig()->getModbus()->to_string();
    /*初始化redis*/
    DRDSDataRedis::setDefaultConnectionInfo("", "127.0.0.1", 6379);

    // /*初始化modbus*/
    // AppModbus appModbus;
    // appModbus.run();

    /*初始化61850 Client*/
    iec61850ClientManger iec61850Client("127.0.0.1", 102);
    iec61850Client.init("/home/wwk/workspaces/IEC61850DataGateway/core/TEMPLATE.icd");

    // ice61850Service appIec61850Service;
    // if(!appIec61850Service.init("eth0")) {
    //     LOG(error) << "Failed to initialize IEC 61850 service.";
    //     return -1;
    // }
    // if (!appIec61850Service.startServer(102)) {
    //     LOG(error) << "Failed to start IEC 61850 server.";
    //     return -1;
    // }

    while (std::cin.get() != '\n') {
    }
    LOG(info) << "Press Enter to stop the program...";
    //appModbus.stop();
    return 0;
}
