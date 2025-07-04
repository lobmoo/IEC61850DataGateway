#include "iec104Client.h"
#include "log/logger.h"


IEC104Client::IEC104Client(const std::string& server_ip, uint16_t server_port,
                           const std::string& local_ip, int local_port,
                           int t0, int originator_address, int gi_interval_ms)
    : server_ip_(server_ip), server_port_(server_port), local_ip_(local_ip),
      local_port_(local_port), t0_(t0), originator_address_(originator_address),
      gi_interval_ms_(gi_interval_ms), connection_(nullptr), running_(false),
      start_dt_sent_(false), last_gi_sent_(0),
      last_event_lock_(Semaphore_create(1)), last_event_((CS104_ConnectionEvent)-1)
{
    connection_ = CS104_Connection_create(server_ip_.c_str(), server_port_);
    if (!connection_) {
        throw std::runtime_error("Failed to create CS104_Connection");
    }
    // 设置APCI参数
    CS104_APCIParameters apci_params = CS104_Connection_getAPCIParameters(connection_);
    apci_params->t0 = t0_;

    // 设置应用层参数
    CS101_AppLayerParameters al_params = CS104_Connection_getAppLayerParameters(connection_);
    al_params->originatorAddress = originator_address_;

    // 设置回调
    CS104_Connection_setConnectionHandler(connection_, connectionHandler, this);
    CS104_Connection_setASDUReceivedHandler(connection_, asduReceivedHandler, this);

    // 设置本地地址（可选）
    if (!local_ip_.empty()) {
        CS104_Connection_setLocalAddress(connection_, local_ip_.c_str(), local_port_);
    }
}

IEC104Client::~IEC104Client()
{
    stop();
    if (connection_) {
        CS104_Connection_destroy(connection_);
        connection_ = nullptr;
    }
    Semaphore_destroy(last_event_lock_);
}

bool IEC104Client::start()
{
    if (running_) {
        LOG(error) << "Client is already running";
        return false;
    }

    running_ = true;
    CS104_Connection_connectAsync(connection_);
    worker_thread_ = std::thread(&IEC104Client::run, this);
    return true;
}

void IEC104Client::stop()
{
    if (running_) {
        running_ = false;
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
        if (start_dt_sent_) {
            sendStopDT();
        }
    }
}

bool IEC104Client::sendInterrogationCommand()
{
    if (!running_ || !start_dt_sent_) {
        return false;
    }
    return CS104_Connection_sendInterrogationCommand(connection_, CS101_COT_ACTIVATION, 1, IEC60870_QOI_STATION);
}

void IEC104Client::sendStartDT()
{
    if (!running_) {
        return;
    }
    CS104_Connection_sendStartDT(connection_);
}

void IEC104Client::sendStopDT()
{
    if (!running_) {
        return;
    }
    CS104_Connection_sendStopDT(connection_);
}

void IEC104Client::setConnectionEventCallback(ConnectionEventCallback callback)
{
    std::lock_guard<std::mutex> lock(event_mutex_);
    connection_callback_ = std::move(callback);
}

void IEC104Client::setAsduReceivedCallback(AsduReceivedCallback callback)
{
    std::lock_guard<std::mutex> lock(event_mutex_);
    asdu_callback_ = std::move(callback);
}

void IEC104Client::run()
{
    while (running_) {
        Semaphore_wait(last_event_lock_);
        if (last_event_ == CS104_CONNECTION_OPENED) {
            sendStartDT();
        } else if (last_event_ == CS104_CONNECTION_CLOSED || last_event_ == CS104_CONNECTION_FAILED) {
            running_ = false;
        } else if (start_dt_sent_) {
            uint64_t current_time = Hal_getTimeInMs();
            if (current_time < last_gi_sent_) {
                last_gi_sent_ = current_time;
            }
            if (current_time > last_gi_sent_ + gi_interval_ms_) {
                last_gi_sent_ = current_time;
                sendInterrogationCommand();
            }
        }
        Semaphore_post(last_event_lock_);
        Thread_sleep(100);
    }
}

void IEC104Client::connectionHandler(void* parameter, CS104_Connection connection, CS104_ConnectionEvent event)
{
    auto* client = static_cast<IEC104Client*>(parameter);
    {
        client->last_event_ = event;
        Semaphore_post(client->last_event_lock_);
    }

    std::lock_guard<std::mutex> lock(client->event_mutex_);
    if (client->connection_callback_) {
        client->connection_callback_(connection, event);
    }
}

bool IEC104Client::asduReceivedHandler(void* parameter, int address, CS101_ASDU asdu)
{
    auto* client = static_cast<IEC104Client*>(parameter);
    std::lock_guard<std::mutex> lock(client->event_mutex_);
    if (client->asdu_callback_) {
        return client->asdu_callback_(address, asdu);
    }
    return true;
}