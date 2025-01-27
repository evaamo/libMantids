#include "mime_sub_content.h"
using namespace Mantids::Network::MIME;
using namespace Mantids;

#ifdef _WIN32
#include <windows.h>
#endif

MIME_Sub_Content::MIME_Sub_Content()
{
#ifdef _WIN32
    char tempPath[MAX_PATH+1];
    GetTempPathA(MAX_PATH,tempPath);
    setFsTmpFolder(tempPath);
#else
    setFsTmpFolder("/tmp");
#endif

    contentContainer = nullptr;
    replaceContentContainer(new Memory::Containers::B_Chunks);
    setParseMode(Memory::Streams::Parsing::PARSE_MODE_DIRECT_DELIMITER);
    setBoundary("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
}

MIME_Sub_Content::~MIME_Sub_Content()
{
    if (contentContainer) delete contentContainer;
}

bool MIME_Sub_Content::stream(Memory::Streams::Status &wrStat)
{
    Memory::Streams::Status cur;
    // TODO: interpret content encoding...
    if (!contentContainer->streamTo(upStream,wrStat)) return false;
    if (!(cur+=contentContainer->writeString("\r\n--" + boundary, wrStat)).succeed) return false;
    return true;
}

uint64_t MIME_Sub_Content::getMaxContentSize() const
{
    return maxContentSize;
}

void MIME_Sub_Content::setMaxContentSize(const uint64_t &value)
{
    maxContentSize = value;
    setParseDataTargetSize(maxContentSize+boundary.size()+4);
}

uint64_t MIME_Sub_Content::getMaxContentSizeUntilGoingToFS() const
{
    return maxContentSizeUntilGoingToFS;
}

void MIME_Sub_Content::setMaxContentSizeUntilGoingToFS(const uint64_t &value)
{
    maxContentSizeUntilGoingToFS = value;
}

std::string MIME_Sub_Content::getFsTmpFolder() const
{
    return fsTmpFolder;
}

void MIME_Sub_Content::setFsTmpFolder(const std::string &value)
{
    fsTmpFolder = value;
}

Memory::Streams::Parsing::ParseStatus MIME_Sub_Content::parse()
{
    // TODO: interpret content encoding...
    getParsedData()->appendTo(*contentContainer);
    if (getDelimiterFound().size())
    {
        // finished (delimiter found).
        return Memory::Streams::Parsing::PARSE_STAT_GOTO_NEXT_SUBPARSER;
    }
    return Memory::Streams::Parsing::PARSE_STAT_GET_MORE_DATA;
}

std::string MIME_Sub_Content::getBoundary() const
{
    return boundary;
}

void MIME_Sub_Content::setBoundary(const std::string &value)
{
    boundary = value;
    setParseDelimiter("\r\n--" + boundary);
    setParseDataTargetSize(maxContentSize+boundary.size()+4);
}

Memory::Containers::B_Base *MIME_Sub_Content::getContentContainer() const
{
    return contentContainer;
}

void MIME_Sub_Content::replaceContentContainer(Memory::Containers::B_Base *value)
{
    if (contentContainer) delete contentContainer;
    contentContainer = value;
}
