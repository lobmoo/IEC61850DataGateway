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

bool Config::init(const std::string &configPath)  // 精确到具体的yaml文件的路径
{
    try {
        loadConfig(configPath);
        LOG(debug) << "Loaded config: " << configPath;
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
    if (!parseModbusConfig(config)) {
        LOG(error) << "Failed to parse Modbus config";
    }
    if (!parse61850Config(config)) {
        LOG(error) << "Failed to parse IEC61850 config";
    }
    if (!parse104Config(config)) {
        LOG(error) << "Failed to parse 104 config";
    }
}

const ConfigData *Config::getConfig()
{
    return data.get();
}

bool Config::parse61850Config(const YAML::Node& config)
{
    const YAML::Node& devicesNode = config["IEC61850Devices"];
    if (!devicesNode) {
        LOG(warning) << "IEC61850Devices config not found";
        return true;  // 空配置是合法的
    }

    // 必须为sequence
    if (!devicesNode.IsSequence()) {
        LOG(error) << "ModbusDevices should be a sequence.";
        return false;
    }

    for (const auto& node : devicesNode) {
        ConfigDataIEC61850 iec;
        LOG(info) << "Parsing IEC61850 Device: " << node["device_id"].as<std::string>();
        // 必填项（不为空）
        iec.device_id        = node["device_id"].as<std::string>();
        iec.ip               = node["ip"].as<std::string>();
        iec.icd_file_path    = node["icd_file"].as<std::string>();
        iec.access_point     = node["access_point"].as<std::string>();
        iec.logical_device   = node["logical_device"].as<std::string>();
        iec.client_mms_name  = node["client_mms_name"].as<std::string>();

        // Todo: 非必填项，允许缺失但不能为空
        if (node["port"].IsNull()) return false;
        iec.port = node["port"].as<int>();

        if (node["poll_interval"].IsNull()) return false;
        iec.poll_interval = node["poll_interval"].as<int>();

        if (node["max_retries"].IsNull()) return false;
        iec.max_retries = node["max_retries"].as<int>();

        if (node["retry_interval"].IsNull()) return false;
        iec.retry_interval = node["retry_interval"].as<int>();

        if (node["report_enabled"].IsNull()) return false;
        iec.report_enabled = node["report_enabled"].as<bool>();

        if (node["goose_enabled"].IsNull()) return false;
        iec.goose_enabled = node["goose_enabled"].as<bool>();

        if (node["tls_enabled"].IsNull()) return false;
        iec.tls_enabled = node["tls_enabled"].as<bool>();

        if (node["description"].IsNull() || !node["description"].IsScalar() || node["description"].as<std::string>().empty()) {
            return false;
        }
        iec.description = node["description"].as<std::string>();

        if (node["data_point_filters"].IsNull() || !node["data_point_filters"].IsSequence()) {
            return false;
        }

        for (const auto& item : node["data_point_filters"]) {
            if (!item.IsScalar() || item.as<std::string>().empty()) {
                return false;
            }
            iec.data_point_filters.emplace_back(item.as<std::string>());
        }

        data->iec61850[iec.device_id] = iec;
    }

    return true;
}

bool Config::parse104Config(const YAML::Node& config)
{
    const YAML::Node& devicesNode = config["IEC104Devices"];
    // 空配置是合法的
    if (!devicesNode) {
        LOG(warning) << "IEC104Devices config not found";
        return true;  
    }

    if (!devicesNode.IsSequence()) {
        LOG(error) << "IEC104Devices should be a sequence.";
        return false;
    }

    for (const auto& node : devicesNode) {
        ConfigDataIEC104 iec;

        // 必填项（不为空）
        iec.device_id = node["device_id"].as<std::string>();
        iec.type      = node["type"].as<std::string>();
        LOG(info) << "Parsing IEC104 Device: " << iec.device_id;

        // TCP部分
        const auto& tcp_node = node["TCP"];
        iec.tcp.ip = tcp_node["ip"].as<std::string>();
        iec.tcp.port = tcp_node["port"].as<int>();

        // local_address
        const auto& local_node = node["local_address"];
        if (local_node["ip"].IsNull() || !local_node["ip"].IsScalar()) return false;
        iec.local_address.ip = local_node["ip"].as<std::string>();

        if (local_node["port"].IsNull()) return false;
        iec.local_address.port = local_node["port"].as<int>();

        // protocol.apci
        const auto& apci_node = node["protocol"]["apci"];
        iec.protocol.apci.t0 = apci_node["t0"].as<int>();
        iec.protocol.apci.t1 = apci_node["t1"].as<int>();
        iec.protocol.apci.t2 = apci_node["t2"].as<int>();
        iec.protocol.apci.t3 = apci_node["t3"].as<int>();
        iec.protocol.apci.k  = apci_node["k"].as<int>();
        iec.protocol.apci.w  = apci_node["w"].as<int>();

        // protocol.application_layer
        const auto& app_node = node["protocol"]["application_layer"];
        iec.protocol.application_layer.originator_address = app_node["originator_address"].as<int>();
        iec.protocol.application_layer.common_address     = app_node["common_address"].as<int>();
        iec.protocol.application_layer.asdu_size          = app_node["asdu_size"].as<int>();

        // communication
        const auto& comm_node = node["communication"];
        iec.communication.gi_interval_ms = comm_node["gi_interval_ms"].as<int>();
        iec.communication.report_enabled = comm_node["report_enabled"].as<bool>();

        // communication.reports
        if (comm_node["reports"].IsNull() || !comm_node["reports"].IsSequence()) {
            return false;
        }

        for (const auto& report_node : comm_node["reports"]) {
            ConfigDataIEC104::Report report;
            report.ioa = report_node["ioa"].as<int>();
            if (report_node["report_id"].IsNull() || report_node["report_id"].as<std::string>().empty()) {
                return false;
            }
            report.report_id = report_node["report_id"].as<std::string>();
            iec.communication.reports.emplace_back(report);
        }

        // error_handling
        const auto& err_node = node["error_handling"];
        iec.error_handling.reconnect_interval_ms = err_node["reconnect_interval_ms"].as<int>();
        iec.error_handling.max_reconnect_attempts = err_node["max_reconnect_attempts"].as<int>();

        // data_points
        if (node["data_points"] && node["data_points"].IsSequence()) {
            for (const auto& dp_node : node["data_points"]) {
                ConfigDataIEC104::DataPoint dp;
                dp.ioa = dp_node["ioa"].as<int>();
                dp.type = dp_node["type"].as<std::string>();
                if (dp_node["description"].IsNull() || dp_node["description"].as<std::string>().empty()) {
                    return false;
                }
                dp.description = dp_node["description"].as<std::string>();

                if (dp_node["iec61850_path"].IsNull() || dp_node["iec61850_path"].as<std::string>().empty()) {
                    return false;
                }
                dp.iec61850_path = dp_node["iec61850_path"].as<std::string>();

                iec.data_points.emplace_back(dp);
            }
        }

        data->iec104[iec.device_id] = iec;
    }

    return true;
}

bool Config::parseModbusConfig(const YAML::Node& config)
{
    const auto& devicesNode = config["ModbusDevices"];
    // 不存在ModbusDevices配置
    if (!devicesNode) {
        LOG(info) << "ModbusDevices not found, skip parsing.";
        return true;
    }
    // 必须为sequence
    if (!devicesNode.IsSequence()) {
        LOG(error) << "ModbusDevices should be a sequence.";
        return false;
    }
    for (const auto& deviceNode : devicesNode) {
        ConfigDataModbus modbus;    
        LOG(info) << "Processing Modbus device: " << deviceNode["device_id"].as<std::string>();
        modbus.device_id      = deviceNode["device_id"].as<std::string>();
        modbus.Type           = deviceNode["type"].as<std::string>();
        modbus.slave_addr     = deviceNode["slave_addr"].as<int>();
        modbus.cmd_interval   = deviceNode["cmd_interval"].as<int>();
        modbus.max_retries    = deviceNode["max_retries"].as<int>();
        modbus.retry_interval = deviceNode["retry_interval"].as<int>();
        modbus.byte_order     = deviceNode["byte_order"].as<std::string>();
        //判断type是TCP还是RTU，然后分别解析
        if (modbus.Type == "TCP") {
            const auto& tcp = deviceNode["TCP"];
            if (!tcp || tcp["ip"].IsNull() || tcp["port"].IsNull()) {
                LOG(error) << "Missing or empty TCP fields for device: " << modbus.device_id;
                return false;
            }
            modbus.tcp.ip   = tcp["ip"].as<std::string>();
            modbus.tcp.port = tcp["port"].as<int>();

            // RTU默认
            modbus.rtu.port_name = "";
            modbus.rtu.baudrate  = 0;
            modbus.rtu.parity    = "";

        } else if (modbus.Type == "RTU") {
            const auto& rtu = deviceNode["RTU"];
            if (!rtu || rtu["port_name"].IsNull() || rtu["baudrate"].IsNull() || rtu["parity"].IsNull()) {
                LOG(error) << "Missing or empty RTU fields for device: " << modbus.device_id;
                return false;
            }
            modbus.rtu.port_name = rtu["port_name"].as<std::string>();
            modbus.rtu.baudrate  = rtu["baudrate"].as<int>();
            modbus.rtu.parity    = rtu["parity"].as<std::string>();

            // TCP默认
            modbus.tcp.ip   = "0.0.0.0";
            modbus.tcp.port = 0;

        } else {
            LOG(error) << "Invalid type (must be 'TCP' or 'RTU'): " << modbus.Type;
            return false;
        }
        // 对data_points进行解析
        const auto& points = deviceNode["data_points"];
        if (!points || !points.IsSequence()) {
            LOG(error) << "Missing or invalid data_points for device: " << modbus.device_id;
            return false;
        }

        for (const auto& point : points) {
            ConfigDataModbus::data_points_t dp;
            dp.name      = point["name"].as<std::string>();
            dp.address   = point["address"].as<uint16_t>();
            dp.type      = point["type"].as<std::string>();
            dp.scale     = point["scale"].as<float>();
            dp.offset    = point["offset"].as<float>();

            std::string dt = point["data_type"].as<std::string>();
            dp.data_type = dp.getDataType(dt);
            if (dp.data_type == ConfigDataModbus::UNKNOWN) {
                LOG(error) << "Unsupported data_type: " << dt << " in point: " << dp.name;
                return false;
            }

            modbus.data_points.push_back(dp);
        }

        data->modbus[modbus.device_id] = modbus;
    }

    return true;
}

// std::vector<std::string> Config::getDeviceConfigPaths(const std::string &baseConfigPath)
// {
//     std::vector<std::string> configPaths;
//     try {
//         // 使用 C++17 文件系统库遍历目录
//         for (const auto &entry : std::filesystem::directory_iterator(baseConfigPath)) {
//             // 检查是否是常规文件且扩展名为 .yaml
//             if (entry.is_regular_file() && entry.path().extension() == ".yaml") {
//                 configPaths.push_back(entry.path().string());
//             }
//         }
//     } catch (const std::filesystem::filesystem_error &e) {
//         LOG(error) << "File system error: " << e.what();
//         throw std::runtime_error("Failed to get device config paths");
//     }
//     return configPaths;
// }
