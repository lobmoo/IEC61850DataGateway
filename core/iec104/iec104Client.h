#ifndef IEC104CLIENT_H_
#define IEC104CLIENT_H_

#include "lib60870/cs104_connection.h"
#include "hal_time.h"
#include "hal_thread.h"
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>

class IEC104Client
{
public:
    // 连接状态回调函数类型
    using ConnectionEventCallback = std::function<void(CS104_Connection, CS104_ConnectionEvent)>;
    // ASDU数据接收回调函数类型
    using AsduReceivedCallback = std::function<bool(int address, CS101_ASDU asdu)>;

    // 构造函数，直接传递参数
    IEC104Client(
        const std::string &server_ip = "localhost",
        uint16_t server_port = IEC_60870_5_104_DEFAULT_PORT, const std::string &local_ip = "",
        int local_port = -1, int t0 = 2, int originator_address = 3, int gi_interval_ms = 10000);
    // 析构函数
    ~IEC104Client();

    // 启动客户端（异步连接）
    bool start();
    // 停止客户端
    void stop();
    // 发送总查询命令
    bool sendInterrogationCommand();
    // 发送STARTDT命令
    void sendStartDT();
    // 发送STOPDT命令
    void sendStopDT();

    // 设置连接事件回调
    void setConnectionEventCallback(ConnectionEventCallback callback);
    // 设置ASDU数据接收回调
    void setAsduReceivedCallback(AsduReceivedCallback callback);

private:
    // 内部方法：运行主循环
    void run();
    // 内部方法：连接事件处理
    static void
    connectionHandler(void *parameter, CS104_Connection connection, CS104_ConnectionEvent event);
    // 内部方法：ASDU数据接收处理
    static bool asduReceivedHandler(void *parameter, int address, CS101_ASDU asdu);

    std::string server_ip_;                       // 服务器IP
    uint16_t server_port_;                        // 服务器端口
    std::string local_ip_;                        // 本地IP
    int local_port_;                              // 本地端口
    int t0_;                                      // 连接超时时间（秒）
    int originator_address_;                      // 发起地址
    int gi_interval_ms_;                          // 总查询间隔（毫秒）
    CS104_Connection connection_;                 // 104连接对象
    std::mutex event_mutex_;                      // 事件锁
    std::atomic<bool> running_;                   // 运行状态
    std::atomic<bool> start_dt_sent_;             // 是否已发送STARTDT
    std::thread worker_thread_;                   // 工作线程
    uint64_t last_gi_sent_;                       // 上次总查询时间
    ConnectionEventCallback connection_callback_; // 连接事件回调
    AsduReceivedCallback asdu_callback_;          // ASDU数据回调
    Semaphore last_event_lock_;                   // 事件锁
    CS104_ConnectionEvent last_event_;            // 最后事件
};

#endif // IEC104CLIENT_H_