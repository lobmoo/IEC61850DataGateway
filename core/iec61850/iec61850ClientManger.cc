#include "iec61850ClientManger.h"
#include "config/config.h"

void iec61850ClientManger::addClient(const std::shared_ptr<iec61850Client> &client)
{
    clients_.push_back(client);
}

void iec61850ClientManger::removeClient(const std::shared_ptr<iec61850Client> &client)
{

    clients_.erase(std::remove(clients_.begin(), clients_.end(), client), clients_.end());
}

void iec61850ClientManger::initClients()
{
    auto config = Config::getInstance().getConfig();
    if (!config) {
        LOG(error) << "Configuration data is not initialized.";
        return;
    }
    for (const auto &clientConfig : config->iec61850) {
        auto client =
            std::make_shared<iec61850Client>(clientConfig.second.ip, clientConfig.second.port);
        LOG(info) << "Initializing IEC 61850 client for device: " << clientConfig.second.device_id
                  << " at " << clientConfig.second.ip << ":" << clientConfig.second.port
                  << " with ICD file: " << clientConfig.second.icd_file_path;
        client->init(clientConfig.second.icd_file_path.c_str());
        if (client->connect()) {
            LOG(info) << "Connected to IEC 61850 server at " << clientConfig.second.ip << ":"
                      << clientConfig.second.port;
            clients_.push_back(client);
        } else {
            LOG(error) << "Failed to connect to IEC 61850 server at " << clientConfig.second.ip
                       << ":" << clientConfig.second.port;
        }
    }
}
