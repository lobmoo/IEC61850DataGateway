/**
 * @file iec61850Client.h
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

#pragma once
#include <string>
#include <vector>
#include <atomic>
#include "icd_parse.h"
#include "iec61850_client.h"
#include "redis-api/app_redis.h"

class iec61850Client
{

public:
    iec61850Client(std::string ip, uint16_t port);
    ~iec61850Client();
    void init(const char *configFilePath);
    bool connect();
    void disconnect();
    void readAllValues(const std::vector<DataNode> &nodes);
    void controlObjects(const std::vector<std::string> &nodes, std::string ctlVal);
    void subscribeToReports(const std::vector<DataNode> &nodes);

private:
    void parseStructure(
        DRDSDataRedis &redisClient, MmsValue *value, const std::string &nodePath, MmsVariableSpecification *varSpec = nullptr,
        int indentLevel = 0, const std::string &targetPath = "");

private:
    std::string ip_;
    uint16_t port_;
    IedConnection con_;
    IedClientError error_;
    std::atomic<bool> running_;
};
