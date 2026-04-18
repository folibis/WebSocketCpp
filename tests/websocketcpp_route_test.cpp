#include <gtest/gtest.h>

#include <random>

#include "Request.h"
#include "Route.h"

using namespace WebSocketCpp;

class RandomGen
{
    std::mt19937 rng;

public:
    RandomGen()
        : rng(std::random_device{}())
    {
    }

    std::string AlphaNumeric(size_t min_len = 5, size_t max_len = 15)
    {
        const char                            chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
        std::uniform_int_distribution<size_t> char_dist(0, sizeof(chars) - 2);

        size_t      len = len_dist(rng);
        std::string result;
        result.reserve(len);
        for (size_t i = 0; i < len; ++i)
            result += chars[char_dist(rng)];
        return result;
    }

    std::string Numeric(size_t min_len = 1, size_t max_len = 5)
    {
        std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
        std::uniform_int_distribution<int>    digit_dist(0, 9);

        size_t      len = len_dist(rng);
        std::string result;
        result.reserve(len);
        for (size_t i = 0; i < len; ++i)
            result += '0' + digit_dist(rng);
        return result;
    }

    std::string Alpha(size_t min_len = 3, size_t max_len = 10)
    {
        const char                            chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
        std::uniform_int_distribution<size_t> char_dist(0, sizeof(chars) - 2);

        size_t      len = len_dist(rng);
        std::string result;
        result.reserve(len);
        for (size_t i = 0; i < len; ++i)
            result += chars[char_dist(rng)];
        return result;
    }

    std::string Lower(size_t min_len = 3, size_t max_len = 10)
    {
        std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
        std::uniform_int_distribution<char>   char_dist('a', 'z');

        size_t      len = len_dist(rng);
        std::string result;
        result.reserve(len);
        for (size_t i = 0; i < len; ++i)
            result += char_dist(rng);
        return result;
    }

    std::string Upper(size_t min_len = 3, size_t max_len = 10)
    {
        std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
        std::uniform_int_distribution<char>   char_dist('A', 'Z');

        size_t      len = len_dist(rng);
        std::string result;
        result.reserve(len);
        for (size_t i = 0; i < len; ++i)
            result += char_dist(rng);
        return result;
    }
};

class RouteTest : public ::testing::Test
{
protected:
    RandomGen rand;
};

TEST_F(RouteTest, Constructor_StoresPathAndMethod)
{
    std::string path = "/users";
    Route       route(path, Method::GET, false);

    EXPECT_EQ(route.GetPath(), path);
    EXPECT_FALSE(route.IsUseAuth());
}

TEST_F(RouteTest, Constructor_WithAuth)
{
    Route route("/admin", Method::POST, true);
    EXPECT_TRUE(route.IsUseAuth());
}

TEST_F(RouteTest, IsMatch_ExactLiteralPath)
{
    std::string path = "/users/profile";
    Route       route(path, Method::GET);

    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse(path, false);

    EXPECT_TRUE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_LiteralPath_WrongMethod)
{
    Route   route("/users", Method::GET);
    Request req;
    req.SetMethod(Method::POST);
    req.GetUrl().Parse("/users", false);

    EXPECT_FALSE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_LiteralPath_WrongPath)
{
    Route   route("/users", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/posts", false);

    EXPECT_FALSE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_LiteralPath_ExtraSegments)
{
    Route   route("/users", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users/extra", false);

    EXPECT_FALSE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_SimpleVariable)
{
    std::string userId = rand.AlphaNumeric(5, 10);
    Route       route("/users/{id}", Method::GET);
    Request     req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users/" + userId, false);

    EXPECT_TRUE(route.IsMatch(req));
    EXPECT_EQ(req.GetArg("id"), userId);
}

TEST_F(RouteTest, IsMatch_MultipleVariables)
{
    std::string userId = rand.AlphaNumeric(5, 10);
    std::string postId = rand.AlphaNumeric(5, 10);
    Route       route("/users/{userId}/posts/{postId}", Method::GET);
    Request     req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users/" + userId + "/posts/" + postId, false);

    EXPECT_TRUE(route.IsMatch(req));
    EXPECT_EQ(req.GetArg("userId"), userId);
    EXPECT_EQ(req.GetArg("postId"), postId);
}

TEST_F(RouteTest, IsMatch_VariableAtEnd)
{
    std::string filename = rand.AlphaNumeric(8, 15);
    Route       route("/files/{filename}", Method::GET);
    Request     req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/files/" + filename, false);

    EXPECT_TRUE(route.IsMatch(req));
    EXPECT_EQ(req.GetArg("filename"), filename);
}

TEST_F(RouteTest, IsMatch_NumericVariable)
{
    std::string id = rand.Numeric(3, 8);
    Route       route("/users/{id:numeric}", Method::GET);
    Request     req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users/" + id, false);

    EXPECT_TRUE(route.IsMatch(req));
    EXPECT_EQ(req.GetArg("id"), id);
}

TEST_F(RouteTest, IsMatch_NumericVariable_RejectsAlpha)
{
    Route   route("/users/{id:numeric}", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users/abc", false);

    EXPECT_FALSE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_AlphaVariable)
{
    std::string name = rand.Alpha(5, 10);
    Route       route("/users/{name:alpha}", Method::GET);
    Request     req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users/" + name, false);

    EXPECT_TRUE(route.IsMatch(req));
    EXPECT_EQ(req.GetArg("name"), name);
}

TEST_F(RouteTest, IsMatch_AlphaVariable_RejectsNumeric)
{
    Route   route("/users/{name:alpha}", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users/12345", false);

    EXPECT_FALSE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_UpperVariable)
{
    std::string code = rand.Upper(3, 6);
    Route       route("/codes/{code:upper}", Method::GET);
    Request     req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/codes/" + code, false);

    EXPECT_TRUE(route.IsMatch(req));
    EXPECT_EQ(req.GetArg("code"), code);
}

TEST_F(RouteTest, IsMatch_LowerVariable)
{
    std::string tag = rand.Lower(4, 8);
    Route       route("/tags/{tag:lower}", Method::GET);
    Request     req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/tags/" + tag, false);

    EXPECT_TRUE(route.IsMatch(req));
    EXPECT_EQ(req.GetArg("tag"), tag);
}

TEST_F(RouteTest, IsMatch_StringVariable_AllowsUnderscoreDashDot)
{
    Route route("/files/{name:string}", Method::GET);

    Request req1;
    req1.SetMethod(Method::GET);
    req1.GetUrl().Parse("/files/my_file", false);
    EXPECT_TRUE(route.IsMatch(req1));

    Request req2;
    req2.SetMethod(Method::GET);
    req2.GetUrl().Parse("/files/my-file", false);
    EXPECT_TRUE(route.IsMatch(req2));

    Request req3;
    req3.SetMethod(Method::GET);
    req3.GetUrl().Parse("/files/file.txt", false);
    EXPECT_TRUE(route.IsMatch(req3));
}

TEST_F(RouteTest, IsMatch_AnyVariable)
{
    Route route("/data/{content:any}", Method::GET);

    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/data/anything!@#$%", false);
    EXPECT_TRUE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_WildcardAtEnd)
{
    Route route("/files/*", Method::GET);

    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/files/documents/2024/report.pdf", false);
    EXPECT_TRUE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_WildcardInMiddle)
{
    Route route("/files/*/download", Method::GET);

    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/files/documents/images/download", false);
    EXPECT_TRUE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_WildcardWithVariable)
{
    std::string filename = rand.AlphaNumeric(8, 15);
    Route       route("/files/*/download/{filename}", Method::GET);

    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/files/docs/images/download/" + filename, false);
    EXPECT_TRUE(route.IsMatch(req));
    EXPECT_EQ(req.GetArg("filename"), filename);
}

TEST_F(RouteTest, IsMatch_Wildcard_NotFoundAfter)
{
    Route route("/files/*/download", Method::GET);

    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/files/documents/images/upload", false);
    EXPECT_FALSE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_OptionalSegment_Present)
{
    Route route("/users[/active]", Method::GET);

    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users/active", false);
    EXPECT_TRUE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_OptionalSegment_Missing)
{
    Route route("/users[/active]", Method::GET);

    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users", false);
    EXPECT_TRUE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_OptionalVariable)
{
    Route route("/users[/{id}]", Method::GET);

    Request req1;
    req1.SetMethod(Method::GET);
    req1.GetUrl().Parse("/users", false);
    EXPECT_TRUE(route.IsMatch(req1));

    std::string id = rand.AlphaNumeric(5, 10);
    Request     req2;
    req2.SetMethod(Method::GET);
    req2.GetUrl().Parse("/users/" + id, false);
    EXPECT_TRUE(route.IsMatch(req2));
    EXPECT_EQ(req2.GetArg("id"), id);
}

TEST_F(RouteTest, IsMatch_OrGroup_FirstOption)
{
    Route route("/api/(v1|v2)/users", Method::GET);

    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/api/v1/users", false);
    EXPECT_TRUE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_OrGroup_SecondOption)
{
    Route route("/api/(v1|v2)/users", Method::GET);

    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/api/v2/users", false);
    EXPECT_TRUE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_OrGroup_NoMatch)
{
    Route route("/api/(v1|v2)/users", Method::GET);

    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/api/v3/users", false);
    EXPECT_FALSE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_OrGroup_LongestMatch)
{
    Route route("/files/(foobar|foo)", Method::GET);

    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/files/foobar", false);
    EXPECT_TRUE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_ComplexPattern_AllFeatures)
{
    std::string version = "v1";
    std::string userId  = rand.Numeric(5, 8);
    std::string action  = rand.Alpha(4, 10);

    Route route("/api/(v1|v2)/users/{id:numeric}[/actions]/{action:alpha}", Method::POST);

    Request req;
    req.SetMethod(Method::POST);
    req.GetUrl().Parse("/api/" + version + "/users/" + userId + "/actions/" + action, false);

    EXPECT_TRUE(route.IsMatch(req));
    EXPECT_EQ(req.GetArg("id"), userId);
    EXPECT_EQ(req.GetArg("action"), action);
}

TEST_F(RouteTest, IsMatch_NestedPath)
{
    std::string projectId = rand.AlphaNumeric(5, 10);
    std::string fileId    = rand.AlphaNumeric(5, 10);

    Route   route("/projects/{projectId}/files/{fileId}", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/projects/" + projectId + "/files/" + fileId, false);

    EXPECT_TRUE(route.IsMatch(req));
    EXPECT_EQ(req.GetArg("projectId"), projectId);
    EXPECT_EQ(req.GetArg("fileId"), fileId);
}

TEST_F(RouteTest, Parse_UnclosedBrace_ReturnsFalse)
{
    Route   route("/users/{id", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users/123", false);

    // Route with parse error should not match
    EXPECT_FALSE(route.IsMatch(req));
}

TEST_F(RouteTest, Parse_UnclosedBracket_ReturnsFalse)
{
    Route   route("/users[/active", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users/active", false);

    EXPECT_FALSE(route.IsMatch(req));
}

TEST_F(RouteTest, Parse_UnmatchedClosingBracket_ReturnsFalse)
{
    Route   route("/users]/active", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users]/active", false);

    EXPECT_FALSE(route.IsMatch(req));
}

TEST_F(RouteTest, Parse_NestedBraces_ReturnsFalse)
{
    Route   route("/users/{{id}}", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users/123", false);

    EXPECT_FALSE(route.IsMatch(req));
}

TEST_F(RouteTest, Parse_EmptyVariableName_ReturnsFalse)
{
    Route   route("/users/{}", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users/123", false);

    EXPECT_FALSE(route.IsMatch(req));
}

TEST_F(RouteTest, ToString_ContainsBasicInfo)
{
    Route       route("/users/{id}", Method::GET, true);
    std::string str = route.ToString();

    EXPECT_NE(str.find("GET"), std::string::npos);
    EXPECT_NE(str.find("/users/{id}"), std::string::npos);
    EXPECT_NE(str.find("auth: true"), std::string::npos);
}

TEST_F(RouteTest, ToString_ContainsTokens)
{
    Route       route("/users/{id}", Method::GET);
    std::string str = route.ToString();

    EXPECT_NE(str.find("tokens:"), std::string::npos);
}

TEST_F(RouteTest, IsMatch_EmptyPath)
{
    Route   route("/", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/", false);

    EXPECT_TRUE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_RootPath)
{
    Route   route("", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("", false);

    EXPECT_TRUE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_TrailingSlash_NotMatched)
{
    Route   route("/users", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/users/", false);

    // Trailing slash is an extra character
    EXPECT_FALSE(route.IsMatch(req));
}

TEST_F(RouteTest, IsMatch_VariableWithSpecialChars)
{
    Route route("/files/{name:string}", Method::GET);

    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/files/my-file_v2.1.txt", false);
    EXPECT_TRUE(route.IsMatch(req));
    EXPECT_EQ(req.GetArg("name"), "my-file_v2.1.txt");
}

TEST_F(RouteTest, IsMatch_MultipleOrGroups)
{
    Route route("/api/(v1|v2)/users/(active|inactive)", Method::GET);

    Request req1;
    req1.SetMethod(Method::GET);
    req1.GetUrl().Parse("/api/v1/users/active", false);
    EXPECT_TRUE(route.IsMatch(req1));

    Request req2;
    req2.SetMethod(Method::GET);
    req2.GetUrl().Parse("/api/v2/users/inactive", false);
    EXPECT_TRUE(route.IsMatch(req2));

    Request req3;
    req3.SetMethod(Method::GET);
    req3.GetUrl().Parse("/api/v1/users/pending", false);
    EXPECT_FALSE(route.IsMatch(req3));
}

TEST_F(RouteTest, WebSocket_SimpleConnection)
{
    Route   route("/ws", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/ws", false);

    EXPECT_TRUE(route.IsMatch(req));
}

TEST_F(RouteTest, WebSocket_RoomConnection)
{
    std::string roomId = rand.AlphaNumeric(8, 15);
    Route       route("/ws/room/{roomId}", Method::GET);
    Request     req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/ws/room/" + roomId, false);

    EXPECT_TRUE(route.IsMatch(req));
    EXPECT_EQ(req.GetArg("roomId"), roomId);
}

TEST_F(RouteTest, WebSocket_UserAndRoom)
{
    std::string userId = rand.Numeric(5, 10);
    std::string roomId = rand.AlphaNumeric(8, 15);

    Route   route("/ws/user/{userId}/room/{roomId}", Method::GET);
    Request req;
    req.SetMethod(Method::GET);
    req.GetUrl().Parse("/ws/user/" + userId + "/room/" + roomId, false);

    EXPECT_TRUE(route.IsMatch(req));
    EXPECT_EQ(req.GetArg("userId"), userId);
    EXPECT_EQ(req.GetArg("roomId"), roomId);
}

TEST_F(RouteTest, WebSocket_OptionalToken)
{
    Route route("/ws/chat[/{token}]", Method::GET);

    Request req1;
    req1.SetMethod(Method::GET);
    req1.GetUrl().Parse("/ws/chat", false);
    EXPECT_TRUE(route.IsMatch(req1));

    std::string token = rand.AlphaNumeric(16, 32);
    Request     req2;
    req2.SetMethod(Method::GET);
    req2.GetUrl().Parse("/ws/chat/" + token, false);
    EXPECT_TRUE(route.IsMatch(req2));
    EXPECT_EQ(req2.GetArg("token"), token);
}
