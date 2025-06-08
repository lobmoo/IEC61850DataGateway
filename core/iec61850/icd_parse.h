/**
 * @file icd_parse.h
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
#include <tinyxml2.h>
#include <string>
#include <vector>
#include <iostream>
#include <set>
#include "log/logger.h"

using namespace tinyxml2;

// 定义 DataNode 结构，存储路径和功能约束 (FC)
struct DataNode {
    std::string
        path; // 数据路径，如 "TemplateGenericIO/GGIO1.SPCSO1.ctlModel" 或 "TemplateGenericIO/GGIO1.SPCSO1"
    std::string fc; // 功能约束，如 "CF", "ST", "CO"
    DataNode(const std::string &p, const std::string &f) : path(p), fc(f) {}
};

// 定义 DataSet 结构，存储数据集信息
struct DataSetInfo {
    std::string name;
    std::vector<std::string> fcdaPaths; // 数据集中的 FCDA 路径
    DataSetInfo(const std::string &n) : name(n) {}
};

// 定义 ReportControl 结构，存储报告控制块信息
struct ReportControlInfo {
    std::string name;
    std::string datSet; // 关联的数据集名称
    bool buffered;      // 是否为缓冲报告
    ReportControlInfo(const std::string &n, const std::string &ds, bool b)
        : name(n), datSet(ds), buffered(b)
    {
    }
};

class IcdParser
{
public:
    struct ParseResult {
        std::vector<DataNode> nodes;            // 所有数据节点和控制对象
        std::vector<DataSetInfo> dataSets;      // 数据集信息
        std::vector<ReportControlInfo> reports; // 报告控制块信息
    };

    ParseResult parse(const std::string &filePath)
    {
        ParseResult result;
        XMLDocument doc;

        // 加载 XML 文件
        if (doc.LoadFile(filePath.c_str()) != XML_SUCCESS) {
            LOG(error) << "Error: Failed to load file: " << filePath;
            return result;
        }

        // 检查 SCL 根元素
        const XMLElement *root = doc.FirstChildElement("SCL");
        if (!root) {
            LOG(error) << "Error: No SCL root element found";
            return result;
        }

        // 遍历所有 IED
        for (const XMLElement *ied = root->FirstChildElement("IED"); ied;
             ied = ied->NextSiblingElement("IED")) {
            const char *iedName = ied->Attribute("name");
            std::string iedNameStr = iedName ? iedName : "UnknownIED";
            LOG(info) << "Processing IED: " << iedNameStr;

            // 检查 Services
            const XMLElement *services = ied->FirstChildElement("Services");
            if (services) {
                LOG(info) << "Found Services in IED: " << iedNameStr;
            }

            // 遍历 AccessPoint -> Server -> LDevice
            for (const XMLElement *ap = ied->FirstChildElement("AccessPoint"); ap;
                 ap = ap->NextSiblingElement("AccessPoint")) {
                const XMLElement *server = ap->FirstChildElement("Server");
                if (!server)
                    continue;

                for (const XMLElement *lDevice = server->FirstChildElement("LDevice"); lDevice;
                     lDevice = lDevice->NextSiblingElement("LDevice")) {
                    const char *ldInst = lDevice->Attribute("inst");
                    std::string ldInstStr = ldInst ? ldInst : "UnknownLDevice";

                    // 遍历 LN 和 LN0
                    for (const XMLElement *ln = lDevice->FirstChildElement(); ln;
                         ln = ln->NextSiblingElement()) {
                        if (strcmp(ln->Name(), "LN") != 0 && strcmp(ln->Name(), "LN0") != 0)
                            continue;

                        const char *lnClass = ln->Attribute("lnClass");
                        const char *inst = ln->Attribute("inst");
                        std::string lnClassStr = lnClass ? lnClass : "";
                        std::string instStr = inst ? inst : "";

                        // 构造逻辑节点引用（包含 IED 名称）
                        std::string lnRef = iedNameStr + ldInstStr + "/" + lnClassStr + instStr;

                        // 解析 DataSet 并添加 FCDA 作为数据节点
                        for (const XMLElement *ds = ln->FirstChildElement("DataSet"); ds;
                             ds = ds->NextSiblingElement("DataSet")) {
                            const char *dsName = ds->Attribute("name");
                            std::string dsNameStr = dsName ? dsName : "";
                            DataSetInfo dsInfo(dsNameStr);

                            for (const XMLElement *fcda = ds->FirstChildElement("FCDA"); fcda;
                                 fcda = fcda->NextSiblingElement("FCDA")) {
                                const char *fcdaLdInst = fcda->Attribute("ldInst");
                                const char *fcdaLnClass = fcda->Attribute("lnClass");
                                const char *fcdaLnInst = fcda->Attribute("lnInst");
                                const char *fcdaDoName = fcda->Attribute("doName");
                                const char *fcdaDaName = fcda->Attribute("daName");
                                const char *fcdaFc = fcda->Attribute("fc");

                                std::string path = iedNameStr
                                                   + std::string(fcdaLdInst ? fcdaLdInst : "") + "/"
                                                   + std::string(fcdaLnClass ? fcdaLnClass : "")
                                                   + std::string(fcdaLnInst ? fcdaLnInst : "") + "."
                                                   + std::string(fcdaDoName ? fcdaDoName : "");
                                if (fcdaDaName) {
                                    path += "." + std::string(fcdaDaName);
                                }
                                if (fcdaFc) {
                                    dsInfo.fcdaPaths.push_back(path);
                                    // 添加 FCDA 作为数据节点（不加 .FC 后缀）
                                    result.nodes.emplace_back(path, fcdaFc);
                                }
                            }
                            result.dataSets.push_back(dsInfo);
                        }

                        // 解析 ReportControl
                        for (const XMLElement *rpt = ln->FirstChildElement("ReportControl"); rpt;
                             rpt = rpt->NextSiblingElement("ReportControl")) {
                            const char *rptName = rpt->Attribute("name");
                            const char *datSet = rpt->Attribute("datSet");
                            bool buffered = rpt->BoolAttribute("buffered", false);
                            std::string rptNameStr = rptName ? rptName : "";
                            std::string datSetStr = datSet ? datSet : "";
                            result.reports.emplace_back(rptNameStr, datSetStr, buffered);
                        }

                        // 解析 DOI 和 DAI
                        for (const XMLElement *doi = ln->FirstChildElement("DOI"); doi;
                             doi = doi->NextSiblingElement("DOI")) {
                            const char *doName = doi->Attribute("name");
                            std::string doNameStr = doName ? doName : "";

                            // 添加控制对象（DO）作为节点，FC 为 CO
                            std::string doPath = lnRef + "." + doNameStr;
                            std::string doTypeId =
                                getDoTypeId(doc, ln->Attribute("lnType"), doName);
                            if (!doTypeId.empty() && isControlObject(doc, doTypeId)) {
                                result.nodes.emplace_back(doPath, "CO");
                            }

                            for (const XMLElement *dai = doi->FirstChildElement("DAI"); dai;
                                 dai = dai->NextSiblingElement("DAI")) {
                                const char *daName = dai->Attribute("name");
                                std::string daNameStr = daName ? daName : "";

                                // 获取功能约束 (FC) 从 DOType
                                std::string fc =
                                    getFcFromDoType(doc, ln->Attribute("lnType"), doName, daName);
                                if (!fc.empty()) {
                                    std::string path = lnRef + "." + doNameStr + "." + daNameStr;
                                    result.nodes.emplace_back(path, fc);
                                }
                            }
                        }

                        // 从 DataTypeTemplates 提取所有 DO/DA（备用）
                        std::string lnType = ln->Attribute("lnType") ? ln->Attribute("lnType") : "";
                        if (!lnType.empty()) {
                            extractAllDoDa(doc, lnRef, lnType, result.nodes);
                        }
                    }
                }
            }

            if (!services && !ied->FirstChildElement("AccessPoint")) {
                LOG(error) << "Warning: No AccessPoint or Services in IED: " << iedNameStr;
            }
        }

        if (result.nodes.empty()) {
            LOG(error) << "Warning: No valid data nodes parsed from file";
        }

        return result;
    }

    void printParseResult(const ParseResult &result)
    {
        LOG(debug) << "Parsed Data Nodes:";
        for (const auto &node : result.nodes) {
            std::cout << "  Path: " << node.path << ", FC: " << node.fc << std::endl;
        }

        LOG(info) << "Parsed Data Sets:";
        for (const auto &ds : result.dataSets) {
            std::cout << "  DataSet: " << ds.name << ", FCDA Paths: ";
            for (const auto &path : ds.fcdaPaths) {
                std::cout << path << " ";
            }
            std::cout << std::endl;
        }

        LOG(info) << "Parsed Report Controls:";
        for (const auto &rpt : result.reports) {
            std::cout << "  ReportControl: " << rpt.name << ", DataSet: " << rpt.datSet
                      << ", Buffered: " << (rpt.buffered ? "Yes" : "No") << std::endl;
        }
    }

private:
    std::string getDoTypeId(const XMLDocument &doc, const char *lnType, const char *doName)
    {
        if (!lnType || !doName)
            return "";

        const XMLElement *dtt =
            doc.FirstChildElement("SCL")->FirstChildElement("DataTypeTemplates");
        if (!dtt)
            return "";

        for (const XMLElement *lnt = dtt->FirstChildElement("LNodeType"); lnt;
             lnt = lnt->NextSiblingElement("LNodeType")) {
            if (strcmp(lnt->Attribute("id"), lnType) == 0) {
                for (const XMLElement *doElem = lnt->FirstChildElement("DO"); doElem;
                     doElem = doElem->NextSiblingElement("DO")) {
                    if (doName && strcmp(doElem->Attribute("name"), doName) == 0) {
                        return doElem->Attribute("type") ? doElem->Attribute("type") : "";
                    }
                }
            }
        }
        return "";
    }

    bool isControlObject(const XMLDocument &doc, const std::string &doTypeId)
    {
        const XMLElement *dtt =
            doc.FirstChildElement("SCL")->FirstChildElement("DataTypeTemplates");
        if (!dtt)
            return false;

        for (const XMLElement *doType = dtt->FirstChildElement("DOType"); doType;
             doType = doType->NextSiblingElement("DOType")) {
            if (strcmp(doType->Attribute("id"), doTypeId.c_str()) == 0) {
                const char *cdc = doType->Attribute("cdc");
                return cdc && (strcmp(cdc, "SPC") == 0 || strcmp(cdc, "DPC") == 0);
            }
        }
        return false;
    }

    std::string getFcFromDoType(
        const XMLDocument &doc, const char *lnType, const char *doName, const char *daName)
    {
        if (!lnType || !doName || !daName)
            return "";

        const XMLElement *dtt =
            doc.FirstChildElement("SCL")->FirstChildElement("DataTypeTemplates");
        if (!dtt)
            return "";

        for (const XMLElement *lnt = dtt->FirstChildElement("LNodeType"); lnt;
             lnt = lnt->NextSiblingElement("LNodeType")) {
            if (strcmp(lnt->Attribute("id"), lnType) == 0) {
                for (const XMLElement *doElem = lnt->FirstChildElement("DO"); doElem;
                     doElem = doElem->NextSiblingElement("DO")) {
                    if (doName && strcmp(doElem->Attribute("name"), doName) == 0) {
                        const char *doTypeId = doElem->Attribute("type");
                        if (doTypeId) {
                            for (const XMLElement *doType = dtt->FirstChildElement("DOType");
                                 doType; doType = doType->NextSiblingElement("DOType")) {
                                if (strcmp(doType->Attribute("id"), doTypeId) == 0) {
                                    for (const XMLElement *da = doType->FirstChildElement("DA"); da;
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

    void extractAllDoDa(
        const XMLDocument &doc, const std::string &lnRef, const std::string &lnType,
        std::vector<DataNode> &nodes)
    {
        const XMLElement *dtt =
            doc.FirstChildElement("SCL")->FirstChildElement("DataTypeTemplates");
        if (!dtt)
            return;

        // 查找 LNodeType
        for (const XMLElement *lnt = dtt->FirstChildElement("LNodeType"); lnt;
             lnt = lnt->NextSiblingElement("LNodeType")) {
            if (strcmp(lnt->Attribute("id"), lnType.c_str()) == 0) {
                // 遍历所有 DO
                for (const XMLElement *doElem = lnt->FirstChildElement("DO"); doElem;
                     doElem = doElem->NextSiblingElement("DO")) {
                    const char *doName = doElem->Attribute("name");
                    std::string doNameStr = doName ? doName : "";
                    const char *doTypeId = doElem->Attribute("type");

                    // 检查是否是控制对象
                    if (doTypeId && isControlObject(doc, doTypeId)) {
                        std::string doPath = lnRef + "." + doNameStr;
                        nodes.emplace_back(doPath, "CO");
                    }

                    // 查找 DOType
                    if (doTypeId) {
                        for (const XMLElement *doType = dtt->FirstChildElement("DOType"); doType;
                             doType = doType->NextSiblingElement("DOType")) {
                            if (strcmp(doType->Attribute("id"), doTypeId) == 0) {
                                // 遍历所有 DA
                                for (const XMLElement *da = doType->FirstChildElement("DA"); da;
                                     da = da->NextSiblingElement("DA")) {
                                    const char *daName = da->Attribute("name");
                                    const char *fc = da->Attribute("fc");
                                    if (daName && fc) {
                                        std::string path =
                                            lnRef + "." + doNameStr + "." + std::string(daName);
                                        nodes.emplace_back(path, fc);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
};