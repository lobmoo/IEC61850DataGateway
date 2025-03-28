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
    std::lock_guard<std::mutex> lock(mutex_); // ������̷߳���ʱ���ݳ�ͻ
    YAML::Node config = YAML::LoadFile(filename);

    if (!data) {
        data = std::make_unique<ConfigData>();
    }
    /*�����صĽ�������*/
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

    // ���� data_points
    for (const YAML::Node &dataPointNode : config["data_points"]) {
        ConfigDataModbus::data_points_t dataPoint;

        // ����ÿ�� data_point ������
        dataPoint.name = dataPointNode["name"].as<std::string>();
        dataPoint.address = dataPointNode["address"].as<uint16_t>();
        dataPoint.type = dataPointNode["type"].as<std::string>();
        dataPoint.data_type = dataPointNode["data_type"].as<std::string>();
        dataPoint.scale = dataPointNode["scale"].as<float>();
        dataPoint.offset = dataPointNode["offset"].as<float>();
        // �洢���ݵ�����
        modbus.data_points_map[dataPoint.name] = dataPoint;
    }
    data->modbus[modbus.device_id] = modbus;
}

std::vector<std::string> Config::getDeviceConfigPaths(const std::string &baseConfigPath)
{
    std::vector<std::string> configPaths;
    try {
        // ʹ�� C++17 �ļ�ϵͳ�����Ŀ¼
        for (const auto &entry : std::filesystem::directory_iterator(baseConfigPath)) {
            // ����Ƿ��ǳ����ļ�����չ��Ϊ .yaml
            if (entry.is_regular_file() && entry.path().extension() == ".yaml") {
                configPaths.push_back(entry.path().string());
            }
        }
    } catch (const std::filesystem::filesystem_error &e) {
        LOG(error) << "File system error: " << e.what();
    }
    return configPaths;
}
