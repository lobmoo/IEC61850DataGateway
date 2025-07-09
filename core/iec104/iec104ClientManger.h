#ifndef iec104ClientManger_h_
#define iec104ClientManger_h_
#include <vector>
#include <memory>
#include "iec104Client.h"

void testFunction();

class IEC104ClientManger
{
private:
    /* data */
public:
    IEC104ClientManger(/* args */);
    ~IEC104ClientManger();

    void addClient(const std::shared_ptr<IEC104Client> &client);
    void removeClient(const std::shared_ptr<IEC104Client> &client);
    void initClients();

private:
    std::vector<std::shared_ptr<IEC104Client>> clients_;
    // 其他成员变量可以根据需要添加
};


#endif // iec104ClientManger_h_