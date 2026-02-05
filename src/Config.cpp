#include "Config.h"

#include "DebugPrint.h"
#include "FileSystem.h"
#include "common.h"

using namespace WebSocketCpp;

Config::Config()
{
}

Config& Config::Instance()
{
    static Config instance;
    if (instance.m_initialized == false)
    {
        instance.Init();
    }

    return instance;
}

bool Config::Init()
{
    if (Load() == false)
    {
        DebugPrint() << "error while loading settings" << std::endl;
        m_initialized = false;
    }
    else
    {
        SetRootFolder();
        m_initialized = true;
    }

    return m_initialized;
}

bool Config::Load()
{
    return true;
}

std::string Config::RootFolder() const
{
    return m_rootFolder;
}

std::string Config::ToString() const
{
    return std::string("Config :") + "\n" +
           "\tname: " + m_ServerName + "\n" +
           "\tWebSocket protocol: " + Protocol2String(m_WsProtocol) + "\n" +
           "\tWebSocket port: " + std::to_string(m_WsServerPort) + "\n" +
           "\tRoot : " + m_rootFolder + "\n";
}

void Config::SetRootFolder()
{
    std::string root      = FileSystem::NormalizePath(GetRoot());
    std::string root_full = FileSystem::NormalizePath(FileSystem::GetFullPath(root));
    if (root != root_full)
    {
        m_rootFolder = FileSystem::NormalizePath(FileSystem::GetApplicationFolder()) + root;
    }
    else
    {
        m_rootFolder = root;
    }
}

void Config::OnChanged(const std::string& value)
{
    switch (_(value.c_str()))
    {
        case _("Root"):
            SetRootFolder();
            break;
    }
}
