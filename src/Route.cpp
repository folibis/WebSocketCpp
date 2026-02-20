#include "Route.h"

#include "StringUtil.h"
#include "common.h"
#include <cstring>

using namespace WebSocketCpp;

Route::Route(const std::string& path, Method method, bool useAuth)
    : m_method(method),
      m_path(path),
      m_useAuth(useAuth)
{
    Parse(path);
}

const std::string& Route::GetPath() const
{
    return m_path;
}

bool Route::IsMatch(Request& request)
{
    const std::string path   = request.GetUrl().GetPath();
    const char*       ch     = path.data();
    size_t            length = path.length();

    if (request.GetMethod() != m_method)
    {
        return false;
    }

    size_t pos    = 0;
    size_t offset = 0;

    for (size_t i = 0; i < m_tokens.size(); i++)
    {
        auto& token = m_tokens[i];

        if (token.type == Token::Type::Any)
        {
            if (i + 1 >= m_tokens.size())
            {
                return true;
            }

            auto& nextToken = m_tokens[i + 1];
            bool  found     = false;

            for (size_t searchPos = pos; searchPos < length; searchPos++)
            {
                if (nextToken.IsMatch(ch + searchPos, length - searchPos, offset))
                {
                    if (nextToken.type == Token::Type::Variable)
                    {
                        request.SetArg(nextToken.text, std::string(ch + searchPos, offset));
                    }
                    pos   = searchPos + offset;
                    found = true;
                    i++;
                    break;
                }
            }

            if (!found)
            {
                return false;
            }
            continue;
        }

        if (token.IsMatch(ch + pos, length - pos, offset))
        {
            if (token.type == Token::Type::Variable)
            {
                request.SetArg(token.text, std::string(ch + pos, offset));
            }
            pos += offset;
        }
        else
        {
            if (token.optional == false)
            {
                return false;
            }
        }
    }

    if (pos < length)
    {
        return false;
    }

    return true;
}

bool Route::IsUseAuth() const
{
    return m_useAuth;
}

std::string Route::ToString() const
{
    std::string result = "Route (method: " + Method2String(m_method) +
                         ", path: " + m_path +
                         ", auth: " + (m_useAuth ? "true" : "false");

    result += ", tokens: [";
    for (size_t i = 0; i < m_tokens.size(); i++)
    {
        if (i > 0) result += ", ";

        switch (m_tokens[i].type)
        {
            case Token::Type::Default:
                result += "'" + m_tokens[i].text + "'";
                break;
            case Token::Type::Variable:
                result += "{" + m_tokens[i].text + "}";
                break;
            case Token::Type::Group:
                result += "(...)";
                break;
            case Token::Type::Any:
                result += "*";
                break;
        }

        if (m_tokens[i].optional)
            result += "?";
    }
    result += "])";

    return result;
}

bool Route::Parse(const std::string& path)
{
    const char* ch    = path.data();
    std::string str   = "";
    State       state = State::Default;
    Token       current;

    for (size_t i = 0; i < path.size(); i++)
    {
        switch (ch[i])
        {
            case '*':
                AddToken(current, str);
                str          = "";
                current.text = "*";
                current.type = Token::Type::Any;
                AddToken(current, str);
                break;

            case '[':
                if (state == State::Optional)
                {
                    return false;
                }
                AddToken(current, str);
                str              = "";
                current.optional = true;
                state            = State::Optional;
                break;

            case ']':
                if (state != State::Optional)
                {
                    return false;
                }
                AddToken(current, str);
                str              = "";
                current.optional = false;
                state            = State::Default;
                break;

            case '}':
                if (state != State::Variable && state != State::VariableType)
                {
                    return false;
                }

                if (state == State::VariableType)
                {
                    current.view = Token::String2View(str);
                    if (current.text.empty())
                    {
                        return false;
                    }
                    AddToken(current, "");
                }
                else
                {
                    current.text = str;
                    if (current.text.empty())
                    {
                        return false;
                    }
                    current.view = Token::View::String;
                    AddToken(current, "");
                }
                str   = "";
                state = State::Default;
                break;

            case '{':
                if (state == State::Variable || state == State::VariableType)
                {
                    return false;
                }

                switch (state)
                {
                    case State::Default:
                    case State::Optional:
                        AddToken(current, str);
                        str = "";
                        break;
                    default:
                        break;
                }

                state        = State::Variable;
                current.type = Token::Type::Variable;
                break;

            case ':':
                if (state == State::Variable)
                {
                    state        = State::VariableType;
                    current.text = str;
                    str          = "";
                }
                else
                {
                    str += ch[i];
                }
                break;

            case '(':
                AddToken(current, str);
                str          = "";
                state        = State::OrGroup;
                current.type = Token::Type::Group;
                break;

            case '|':
                current.group.push_back(str);
                str = "";
                break;

            case ')':
                current.group.push_back(str);
                current.SortGroup();
                str   = "";
                state = State::Default;
                AddToken(current, "");
                break;

            default:
                str += ch[i];
        }
    }

    if (state != State::Default)
    {
        return false;
    }

    AddToken(current, str);

    return true;
}

bool Route::AddToken(Token& token, const std::string& str)
{
    if (!str.empty())
    {
        token.text = str;
    }

    if (token.IsEmpty())
    {
        return false;
    }

    m_tokens.push_back(token);
    token.Clear();
    return true;
}

bool Route::Token::IsMatch(const char* ch, size_t length, size_t& pos)
{
    bool retval = false;

    switch (type)
    {
        case Type::Default:
            if (text.length() > length)
            {
                break;
            }
            if (std::memcmp(ch, text.data(), text.length()) == 0)
            {
                pos    = text.length();
                retval = true;
            }
            break;

        case Type::Variable:
        {
            bool (Route::Token::*fptr)(char) const = &Route::Token::IsString;

            switch (view)
            {
                case View::Any:
                    fptr = &Route::Token::IsAny;
                    break;
                case View::Alpha:
                    fptr = &Route::Token::IsAlpha;
                    break;
                case View::Numeric:
                    fptr = &Route::Token::IsNumeric;
                    break;
                case View::String:
                    fptr = &Route::Token::IsString;
                    break;
                case View::Upper:
                    fptr = &Route::Token::IsUpper;
                    break;
                case View::Lower:
                    fptr = &Route::Token::IsLower;
                    break;
                case View::Default:
                    fptr = &Route::Token::IsString;
                    break;
                default:
                    return false;
            }

            auto   f = std::mem_fn(fptr);
            size_t i = 0;
            for (i = 0; i < length; i++)
            {
                if (f(this, ch[i]) == false)
                {
                    break;
                }
            }

            retval = (i > 0);
            if (retval)
            {
                pos = i;
            }
        }
        break;

        case Type::Group:
            for (auto& str : group)
            {
                if (str.length() > length)
                {
                    continue;
                }
                if (std::memcmp(ch, str.data(), str.length()) == 0)
                {
                    pos    = str.length();
                    retval = true;
                    break;
                }
            }
            break;

        case Type::Any:
            return true;
            break;
    }

    return retval;
}

bool Route::Token::IsAny(char ch) const
{
    return true;
}

bool Route::Token::IsString(char ch) const
{
    static ByteArray allowed = {'.', '_', '-'};

    return (IsAlpha(ch) || IsNumeric(ch) || StringUtil::Contains(allowed, ch));
}

bool Route::Token::IsAlpha(char ch) const
{
    return (IsLower(ch) || IsUpper(ch));
}

bool Route::Token::IsNumeric(char ch) const
{
    return (ch >= '0' && ch <= '9');
}

bool Route::Token::IsLower(char ch) const
{
    return (ch >= 'a' && ch <= 'z');
}

bool Route::Token::IsUpper(char ch) const
{
    return (ch >= 'A' && ch <= 'Z');
}

Route::Token::View Route::Token::String2View(const std::string& str)
{
    std::string s = str;
    StringUtil::ToLower(s);
    switch (_(s.c_str()))
    {
        case _("any"):
            return Route::Token::View::Any;
        case _("alpha"):
            return Route::Token::View::Alpha;
        case _("numeric"):
            return Route::Token::View::Numeric;
        case _("string"):
            return Route::Token::View::String;
        case _("upper"):
            return Route::Token::View::Upper;
        case _("lower"):
            return Route::Token::View::Lower;
        default:
            break;
    }

    return Route::Token::View::Default;
}
