#include "HttpHeader.h"

#include "StringUtil.h"
#include "common.h"

using namespace WebSocketCpp;

HttpHeader::HttpHeader(Header::HeaderRole role)
    : m_role(role)
{
}

bool HttpHeader::Parse(const ByteArray& data, size_t start)
{
    m_complete = false;

    size_t pos = StringUtil::SearchPosition(data, {CRLFCRLF}, start);
    if (pos != SIZE_MAX)
    {
        auto arr = StringUtil::Split(data, {CRLF}, start, pos);
        if (ParseHeaders(data, arr))
        {
            m_headerSize = pos - start;
            m_complete   = true;
        }
    }

    return m_complete;
}

bool HttpHeader::ParseHeader(const ByteArray& data)
{
    auto arr = StringUtil::Split(data, {CRLF}, 0, data.size());
    return ParseHeaders(data, arr);
}

ByteArray HttpHeader::ToByteArray() const
{
    std::string headers;
    for (auto const& header : m_headers)
    {
        headers += header.getName() + ": " + header.getValue() + CR + LF;
    }

    return ByteArray(headers.begin(), headers.end());
}

bool HttpHeader::IsComplete() const
{
    return m_complete;
}

size_t HttpHeader::GetHeaderSize() const
{
    return m_headerSize;
}

size_t HttpHeader::GetBodySize() const
{
    size_t size = 0;

    if (m_complete)
    {
        auto str = GetHeader(Header::HeaderType::ContentLength);
        if (str.empty() == false)
        {
            size_t num;
            if (StringUtil::String2uint64(str, num))
            {
                size = num;
            }
        }
        else
        {
            size = m_chunkedSize;
        }
    }

    return size;
}

size_t HttpHeader::GetRequestSize() const
{
    return GetHeaderSize() + 4 + GetBodySize(); // header + delimiter(CRLFCRLF, 4 bytes) + body
}

void HttpHeader::SetChunckedSize(size_t size)
{
    m_chunkedSize = size;
}

Header::HeaderRole HttpHeader::GetRole() const
{
    return m_role;
}

void HttpHeader::SetVersion(const std::string& version)
{
    m_version = version;
}

bool HttpHeader::ParseHeaders(const ByteArray& data, const StringUtil::Ranges& ranges)
{
    bool statusLineParsed = false;

    if (ranges.empty())
    {
        return false;
    }

    for (auto& range : ranges)
    {
        auto headerDelimiter = StringUtil::SearchPosition(data, {':'}, range.start, range.end);
        if (headerDelimiter != SIZE_MAX)
        {
            std::string name  = std::string(data.begin() + range.start, data.begin() + headerDelimiter);
            std::string value = std::string(data.begin() + headerDelimiter + 1, data.begin() + range.end + 1);
            StringUtil::Trim(name);
            StringUtil::Trim(value);

            if (name.empty())
            {
                continue;
            }

            SetHeader(name, value);
        }
    }

    return true;
}

std::string HttpHeader::ToString() const
{
    return "Header (" + std::to_string(m_headers.size()) + " records, ver. " + m_version + ", size: " + std::to_string(m_headerSize) + ")";
}

const std::vector<Header>& HttpHeader::GetHeaders() const
{
    return m_headers;
}

void HttpHeader::SetHeader(Header::HeaderType type, const std::string& value)
{
    SetHeader(Header::HeaderType2String(type), value);
}

void HttpHeader::SetHeader(const std::string& name, const std::string& value)
{
    for (auto& header : m_headers)
    {
        if (StringUtil::Compare(header.getName(), name))
        {
            header.getValue() = value;
            return;
        }
    }

    m_headers.emplace_back(Header::String2HeaderType(name), name, value);
}

void HttpHeader::Clear()
{
    m_role     = Header::HeaderRole::Undefined;
    m_complete = false;
    m_headers.clear();
    m_version       = "HTTP/1.1";
    m_headerSize    = 0;
    m_remoteAddress = "";
    m_remotePort    = (-1);
    m_chunkedSize   = 0;
}

std::string HttpHeader::GetHeader(Header::HeaderType headerType) const
{
    return GetHeader(Header::HeaderType2String(headerType));
}

std::string HttpHeader::GetHeader(const std::string& headerType) const
{
    for (auto& header : m_headers)
    {
        if (StringUtil::Compare(header.getName(), headerType))
        {
            return header.getValue();
        }
    }

    return "";
}

std::vector<std::string> HttpHeader::GetAllHeaders(const std::string& headerType) const
{
    std::vector<std::string> value;
    for (auto& header : m_headers)
    {
        if (header.getName() == headerType)
        {
            value.push_back(header.getValue());
        }
    }

    return value;
}

std::string HttpHeader::GetVersion() const
{
    return m_version;
}

std::string HttpHeader::GetRemote() const
{
    if (m_remotePort != (-1))
    {
        return m_remoteAddress + ":" + std::to_string(m_remotePort);
    }

    return m_remoteAddress;
}

void HttpHeader::SetRemote(const std::string& address)
{
    auto list = StringUtil::Split(address, ':');
    if (list.size() == 2)
    {
        m_remoteAddress = list[0];
        int port;
        if (StringUtil::String2int(list[1], port))
        {
            m_remotePort = port;
        }
    }
}

std::string HttpHeader::GetRemoteAddress() const
{
    return m_remoteAddress;
}

int HttpHeader::GetRemotePort() const
{
    return m_remotePort;
}

size_t HttpHeader::GetCount() const
{
    return m_headers.size();
}
