#ifndef COMMON_H_
#define COMMON_H_
#include <stdint.h>


int client (const char* hostname, uint16_t tcpPort);
int service(uint16_t port);


#endif /* COMMON_H_ */