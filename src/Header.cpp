#include "Header.h"

#include "common.h"

using namespace WebSocketCpp;

Header::Header()
{
}

Header::~Header()
{
}

Header::Header(Header::HeaderType type, const std::string& name, const std::string& value)
    : m_type(type),
      m_name(name),
      m_value(value)
{
}

Header::HeaderType Header::getType() const
{
    return m_type;
}

std::string Header::getName() const
{
    return m_name;
}

std::string Header::getValue() const
{
    return m_value;
}

Header::HeaderType Header::String2HeaderType(const std::string& str)
{
    switch (_(str.c_str()))
    {
        case _("Accept"):
            return Header::HeaderType::Accept;
        case _("Accept-Charset"):
            return Header::HeaderType::AcceptCharset;
        case _("Accept-Encoding"):
            return Header::HeaderType::AcceptEncoding;
        case _("Accept-Datetime"):
            return Header::HeaderType::AcceptDatetime;
        case _("Accept-Language"):
            return Header::HeaderType::AcceptLanguage;
        case _("Authorization"):
            return Header::HeaderType::Authorization;
        case _("Cache-Control"):
            return Header::HeaderType::CacheControl;
        case _("Connection"):
            return Header::HeaderType::Connection;
        case _("Content-Encoding"):
            return Header::HeaderType::ContentEncoding;
        case _("Content-Length"):
            return Header::HeaderType::ContentLength;
        case _("Content-MD5"):
            return Header::HeaderType::ContentMD5;
        case _("Content-Type"):
            return Header::HeaderType::ContentType;
        case _("Cookie"):
            return Header::HeaderType::Cookie;
        case _("Date"):
            return Header::HeaderType::Date;
        case _("Expect"):
            return Header::HeaderType::Expect;
        case _("Forwarded"):
            return Header::HeaderType::Forwarded;
        case _("From"):
            return Header::HeaderType::From;
        case _("HTTP2-Settings"):
            return Header::HeaderType::HTTP2Settings;
        case _("Host"):
            return Header::HeaderType::Host;
        case _("If-Match"):
            return Header::HeaderType::IfMatch;
        case _("If-Modified-Since"):
            return Header::HeaderType::IfModifiedSince;
        case _("If-None-Match"):
            return Header::HeaderType::IfNoneMatch;
        case _("If-Range"):
            return Header::HeaderType::IfRange;
        case _("If-Unmodified-Since"):
            return Header::HeaderType::IfUnmodifiedSince;
        case _("Max-Forwards"):
            return Header::HeaderType::MaxForwards;
        case _("Origin"):
            return Header::HeaderType::Origin;
        case _("Pragma"):
            return Header::HeaderType::Pragma;
        case _("Prefer"):
            return Header::HeaderType::Prefer;
        case _("Proxy-Authorization"):
            return Header::HeaderType::ProxyAuthorization;
        case _("Range"):
            return Header::HeaderType::Range;
        case _("Referer"):
            return Header::HeaderType::Referer;
        case _("TE"):
            return Header::HeaderType::TE;
        case _("Trailer"):
            return Header::HeaderType::Trailer;
        case _("TransferEncoding"):
            return Header::HeaderType::TransferEncoding;
        case _("User-Agent"):
            return Header::HeaderType::UserAgent;
        case _("Upgrade"):
            return Header::HeaderType::Upgrade;
        case _("Via"):
            return Header::HeaderType::Via;
        case _("Warning"):
            return Header::HeaderType::Warning;

        case _("Accept-CH"):
            return HeaderType::AcceptCH;
        case _("Access-Control-Allow-Origin"):
            return HeaderType::AccessControlAllowOrigin;
        case _("Access-Control-Allow-Credential"):
            return HeaderType::AccessControlAllowCredentials;
        case _("Access-Control-Expose-Headers"):
            return HeaderType::AccessControlExposeHeaders;
        case _("Access-Control-Max-Age"):
            return HeaderType::AccessControlMaxAge;
        case _("Access-Control-Allow-Methods"):
            return HeaderType::AccessControlAllowMethods;
        case _("Access-Control-Allow-Headers"):
            return HeaderType::AccessControlAllowHeaders;
        case _("Accept-Patch"):
            return HeaderType::AcceptPatch;
        case _("Accept-Ranges"):
            return HeaderType::AcceptRanges;
        case _("Age"):
            return HeaderType::Age;
        case _("Allow"):
            return HeaderType::Allow;
        case _("Alt-Svc"):
            return HeaderType::AltSvc;
        case _("Content-Disposition"):
            return HeaderType::ContentDisposition;
        case _("Content-Language"):
            return HeaderType::ContentLanguage;
        case _("Content-Location"):
            return HeaderType::ContentLocation;
        case _("Content-Range"):
            return HeaderType::ContentRange;
        case _("Delta-Base"):
            return HeaderType::DeltaBase;
        case _("ETag"):
            return HeaderType::ETag;
        case _("Expires"):
            return HeaderType::Expires;
        case _("IM"):
            return HeaderType::IM;
        case _("Last-Modified"):
            return HeaderType::LastModified;
        case _("Link"):
            return HeaderType::Link;
        case _("Location"):
            return HeaderType::Location;
        case _("P3P"):
            return HeaderType::P3P;
        case _("Preference-Applied"):
            return HeaderType::PreferenceApplied;
        case _("Proxy-Authenticate"):
            return HeaderType::ProxyAuthenticate;
        case _("Public-Key-Pins"):
            return HeaderType::PublicKeyPins;
        case _("Retry-After"):
            return HeaderType::RetryAfter;
        case _("Server"):
            return HeaderType::Server;
        case _("Set-Cookie"):
            return HeaderType::SetCookie;
        case _("Strict-Transport-Security"):
            return HeaderType::StrictTransportSecurity;
        case _("Transfer-Encoding"):
            return HeaderType::TransferEncoding;
        case _("Tk"):
            return HeaderType::Tk;
        case _("Vary"):
            return HeaderType::Vary;
        case _("WWW-Authenticate"):
            return HeaderType::WWWAuthenticate;
        case _("X-Frame-Options"):
            return HeaderType::XFrameOptions;

        default:
            break;
    }

    return Header::HeaderType::Undefined;
}

std::string Header::HeaderType2String(Header::HeaderType headerType)
{
    switch (headerType)
    {
        case Header::HeaderType::Accept:
            return "Accept";
        case Header::HeaderType::AcceptCharset:
            return "Accept-Charset";
        case Header::HeaderType::AcceptEncoding:
            return "Accept-Encoding";
        case Header::HeaderType::AcceptDatetime:
            return "Accept-Datetime";
        case Header::HeaderType::AcceptLanguage:
            return "Accept-Language";
        case Header::HeaderType::Authorization:
            return "Authorization";
        case Header::HeaderType::CacheControl:
            return "Cache-Control";
        case Header::HeaderType::Connection:
            return "Connection";
        case Header::HeaderType::ContentEncoding:
            return "Content-Encoding";
        case Header::HeaderType::ContentLength:
            return "Content-Length";
        case Header::HeaderType::ContentMD5:
            return "Content-MD5";
        case Header::HeaderType::ContentType:
            return "Content-Type";
        case Header::HeaderType::Cookie:
            return "Cookie";
        case Header::HeaderType::Date:
            return "Date";
        case Header::HeaderType::Expect:
            return "Expect";
        case Header::HeaderType::Forwarded:
            return "Forwarded";
        case Header::HeaderType::From:
            return "From";
        case Header::HeaderType::HTTP2Settings:
            return "HTTP2-Settings";
        case Header::HeaderType::Host:
            return "Host";
        case Header::HeaderType::IfMatch:
            return "If-Match";
        case Header::HeaderType::IfModifiedSince:
            return "If-Modified-Since";
        case Header::HeaderType::IfNoneMatch:
            return "If-None-Match";
        case Header::HeaderType::IfRange:
            return "If-Range";
        case Header::HeaderType::IfUnmodifiedSince:
            return "If-Unmodified-Since";
        case Header::HeaderType::MaxForwards:
            return "Max-Forwards";
        case Header::HeaderType::Origin:
            return "Origin";
        case Header::HeaderType::Pragma:
            return "Pragma";
        case Header::HeaderType::Prefer:
            return "Prefer";
        case Header::HeaderType::ProxyAuthorization:
            return "Proxy-Authorization";
        case Header::HeaderType::Range:
            return "Range";
        case Header::HeaderType::Referer:
            return "Referer";
        case Header::HeaderType::TE:
            return "TE";
        case Header::HeaderType::Trailer:
            return "Trailer";
        case Header::HeaderType::TransferEncoding:
            return "Transfer-Encoding";
        case Header::HeaderType::UserAgent:
            return "User-Agent";
        case Header::HeaderType::Upgrade:
            return "Upgrade";
        case Header::HeaderType::Via:
            return "Via";
        case Header::HeaderType::Warning:
            return "Warning";

        case HeaderType::AcceptCH:
            return "Accept-CH";
        case HeaderType::AccessControlAllowOrigin:
            return "Access-Control-Allow-Origin";
        case HeaderType::AccessControlAllowCredentials:
            return "Access-Control-Allow-Credential";
        case HeaderType::AccessControlExposeHeaders:
            return "Access-Control-Expose-Headers";
        case HeaderType::AccessControlMaxAge:
            return "Access-Control-Max-Age";
        case HeaderType::AccessControlAllowMethods:
            return "Access-Control-Allow-Methods";
        case HeaderType::AccessControlAllowHeaders:
            return "Access-Control-Allow-Headers";
        case HeaderType::AcceptPatch:
            return "Accept-Patch";
        case HeaderType::AcceptRanges:
            return "Accept-Ranges";
        case HeaderType::Age:
            return "Age";
        case HeaderType::Allow:
            return "Allow";
        case HeaderType::AltSvc:
            return "Alt-Svc";
        case HeaderType::ContentDisposition:
            return "Content-Disposition";
        case HeaderType::ContentLanguage:
            return "Content-Language";
        case HeaderType::ContentLocation:
            return "Content-Location";
        case HeaderType::ContentRange:
            return "Content-Range";
        case HeaderType::DeltaBase:
            return "Delta-Base";
        case HeaderType::ETag:
            return "ETag";
        case HeaderType::Expires:
            return "Expires";
        case HeaderType::IM:
            return "IM";
        case HeaderType::LastModified:
            return "Last-Modified";
        case HeaderType::Link:
            return "Link";
        case HeaderType::Location:
            return "Location";
        case HeaderType::P3P:
            return "P3P";
        case HeaderType::PreferenceApplied:
            return "Preference-Applied";
        case HeaderType::ProxyAuthenticate:
            return "Proxy-Authenticate";
        case HeaderType::PublicKeyPins:
            return "Public-Key-Pins";
        case HeaderType::RetryAfter:
            return "Retry-After";
        case HeaderType::Server:
            return "Server";
        case HeaderType::SetCookie:
            return "Set-Cookie";
        case HeaderType::StrictTransportSecurity:
            return "Strict-Transport-Security";
        case HeaderType::Tk:
            return "Tk";
        case HeaderType::Vary:
            return "Vary";
        case HeaderType::WWWAuthenticate:
            return "WWW-Authenticate";
        case HeaderType::XFrameOptions:
            return "X-Frame-Options";
        default:
            break;
    }

    return "";
}
