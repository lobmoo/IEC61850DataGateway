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

    LOG(info) << "Reading all values from IEC 61850 nodes...";
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
            if (node.path.find("/GGIO1.") == std::string::npos || node.fc == "CO"
                || (node.fc != "ST" && node.fc != "CF")) {
                LOG(debug) << "Skipping node: " << node.path << " (FC: " << node.fc << ")";
                continue;
            }

            MmsValue *value = IedConnection_readObject(
                con_, &error, node.path.c_str(), FunctionalConstraint_fromString(node.fc.c_str()));

            if (error == IED_ERROR_OK && value) {
                // 成功读取 处理数据
                //LOG(info) << "Read " << node.path << " value: " << valueStr;
                if (std::string::npos == node.path.find("GGIO1.SPCSO")) {
                    continue;
                }

                MmsType type = MmsValue_getType(value);
                switch (type) {
                    case MMS_BOOLEAN: {
                        bool val = MmsValue_getBoolean(value);
                        LOG(info) << "Node: " << node.path << " is: " << val;
                        break;
                    }
                    case MMS_INTEGER:

                    case MMS_UNSIGNED:

                    case MMS_FLOAT:

                    case MMS_VISIBLE_STRING:

                    case MMS_STRING:
                        break;
                    default:
                        LOG(info) << "Node: " << node.path << " has an unsupported type: " << type;
                        break;
                }
            } else {
                // 读取失败，记录错误
                std::string valueStr = value ? MmsValue_toString(value) : "nullptr";
                LOG(error) << "Failed to read " << node.path << ": "
                           << IedClientError_toString(error) << ", value: " << valueStr;
                if (value) {
                    MmsValue_delete(value);
                }
                // 读取失败时等待
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }
            // 统一释放 value
            if (value) {
                MmsValue_delete(value);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 每秒读取一次
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
