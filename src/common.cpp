#include "common.h"
#include "StringUtil.h"
#include "common.h"


std::string WebCpp::Protocol2String(Protocol protocol) {
    switch (protocol) {
    case Protocol::HTTP:
        return "HTTP";
    case Protocol::HTTPS:
        return "HTTPS";
    case Protocol::WS:
        return "WS";
    case Protocol::WSS:
        return "WSS";
    default:
        break;
    }

    return "";
}

WebCpp::Protocol WebCpp::String2Protocol(const std::string &str) {
    std::string s = str;
    StringUtil::ToUpper(s);

    switch (_(s.c_str())) {
    case _("HTTP"):
        return Protocol::HTTP;
    case _("HTTPS"):
        return Protocol::HTTPS;
    case _("WS"):
        return Protocol::WS;
    case _("WSS"):
        return Protocol::WSS;
    default:
        break;
    }

    return Protocol::Undefined;
}

WebCpp::Method WebCpp::String2Method(const std::string &str) {
    std::string s = str;
    StringUtil::ToUpper(s);

    switch (_(s.c_str())) {
    case _("OPTIONS"):
        return Method::OPTIONS;
    case _("GET"):
        return Method::GET;
    case _("HEAD"):
        return Method::HEAD;
    case _("POST"):
        return Method::POST;
    case _("PUT"):
        return Method::PUT;
    case _("DELETE"):
        return Method::DELETE;
    case _("TRACE"):
        return Method::TRACE;
    case _("CONNECT"):
        return Method::CONNECT;
    default:
        break;
    }

    return Method::Undefined;
}

std::string WebCpp::Method2String(Method method) {
    switch (method) {
    case Method::OPTIONS:
        return "OPTIONS";
    case Method::GET:
        return "GET";
    case Method::HEAD:
        return "HEAD";
    case Method::POST:
        return "POST";
    case Method::PUT:
        return "PUT";
    case Method::DELETE:
        return "DELETE";
    case Method::TRACE:
        return "TRACE";
    case Method::CONNECT:
        return "CONNECT";
    default:
        break;
    }

    return "";
}
