/**
 * @file config.h
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

#ifndef CONFIG_H
#define CONFIG_H

#include <yaml-cpp/yaml.h>
#include <unordered_map>
#include <mutex>
#include <string>
#include <log/logger.h>

struct ConfigDataModbus {
    enum data_type_e {
        UNKNOWN = -1,
        INT16,
        UINT16,
        INT32,
        UINT32,
        FLOAT32,
        STRING,
    };

    struct TCP {
        std::string ip;
        int port;
    };
    struct RTU {
        std::string port_name;
        int baudrate;
        std::string parity;
    };

    struct data_points_t {
        std::string name;
        uint16_t address;
        std::string type;
        data_type_e data_type;
        float scale;
        float offset;

        data_type_e getDataType(const std::string &type) const
        {
            if (type == "int16")
                return INT16;
            else if (type == "uint16")
                return UINT16;
            else if (type == "int32")
                return INT32;
            else if (type == "uint32")
                return UINT32;
            else if (type == "float32")
                return FLOAT32;
            else
                return UNKNOWN;
        }

        std::string getDataTypeString(data_type_e type) const
        {
            switch (type) {
                case INT16:
                    return "int16";
                case UINT16:
                    return "uint16";
                case INT32:
                    return "int32";
                case UINT32:
                    return "uint32";
                case FLOAT32:
                    return "float32";
                default:
                    return "unknown";
            }
        }

        uint32_t getDataSize(data_type_e type) const
        {
            switch (type) {
                case INT16:
                case UINT16:
                    return 2;
                case INT32:
                case UINT32:
                case FLOAT32:
                    return 4;
                default:
                    return 0;
            }
        }

        void to_string()
        {
            LOG(info) << "name: " << name;
            LOG(info) << "address: " << address;
            LOG(info) << "type: " << type;
            LOG(info) << "data_type: " << getDataTypeString(data_type);
            LOG(info) << "scale: " << scale;
            LOG(info) << "offset: " << offset;
        }
    };

    std::string device_id;
    std::string Type;
    TCP tcp;
    RTU rtu;
    int slave_addr;
    int cmd_interval;
    int max_retries;
    int retry_interval;
    std::string byte_order;
    std::vector<data_points_t> data_points;

    void to_string() const
    {
        LOG(info) << "device_id: " << device_id;
        LOG(info) << "Type: " << Type;
        LOG(info) << "cmd_interval: " << cmd_interval;
        LOG(info) << "max_retries: " << max_retries;
        LOG(info) << "retry_interval: " << retry_interval;
        LOG(info) << "slave_addr: " << slave_addr;
        LOG(info) << "tcp.ip: " << tcp.ip;
        LOG(info) << "tcp.port: " << tcp.port;
        LOG(info) << "rtu.port_name: " << rtu.port_name;
        LOG(info) << "rtu.baudrate: " << rtu.baudrate;
        LOG(info) << "rtu.parity: " << rtu.parity;
        LOG(info) << "byte_order: " << byte_order;
        for (auto &data_point : data_points) {
            LOG(info) << "data_point.name: " << data_point.name;
            LOG(info) << "data_point.address: " << data_point.address;
            LOG(info) << "data_point.type: " << data_point.type;
            LOG(info) << "data_point.data_type: " << data_point.data_type;
            LOG(info) << "data_point.scale: " << data_point.scale;
            LOG(info) << "data_point.offset: " << data_point.offset;
            LOG(info) << "\n------------------------\n";
        }
    }
};

struct ConfigDataIEC61850 {
    //必填项
    std::string device_id;
    std::string ip;
    std::string icd_file_path;
    std::string access_point;
    std::string logical_device;
    std::string client_mms_name;
    //非必填项
    int port;
    int poll_interval;
    int max_retries;
    int retry_interval;
    bool report_enabled;
    bool goose_enabled; 
    bool tls_enabled;
    std::string description;
    std::vector<std::string> data_point_filters;


    void to_string() const
    {
        // 必填项
        LOG(info) << "device_id: " << device_id;
        LOG(info) << "ip: " << ip;
        LOG(info) << "icd_file_path: " << icd_file_path;
        LOG(info) << "access_point: " << access_point;
        LOG(info) << "logical_device: " << logical_device;
        LOG(info) << "client_mms_name: " << client_mms_name;
        // 非必填项
        LOG(info) << "port: " << port;
        LOG(info) << "poll_interval: " << poll_interval;
        LOG(info) << "max_retries: " << max_retries;
        LOG(info) << "retry_interval: " << retry_interval;
        LOG(info) << "report_enabled: " << report_enabled;
        LOG(info) << "goose_enabled: " << goose_enabled;
        LOG(info) << "tls_enabled: " << tls_enabled;
        LOG(info) << "description: " << description;
        LOG(info) << "data_point_filters: ";       
        for (const auto &filter : data_point_filters) 
        {
            LOG(info) << filter;
        }
    }
};

struct ConfigDataIEC104 {
    struct TCP {
        std::string ip;
        int port;
    };

    struct LocalAddress {
        std::string ip;
        int port;
    };

    struct APCI {
        int t0;
        int t1;
        int t2;
        int t3;
        int k;
        int w;
    };

    struct ApplicationLayer {
        int originator_address;
        int common_address;
        int asdu_size;
    };

    struct Protocol {
        APCI apci;
        ApplicationLayer application_layer;
    };

    struct Report {
        int ioa;
        std::string report_id;
    };

    struct Communication {
        int gi_interval_ms;
        bool report_enabled;
        std::vector<Report> reports;
    };

    struct ErrorHandling {
        int reconnect_interval_ms;
        int max_reconnect_attempts;
    };

    struct DataPoint {
        int ioa;
        std::string type;
        std::string description;
        std::string iec61850_path;
    };

    std::string device_id;
    std::string type;  // TCP or other
    TCP tcp;
    LocalAddress local_address;
    Protocol protocol;
    Communication communication;
    ErrorHandling error_handling;
    std::vector<DataPoint> data_points;

    void to_string() const
    {
        LOG(info) << "device_id: " << device_id;
        LOG(info) << "type: " << type;

        LOG(info) << "tcp.ip: " << tcp.ip;
        LOG(info) << "tcp.port: " << tcp.port;

        LOG(info) << "local_address.ip: " << local_address.ip;
        LOG(info) << "local_address.port: " << local_address.port;

        LOG(info) << "protocol.apci.t0: " << protocol.apci.t0;
        LOG(info) << "protocol.apci.t1: " << protocol.apci.t1;
        LOG(info) << "protocol.apci.t2: " << protocol.apci.t2;
        LOG(info) << "protocol.apci.t3: " << protocol.apci.t3;
        LOG(info) << "protocol.apci.k: " << protocol.apci.k;
        LOG(info) << "protocol.apci.w: " << protocol.apci.w;

        LOG(info) << "protocol.application_layer.originator_address: " << protocol.application_layer.originator_address;
        LOG(info) << "protocol.application_layer.common_address: " << protocol.application_layer.common_address;
        LOG(info) << "protocol.application_layer.asdu_size: " << protocol.application_layer.asdu_size;

        LOG(info) << "communication.gi_interval_ms: " << communication.gi_interval_ms;
        LOG(info) << "communication.report_enabled: " << (communication.report_enabled ? "true" : "false");

        for (size_t i = 0; i < communication.reports.size(); ++i) {
            const auto& r = communication.reports[i];
            LOG(info) << "communication.reports[" << i << "].ioa: " << r.ioa;
            LOG(info) << "communication.reports[" << i << "].report_id: " << r.report_id;
        }

        LOG(info) << "error_handling.reconnect_interval_ms: " << error_handling.reconnect_interval_ms;
        LOG(info) << "error_handling.max_reconnect_attempts: " << error_handling.max_reconnect_attempts;

        for (size_t i = 0; i < data_points.size(); ++i) {
            const auto& d = data_points[i];
            LOG(info) << "data_points[" << i << "].ioa: " << d.ioa;
            LOG(info) << "data_points[" << i << "].type: " << d.type;
            LOG(info) << "data_points[" << i << "].description: " << d.description;
            LOG(info) << "data_points[" << i << "].iec61850_path: " << d.iec61850_path;
            LOG(info) << "\n------------------------\n";
        }
    }
};




struct ConfigData {
    std::unordered_map<std::string, ConfigDataModbus> modbus;
    std::unordered_map<std::string, ConfigDataIEC61850> iec61850;
    std::unordered_map<std::string, ConfigDataIEC104> iec104;

    const ConfigDataModbus *getModbus(const std::string &deviceId) const
    {
        auto it = modbus.find(deviceId);
        if (it != modbus.end()) {
            return &it->second;
        }
        return nullptr;
    }

    const ConfigDataIEC61850 *getIec61850(const std::string &deviceId) const
    {
        auto it = iec61850.find(deviceId);
        if (it != iec61850.end()) {
            return &it->second;
        }
        return nullptr;
    }

    const ConfigDataIEC104 *getIec104(const std::string &deviceId) const
    {
        auto it = iec104.find(deviceId);
        if (it != iec104.end()) {
            return &it->second;
        }
        return nullptr;
    }
};

class Config
{
public:
    ~Config();
    static Config &getInstance();
    bool init(const std::string &BasePath);
    const ConfigData *getConfig();

private:
    std::mutex mutex_;
    std::unique_ptr<ConfigData> data = nullptr;
private:
    Config();
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;
    bool parseModbusConfig(const YAML::Node &config);
    bool parse61850Config(const YAML::Node &config);
    bool parse104Config(const YAML::Node &config);
    // std::vector<std::string> getDeviceConfigPaths(const std::string &baseConfigPath);
    void loadConfig(const std::string &filename);
};

#endif // CONFIG_H