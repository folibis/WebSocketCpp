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
 *  */

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
