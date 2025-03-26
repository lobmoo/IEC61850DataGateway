#include "config.h"

#include <yaml-cpp/yaml.h>

Config::Config()
{
}

Config::~Config()
{
}

Config &Config::getInstance()
{
    static Config instance;
    return instance;
}

void Config::loadConfig(const std::string &filename)
{
    std::lock_guard<std::mutex> lock(mutex_); // 避免多线程访问时数据冲突
    YAML::Node config = YAML::LoadFile(filename);

    if (!data) {
        data = std::make_unique<ConfigData>();
    }

    /*添加相关的解析函数*/
    parseModbusConfig(config, data);
}

const ConfigData *Config::getConfig()
{
    return data.get();
}

void Config::parseModbusConfig(const YAML::Node &config, std::unique_ptr<ConfigData> &data)
{
    if (config["ConfigModbus"]["TCP"]["ip"])
        data->modbus.tcp.ip = config["ConfigModbus"]["TCP"]["ip"].as<std::string>();

    if (config["ConfigModbus"]["TCP"]["port"])
        data->modbus.tcp.port = config["ConfigModbus"]["TCP"]["port"].as<int>();

    if (config["ConfigModbus"]["RTU"]["port_name"])
        data->modbus.rtu.port_name = config["ConfigModbus"]["RTU"]["port_name"].as<std::string>();

    if (config["ConfigModbus"]["RTU"]["baudrate"])
        data->modbus.rtu.baudrate = config["ConfigModbus"]["RTU"]["baudrate"].as<int>();

    if (config["ConfigModbus"]["RTU"]["parity"])
        data->modbus.rtu.parity = config["ConfigModbus"]["RTU"]["parity"].as<std::string>();

    if (config["ConfigModbus"]["RTU"]["slave_addr"])
        data->modbus.rtu.slave_addr = config["ConfigModbus"]["RTU"]["slave_addr"].as<int>();

    if (config["ConfigModbus"]["cmd_interval"])
        data->modbus.cmd_interval = config["ConfigModbus"]["cmd_interval"].as<int>();

    if (config["ConfigModbus"]["max_retries"])
        data->modbus.max_retries = config["ConfigModbus"]["max_retries"].as<int>();

    if (config["ConfigModbus"]["retry_interval"])
        data->modbus.retry_interval = config["ConfigModbus"]["retry_interval"].as<int>();
}
