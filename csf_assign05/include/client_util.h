#ifndef CLIENT_UTIL_H
#define CLIENT_UTIL_H
#include <string>
#include "model.h"
std::string inputLogin();
int connectLogin(const char *host, const char *port, ClientMode mode);


#endif // CLIENT_UTIL_H
