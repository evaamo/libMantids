#include "socketchain_xor.h"

#include <stdlib.h>
#include <string.h>

using namespace Mantids::Network::Chains::Protocols;

SocketChain_XOR::SocketChain_XOR()
{
    setXorByte(0x77);
}

int SocketChain_XOR::partialRead(void *data, const uint32_t &datalen)
{
    if (!datalen) return 0;

    int r = Mantids::Network::Sockets::Socket::partialRead(data,datalen);
    if (r<=0) return r;

    char * datacp = getXorCopy(data,r);
    if (!datacp) return 0;
    memcpy(data,datacp,r);
    delete [] datacp;

    return r;
}

int SocketChain_XOR::partialWrite(const void *data, const uint32_t &datalen)
{
    if (!datalen) return 0;

    char * datacp = getXorCopy(data,datalen);
    if (!datacp) return 0;

    int r = Mantids::Network::Sockets::Socket::partialWrite(datacp,datalen);
    delete [] datacp;
    return r;
}

char SocketChain_XOR::getXorByte() const
{
    return xorByte;
}

void SocketChain_XOR::setXorByte(char value)
{
    xorByte = value;
}

char *SocketChain_XOR::getXorCopy(const void *data, const uint32_t &datalen)
{
    char * datacp = new char[datalen];
    if (!datacp) return nullptr;
    for (uint32_t i=0; i<datalen; i++)
        datacp[i] = ((char *)data)[i]^xorByte;
    return datacp;
}

