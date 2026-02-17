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

#ifndef HEADER_H
#define HEADER_H

#include <string>

namespace WebSocketCpp
{

class Header
{
public:
    enum class HeaderRole
    {
        Undefined = 0,
        Request,
        Response,
    };

    enum class HeaderType
    {
        Undefined = 0,
        Accept,
        AcceptCharset,
        AcceptEncoding,
        AcceptDatetime,
        AcceptLanguage,
        Authorization,
        CacheControl,
        Connection,
        ContentEncoding,
        ContentLength,
        ContentMD5,
        ContentType,
        Cookie,
        Date,
        Expect,
        Forwarded,
        From,
        Host,
        HTTP2Settings,
        IfMatch,
        IfModifiedSince,
        IfNoneMatch,
        IfRange,
        IfUnmodifiedSince,
        MaxForwards,
        Origin,
        Pragma,
        Prefer,
        ProxyAuthorization,
        Range,
        Referer,
        TE,
        Trailer,
        TransferEncoding,
        UserAgent,
        Upgrade,
        Via,
        Warning,

        AcceptCH,
        AccessControlAllowOrigin,
        AccessControlAllowCredentials,
        AccessControlExposeHeaders,
        AccessControlMaxAge,
        AccessControlAllowMethods,
        AccessControlAllowHeaders,
        AcceptPatch,
        AcceptRanges,
        Age,
        Allow,
        AltSvc,
        ContentDisposition,
        ContentLanguage,
        ContentLocation,
        ContentRange,
        DeltaBase,
        ETag,
        Expires,
        IM,
        LastModified,
        Link,
        Location,
        P3P,
        PreferenceApplied,
        ProxyAuthenticate,
        PublicKeyPins,
        RetryAfter,
        Server,
        SetCookie,
        StrictTransportSecurity,
        Tk,
        Vary,
        WWWAuthenticate,
        XFrameOptions,
    };

    Header();
    ~Header();
    Header(Header::HeaderType type, const std::string& name, const std::string& value);

    Header::HeaderType getType() const;
    std::string        getName() const;
    std::string        getValue() const;

    static Header::HeaderType String2HeaderType(const std::string& str);
    static std::string        HeaderType2String(Header::HeaderType headerType);

private:
    Header::HeaderType m_type{Header::HeaderType::Undefined};
    std::string        m_name;
    std::string        m_value;
};

} // namespace WebSocketCpp
#endif // HEADER_H
