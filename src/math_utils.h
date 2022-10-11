#pragma once
//#include <cstdint>

#include "typedefs.h"

#include <Urho3D/Math/Ray.h>
#include <Urho3D/Math/Rect.h>
#include <Urho3D/Math/Color.h>
#include <Urho3D/Math/Plane.h>
#include <Urho3D/Math/Random.h>
#include <Urho3D/Math/Sphere.h>
#include <Urho3D/Math/Frustum.h>
#include <Urho3D/Math/Matrix2.h>
#include <Urho3D/Math/Matrix3.h>
#include <Urho3D/Math/Matrix4.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Math/Vector4.h>
#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Math/Matrix3x4.h>
#include <Urho3D/Math/Polyhedron.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/StringHash.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Math/AreaAllocator.h>
#include <Urho3D/Container/Vector.h>
#include <Urho3D/Container/HashMap.h>
#include <Urho3D/Container/HashSet.h>
#include <Urho3D/Container/List.h>
#include <Urho3D/Container/LinkedList.h>

#define PI 3.14159265359f

using urho::AreaAllocator;
using urho::Color;
using urho::Frustum;
using urho::FrustumPlane;
using urho::HashMap;
using urho::HashSet;
using urho::LinkedList;
using urho::List;
using urho::Plane;
using urho::Polyhedron;
using urho::Sphere;
using urho::String;
using urho::StringHash;
using urho::Vector;

using ray = urho::Ray;
using rect = urho::Rect;
using irect = urho::IntRect;
using mat2 = urho::Matrix2;
using mat3 = urho::Matrix3;
using mat4 = urho::Matrix4;
using mat3x4 = urho::Matrix3x4;
using vec2 = urho::Vector2;
using vec3 = urho::Vector3;
using vec4 = urho::Vector4;
using ivec2 = urho::IntVector2;
using ivec3 = urho::IntVector3;
using bbox = urho::BoundingBox;
using quat = urho::Quaternion;

template<class T>
T degrees(const T & val_)
{
    return (180 / PI) * val_;
}

template<class T>
T radians(const T & val_)
{
    return (PI / 180) * val_;
}

mat4 perspective_from(float fov_, float aspect_ratio_, float z_near_, float z_far_);

mat4 perspective_from(float near_left,
                       float near_right,
                       float near_top,
                       float near_bottom,
                       float z_near,
                       float z_far);

mat4 ortho_from(float left_, float right_, float top_, float bottom_, float near_, float far_);

vec2 normalize_coords(const vec2 &coord, const ivec2 &screen_size);
