#pragma once


#include <tinyxml2.h>
#include <string>
#include <vector>
#include <iostream>

using namespace tinyxml2;

// ���� DataNode �ṹ���洢·���͹���Լ�� (FC)
struct DataNode {
    std::string path;
    std::string fc; // ����Լ������ MX, ST ��
    DataNode(const std::string& p, const std::string& f) : path(p), fc(f) {}
};

// ���� DataSet �ṹ���洢���ݼ���Ϣ
struct DataSetInfo {
    std::string name;
    std::vector<std::string> fcdaPaths; // ���ݼ��е� FCDA ·��
    DataSetInfo(const std::string& n) : name(n) {}
};

// ���� ReportControl �ṹ���洢������ƿ���Ϣ
struct ReportControlInfo {
    std::string name;
    std::string datSet; // ���������ݼ�����
    bool buffered;      // �Ƿ�Ϊ���屨��
    ReportControlInfo(const std::string& n, const std::string& ds, bool b) 
        : name(n), datSet(ds), buffered(b) {}
};

class IcdParser {
public:
    struct ParseResult {
        std::vector<DataNode> nodes;             // �������ݽڵ�
        std::vector<DataSetInfo> dataSets;       // ���ݼ���Ϣ
        std::vector<ReportControlInfo> reports;   // ������ƿ���Ϣ
    };

    ParseResult parse(const std::string& filePath) {
        ParseResult result;
        XMLDocument doc;

        // ���� XML �ļ�
        if (doc.LoadFile(filePath.c_str()) != XML_SUCCESS) {
            std::cerr << "Error: Failed to load file: " << filePath << std::endl;
            return result;
        }

        // ��� SCL ��Ԫ��
        const XMLElement* root = doc.FirstChildElement("SCL");
        if (!root) {
            std::cerr << "Error: No SCL root element found" << std::endl;
            return result;
        }

        // �������� IED
        for (const XMLElement* ied = root->FirstChildElement("IED"); ied; 
             ied = ied->NextSiblingElement("IED")) {
            const char* iedName = ied->Attribute("name");
            std::string iedNameStr = iedName ? iedName : "UnknownIED";
            std::cout << "Processing IED: " << iedNameStr << std::endl;

            // ��� Services
            const XMLElement* services = ied->FirstChildElement("Services");
            if (services) {
                std::cout << "Found Services in IED: " << iedNameStr << std::endl;
            }

            // ���� AccessPoint -> Server -> LDevice
            for (const XMLElement* ap = ied->FirstChildElement("AccessPoint"); ap; 
                 ap = ap->NextSiblingElement("AccessPoint")) {
                const XMLElement* server = ap->FirstChildElement("Server");
                if (!server) continue;

                for (const XMLElement* lDevice = server->FirstChildElement("LDevice"); lDevice; 
                     lDevice = lDevice->NextSiblingElement("LDevice")) {
                    const char* ldInst = lDevice->Attribute("inst");
                    std::string ldInstStr = ldInst ? ldInst : "UnknownLDevice";

                    // ���� LN �� LN0
                    for (const XMLElement* ln = lDevice->FirstChildElement(); ln; 
                         ln = ln->NextSiblingElement()) {
                        if (strcmp(ln->Name(), "LN") != 0 && strcmp(ln->Name(), "LN0") != 0) continue;

                        const char* lnClass = ln->Attribute("lnClass");
                        const char* inst = ln->Attribute("inst");
                        std::string lnClassStr = lnClass ? lnClass : "";
                        std::string instStr = inst ? inst : "";

                        // �����߼��ڵ�����
                        std::string lnRef = ldInstStr + "/" + lnClassStr + instStr;

                        // ���� DataSet
                        for (const XMLElement* ds = ln->FirstChildElement("DataSet"); ds; 
                             ds = ds->NextSiblingElement("DataSet")) {
                            const char* dsName = ds->Attribute("name");
                            std::string dsNameStr = dsName ? dsName : "";
                            DataSetInfo dsInfo(dsNameStr);

                            for (const XMLElement* fcda = ds->FirstChildElement("FCDA"); fcda; 
                                 fcda = fcda->NextSiblingElement("FCDA")) {
                                const char* fcdaLdInst = fcda->Attribute("ldInst");
                                const char* fcdaLnClass = fcda->Attribute("lnClass");
                                const char* fcdaLnInst = fcda->Attribute("lnInst");
                                const char* fcdaDoName = fcda->Attribute("doName");
                                const char* fcdaFc = fcda->Attribute("fc");

                                std::string path = std::string(fcdaLdInst ? fcdaLdInst : "") + "/" +
                                                  std::string(fcdaLnClass ? fcdaLnClass : "") +
                                                  std::string(fcdaLnInst ? fcdaLnInst : "") + "." +
                                                  std::string(fcdaDoName ? fcdaDoName : "");
                                if (fcdaFc) {
                                    path += "." + std::string(fcdaFc);
                                    dsInfo.fcdaPaths.push_back(path);
                                }
                            }
                            result.dataSets.push_back(dsInfo);
                        }

                        // ���� ReportControl
                        for (const XMLElement* rpt = ln->FirstChildElement("ReportControl"); rpt; 
                             rpt = rpt->NextSiblingElement("ReportControl")) {
                            const char* rptName = rpt->Attribute("name");
                            const char* datSet = rpt->Attribute("datSet");
                            bool buffered = rpt->BoolAttribute("buffered", false);
                            std::string rptNameStr = rptName ? rptName : "";
                            std::string datSetStr = datSet ? datSet : "";
                            result.reports.emplace_back(rptNameStr, datSetStr, buffered);
                        }

                        // ���� DOI �� DAI
                        for (const XMLElement* doi = ln->FirstChildElement("DOI"); doi; 
                             doi = doi->NextSiblingElement("DOI")) {
                            const char* doName = doi->Attribute("name");
                            std::string doNameStr = doName ? doName : "";

                            for (const XMLElement* dai = doi->FirstChildElement("DAI"); dai; 
                                 dai = dai->NextSiblingElement("DAI")) {
                                const char* daName = dai->Attribute("name");
                                std::string daNameStr = daName ? daName : "";

                                // ��ȡ����Լ�� (FC) �� DOType
                                std::string fc = getFcFromDoType(doc, ln->Attribute("lnType"), doName, daName);
                                if (!fc.empty()) {
                                    std::string path = lnRef + "." + doNameStr + "." + daNameStr;
                                    result.nodes.emplace_back(path, fc);
                                }
                            }
                        }
                    }
                }
            }

            if (!services && !ied->FirstChildElement("AccessPoint")) {
                std::cerr << "Warning: No AccessPoint or Services in IED: " << iedNameStr << std::endl;
            }
        }

        if (result.nodes.empty()) {
            std::cerr << "Warning: No valid data nodes parsed from file" << std::endl;
        }

        return result;
    }

    void printParseResult(const ParseResult& result) {
        std::cout << "Parsed Data Nodes:" << std::endl;
        for (const auto& node : result.nodes) {
            std::cout << "Path: " << node.path << ", FC: " << node.fc << std::endl;
        }

        std::cout << "\nParsed Data Sets:" << std::endl;
        for (const auto& ds : result.dataSets) {
            std::cout << "DataSet Name: " << ds.name << ", FCDA Paths: ";
            for (const auto& path : ds.fcdaPaths) {
                std::cout << path << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "\nParsed Report Controls:" << std::endl;
        for (const auto& rpt : result.reports) {
            std::cout << "Report Name: " << rpt.name 
                      << ", DataSet: " << rpt.datSet 
                      << ", Buffered: " << (rpt.buffered ? "Yes" : "No") 
                      << std::endl;
        }
    }

private:
    std::string getFcFromDoType(const XMLDocument& doc, const char* lnType, const char* doName, const char* daName) {
        if (!lnType || !doName || !daName) return "";

        const XMLElement* dtt = doc.FirstChildElement("SCL")->FirstChildElement("DataTypeTemplates");
        if (!dtt) return "";

        for (const XMLElement* lnt = dtt->FirstChildElement("LNodeType"); lnt; 
             lnt = lnt->NextSiblingElement("LNodeType")) {
            if (strcmp(lnt->Attribute("id"), lnType) == 0) {
                for (const XMLElement* doElem = lnt->FirstChildElement("DO"); doElem; 
                     doElem = doElem->NextSiblingElement("DO")) {
                    if (doName && strcmp(doElem->Attribute("name"), doName) == 0) {
                        const char* doTypeId = doElem->Attribute("type");
                        if (doTypeId) {
                            for (const XMLElement* doType = dtt->FirstChildElement("DOType"); doType; 
                                 doType = doType->NextSiblingElement("DOType")) {
                                if (strcmp(doType->Attribute("id"), doTypeId) == 0) {
                                    for (const XMLElement* da = doType->FirstChildElement("DA"); da; 
                                         da = da->NextSiblingElement("DA")) {
                                        if (daName && strcmp(da->Attribute("name"), daName) == 0) {
                                            return da->Attribute("fc") ? da->Attribute("fc") : "";
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return "";
    }
};

