/**
 * @file ice61850Service.h
 * @brief 
 * @author wwk (1162431386@qq.com)
 * @version 1.0
 * @date 2025-06-08
 * 
 * @copyright Copyright (c) 2025  by  wwk : wwk.lobmo@gmail.com
 * 
 * @par 修改日志:
 * <table>00
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2025-06-08     <td>1.0     <td>wwk   <td>修改?
 * </table>
 */
#ifndef ICE61850SERVICE_H_
#define ICE61850SERVICE_H_

#include <libiec61850/iec61850_server.h>
#include <libiec61850/goose_publisher.h>
#include <libiec61850/sv_publisher.h>
#include <string>
#include <map>
#include <thread>

class ice61850Service
{
public:
    ice61850Service();
    ~ice61850Service();

    bool initialize(const std::string &ipAddress, int port, const std::string &interface);
    bool startServer();
    void stopServer();
    void setLocalData(const std::string &dataRef, float value, Quality quality);

private:
    IedServer server;
    IedModel *model;
    GoosePublisher goosePublisher;
    SvPublisher svPublisher;
    bool running;
    std::thread dataUpdateThread;
    std::thread gooseThread;
    std::thread svThread;
    std::map<std::string, LocalData> localDataStore;

    void initializeLocalData();
    void initializeGoosePublisher(const std::string &interface);
    void initializeSvPublisher(const std::string &interface);
    void updateDataLoop();
    void updateServerData();
    void publishGooseLoop();
    void publishSvLoop();
};

struct LocalData {
    float value;
    Quality quality;
    uint64_t timestamp;
};

#endif // ICE61850SERVICE_H_