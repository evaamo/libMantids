#ifndef STREAMURL_H
#define STREAMURL_H

#include <mdz_mem_vars/streamable.h>

namespace Mantids { namespace Memory { namespace Streams { namespace Encoders {

class URL : public Memory::Streams::Streamable
{
public:
    URL(Memory::Streams::Streamable * orig);
    bool streamTo(Memory::Streams::Streamable *, Streams::Status & ) override;
    Memory::Streams::Status write(const void * buf, const size_t &count, Streams::Status &wrStat) override;
    uint64_t getFinalBytesWritten() const;
    static std::string encodeURLStr(const std::string &url);

private:
    size_t getPlainBytesSize(const unsigned char * buf, size_t count);
    inline bool shouldEncodeThisByte(const unsigned char & byte) const;

    uint64_t finalBytesWritten;
    Memory::Streams::Streamable * orig;
};


}}}}

#endif // STREAMURL_H
