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
