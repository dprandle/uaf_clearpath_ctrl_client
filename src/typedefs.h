#pragma once

#ifdef DEBUG_VERSION
#include <cassert>
#define ASSERT assert
#else
#define ASSERT
#endif

#define intern static

namespace Urho3D {}
namespace urho = Urho3D;