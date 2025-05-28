#include "iec61850ClientManger.h"
#include <stdlib.h>
#include <stdio.h>
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
    readAllValues(nodes.nodes);
    controlObjects(nodes.nodes);
}

iec61850ClientManger::iec61850ClientManger(std::string ip, uint16_t port)
    : ip_(std::move(ip)), port_(port), con_(IedConnection_create())
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
    for (const auto &node : nodes) {
        // 过滤：只读 GGIO 的 ST、CF 节点，跳过 CO
        if (node.fc == "CO" || node.path.find("/GGIO1.") == std::string::npos
            || (node.fc != "ST" && node.fc != "CF")) {
            continue;
        }
        MmsValue *value = IedConnection_readObject(
            con_, &error, node.path.c_str(), FunctionalConstraint_fromString(node.fc.c_str()));
        if (error == IED_ERROR_OK && value) {
            LOG(error) << node.path << ": " << MmsValue_toString(value);
            MmsValue_delete(value);
        } else {
            LOG(error) << "Failed to read " << node.path << ": " << error;
        }
    }
}

void iec61850ClientManger::controlObjects(const std::vector<DataNode> &nodes)
{
    LOG(info) << "\nOperating control objects:";
    IedClientError error;

    for (const auto &node : nodes) {
        // 只处理 GGIO 的 CO 节点
        if (node.fc != "CO" || node.path.find("/GGIO1.") == std::string::npos) {
            continue;
        }

        ControlObjectClient control = ControlObjectClient_create(node.path.c_str(), con_);
        if (control) {
            // 动态输入控制值
            LOG(info) << "Enter control value for " << node.path << " (true/false): ";
            std::string input;
            std::getline(std::cin, input);
            bool ctlValBool = (input == "true" || input == "True" || input == "TRUE");

            MmsValue *ctlVal = MmsValue_newBoolean(ctlValBool);
            if (!ControlObjectClient_operate(control, ctlVal, 0)) {
                LOG(info) << "Operated control: " << node.path
                          << " with value: " << (ctlValBool ? "true" : "false");
            } else {
                LOG(error) << "Failed to operate " << node.path << ": " << error;
            }
            MmsValue_delete(ctlVal);
            ControlObjectClient_destroy(control);
        } else {
            LOG(error) << "Failed to create control for " << node.path;
        }
    }
}


void iec61850ClientManger::subscribeToReports(const std::vector<DataNode> &nodes) {
        LOG(info) << "\nSubscribing to reports:" << std::endl;
        IedClientError error;

        for (const auto& report : nodes) {
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
