#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/Log.h>

#include "typedefs.h"
#include "logging.h"

intern Urho3D::Log * logger {nullptr};

void log_init(urho::Context* uctxt)
{
    logger = uctxt->GetSubsystem<urho::Log>();
}

void log_term()
{
    logger = nullptr;
}

void log_set_level(int level)
{
    if (logger)
        logger->SetLevel(level);
}
