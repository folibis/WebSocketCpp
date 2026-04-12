#include "Config.h"

#include "DebugPrint.h"
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
        m_initialized = true;
    }

    return m_initialized;
}

bool Config::Load()
{
    return true;
}

std::string Config::ToString() const
{
    return std::string("Config :") + "\n" +
           "\tname: " + m_ServerName + "\n" +
           "\tprotocol: " + Protocol2String(m_WsProtocol) + "\n" +
           "\tport: " + std::to_string(m_WsServerPort) + "\n";
}

void Config::OnChange(OnChangeCallback callback)
{
    m_change_callback = std::move(callback);
}

void Config::Changed(const std::string& value)
{
    if(m_change_callback)
    {
        m_change_callback(value);
    }
}
