/**
 * @file config.cc
 * @brief 
 * @author wwk (1162431386@qq.com)
 * @version 1.0
 * @date 2025-06-08
 * 
 * @copyright Copyright (c) 2025  by  wwk : wwk.lobmo@gmail.com
 * 
 * @par �޸���־:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2025-06-08     <td>1.0     <td>wwk   <td>�޸�?
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
        return true; // ������ýڵ㲻���ڣ�ֱ�ӷ��� false
    }

    const YAML::Node &iec61850Config = rootIEC61850["general61850"];
    if (!iec61850Config) {
        LOG(error) << "IEC61850 general61850 node not found";
        return false; // ��� general61850 �ڵ㲻���ڣ�ֱ�ӷ��� false
    }

    if (!iec61850Config["name_data_object"]) {
        LOG(error) << "IEC61850 device_id not found";
        return false; // �豸 ID ȱʧ
    }
    iec61850.device_id = iec61850Config["name_data_object"].as<std::string>();
    if (!iec61850Config["device_model"]) {
        LOG(error) << "IEC61850 icd_file_path not found";
        return false; // ICD �ļ�·��ȱʧ
    }
    iec61850.icd_file_path = iec61850Config["device_model"].as<std::string>();
    if (!iec61850Config["server_ip"]) {
        LOG(error) << "IEC61850 IP address not found";
        return false; // IP ��ַȱʧ
    }
    iec61850.ip = iec61850Config["server_ip"].as<std::string>();
    if (!iec61850Config["server_port"]) {
        LOG(error) << "IEC61850 port not found";
        return false; // �˿�ȱʧ
    }
    iec61850.port = iec61850Config["server_port"].as<int>();
    // �������õ� IEC61850 ���ô洢�����ݽṹ��

     // �������õ� Modbus ���ô洢�����ݽṹ��
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

    // ���� TCP ����
    const YAML::Node &tcpConfig = modbusConfig["TCP"];
    if (tcpConfig) {
        if (!tcpConfig["ip"]) {
            LOG(error) << "TCP IP address not found";
            return false;
        }
        modbus.tcp.ip = tcpConfig["ip"].as<std::string>();

        if (!tcpConfig["port"]) {
            LOG(error) << "TCP port not found";
            return false; // �˿�ȱʧ
        }
        modbus.tcp.port = tcpConfig["port"].as<int>();
    } else {
        return false; // ��� TCP �ڵ㲻���ڣ�ֱ�ӷ��� false
    }

    // ���� RTU ����
    const YAML::Node &rtuConfig = modbusConfig["RTU"];
    if (rtuConfig) {
        if (!rtuConfig["port_name"]) {
            LOG(error) << "RTU port_name not found";
            return false; // ��������ȱʧ
        }
        modbus.rtu.port_name = rtuConfig["port_name"].as<std::string>();

        if (!rtuConfig["baudrate"]) {
            LOG(error) << "RTU baudrate not found";
            return false; // ������ȱʧ
        }
        modbus.rtu.baudrate = rtuConfig["baudrate"].as<int>();

        if (!rtuConfig["parity"]) {
            LOG(error) << "RTU parity not found";
            return false; // У�鷽ʽȱʧ
        }
        modbus.rtu.parity = rtuConfig["parity"].as<std::string>();
    } else {
        return false; // ��� RTU �ڵ㲻���ڣ�ֱ�ӷ��� false
    }

    // ����ͨ������
    if (!modbusConfig["cmd_interval"]) {
        LOG(error) << "Modbus command interval not found";
        return false; // ������ȱʧ
    }
    modbus.cmd_interval = modbusConfig["cmd_interval"].as<int>();

    if (!modbusConfig["slave_addr"]) {
        LOG(error) << "Modbus slave address not found";
        return false; // ��վ��ַȱʧ
    }
    modbus.slave_addr = modbusConfig["slave_addr"].as<int>();

    if (!modbusConfig["max_retries"]) {
        LOG(error) << "Modbus max retries not found";
        return false; // ������Դ���ȱʧv
    }
    modbus.max_retries = modbusConfig["max_retries"].as<int>();

    if (!modbusConfig["retry_interval"]) {
        LOG(error) << "Modbus retry interval not found";
        return false; // ���Լ��ȱʧ
    }
    modbus.retry_interval = modbusConfig["retry_interval"].as<int>();

    if (!modbusConfig["type"]) {
        LOG(error) << "Modbus type not found";
        return false; // ����ȱʧ
    }
    modbus.Type = modbusConfig["type"].as<std::string>();

    if (!modbusConfig["device_id"]) {
        LOG(error) << "Modbus device_id not found";
        return false; // �豸 ID ȱʧ
    }
    modbus.device_id = modbusConfig["device_id"].as<std::string>();
    modbus.byte_order = modbusConfig["byte_order"].as<std::string>();

    // ���� data_points
    const YAML::Node &dataPoints = config["data_points"];
    if (dataPoints && dataPoints.IsSequence()) {
        for (const YAML::Node &dataPointNode : dataPoints) {
            ConfigDataModbus::data_points_t dataPoint;

            if (!dataPointNode["name"] || !dataPointNode["address"] || !dataPointNode["type"]
                || !dataPointNode["data_type"] || !dataPointNode["scale"]
                || !dataPointNode["offset"]) {
                return false; // ���ݵ��ĳ���ֶ�ȱʧ
            }

            // ����ÿ�� data_point ������
            dataPoint.name = dataPointNode["name"].as<std::string>();
            dataPoint.address = dataPointNode["address"].as<uint16_t>();
            dataPoint.type = dataPointNode["type"].as<std::string>();
            dataPoint.data_type =
                dataPoint.getDataType(dataPointNode["data_type"].as<std::string>());
            dataPoint.scale = dataPointNode["scale"].as<float>();
            dataPoint.offset = dataPointNode["offset"].as<float>();

            // �洢���ݵ�����
            modbus.data_points.push_back(dataPoint);
        }
    } else {
        return false; // ��� data_points �ڵ㲻���ڻ������У�ֱ�ӷ��� false
    }

    // �������õ� Modbus ���ô洢�����ݽṹ��
    data->modbus[modbus.device_id] = modbus;

    // �����ɹ�
    return true;
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
