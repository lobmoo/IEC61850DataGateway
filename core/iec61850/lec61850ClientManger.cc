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
    // std::thread([this, nodes]() { readAllValues(nodes.nodes); }).detach();
    controlObjects(nodes.nodes);
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
                const char *valueStr = MmsValue_toString(value);
                LOG(info) << "Read " << node.path << " type: " << type << ", value: " << valueStr;
                if (NULL == valueStr) {
                    if (type == MMS_BOOLEAN) {
                        bool val = MmsValue_getBoolean(value);
                        LOG(info) << node.path << " boolean value: " << (val ? "true" : "false");
                    } else {
                    }
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

void iec61850ClientManger::controlObjects(const std::vector<DataNode> &nodes)
{
    LOG(info) << "\nOperating control objects:";
    IedClientError error;
    bool ctlValBool = false;
    while (running_) {
        ctlValBool = !ctlValBool; // 切换控制值
        for (const auto &node : nodes) {
            // 只处理 GGIO 的 CO 节点
            if (node.fc != "CO" || node.path.find("/GGIO1.") == std::string::npos) {
                continue;
            }
            ControlObjectClient control = ControlObjectClient_create(node.path.c_str(), con_);
            if (!control) {
                LOG(error) << "Failed to create control for " << node.path;
                continue;
            }

            // MmsValue *mmsVal = nullptr;
            // if (node.dataType == "Boolean")
            // {
            //     mmsVal = MmsValue_newBoolean(ctlValBool);
            //     LOG(info) << "Operating " << node.path << " with Boolean value: " << (ctlValBool ? "true" : "false");
            // }

            // LOG(info) << "Enter control value for " << node.path << " (true/false): ";
            // MmsValue *ctlVal = MmsValue_newBoolean(ctlValBool);
            // if (!ControlObjectClient_operate(control, ctlVal, 0)) {
            //     LOG(info) << "Operated control: " << node.path
            //               << " with value: " << (ctlValBool ? "true" : "false");
            // } else {
            //     LOG(error) << "Failed to operate " << node.path << ": " << error;
            // }
            // MmsValue_delete(ctlVal);
            // ControlObjectClient_destroy(control);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 每秒操作一次
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
