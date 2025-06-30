/**
 * @file lec61850ClientManger.cc
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

#include "iec61850ClientManger.h"
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include "log/logger.h"

iec61850ClientManger::~iec61850ClientManger()
{
}

void iec61850ClientManger::init(const char *configFilePath)
{
    IcdParser parser;
    auto nodes = parser.parse(configFilePath);
    parser.printParseResult(nodes);
    if (!connect()) {
        LOG(error) << "Failed to connect to IEC 61850 server at " << ip_ << ":" << port_;
        return;
    }
    LOG(info) << "Connected to IEC 61850 server at " << ip_ << ":" << port_;
    std::thread([this, nodes]() { readAllValues(nodes.nodes); }).detach();
    // std::string ctlVal = "true";
    // while (1) {

    //     ctlVal = ctlVal == "true" ? "false" : "true"; // 切换控制值
    //     controlObjects(
    //         {"beagleGenericIO/GGIO1.SPCSO1", "beagleGenericIO/GGIO1.SPCSO2",
    //          "beagleGenericIO/GGIO1.SPCSO3", "beagleGenericIO/GGIO1.DPCSO1"},
    //         ctlVal);
    //     std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 每秒读取一次
    // }
}

iec61850ClientManger::iec61850ClientManger(std::string ip, uint16_t port)
    : ip_(std::move(ip)), port_(port), con_(IedConnection_create()), running_(true)
{
}

bool iec61850ClientManger::connect()
{
    IedConnection_connect(con_, &error_, ip_.c_str(), port_);
    if (error_ != IED_ERROR_OK) {
        LOG(error) << "Failed to connect: " << error_;
        return false;
    }
    return true;
}

void iec61850ClientManger::disconnect()
{
    IedConnection_close(con_);
    IedConnection_destroy(con_);
}

void iec61850ClientManger::readAllValues(const std::vector<DataNode> &nodes)
{

    DRDSDataRedis redisClient;
    if(!redisClient.isInitialized())
    {
        LOG(error) << "Redis client is not initialized.";
        return;
    }
    IedClientError error;
    if (nodes.empty()) {
        LOG(info) << "No nodes to read.";
        return;
    }
    // 检查连接是否有效
    if (!con_) {
        LOG(error) << "Invalid IedConnection.";
        return;
    }

    while (running_) {
        for (const auto &node : nodes) {
            // 过滤：只读 GGIO1 的 ST、CF 节点，跳过 CO
            if (node.path.find("/SPTR01.") == std::string::npos || node.fc == "CO"
                || (node.fc != "ST" && node.fc != "CF")) {
                LOG(debug) << "Skipping node: " << node.path << " (FC: " << node.fc << ")";
                continue;
            }

            MmsValue *value = IedConnection_readObject(
                con_, &error, node.path.c_str(), FunctionalConstraint_fromString(node.fc.c_str()));
            if (error == IED_ERROR_OK && value) {
                // 成功读取 处理数据
                //LOG(info) << "Read " << node.path << " value: " << valueStr;
                if (std::string::npos == node.path.find("/SPTR01.")) {
                    continue;
                }

                MmsType type = MmsValue_getType(value);
                switch (type) {
                    case MMS_BOOLEAN: {
                        bool val = MmsValue_getBoolean(value);
                        redisClient.storeBool(node.path, val);
                        LOG(info) << "Node: " << node.path << " is: " << val;
                        break;
                    }
                    case MMS_INTEGER: {
                        int64_t val = MmsValue_toInt64(value);
                        redisClient.storeLInt(node.path, val);
                        LOG(info) << "Node: " << node.path << " is: " << val;
                        break;
                    }
                    case MMS_UNSIGNED: {
                        uint64_t val = MmsValue_toInt64(value);
                        redisClient.storeULInt(node.path, val);
                        LOG(info) << "Node: " << node.path << " is: " << val;
                        break;
                    }

                    case MMS_FLOAT: {
                        double val = MmsValue_toDouble(value);
                        redisClient.storeLReal(node.path, val);
                        LOG(info) << "Node: " << node.path << " is: " << val;
                        break;
                    }

                    case MMS_STRING:
                    case MMS_VISIBLE_STRING: {
                        const char *str = MmsValue_toString(value);
                        redisClient.storeString(node.path, str ? str : "");
                        if (str) {
                            LOG(info) << "Node: " << node.path << " is: " << str;
                        } else {
                            LOG(info) << "Node: " << node.path << " has an empty string value.";
                        }
                        break;
                    }
                    case MMS_BIT_STRING: {
                        int size = MmsValue_getBitStringSize(value);
                        std::string bitStr;
                        bitStr.reserve(size); // 预分配空间以提高性能
                        for (int i = 0; i < size; ++i) {
                            bitStr += MmsValue_getBitStringBit(value, i) ? '1' : '0';
                        }
                        redisClient.storeString(node.path, bitStr);
                        LOG(info) << "Node: " << node.path << " is bit string: " << bitStr;
                        break;
                    }

                    case MMS_UTC_TIME: {
                        uint64_t msTimestamp = MmsValue_getUtcTimeInMs(value); // 单位是毫秒
                        std::time_t seconds = msTimestamp / 1000;
                        int milliseconds = msTimestamp % 1000;

                        char buf[64];
                        std::strftime(buf, sizeof(buf), "%F %T", std::localtime(&seconds));
                        LOG(info) << "Node: " << node.path << " is UTC time: " << buf << "."
                                  << milliseconds << " (" << msTimestamp << " ms)";
                        redisClient.storeLInt(node.path, msTimestamp);
                        break;
                    }

                    case MMS_STRUCTURE: {
                        parseStructure(redisClient, value, node.path);
                        break;
                    }

                    default:
                        LOG(warning)
                            << "Node: " << node.path << " has an unsupported type: " << type;
                        break;
                }
            } else {
                // 读取失败，记录错误
                std::string valueStr = value ? MmsValue_toString(value) : "nullptr";
                LOG(error) << "Failed to read " << node.path << ": "
                           << IedClientError_toString(error) << ", value: " << valueStr;
            }
            // 统一释放 value
            if (value) {
                MmsValue_delete(value);
                value = nullptr;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 每秒读取一次
    }
}

void iec61850ClientManger::parseStructure(DRDSDataRedis &redisClient,
    MmsValue *value, const std::string &nodePath, MmsVariableSpecification *varSpec,
    int indentLevel, const std::string &targetPath)
{
    if (!value || MmsValue_getType(value) != MMS_STRUCTURE) {
        LOG(error) << "Invalid structure for node: " << nodePath;
        return;
    }

    std::string indent(indentLevel * 2, ' '); // 用于日志缩进

    // 限制递归深度，防止栈溢出
    if (indentLevel > 10) {
        LOG(error) << indent << "Max recursion depth exceeded at: " << nodePath;
        return;
    }
    // 如果指定了目标路径，使用 MmsValue_getSubElement
    if (!targetPath.empty() && varSpec) {

        MmsValue *subValue =
            MmsValue_getSubElement(value, varSpec, const_cast<char *>(targetPath.c_str()));

        if (subValue) {
            std::string subPath = nodePath + "." + targetPath;
            MmsType subType = MmsValue_getType(subValue);
            switch (subType) {
                case MMS_BOOLEAN: {
                    bool val = MmsValue_getBoolean(subValue);
                    redisClient.storeBool(subPath, val);
                    LOG(info) << indent << "Node: " << subPath
                              << " is: " << (val ? "true" : "false");
                    break;
                }
                case MMS_INTEGER: {
                    int64_t val = MmsValue_toInt64(subValue);
                    redisClient.storeLInt(subPath, val);
                    LOG(info) << indent << "Node: " << subPath << " is: " << val;
                    break;
                }
                case MMS_UNSIGNED: {
                    uint64_t val = MmsValue_toInt64(subValue);
                    redisClient.storeULInt(subPath, val);
                    LOG(info) << indent << "Node: " << subPath << " is: " << val;
                    break;
                }
                case MMS_FLOAT: {
                    double val = MmsValue_toDouble(subValue);
                    redisClient.storeLReal(subPath, val);
                    LOG(info) << indent << "Node: " << subPath << " is: " << val;
                    break;
                }
                case MMS_STRING:
                case MMS_VISIBLE_STRING: {
                    const char *str = MmsValue_toString(subValue);
                    redisClient.storeString(subPath, str ? str : "");
                    LOG(info) << indent << "Node: " << subPath
                              << " is: " << (str ? str : "empty string");
                    break;
                }
                case MMS_BIT_STRING: {
                    int size = MmsValue_getBitStringSize(subValue);
                    std::string bitStr;
                    bitStr.reserve(size); // 预分配空间以提高性能
                    for (int j = 0; j < size; ++j) {
                        bitStr += MmsValue_getBitStringBit(subValue, j) ? '1' : '0';
                    }
                    redisClient.storeString(subPath, bitStr);
                    LOG(info) << indent << "Node: " << subPath << " is bit string: " << bitStr;
                    break;
                }
                case MMS_UTC_TIME: {
                    uint64_t msTimestamp = MmsValue_getUtcTimeInMs(value); // 单位是毫秒
                    redisClient.storeLInt(subPath, msTimestamp);
                    std::time_t seconds = msTimestamp / 1000;
                    int milliseconds = msTimestamp % 1000;

                    char buf[64];
                    std::strftime(buf, sizeof(buf), "%F %T", std::localtime(&seconds));
                    LOG(info) << "Node: " << subPath << " is UTC time: " << buf << "."
                              << milliseconds << " (" << msTimestamp << " ms)";
                    break;
                }
                case MMS_STRUCTURE: {
                    LOG(info) << indent << "Node: " << subPath << " is a structure";
                    parseStructure(redisClient, subValue, subPath, nullptr, indentLevel + 1); // 递归解析子结构体
                    break;
                }
                default:
                    LOG(info) << indent << "Node: " << subPath
                              << " has an unsupported type: " << subType;
                    break;
            }
            if (subValue) {
                MmsValue_delete(subValue);
            }

        } else {
            LOG(error) << indent << "Failed to get sub-element: " << targetPath
                       << " for node: " << nodePath;
        }
        return;
    }

    // 遍历所有子元素
    int numSubValues = MmsValue_getArraySize(value);

    for (int i = 0; i < numSubValues; ++i) {
        MmsValue *subValue = MmsValue_getElement(value, i);
        if (!subValue) {
            LOG(error) << indent << "Failed to get sub-element " << i << " for node: " << nodePath;
            continue;
        }
        std::string subPath = nodePath + "[" + std::to_string(i) + "]";
        MmsType subType = MmsValue_getType(subValue);
        switch (subType) {
            case MMS_BOOLEAN: {
                bool val = MmsValue_getBoolean(subValue);
                LOG(info) << indent << "Node: " << subPath << " is: " << (val ? "true" : "false");
                break;
            }
            case MMS_INTEGER: {
                int64_t val = MmsValue_toInt64(subValue);
                LOG(info) << indent << "Node: " << subPath << " is: " << val;
                break;
            }
            case MMS_UNSIGNED: {
                uint64_t val = MmsValue_toInt64(subValue);
                LOG(info) << indent << "Node: " << subPath << " is: " << val;
                break;
            }
            case MMS_FLOAT: {
                double val = MmsValue_toDouble(subValue);
                LOG(info) << indent << "Node: " << subPath << " is: " << val;
                break;
            }
            case MMS_STRING:
            case MMS_VISIBLE_STRING: {
                const char *str = MmsValue_toString(subValue);
                LOG(info) << indent << "Node: " << subPath
                          << " is: " << (str ? str : "empty string");
                break;
            }
            case MMS_BIT_STRING: {
                int size = MmsValue_getBitStringSize(subValue);
                std::string bitStr;
                for (int j = 0; j < size; ++j) {
                    bitStr += MmsValue_getBitStringBit(subValue, j) ? '1' : '0';
                }
                LOG(info) << indent << "Node: " << subPath << " is bit string: " << bitStr;
                break;
            }
            case MMS_UTC_TIME: {
                uint64_t msTimestamp = MmsValue_getUtcTimeInMs(value); // 单位是毫秒
                std::time_t seconds = msTimestamp / 1000;
                int milliseconds = msTimestamp % 1000;

                char buf[64];
                std::strftime(buf, sizeof(buf), "%F %T", std::localtime(&seconds));
                LOG(info) << "Node: " << subPath << " is UTC time: " << buf << "." << milliseconds
                          << " (" << msTimestamp << " ms)";
                break;
            }
            case MMS_STRUCTURE: {
                LOG(info) << indent << "Node: " << subPath << " is a structure";
                parseStructure(redisClient, subValue, subPath, nullptr, indentLevel + 1);
                break;
            }
            default:
                LOG(info) << indent << "Node: " << subPath
                          << " has an unsupported type: " << subType;
                break;
        }
        // if (subValue) {
        //     MmsValue_delete(subValue);
        //     printf("eeeeee++++++++++++++++++++%d\n", MmsValue_getType(value));
        // }
    }
}

void iec61850ClientManger::controlObjects(const std::vector<std::string> &nodes, std::string ctlVal)
{
    IedClientError error;
    for (const auto &node : nodes) {
        // 只处理  CO 节点 todo
        ControlObjectClient control = ControlObjectClient_create(node.c_str(), con_);
        if (!control) {
            LOG(error) << "Failed to create control for " << node;
            continue;
        }
        MmsType type = ControlObjectClient_getCtlValType(control);
        MmsValue *MmsctlVal = nullptr;
        switch (type) {
            case MMS_BOOLEAN: {

                bool ctlValBool = ctlVal == "true" || ctlVal == "1" || ctlVal == "True"
                                  || ctlVal == "TRUE"; // 假设 ctlVal 是字符串 "true" 或 "1"
                MmsctlVal = MmsValue_newBoolean(ctlValBool);
                break;
            }
            case MMS_INTEGER: {
                int ctlValInt = std::stoi(ctlVal); // 假设 ctlVal 是字符串 "1" 或 "0"
                MmsctlVal = MmsValue_newInteger(ctlValInt ? 1 : 0);
                break;
            }
            case MMS_UNSIGNED:
            case MMS_BIT_STRING:
            case MMS_FLOAT:
            case MMS_VISIBLE_STRING:
            case MMS_STRING:
            case MMS_UTC_TIME:
            case MMS_OCTET_STRING:
            case MMS_BINARY_TIME:
            case MMS_BCD:
            case MMS_OBJ_ID:
                break; // 其他类型暂不处理
            default:
                break;
        }
        LOG(info) << "Enter control value for " << node << " (type: " << type << ")";
        if (!ControlObjectClient_operate(control, MmsctlVal, 0)) {
            LOG(error) << "Failed to operate " << node << ": " << error;
        }
        MmsValue_delete(MmsctlVal);
        ControlObjectClient_destroy(control);
    }
}

void iec61850ClientManger::subscribeToReports(const std::vector<DataNode> &nodes)
{
    LOG(info) << "\nSubscribing to reports:" << std::endl;
    IedClientError error;

    for (const auto &report : nodes) {
        // std::string rcbRef = "TemplateGenericIO/LLN0." + report.name;
        // ClientReportControlBlock* rcb = ClientReportControlBlock_create(rcbRef.c_str());

        // error = IedConnection_getRCBValues(connection_, &error, rcbRef.c_str(), rcb);
        // if (error != IED_ERROR_OK) {
        //     std::cerr << "Failed to get RCB: " << rcbRef << std::endl;
        //     ClientReportControlBlock_destroy(rcb);
        //     continue;
        // }

        // MmsValue* rptEna = MmsValue_newBoolean(true);
        // ClientReportControlBlock_setRptEna(rcb, rptEna);
        // error = IedConnection_setRCBValues(connection_, &error, rcb, RCB_ELEMENT_RPT_ENA, true);
        // MmsValue_delete(rptEna);

        // if (error == IED_ERROR_OK) {
        //     LOG(info) << "Enabled report: " << rcbRef << std::endl;
        // } else {
        //     std::cerr << "Failed to enable report: " << rcbRef << std::endl;
        // }

        // IedConnection_installReportHandler(connection_, rcbRef.c_str(),
        //     ClientReportControlBlock_getRptId(rcb), reportHandler, this);

        // ClientReportControlBlock_destroy(rcb);
    }
}
