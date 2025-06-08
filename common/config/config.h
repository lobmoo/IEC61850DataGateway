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

struct ConfigData {
    std::unordered_map<std::string, ConfigDataModbus> modbus;

    const ConfigDataModbus *getModbus(const std::string &deviceId) const
    {
        auto it = modbus.find(deviceId);
        if (it != modbus.end()) {
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
    std::unique_ptr<ConfigData> data = nullptr;
    std::mutex mutex_;

private:
    Config();
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;
    bool parseModbusConfig(const YAML::Node &config, std::unique_ptr<ConfigData> &data);
    std::vector<std::string> getDeviceConfigPaths(const std::string &baseConfigPath);
    void loadConfig(const std::string &filename);
};

#endif // CONFIG_H