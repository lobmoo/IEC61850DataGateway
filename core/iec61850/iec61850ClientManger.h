#pragma once
#include <string>
#include <vector>
#include <atomic>
#include "icd_parse.h"
#include <libiec61850/iec61850_client.h>

class iec61850ClientManger
{

public:
    iec61850ClientManger(std::string ip, uint16_t port);
    ~iec61850ClientManger();
    void init(const char *configFilePath);
    bool connect();
    void disconnect();
    void readAllValues(const std::vector<DataNode> &nodes);
    void controlObjects(const std::vector<std::string> &nodes, std::string ctlVal);
    void subscribeToReports(const std::vector<DataNode> &nodes);

private:
    std::string ip_;
    uint16_t port_;
    IedConnection con_;
    IedClientError error_;
    std::atomic<bool> running_; 
};
