#ifndef MIME_MULTIPART_H
#define MIME_MULTIPART_H

#include <map>

#include <mdz_mem_vars/vars.h>
#include <mdz_mem_vars/streamparser.h>

#include "mime_partmessage.h"
#include "mime_sub_firstboundary.h"
#include "mime_sub_endpboundary.h"


namespace Mantids { namespace Network { namespace MIME {

enum eMIME_VarStat
{
    MP_STATE_FIRST_BOUNDARY,
    MP_STATE_ENDPOINT,
    MP_STATE_HEADERS,
    MP_STATE_CONTENT
};

class MIME_Vars : public Memory::Abstract::Vars, public Memory::Streams::Parsing::Parser
{
public:
    MIME_Vars(Memory::Streams::Streamable *value= nullptr);
    ~MIME_Vars() override;

    bool streamTo(Memory::Streams::Streamable * out, Memory::Streams::Status & wrStat) override;

    ///////////////////////////////////////
    // Virtuals for Vars...
    uint32_t varCount(const std::string & varName) override;
    Memory::Containers::B_Base * getValue(const std::string & varName) override;
    std::list<Memory::Containers::B_Base *> getValues(const std::string & varName) override;
    std::set<std::string> getKeysList() override;
    bool isEmpty() override;

    void makeDataSizeExceptionForPart(const std::string & partName, const uint64_t & size);
    void writeVarToFS(const std::string &varName, const std::string &fileName);

    // TODO: decode as BC pos reference (for file reading).
    std::list<MIME_PartMessage *> getMultiPartMessagesByName(const std::string & varName);
    MIME_PartMessage * getFirstMessageByName(const std::string & varName);

    std::string getMultiPartType() const;
    void setMultiPartType(const std::string &value);

    std::string getMultiPartBoundary() const;
    void setMultiPartBoundary(const std::string &value);

    size_t getMaxParts() const;
    void setMaxParts(const size_t &value);

    size_t getMaxHeaderSubOptionsCount() const;
    void setMaxHeaderSubOptionsCount(const size_t &value);

    size_t getMaxHeaderSubOptionsSize() const;
    void setMaxHeaderSubOptionsSize(const size_t &value);

    size_t getMaxHeaderOptionsCount() const;
    void setMaxHeaderOptionsCount(const size_t &value);

    size_t getMaxHeaderOptionSize() const;
    void setMaxHeaderOptionSize(const size_t &value);

protected:
    bool initProtocol() override;
    void endProtocol() override;
    bool changeToNextParser() override;


    ///////////////////////////////////////
    // Virtuals for Vars...  Security
    void iSetMaxVarContentSize() override;
    ///////////////////////////////////////

private:
    void addMultiPartMessage(MIME_PartMessage * part);
    std::string getMultiPartMessageName(MIME_PartMessage * part);

    void renewCurrentPart();

    size_t maxParts;
    size_t maxHeaderSubOptionsCount, maxHeaderSubOptionsSize;
    size_t maxHeaderOptionsCount, maxHeaderOptionSize;

    std::string multiPartType, multiPartBoundary;

    std::map<std::string,uint64_t> dataSizeExceptions;
    std::map<std::string,std::string> varToFS;
    std::list<MIME_PartMessage *> parts;
    std::multimap<std::string,MIME_PartMessage *> partsByName;

    eMIME_VarStat currentState;
    MIME_PartMessage * currentPart;
    MIME_Sub_FirstBoundary subFirstBoundary;
    MIME_Sub_EndPBoundary subEndPBoundary;
};


}}}

#endif // MIME_MULTIPART_H
