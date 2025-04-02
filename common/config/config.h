/**
 * @file config.cc
 * @brief 
 * @author wwk (1162431386@qq.com)
 * @version 1.0
 * @date 2025-03-28
 * 
 * @copyright Copyright (c) 2025  by  wwk
 * 
 * @par 修改日志:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2025-03-28     <td>1.0     <td>wwk   <td>修改?
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
    struct TCP {
        std::string ip;
        int port;
    };
    struct RTU {
        std::string port_name;
        int baudrate;
        std::string parity;
        int slave_addr;
    };

    struct data_points_t {
        std::string name;
        int address;
        std::string type;
        std::string data_type;
        float scale;
        float offset;
    };

    std::string device_id;
    std::string Type;
    TCP tcp;
    RTU rtu;
    int cmd_interval;
    int max_retries;
    int retry_interval;
    std::string byte_order;
    std::unordered_map<std::string, data_points_t> data_points_map;

    void to_string() const{
        LOG(info) << "device_id: " << device_id;
        LOG(info) << "Type: " << Type;
        LOG(info) << "cmd_interval: " << cmd_interval;
        LOG(info) << "max_retries: " << max_retries;
        LOG(info) << "retry_interval: " << retry_interval;
        LOG(info) << "tcp.ip: " << tcp.ip;
        LOG(info) << "tcp.port: " << tcp.port;
        LOG(info) << "rtu.port_name: " << rtu.port_name;
        LOG(info) << "rtu.baudrate: " << rtu.baudrate;
        LOG(info) << "rtu.parity: " << rtu.parity;
        LOG(info) << "rtu.slave_addr: " << rtu.slave_addr;
        LOG(info) << "byte_order: " << byte_order;
        for (auto &data_point : data_points_map) {
            LOG(info) << "data_point.name: " << data_point.second.name;
            LOG(info) << "data_point.address: " << data_point.second.address;
            LOG(info) << "data_point.type: " << data_point.second.type;
            LOG(info) << "data_point.data_type: " << data_point.second.data_type;
            LOG(info) << "data_point.scale: " << data_point.second.scale;
            LOG(info) << "data_point.offset: " << data_point.second.offset;
        }
    }

    const data_points_t *getDataPoint(const std::string &name) const{
        auto it = data_points_map.find(name);
        if (it != data_points_map.end()) {
            return &it->second;
        }
        return nullptr;
    }

};



struct ConfigData {
    std::unordered_map<std::string, ConfigDataModbus> modbus;
    
    const ConfigDataModbus *getModbus(const std::string &deviceId) const {
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