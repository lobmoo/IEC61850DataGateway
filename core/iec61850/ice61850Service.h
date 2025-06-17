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

    bool init(
        const std::string &interface, const std::string &specificGooseInterface = "",
        const std::string &specificGcbName = "gcbAnalogValues");
    bool startServer(uint16_t port = 102); // 默认端口为102
    void stopServer();
    void setLocalData(const std::string &dataRef, float value, Quality quality);

private:
    IedServer server_;
    IedModel *model_;
    SVPublisher svPublisher_;
    bool running_;
    std::thread dataUpdateThread_;
    std::thread svThread_;

private:
    void initializeLocalData();
    void initializeGoose(
        const std::string &interface, const std::string &specificGooseInterface,
        const std::string &specificGcbName);
    void initializeSvPublisher(const std::string &interface);
    void updateServerData();
    void updateDataLoop();
    void publishSvLoop();
    void publishGooseLoop();
    // 控制处理程序
    static ControlAction controlHandlerForBinaryOutput(void *parameter, MmsValue *value, bool test);
    // GOOSE 事件处理程序
    static void goCbEventHandler(MmsGooseControlBlock goCb, void *parameter);
};

struct LocalData {
    float value;
    Quality quality;
    uint64_t timestamp;
};

#endif // ICE61850SERVICE_H_