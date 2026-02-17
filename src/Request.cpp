#include "Request.h"

#include "Config.h"

#define EOL_LENGTH             2
#define ENTRY_DELIMITER_LENGTH 4

using namespace WebSocketCpp;

Request::Request()
    : m_header(Header::HeaderRole::Request)
{
}

bool Request::Parse(const ByteArray& data)
{
    ClearError();

    if (m_requestLineLength == 0)
    {
        if (ParseRequestLine(data, m_requestLineLength) == false)
        {
            SetLastError("Request: error parsing request line: " + GetLastError());
            return false;
        }
    }
    if (m_header.IsComplete() == false)
    {
        if (m_header.Parse(data, m_requestLineLength + EOL_LENGTH) == false)
        {
            SetLastError("Request: error parsing header: " + GetLastError());
            return false;
        }
    }

    if (m_header.GetBodySize() > 0)
    {
        return ParseBody(data, m_requestLineLength + EOL_LENGTH + m_header.GetHeaderSize() + ENTRY_DELIMITER_LENGTH);
    }

    return true;
}

bool Request::ParseRequestLine(const ByteArray& data, size_t& pos)
{
    pos = StringUtil::SearchPosition(data, {CR, LF});
    if (pos != SIZE_MAX) // request line presents
    {
        auto ranges = StringUtil::Split(data, {' '}, 0, pos);
        if (ranges.size() == 3)
        {
            m_method = String2Method(std::string(data.begin() + ranges[0].start, data.begin() + ranges[0].end + 1));
            if (m_method == Method::Undefined)
            {
                SetLastError("wrong method");
                return false;
            }
            m_url.Parse(std::string(data.begin() + ranges[1].start, data.begin() + ranges[1].end + 1), false);
            if (m_url.IsInitiaized() == false)
            {
                SetLastError("wrong URL");
                return false;
            }
            m_httpVersion = std::string(data.begin() + ranges[2].start, data.begin() + ranges[2].end + 1);
            StringUtil::Trim(m_httpVersion);
            return true;
        }
    }

    return false;
}

int Request::GetConnectionID() const
{
    return m_connID;
}

void Request::SetConnectionID(int connID)
{
    m_connID = connID;
}

const Url& Request::GetUrl() const
{
    return m_url;
}

Url& Request::GetUrl()
{
    return m_url;
}

const HttpHeader& Request::GetHeader() const
{
    return m_header;
}

HttpHeader& Request::GetHeader()
{
    return m_header;
}

Method Request::GetMethod() const
{
    return m_method;
}

void Request::SetMethod(Method method)
{
    m_method = method;
}

std::string Request::GetHttpVersion() const
{
    return m_httpVersion;
}

bool Request::ParseBody(const ByteArray& data, size_t bodyPosition)
{
    auto config      = Config::Instance();
    auto contentType = m_header.GetHeader(Header::HeaderType::ContentType);
    if (m_requestBody.Parse(data, bodyPosition, ByteArray(contentType.begin(), contentType.end()), config.GetTempFile()) == false)
    {
        SetLastError("body parsing error: " + m_requestBody.GetLastError());
        return false;
    }

    return true;
}

const RequestBody& Request::GetRequestBody() const
{
    return m_requestBody;
}

RequestBody& Request::GetRequestBody()
{
    return m_requestBody;
}

void Request::SetArg(const std::string& name, const std::string& value)
{
    m_args[name] = value;
}

Protocol Request::GetProtocol() const
{
    if (StringUtil::Compare(m_header.GetHeader(Header::HeaderType::Upgrade), "websocket"))
    {
        return Protocol::WS;
    }

    return Protocol::HTTP;
}

size_t Request::GetRequestLineLength() const
{
    return m_requestLineLength;
}

size_t Request::GetRequestSize() const
{
    // Request line + CRLF (2 bytes) + Header + CRLFCRLF (4 bytes) + Body
    return m_requestLineLength + EOL_LENGTH + m_header.GetHeaderSize() + ENTRY_DELIMITER_LENGTH + m_header.GetBodySize();
}

std::string Request::GetRemote() const
{
    return m_remote;
}

void Request::SetRemote(const std::string& remote)
{
    m_remote = remote;
}

bool Request::Send(const std::shared_ptr<CommunicationClientBase>& communication)
{
    ClearError();

    if (communication == nullptr)
    {
        SetLastError("connection not set");
        return false;
    }

    ByteArray header;

    const ByteArray& body = m_requestBody.ToByteArray();

    const ByteArray& rl = BuildRequestLine();
    header.insert(header.end(), rl.begin(), rl.end());

    auto& hd = GetHeader();
    if (body.size() > 0)
    {
        hd.SetHeader(Header::HeaderType::ContentType, m_requestBody.BuildContentType());
        hd.SetHeader(Header::HeaderType::ContentLength, std::to_string(body.size()));
    }

    hd.SetHeader(Header::HeaderType::UserAgent, WEB_SOCKET_CPP_CANONICAL_NAME);
    hd.SetHeader(Header::HeaderType::Host, m_url.GetHost());
    hd.SetHeader(Header::HeaderType::Accept, "*/*");

    std::string encoding = "";
#ifdef WITH_ZLIB
    encoding += "gzip, deflate";
#endif
    if (!encoding.empty())
    {
        hd.SetHeader(Header::HeaderType::AcceptEncoding, encoding);
    }

    const ByteArray& h = m_header.ToByteArray();
    header.insert(header.end(), h.begin(), h.end());

    header.push_back(CR);
    header.push_back(LF);

    if (communication->Write(header) == false)
    {
        SetLastError("error sending header: " + communication->GetLastError());
        return false;
    }
    if (body.size() > 0)
    {
        if (communication->Write(body) == false)
        {
            SetLastError("error sending body: " + communication->GetLastError());
            return false;
        }
    }

    return true;
}

void Request::Clear()
{
    m_connID            = -1;
    m_method            = Method::Undefined;
    m_httpVersion       = "HTTP/1.1";
    m_requestLineLength = 0;
    m_remote            = "";
    m_session           = nullptr;
    m_url.Clear();
    m_header.Clear();
    m_args.clear();
    m_requestBody.Clear();
}

void Request::SetSession(Session* session)
{
    m_session = session;
}

Session* Request::GetSession() const
{
    return m_session;
}

ByteArray Request::BuildRequestLine() const
{

    std::string line = Method2String(m_method) + " " + m_url.GetNormalizedPath() + (m_url.HasQuery() ? ("?" + m_url.Query2String()) : "") + " " + m_httpVersion + CR + LF;

    return StringUtil::String2ByteArray(line);
}

ByteArray Request::BuildHeaders() const
{
    auto& header = GetHeader();

    std::string headers;
    for (auto& entry : header.GetHeaders())
    {
        headers += entry.getName() + ": " + entry.getValue() + CR + LF;
    }

    return ByteArray(headers.begin(), headers.end());
}

std::string Request::ToString() const
{
    return std::string("connID: ") + std::to_string(m_connID) + ", " + std::to_string(m_header.GetCount()) + " headers, body size: " + std::to_string(m_header.GetBodySize());
}

std::string Request::GetArg(const std::string& name) const
{
    auto it = m_args.find(name);
    return (it != m_args.end()) ? it->second : "";
}
