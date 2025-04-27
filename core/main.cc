#include <unistd.h>

#include <iostream>
#include <thread>
#include <vector>

#include "modbus/app_modbus.h"
#include "log/logger.h"
#include "config/config.h"
#include "redis-api/app_redis.h"

std::string girl = R"(
                +++++++IEC61850+++++++
                       .::::.
                     .::::::::.
                    :::::::::::
                 ..::::::::::'
              '::::::::::::'
                .::::::::::
           '::::::::::::::..
                ..::::::::::::.
              ``::::::::::::::::
               ::::``:::::::::'        .:::.
              ::::'   ':::::'       .::::::::.
            .::::'      ::::     .:::::::'::::.
           .:::'       :::::  .:::::::::' ':::::.
          .::'        :::::.:::::::::'      ':::::.
         .::'         ::::::::::::::'         ``::::.
     ...:::           ::::::::::::'              ``::.
    ````':.          ':::::::::'                  ::::..
                       '.:::::'                    ':'````..
    )";

int main()
{
    Logger::Instance().setFlushOnLevel(Logger::info);
    Logger::Instance().Init("log/myapp.log", Logger::both, Logger::trace, 60, 5);
    std::cout << girl << std::endl;
    auto &ptr = Config::getInstance();
    ptr.init("config");
    AppModbus appModbus;
    appModbus.run();
    while (1)
        ;
    return 0;
}
