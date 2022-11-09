#pragma once

#include <Urho3D/IO/Log.h>
#include "typedefs.h"

#define tlog URHO3D_LOGTRACEF
#define dlog URHO3D_LOGDEBUGF
#define ilog URHO3D_LOGINFOF
#define wlog URHO3D_LOGWARNINGF
#define elog URHO3D_LOGERRORF
#define rlog URHO3D_LOGRAWF

namespace Urho3D
{
class Context;
}

void log_init(urho::Context* uctxt);
void log_term();
void log_set_level(int level);
