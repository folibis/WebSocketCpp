#include "DebugPrint.h"

using namespace WebSocketCpp;

#ifdef NDEBUG
bool DebugPrint::AllowPrint = false;
#else
bool DebugPrint::AllowPrint = true;
#endif

DebugPrint::DebugPrint()
{
}
