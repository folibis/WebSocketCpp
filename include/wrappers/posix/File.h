#ifndef WEB_SOCKET_CPP_FILE_H
#define WEB_SOCKET_CPP_FILE_H

#include <string>

#include "IErrorable.h"

namespace WebSocketCpp
{

class File : public IErrorable
{
public:
    enum class Mode
    {
        Undefined = 0,
        Read,
        Write
    };
    File();
    File(const std::string& file, Mode mode);
    ~File();
    bool   Open(const std::string& file, Mode mode);
    bool   Close();
    size_t Read(char* buffer, size_t size);
    size_t Write(const char* buffer, size_t size);
    bool   IsOpened() const;

protected:
    int Mode2Flag(Mode mode);

private:
    std::string m_file;
    Mode        m_mode = Mode::Undefined;
    int         m_fd   = (-1);
};

inline File::Mode operator|(File::Mode a, File::Mode b)
{
    return static_cast<File::Mode>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator==(File::Mode a, File::Mode b)
{
    return (static_cast<int>(a) == static_cast<int>(b));
}

inline bool contains(File::Mode a, File::Mode b)
{
    return ((a | b) == b);
}

} // namespace WebSocketCpp

#endif // WEB_SOCKET_CPP_FILE_H
