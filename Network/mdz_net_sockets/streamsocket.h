#ifndef StreamSocket_H_
#define StreamSocket_H_

#include "socket.h"
#include "streamsocketreader.h"
#include "streamsocketwriter.h"
#include <utility>
#include <mdz_mem_vars/streamable.h>

namespace Mantids { namespace Network { namespace Streams {

class StreamSocket : public Memory::Streams::Streamable, public Sockets::Socket, public StreamSocketReader, public StreamSocketWriter
{
public:
    StreamSocket();
    virtual ~StreamSocket()override;

    virtual void writeEOF(bool) override;

    bool streamTo(Memory::Streams::Streamable * out, Memory::Streams::Status & wrsStat) override;

    Memory::Streams::Status write(const void * buf, const size_t &count, Memory::Streams::Status & wrStatUpd) override;

    /**
     * @brief GetSocketPair Create a Pair of interconnected sockets
     * @return pair of interconnected StreamSockets. (remember to delete them)
    */
    static std::pair<StreamSocket *, StreamSocket *> GetSocketPair();

    virtual bool isConnected() override;
    // This methods are virtual and should be implemented in sub-classes.
    // TODO: virtual redefinition?
    virtual bool listenOn(const uint16_t & port, const char * listenOnAddr = "*", const int32_t & recvbuffer = 0, const int32_t &backlog = 10) override;
    virtual bool connectFrom(const char * bindAddress, const char * remoteHost, const uint16_t &port, const uint32_t &timeout = 30) override;
    virtual StreamSocket * acceptConnection();

    /**
     * Virtual function for protocol initialization after the connection starts...
     * useful for SSL server, it runs in blocking mode and should be called apart to avoid tcp accept while block
     * @return returns true if was properly initialized.
     */
    virtual bool postAcceptSubInitialization();
    /**
     * Virtual function for protocol initialization after the connection starts (client-mode)...
     * useful for sub protocols initialization (eg. ssl), it runs in blocking mode.
     * @return returns true if was properly initialized.
     */
    virtual bool postConnectSubInitialization();


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Basic R/W options:
    /**
     * Write Null Terminated String on the socket
     * note: it writes the '\0' on the socket.
     * @param data null terminated string
     * @return true if the string was successfully sent
     */
    virtual bool writeFull(const void * buf);
    /**
     * Write a data block on the socket
     *
     * You can specify sizes bigger than 4k/8k, or megabytes (be careful with memory), and it will be fully sent in chunks.
     * @param data data block.
     * @param datalen data length in bytes
     * @return true if the data block was sucessfully sent.
     */
    virtual bool writeFull(const void * data, const uint64_t & datalen) override;
    /**
     * @brief readBlock Read a data block from the socket
     *                  Receive the data block in 4k chunks (or less) until it ends or fail.
     * @param data Memory address where the received data will be allocated
     * @param expectedDataBytesCount Expected data size (in bytes) to be read from the socket
     * @param receivedBytesCount if not null, this function returns the received byte count.
     * @return if it's disconnected (invalid sockfd) or it's not able to obtain at least 1 byte, then return false, otherwise true, you should check by yourself that expectedDataBytesCount == *receivedDataBytesCount
     */
    virtual bool readFull(void * data, const uint64_t &expectedDataBytesCount, uint64_t * receivedDataBytesCount = nullptr) override;
    /**
     * @brief iShutdown Internal protocol Shutdown
     * @return depends on protocol.
     */
    virtual int iShutdown(int mode = SHUT_RDWR) override;


protected:
    void writeDeSync() override;
    void readDeSync() override;
};

typedef std::shared_ptr<StreamSocket> StreamSocket_SP;
}}}


#endif /* StreamSocket_H_ */
