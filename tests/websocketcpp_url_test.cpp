#include <gtest/gtest.h>
#include "Url.h"

using namespace WebSocketCpp;

// ─────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────
class UrlTest : public ::testing::Test
{
protected:
    Url url;
};


// ─────────────────────────────────────────────────────────────
// Construction & default state
// ─────────────────────────────────────────────────────────────
TEST_F(UrlTest, DefaultConstructor_IsNotInitialized)
{
    EXPECT_FALSE(url.IsInitiaized());
    EXPECT_EQ(url.GetScheme(), Url::Scheme::Undefined);
    EXPECT_TRUE(url.GetHost().empty());
    EXPECT_TRUE(url.GetPath().empty());
    EXPECT_TRUE(url.GetUser().empty());
    EXPECT_TRUE(url.GetFragment().empty());
}

TEST_F(UrlTest, ConstructFromString_ParsesCorrectly)
{
    Url u("ws://example.com/chat");
    EXPECT_TRUE(u.IsInitiaized());
    EXPECT_EQ(u.GetScheme(), Url::Scheme::WS);
    EXPECT_EQ(u.GetHost(), "example.com");
}


// ─────────────────────────────────────────────────────────────
// Scheme parsing
// ─────────────────────────────────────────────────────────────
TEST_F(UrlTest, Parse_SchemeWS)
{
    EXPECT_TRUE(url.Parse("ws://example.com/path"));
    EXPECT_EQ(url.GetScheme(), Url::Scheme::WS);
}

TEST_F(UrlTest, Parse_SchemeWSS)
{
    EXPECT_TRUE(url.Parse("wss://example.com/path"));
    EXPECT_EQ(url.GetScheme(), Url::Scheme::WSS);
}

TEST_F(UrlTest, Parse_SchemeHTTP)
{
    EXPECT_TRUE(url.Parse("http://example.com/path"));
    EXPECT_EQ(url.GetScheme(), Url::Scheme::HTTP);
}

TEST_F(UrlTest, Parse_SchemeHTTPS)
{
    EXPECT_TRUE(url.Parse("https://example.com/path"));
    EXPECT_EQ(url.GetScheme(), Url::Scheme::HTTPS);
}

TEST_F(UrlTest, Parse_SchemeIsCaseInsensitive)
{
    EXPECT_TRUE(url.Parse("WS://example.com/path"));
    EXPECT_EQ(url.GetScheme(), Url::Scheme::WS);
}

TEST_F(UrlTest, Parse_UnsupportedScheme_ReturnsFalse)
{
    EXPECT_FALSE(url.Parse("ftp://example.com/path"));
    EXPECT_EQ(url.GetScheme(), Url::Scheme::Undefined);
}

TEST_F(UrlTest, Parse_NoScheme_ReturnsFalse)
{
    EXPECT_FALSE(url.Parse("example.com/path"));
}


// ─────────────────────────────────────────────────────────────
// Default ports
// ─────────────────────────────────────────────────────────────
TEST_F(UrlTest, Parse_DefaultPortForWS)
{
    url.Parse("ws://example.com/path");
    EXPECT_EQ(url.GetPort(), 80);
}

TEST_F(UrlTest, Parse_DefaultPortForHTTP)
{
    url.Parse("http://example.com/path");
    EXPECT_EQ(url.GetPort(), 80);
}

TEST_F(UrlTest, Parse_DefaultPortForWSS)
{
    url.Parse("wss://example.com/path");
    EXPECT_EQ(url.GetPort(), 443);
}

TEST_F(UrlTest, Parse_DefaultPortForHTTPS)
{
    url.Parse("https://example.com/path");
    EXPECT_EQ(url.GetPort(), 443);
}

TEST_F(UrlTest, Parse_ExplicitPortOverridesDefault)
{
    url.Parse("ws://example.com:9000/path");
    EXPECT_EQ(url.GetPort(), 9000);
}

TEST_F(UrlTest, Parse_NonNumericPort_ReturnsFalse)
{
    EXPECT_FALSE(url.Parse("ws://example.com:abc/path"));
}


// ─────────────────────────────────────────────────────────────
// Host parsing
// ─────────────────────────────────────────────────────────────
TEST_F(UrlTest, Parse_SimpleHost)
{
    url.Parse("ws://example.com/path");
    EXPECT_EQ(url.GetHost(), "example.com");
}

TEST_F(UrlTest, Parse_IPAddress)
{
    url.Parse("ws://127.0.0.1/path");
    EXPECT_EQ(url.GetHost(), "127.0.0.1");
}

TEST_F(UrlTest, Parse_IPAddressWithPort)
{
    url.Parse("ws://127.0.0.1:8080/path");
    EXPECT_EQ(url.GetHost(), "127.0.0.1");
    EXPECT_EQ(url.GetPort(), 8080);
}

TEST_F(UrlTest, Parse_FullUrlWithPort)
{
    url.Parse("http://some.host:8080/path/to/resource?param=value#fragment");
    EXPECT_EQ(url.GetHost(), "some.host");
    EXPECT_EQ(url.GetPort(), 8080);
    EXPECT_EQ(url.GetPath(), "/path/to/resource");
    EXPECT_EQ(url.GetQueryValue("param"), "value");
    EXPECT_EQ(url.GetFragment(), "fragment");
}


// ─────────────────────────────────────────────────────────────
// User info
// ─────────────────────────────────────────────────────────────
TEST_F(UrlTest, Parse_UserInAuthority)
{
    url.Parse("ws://alice@example.com/path");
    EXPECT_EQ(url.GetUser(), "alice");
    EXPECT_EQ(url.GetHost(), "example.com");
}

TEST_F(UrlTest, Parse_NoUser_UserIsEmpty)
{
    url.Parse("ws://example.com/path");
    EXPECT_TRUE(url.GetUser().empty());
}


// ─────────────────────────────────────────────────────────────
// Path parsing
// ─────────────────────────────────────────────────────────────
TEST_F(UrlTest, Parse_SimplePath)
{
    url.Parse("ws://example.com/chat/room");
    // Accept either with or without leading slash depending on implementation
    EXPECT_NE(url.GetPath().find("chat/room"), std::string::npos);
}

TEST_F(UrlTest, Parse_RootPath)
{
    url.Parse("ws://example.com/");
    EXPECT_TRUE(url.GetPath().empty() || url.GetPath() == "/");
}

TEST_F(UrlTest, GetNormalizedPath_AlwaysHasLeadingSlash)
{
    url.Parse("ws://example.com/chat");
    EXPECT_EQ(url.GetNormalizedPath().front(), '/');
}

TEST_F(UrlTest, GetNormalizedPath_DoesNotDoubleSlash)
{
    url.SetPath("/chat");
    std::string norm = url.GetNormalizedPath();
    EXPECT_EQ(norm, "/chat");
    EXPECT_NE(norm.substr(0, 2), "//");
}


// ─────────────────────────────────────────────────────────────
// Query string
// ─────────────────────────────────────────────────────────────
TEST_F(UrlTest, Parse_SingleQueryParam)
{
    url.Parse("ws://example.com/path?key=value");
    EXPECT_TRUE(url.HasQuery());
    EXPECT_EQ(url.GetQueryValue("key"), "value");
}

TEST_F(UrlTest, Parse_MultipleQueryParams)
{
    url.Parse("ws://example.com/path?a=1&b=2&c=3");
    EXPECT_EQ(url.GetQueryValue("a"), "1");
    EXPECT_EQ(url.GetQueryValue("b"), "2");
    EXPECT_EQ(url.GetQueryValue("c"), "3");
}

TEST_F(UrlTest, Parse_UrlEncodedQueryValues)
{
    url.Parse("ws://example.com/path?name=hello%20world");
    EXPECT_EQ(url.GetQueryValue("name"), "hello world");
}

TEST_F(UrlTest, Parse_NoQuery_HasQueryIsFalse)
{
    url.Parse("ws://example.com/path");
    EXPECT_FALSE(url.HasQuery());
}

TEST_F(UrlTest, Parse_MissingQueryValue_ReturnsEmpty)
{
    url.Parse("ws://example.com/path?key=value");
    EXPECT_EQ(url.GetQueryValue("nonexistent"), "");
}

TEST_F(UrlTest, SetQueryValue_AddsEntry)
{
    url.Parse("ws://example.com/path");
    url.SetQueryValue("token", "abc123");
    EXPECT_EQ(url.GetQueryValue("token"), "abc123");
    EXPECT_TRUE(url.HasQuery());
}

TEST_F(UrlTest, SetQueryValue_OverwritesExisting)
{
    url.Parse("ws://example.com/path?key=old");
    url.SetQueryValue("key", "new");
    EXPECT_EQ(url.GetQueryValue("key"), "new");
}


// ─────────────────────────────────────────────────────────────
// Fragment
// ─────────────────────────────────────────────────────────────
TEST_F(UrlTest, Parse_FragmentWithQuery)
{
    url.Parse("ws://example.com/path?key=val#section1");
    EXPECT_EQ(url.GetFragment(), "section1");
    EXPECT_EQ(url.GetQueryValue("key"), "val");
}

TEST_F(UrlTest, Parse_FragmentWithoutQuery)
{
    url.Parse("ws://example.com/path#section1");
    EXPECT_EQ(url.GetFragment(), "section1"); // currently fails — path swallows it
}

TEST_F(UrlTest, Parse_NoFragment_FragmentIsEmpty)
{
    url.Parse("ws://example.com/path?key=val");
    EXPECT_TRUE(url.GetFragment().empty());
}


// ─────────────────────────────────────────────────────────────
// ToString round-trip
// ─────────────────────────────────────────────────────────────
TEST_F(UrlTest, ToString_Full_ReconstructsSchemeAndHost)
{
    url.Parse("ws://example.com/chat");
    std::string result = url.ToString(true);
    EXPECT_NE(result.find("ws://"), std::string::npos);
    EXPECT_NE(result.find("example.com"), std::string::npos);
}

TEST_F(UrlTest, ToString_Full_IncludesQuery)
{
    url.Parse("ws://example.com/path?key=value");
    std::string result = url.ToString(true);
    EXPECT_NE(result.find("key=value"), std::string::npos);
}

TEST_F(UrlTest, ToString_Full_IncludesFragment)
{
    url.Parse("ws://example.com/path?k=v#frag");
    std::string result = url.ToString(true);
    EXPECT_NE(result.find("#frag"), std::string::npos);
}

TEST_F(UrlTest, ToString_NonDefaultPort_IncludesPort)
{
    url.Parse("ws://example.com:9000/path");
    std::string result = url.ToString(true);
    EXPECT_NE(result.find(":9000"), std::string::npos);
}

TEST_F(UrlTest, ToString_DefaultPort_OmitsPort)
{
    url.Parse("ws://example.com/path");
    std::string result = url.ToString(true);
    EXPECT_EQ(result.find(":80"), std::string::npos);
}

TEST_F(UrlTest, ToString_PathOnly_NoSchemeOrHost)
{
    url.Parse("ws://example.com/chat");
    std::string result = url.ToString(false);
    EXPECT_EQ(result.find("ws://"), std::string::npos);
    EXPECT_EQ(result.find("example.com"), std::string::npos);
    EXPECT_NE(result.find("chat"), std::string::npos);
}

TEST_F(UrlTest, ToString_User_CorrectFormat)
{
    // Verifies user appears as user@host, not @userhost
    url.Parse("ws://alice@example.com/path");
    std::string result = url.ToString(true);
    EXPECT_EQ(url.GetUser(), "alice");
    EXPECT_EQ(url.GetHost(), "example.com");
    EXPECT_EQ(url.GetPath(), "/path");
    EXPECT_NE(result.find("alice@example.com"), std::string::npos);
}


// ─────────────────────────────────────────────────────────────
// Setters & mutability
// ─────────────────────────────────────────────────────────────
TEST_F(UrlTest, SetScheme_UpdatesScheme)
{
    url.Parse("ws://example.com/path");
    url.SetScheme(Url::Scheme::WSS);
    EXPECT_EQ(url.GetScheme(), Url::Scheme::WSS);
}

TEST_F(UrlTest, SetHost_UpdatesHost)
{
    url.Parse("ws://example.com/path");
    url.SetHost("other.com");
    EXPECT_EQ(url.GetHost(), "other.com");
}

TEST_F(UrlTest, SetPath_UpdatesPath)
{
    url.Parse("ws://example.com/old");
    url.SetPath("new/path");
    EXPECT_EQ(url.GetPath(), "new/path");
}

TEST_F(UrlTest, SetPort_UpdatesPort)
{
    url.Parse("ws://example.com/path");
    url.SetPort(1234);
    EXPECT_EQ(url.GetPort(), 1234);
}

TEST_F(UrlTest, SetFragment_UpdatesFragment)
{
    url.Parse("ws://example.com/path");
    url.SetFragment("anchor");
    EXPECT_EQ(url.GetFragment(), "anchor");
}


// ─────────────────────────────────────────────────────────────
// Clear
// ─────────────────────────────────────────────────────────────
TEST_F(UrlTest, Clear_ResetsAllFields)
{
    url.Parse("wss://alice@example.com:9000/path?k=v#frag");
    url.Clear();

    EXPECT_FALSE(url.IsInitiaized());
    EXPECT_EQ(url.GetScheme(), Url::Scheme::Undefined);
    EXPECT_TRUE(url.GetHost().empty());
    EXPECT_TRUE(url.GetUser().empty());
    EXPECT_TRUE(url.GetPath().empty());
    EXPECT_TRUE(url.GetFragment().empty());
    EXPECT_FALSE(url.HasQuery());
}

TEST_F(UrlTest, Clear_ThenReParse_Works)
{
    url.Parse("ws://example.com/first");
    url.Clear();
    url.Parse("wss://other.com/second");
    EXPECT_EQ(url.GetScheme(), Url::Scheme::WSS);
    EXPECT_EQ(url.GetHost(), "other.com");
}


// ─────────────────────────────────────────────────────────────
// GetOriginalSize
// ─────────────────────────────────────────────────────────────
TEST_F(UrlTest, GetOriginalSize_MatchesInputLength)
{
    std::string input = "ws://example.com/path?key=val";
    url.Parse(input);
    EXPECT_EQ(url.GetOriginalSize(), input.size());
}


// ─────────────────────────────────────────────────────────────
// Edge cases
// ─────────────────────────────────────────────────────────────
TEST_F(UrlTest, Parse_EmptyString_ReturnsFalse)
{
    EXPECT_FALSE(url.Parse(""));
}

TEST_F(UrlTest, Parse_SchemeOnly_ReturnsFalse)
{
    EXPECT_FALSE(url.Parse("ws://"));
}

TEST_F(UrlTest, Parse_Reparse_ClearsOldState)
{
    url.Parse("ws://example.com/path?key=val");
    url.Parse("wss://other.com/newpath");
    EXPECT_EQ(url.GetScheme(), Url::Scheme::WSS);
    EXPECT_EQ(url.GetHost(), "other.com");
    EXPECT_FALSE(url.HasQuery());
}
