/**
 * @file config.cc
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
    try {
        std::vector<std::string> configPaths = getDeviceConfigPaths(BasePath);
        for (const auto &configPath : configPaths) {
            loadConfig(configPath);
            LOG(debug) << "Loaded config: " << configPath;
        }
    } catch (const std::exception &e) {
        LOG(error) << "Failed to load configuration files: " << e.what();
        return false; // 如果加载配置文件失败，返回 false
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
    // if (!parseModbusConfig(config, data)) {
    //     LOG(error) << "Failed to parse Modbus config";
    // }
    if (!parse61850Config(config, data)) {
        LOG(error) << "Failed to parse IEC61850 config";
    }
}

const ConfigData *Config::getConfig()
{
    return data.get();
}

bool Config::parse61850Config(const YAML::Node &config, std::unique_ptr<ConfigData> &data)
{
    ConfigDataIEC61850 iec61850;
    const YAML::Node &rootIEC61850 = config["config61850"];
    if (!rootIEC61850) {
        LOG(warning) << "IEC61850 config node not found";
        return true; // 如果配置节点不存在，直接返回 false
    }

    const YAML::Node &iec61850Config = rootIEC61850["general61850"];
    if (!iec61850Config) {
        LOG(error) << "IEC61850 general61850 node not found";
        return false; // 如果 general61850 节点不存在，直接返回 false
    }

    if (!iec61850Config["name_data_object"]) {
        LOG(error) << "IEC61850 device_id not found";
        return false; // 设备 ID 缺失
    }
    iec61850.device_id = iec61850Config["name_data_object"].as<std::string>();
    if (!iec61850Config["device_model"]) {
        LOG(error) << "IEC61850 icd_file_path not found";
        return false; // ICD 文件路径缺失
    }
    iec61850.icd_file_path = iec61850Config["device_model"].as<std::string>();
    if (!iec61850Config["server_ip"]) {
        LOG(error) << "IEC61850 IP address not found";
        return false; // IP 地址缺失
    }
    iec61850.ip = iec61850Config["server_ip"].as<std::string>();
    if (!iec61850Config["server_port"]) {
        LOG(error) << "IEC61850 port not found";
        return false; // 端口缺失
    }
    iec61850.port = iec61850Config["server_port"].as<int>();
    // 将解析好的 IEC61850 配置存储到数据结构中

    // 将解析好的 Modbus 配置存储到数据结构中
    data->iec61850[iec61850.device_id] = iec61850;
    return true;
}

bool Config::parse104Config(const YAML::Node &config, std::unique_ptr<ConfigData> &data)
{
    return true;
}

bool Config::parseModbusConfig(const YAML::Node &config, std::unique_ptr<ConfigData> &data)
{

    ConfigDataModbus modbus;

    const YAML::Node &modbusConfig = config["configModbus"];
    if (!modbusConfig) {
        LOG(warning) << "Modbus config node not found";
        return true;
    }

    // 解析 TCP 配置
    const YAML::Node &tcpConfig = modbusConfig["TCP"];
    if (tcpConfig) {
        if (!tcpConfig["ip"]) {
            LOG(error) << "TCP IP address not found";
            return false;
        }
        modbus.tcp.ip = tcpConfig["ip"].as<std::string>();

        if (!tcpConfig["port"]) {
            LOG(error) << "TCP port not found";
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
            LOG(error) << "RTU port_name not found";
            return false; // 串口名称缺失
        }
        modbus.rtu.port_name = rtuConfig["port_name"].as<std::string>();

        if (!rtuConfig["baudrate"]) {
            LOG(error) << "RTU baudrate not found";
            return false; // 波特率缺失
        }
        modbus.rtu.baudrate = rtuConfig["baudrate"].as<int>();

        if (!rtuConfig["parity"]) {
            LOG(error) << "RTU parity not found";
            return false; // 校验方式缺失
        }
        modbus.rtu.parity = rtuConfig["parity"].as<std::string>();
    } else {
        return false; // 如果 RTU 节点不存在，直接返回 false
    }

    // 其他通用配置
    if (!modbusConfig["cmd_interval"]) {
        LOG(error) << "Modbus command interval not found";
        return false; // 命令间隔缺失
    }
    modbus.cmd_interval = modbusConfig["cmd_interval"].as<int>();

    if (!modbusConfig["slave_addr"]) {
        LOG(error) << "Modbus slave address not found";
        return false; // 从站地址缺失
    }
    modbus.slave_addr = modbusConfig["slave_addr"].as<int>();

    if (!modbusConfig["max_retries"]) {
        LOG(error) << "Modbus max retries not found";
        return false; // 最大重试次数缺失v
    }
    modbus.max_retries = modbusConfig["max_retries"].as<int>();

    if (!modbusConfig["retry_interval"]) {
        LOG(error) << "Modbus retry interval not found";
        return false; // 重试间隔缺失
    }
    modbus.retry_interval = modbusConfig["retry_interval"].as<int>();

    if (!modbusConfig["type"]) {
        LOG(error) << "Modbus type not found";
        return false; // 类型缺失
    }
    modbus.Type = modbusConfig["type"].as<std::string>();

    if (!modbusConfig["device_id"]) {
        LOG(error) << "Modbus device_id not found";
        return false; // 设备 ID 缺失
    }
    modbus.device_id = modbusConfig["device_id"].as<std::string>();
    modbus.byte_order = modbusConfig["byte_order"].as<std::string>();

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
            dataPoint.data_type =
                dataPoint.getDataType(dataPointNode["data_type"].as<std::string>());
            dataPoint.scale = dataPointNode["scale"].as<float>();
            dataPoint.offset = dataPointNode["offset"].as<float>();

            // 存储数据点配置
            modbus.data_points.push_back(dataPoint);
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
        throw std::runtime_error("Failed to get device config paths");
    }
    return configPaths;
}
