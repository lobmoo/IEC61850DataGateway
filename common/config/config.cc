#include "config.h"
#include <filesystem>
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


bool Config::init(const std::string &BasePath) 
{
    std::vector<std::string> configPaths = getDeviceConfigPaths(BasePath);
    for (const auto &configPath : configPaths) {
        loadConfig(configPath);
        LOG(debug) << "Loaded config: " << configPath;
    }
    return true;
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
    ConfigDataModbus modbus;
    if (config["ConfigModbus"]["TCP"]["ip"])
        modbus.tcp.ip = config["ConfigModbus"]["TCP"]["ip"].as<std::string>();

    if (config["ConfigModbus"]["TCP"]["port"])
        modbus.tcp.port = config["ConfigModbus"]["TCP"]["port"].as<int>();

    if (config["ConfigModbus"]["RTU"]["port_name"])
        modbus.rtu.port_name = config["ConfigModbus"]["RTU"]["port_name"].as<std::string>();

    if (config["ConfigModbus"]["RTU"]["baudrate"])
        modbus.rtu.baudrate = config["ConfigModbus"]["RTU"]["baudrate"].as<int>();

    if (config["ConfigModbus"]["RTU"]["parity"])
        modbus.rtu.parity = config["ConfigModbus"]["RTU"]["parity"].as<std::string>();

    if (config["ConfigModbus"]["RTU"]["slave_addr"])
        modbus.rtu.slave_addr = config["ConfigModbus"]["RTU"]["slave_addr"].as<int>();

    if (config["ConfigModbus"]["cmd_interval"])
        modbus.cmd_interval = config["ConfigModbus"]["cmd_interval"].as<int>();

    if (config["ConfigModbus"]["max_retries"])
        modbus.max_retries = config["ConfigModbus"]["max_retries"].as<int>();

    if (config["ConfigModbus"]["retry_interval"])
        modbus.retry_interval = config["ConfigModbus"]["retry_interval"].as<int>();

    if (config["ConfigModbus"]["retry_interval"])
        modbus.retry_interval = config["ConfigModbus"]["retry_interval"].as<int>();

    if (config["ConfigModbus"]["type"])
        modbus.Type = config["ConfigModbus"]["type"].as<std::string>();

    if (config["ConfigModbus"]["device_id"])
        modbus.device_id = config["ConfigModbus"]["device_id"].as<std::string>();

    // 解析 data_points
    for (const YAML::Node &dataPointNode : config["data_points"]) {
        ConfigDataModbus::data_points_t dataPoint;

        // 解析每个 data_point 的配置
        dataPoint.name = dataPointNode["name"].as<std::string>();
        dataPoint.address = dataPointNode["address"].as<uint16_t>();
        dataPoint.type = dataPointNode["type"].as<std::string>();
        dataPoint.data_type = dataPointNode["data_type"].as<std::string>();
        dataPoint.scale = dataPointNode["scale"].as<float>();
        dataPoint.offset = dataPointNode["offset"].as<float>();
        // 存储数据点配置
        modbus.data_points_map[dataPoint.name] = dataPoint;
    }
    data->modbus[modbus.device_id] = modbus;
}

std::vector<std::string> Config::getDeviceConfigPaths(const std::string &baseConfigPath)
{
    std::vector<std::string> configPaths;
    try {
        // 使用 C++17 文件系统库遍历目录
        for (const auto &entry : std::filesystem::directory_iterator(baseConfigPath)) {
            // 检查是否是常规文件且扩展名为 .yaml
            if (entry.is_regular_file() && entry.path().extension() == ".yaml") {
                configPaths.push_back(entry.path().string());
            }
        }
    } catch (const std::filesystem::filesystem_error &e) {
        LOG(error) << "File system error: " << e.what();
    }
    return configPaths;
}
