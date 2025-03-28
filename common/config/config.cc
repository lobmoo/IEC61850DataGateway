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

    // ���� TCP ����
    const YAML::Node &tcpConfig = modbusConfig["TCP"];
    if (tcpConfig) {
        if (!tcpConfig["ip"]) {
            return false; 
        }
        modbus.tcp.ip = tcpConfig["ip"].as<std::string>();

        if (!tcpConfig["port"]) {
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
            return false; // ��������ȱʧ
        }
        modbus.rtu.port_name = rtuConfig["port_name"].as<std::string>();

        if (!rtuConfig["baudrate"]) {
            return false; // ������ȱʧ
        }
        modbus.rtu.baudrate = rtuConfig["baudrate"].as<int>();

        if (!rtuConfig["parity"]) {
            return false; // У�鷽ʽȱʧ
        }
        modbus.rtu.parity = rtuConfig["parity"].as<std::string>();

        if (!rtuConfig["slave_addr"]) {
            return false; // ��վ��ַȱʧ
        }
        modbus.rtu.slave_addr = rtuConfig["slave_addr"].as<int>();
    } else {
        return false; // ��� RTU �ڵ㲻���ڣ�ֱ�ӷ��� false
    }

    // ����ͨ������
    if (!modbusConfig["cmd_interval"]) {
        return false; // ������ȱʧ
    }
    modbus.cmd_interval = modbusConfig["cmd_interval"].as<int>();

    if (!modbusConfig["max_retries"]) {
        return false; // ������Դ���ȱʧ
    }
    modbus.max_retries = modbusConfig["max_retries"].as<int>();

    if (!modbusConfig["retry_interval"]) {
        return false; // ���Լ��ȱʧ
    }
    modbus.retry_interval = modbusConfig["retry_interval"].as<int>();

    if (!modbusConfig["type"]) {
        return false; // ����ȱʧ
    }
    modbus.Type = modbusConfig["type"].as<std::string>();

    if (!modbusConfig["device_id"]) {
        return false; // �豸 ID ȱʧ
    }
    modbus.device_id = modbusConfig["device_id"].as<std::string>();

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
            dataPoint.data_type = dataPointNode["data_type"].as<std::string>();
            dataPoint.scale = dataPointNode["scale"].as<float>();
            dataPoint.offset = dataPointNode["offset"].as<float>();

            // �洢���ݵ�����
            modbus.data_points_map[dataPoint.name] = dataPoint;
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
