/*
 *  * Copyright (c) 2026 ruslan@muhlinin.com
 *  * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *  * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef WEB_SOCKET_CPP_HTTPHEADER_H
#define WEB_SOCKET_CPP_HTTPHEADER_H

#include <string>
#include <vector>

#include "Header.h"
#include "StringUtil.h"
#include "common.h"

namespace WebSocketCpp
{

class HttpHeader
{
public:
    explicit HttpHeader(Header::HeaderRole role);
    HttpHeader(const HttpHeader& other)                = delete;
    HttpHeader& operator=(const HttpHeader& other)     = delete;
    HttpHeader(HttpHeader&& other) noexcept            = delete;
    HttpHeader& operator=(HttpHeader&& other) noexcept = delete;

    bool               Parse(const ByteArray& data, size_t start = 0);
    bool               ParseHeader(const ByteArray& data);
    ByteArray          ToByteArray() const;
    bool               IsComplete() const;
    size_t             GetHeaderSize() const;
    size_t             GetBodySize() const;
    size_t             GetRequestSize() const;
    void               SetChunckedSize(size_t size);
    Header::HeaderRole GetRole() const;

    void        SetVersion(const std::string& version);
    std::string GetVersion() const;

    std::string GetRemote() const;
    void        SetRemote(const std::string& remote);
    std::string GetRemoteAddress() const;
    int         GetRemotePort() const;

    size_t                     GetCount() const;
    std::string                GetHeader(Header::HeaderType headerType) const;
    std::string                GetHeader(const std::string& headerType) const;
    std::vector<std::string>   GetAllHeaders(const std::string& headerType) const;
    const std::vector<Header>& GetHeaders() const;
    void                       SetHeader(Header::HeaderType type, const std::string& value);
    void                       SetHeader(const std::string& name, const std::string& value);
    void                       Clear();

    std::string ToString() const;

protected:
    bool ParseHeaders(const ByteArray& data, const StringUtil::Ranges& ranges);

private:
    Header::HeaderRole  m_role{Header::HeaderRole::Undefined};
    bool                m_complete{false};
    std::vector<Header> m_headers{};
    std::string         m_version{"HTTP/1.1"};
    size_t              m_headerSize{};
    std::string         m_remoteAddress;
    int                 m_remotePort{-1};
    size_t              m_chunkedSize{0};
};

} // namespace WebSocketCpp

#endif // WEB_SOCKET_CPP_HTTPHEADER_H
