#ifndef CONFIG_H
#define CONFIG_H

#include <yaml-cpp/yaml.h>

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

    std::string Type;
    TCP tcp;
    RTU rtu;
    int cmd_interval;
    int max_retries;
    int retry_interval;

    inline void to_string() const
    {
        LOG(info) << "Type: " << Type;
        LOG(info) << "cmd_interval: " << cmd_interval;
        LOG(info) << "max_retries: " << max_retries;
        LOG(info) << "retry_interval: " << retry_interval;
        LOG(info) << "TCP: ";
        LOG(info) << "ip: " << tcp.ip;
        LOG(info) << "port: " << tcp.port;
        LOG(info) << "RTU: ";
        LOG(info) << "port_name: " << rtu.port_name;
        LOG(info) << "baudrate: " << rtu.baudrate;
        LOG(info) << "parity: " << rtu.parity;
        LOG(info) << "slave_addr: " << rtu.slave_addr;
    }
};

struct ConfigData {
    ConfigDataModbus modbus;
};

class Config
{
public:
    ~Config();

    static Config &getInstance();

    void loadConfig(const std::string &filename);

    const ConfigData *getConfig();

private:
    Config();
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;

    std::unique_ptr<ConfigData> data = nullptr;
    std::mutex mutex_;

private:
    void parseModbusConfig(const YAML::Node &config, std::unique_ptr<ConfigData> &data);
};

#endif // CONFIG_H