#ifndef IEC61850CLIENTMANGER_H_
#define IEC61850CLIENTMANGER_H_

#include <string>
#include <vector>
#include "iec61850/iec61850Client.h"

class iec61850ClientManger {
public:
    iec61850ClientManger() = default;
    ~iec61850ClientManger();

    void addClient(const std::shared_ptr<iec61850Client> &client);
    void removeClient(const std::shared_ptr<iec61850Client> &client);
    void initClients();

private:
    std::vector<std::shared_ptr<iec61850Client>> clients_;
};

#endif // IEC61850CLIENTMANGER_H_ 