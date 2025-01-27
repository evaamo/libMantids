#include "mime_vars.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
using namespace boost::algorithm;
using namespace Mantids::Network::MIME;
using namespace Mantids;

MIME_Vars::MIME_Vars(Memory::Streams::Streamable *value) : Memory::Streams::Parsing::Parser(value, false)
{
    initSubParser(&subFirstBoundary);
    initSubParser(&subEndPBoundary);

    maxParts=128;

    maxHeaderSubOptionsCount=16;
    maxHeaderSubOptionsSize=8*KB_MULT;

    maxHeaderOptionsCount=64;
    maxHeaderOptionSize=8*KB_MULT;

    maxVarContentSize = 32*KB_MULT;
    renewCurrentPart();

    currentParser = &subFirstBoundary;
    currentState = MP_STATE_FIRST_BOUNDARY;
}

MIME_Vars::~MIME_Vars()
{
    if (currentPart) delete currentPart;
    for (MIME_PartMessage * i : parts) delete i;
}

bool MIME_Vars::streamTo(Memory::Streams::Streamable *out, Memory::Streams::Status &wrStat)
{
    Memory::Streams::Status cur;
    // first boundary:
    if (!(cur+=out->writeString("--" + multiPartBoundary, wrStat)).succeed) return false;
    for (MIME_PartMessage * i : parts)
    {
        if (!(cur+=out->writeString("\r\n", wrStat)).succeed) return false;
        i->getContent()->initElemParser(out,true);
        i->getHeader()->initElemParser(out,true);
        if (!i->stream(wrStat)) return false;
    }
    if (!(cur+=out->writeString("--", wrStat)).succeed) return false;
    return true;
}

uint32_t MIME_Vars::varCount(const std::string &varName)
{
    uint32_t ix=0;
    auto range = partsByName.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i) ix++;
    return ix;
}

Memory::Containers::B_Base *MIME_Vars::getValue(const std::string &varName)
{
    auto range = partsByName.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i) return (((MIME_PartMessage *)i->second)->getContent()->getContentContainer());
    return nullptr;
}

std::list<Memory::Containers::B_Base *> MIME_Vars::getValues(const std::string &varName)
{
    std::list<Memory::Containers::B_Base *> values;
    auto range = partsByName.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i) values.push_back(((MIME_PartMessage *)i->second)->getContent()->getContentContainer());
    return values;
}

std::set<std::string> MIME_Vars::getKeysList()
{
    std::set<std::string> r;
    for ( const auto & i : partsByName ) r.insert(i.first);
    return r;
}

bool MIME_Vars::isEmpty()
{
    return partsByName.empty();
}

void MIME_Vars::iSetMaxVarContentSize()
{
    currentPart->getContent()->setMaxContentSize(maxVarContentSize);
}

void MIME_Vars::renewCurrentPart()
{
    currentPart = new MIME_PartMessage;

    initSubParser(currentPart->getContent());
    initSubParser(currentPart->getHeader());

    currentPart->getContent()->setMaxContentSize(maxVarContentSize);

    // Header:
    currentPart->getHeader()->setMaxOptions(maxHeaderOptionsCount);
    currentPart->getHeader()->setMaxOptionSize(maxHeaderOptionSize);

    // Sub Header:
    currentPart->getHeader()->setMaxSubOptionCount(maxHeaderSubOptionsCount);
    currentPart->getHeader()->setMaxSubOptionSize(maxHeaderSubOptionsSize);
}

size_t MIME_Vars::getMaxHeaderOptionSize() const
{
    return maxHeaderOptionSize;
}

void MIME_Vars::setMaxHeaderOptionSize(const size_t &value)
{
    maxHeaderOptionSize = value;
    currentPart->getHeader()->setMaxOptionSize(maxHeaderOptionSize);
}

size_t MIME_Vars::getMaxHeaderOptionsCount() const
{
    return maxHeaderOptionsCount;
}

void MIME_Vars::setMaxHeaderOptionsCount(const size_t &value)
{
    maxHeaderOptionsCount = value;
    currentPart->getHeader()->setMaxOptions(maxHeaderOptionsCount);
}

size_t MIME_Vars::getMaxHeaderSubOptionsSize() const
{
    return maxHeaderSubOptionsSize;
}

void MIME_Vars::setMaxHeaderSubOptionsSize(const size_t &value)
{
    maxHeaderSubOptionsSize = value;
    currentPart->getHeader()->setMaxSubOptionSize(maxHeaderSubOptionsSize);
}

size_t MIME_Vars::getMaxHeaderSubOptionsCount() const
{
    return maxHeaderSubOptionsCount;
}

void MIME_Vars::setMaxHeaderSubOptionsCount(const size_t &value)
{
    maxHeaderSubOptionsCount = value;
    currentPart->getHeader()->setMaxSubOptionCount(maxHeaderSubOptionsCount);
}

size_t MIME_Vars::getMaxParts() const
{
    return maxParts;
}

void MIME_Vars::setMaxParts(const size_t &value)
{
    maxParts = value;
}

void MIME_Vars::makeDataSizeExceptionForPart(const std::string &partName, const uint64_t &size)
{
    dataSizeExceptions[partName] = size;
}

void MIME_Vars::writeVarToFS(const std::string &varName, const std::string &fileName)
{
    varToFS[varName] = fileName;
}

bool MIME_Vars::changeToNextParser()
{
    switch (currentState)
    {
    case MP_STATE_FIRST_BOUNDARY:
    {
        currentState = MP_STATE_ENDPOINT;
        currentParser = &subEndPBoundary;
    }break;
    case MP_STATE_ENDPOINT:
    {
        if (subEndPBoundary.getStatus() == ENDP_STAT_CONTINUE)
        {
            currentState = MP_STATE_HEADERS;
            currentParser = currentPart->getHeader();
        }
        else
            currentParser = nullptr;
    }break;
    case MP_STATE_HEADERS:
    {
        currentState = MP_STATE_CONTENT;
        currentParser = currentPart->getContent();
        std::string currentPartName = getMultiPartMessageName(currentPart);
        if (varToFS.find(currentPartName) != varToFS.end())
        {
            // TODO: why mmap
            Memory::Containers::B_MMAP * fContainer = new Memory::Containers::B_MMAP;
            if (fContainer->referenceFile(varToFS[currentPartName]))
                currentPart->getContent()->replaceContentContainer(fContainer);
            else // Can't write on this location :(
                currentParser = nullptr;
        }
        if (dataSizeExceptions.find(currentPartName) != dataSizeExceptions.end())
            currentPart->getContent()->setMaxContentSize(dataSizeExceptions[currentPartName]);
    }break;
    case MP_STATE_CONTENT:
    {
        // Put current Part.
        addMultiPartMessage(currentPart);
        // new current part definition.
        renewCurrentPart();
        // Check if the max parts target is reached.
        if (parts.size()==maxParts) currentParser = nullptr; // End here.
        else
        {
            // Goto boundary.
            currentState = MP_STATE_ENDPOINT;
            currentParser = &subEndPBoundary;
        }
    }break;
    default:
    {
    }break;
    }
    return true;
}

void MIME_Vars::addMultiPartMessage(MIME_PartMessage *part)
{
    parts.push_back(part);
    // Insert by name:
    std::string varName = getMultiPartMessageName(part);
    if (varName!="") partsByName.insert(std::pair<std::string,MIME_PartMessage*>(boost::to_upper_copy(varName),part));
}

std::string MIME_Vars::getMultiPartMessageName(MIME_PartMessage *part)
{
    MIME_HeaderOption * opt = part->getHeader()->getOptionByName("content-disposition");
    if (opt) return opt->getSubVar("name");
    return "";
}

std::list<MIME_PartMessage *> MIME_Vars::getMultiPartMessagesByName(const std::string &varName)
{
    std::list<MIME_PartMessage *> values;
    auto range = partsByName.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i) values.push_back(i->second);
    return values;
}

MIME_PartMessage *MIME_Vars::getFirstMessageByName(const std::string &varName)
{
    if (partsByName.find(boost::to_upper_copy(varName)) == partsByName.end()) return nullptr;
    return partsByName.find(boost::to_upper_copy(varName))->second;
}

std::string MIME_Vars::getMultiPartType() const
{
    return multiPartType;
}

void MIME_Vars::setMultiPartType(const std::string &value)
{
    multiPartType = value;
}

std::string MIME_Vars::getMultiPartBoundary() const
{
    return multiPartBoundary;
}

void MIME_Vars::setMultiPartBoundary(const std::string &value)
{
    multiPartBoundary = value;
    subFirstBoundary.setBoundary(multiPartBoundary);
}

bool MIME_Vars::initProtocol()
{
    return true;
}

void MIME_Vars::endProtocol()
{

}
