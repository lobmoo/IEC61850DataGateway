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
    if (!parseModbusConfig(config, data)) {
        LOG(error) << "Failed to parse Modbus config";
    }
}

const ConfigData *Config::getConfig()
{
    return data.get();
}


bool Config::parseModbusConfig(const YAML::Node &config, std::unique_ptr<ConfigData> &data)
{
 
    ConfigDataModbus modbus;

   
    const YAML::Node &modbusConfig = config["ConfigModbus"];
    if (!modbusConfig) {
        return false; 
    }

    // 解析 TCP 配置
    const YAML::Node &tcpConfig = modbusConfig["TCP"];
    if (tcpConfig) {
        if (!tcpConfig["ip"]) {
            return false; 
        }
        modbus.tcp.ip = tcpConfig["ip"].as<std::string>();

        if (!tcpConfig["port"]) {
            return false; // 端口缺失
        }
        modbus.tcp.port = tcpConfig["port"].as<int>();
    } else {
        return false; // 如果 TCP 节点不存在，直接返回 false
    }

    // 解析 RTU 配置
    const YAML::Node &rtuConfig = modbusConfig["RTU"];
    if (rtuConfig) {
        if (!rtuConfig["port_name"]) {
            return false; // 串口名称缺失
        }
        modbus.rtu.port_name = rtuConfig["port_name"].as<std::string>();

        if (!rtuConfig["baudrate"]) {
            return false; // 波特率缺失
        }
        modbus.rtu.baudrate = rtuConfig["baudrate"].as<int>();

        if (!rtuConfig["parity"]) {
            return false; // 校验方式缺失
        }
        modbus.rtu.parity = rtuConfig["parity"].as<std::string>();

        if (!rtuConfig["slave_addr"]) {
            return false; // 从站地址缺失
        }
        modbus.rtu.slave_addr = rtuConfig["slave_addr"].as<int>();
    } else {
        return false; // 如果 RTU 节点不存在，直接返回 false
    }

    // 其他通用配置
    if (!modbusConfig["cmd_interval"]) {
        return false; // 命令间隔缺失
    }
    modbus.cmd_interval = modbusConfig["cmd_interval"].as<int>();

    if (!modbusConfig["max_retries"]) {
        return false; // 最大重试次数缺失
    }
    modbus.max_retries = modbusConfig["max_retries"].as<int>();

    if (!modbusConfig["retry_interval"]) {
        return false; // 重试间隔缺失
    }
    modbus.retry_interval = modbusConfig["retry_interval"].as<int>();

    if (!modbusConfig["type"]) {
        return false; // 类型缺失
    }
    modbus.Type = modbusConfig["type"].as<std::string>();

    if (!modbusConfig["device_id"]) {
        return false; // 设备 ID 缺失
    }
    modbus.device_id = modbusConfig["device_id"].as<std::string>();

    // 解析 data_points
    const YAML::Node &dataPoints = config["data_points"];
    if (dataPoints && dataPoints.IsSequence()) {
        for (const YAML::Node &dataPointNode : dataPoints) {
            ConfigDataModbus::data_points_t dataPoint;

            if (!dataPointNode["name"] || !dataPointNode["address"] || !dataPointNode["type"]
                || !dataPointNode["data_type"] || !dataPointNode["scale"]
                || !dataPointNode["offset"]) {
                return false; // 数据点的某个字段缺失
            }

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
    } else {
        return false; // 如果 data_points 节点不存在或不是序列，直接返回 false
    }

    // 将解析好的 Modbus 配置存储到数据结构中
    data->modbus[modbus.device_id] = modbus;

    // 解析成功
    return true;
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
