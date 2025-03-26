#ifndef CONFIG_H
#define CONFIG_H

#include <yaml-cpp/yaml.h>

#include <mutex>
#include <string>

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

    TCP tcp;
    RTU rtu;
    int cmd_interval;
    int max_retries;
    int retry_interval;
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