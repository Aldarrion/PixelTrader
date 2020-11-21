#include "World/Camera.h"

#include "input/Input.h"
#include "render/Render.h"
#include "Resources/Serialization.h"

namespace hs
{

//------------------------------------------------------------------------------
void Camera::Init(const PropertyContainer& data)
{
    pos_ = data.GetValue(CameraDef::POSITION).V3;
    angles_ = data.GetValue(CameraDef::ANGLES).V2;
    UpdateCameraVectors();
}

//------------------------------------------------------------------------------
void Camera::FillData(PropertyContainer& data)
{
    data.Insert({ CameraDef::POSITION, PropertyValue(PropertyType::Vec3, pos_) });
    data.Insert({ CameraDef::ANGLES, PropertyValue(PropertyType::Vec2, angles_) });
}

//------------------------------------------------------------------------------
void Camera::UpdateCameraVectors()
{
    if (angles_.x > 89.0f)
        angles_.x = 89;
    else if (angles_.x < -89.0f)
        angles_.x = -89;

    forward_.x = cos(DegToRad(angles_.y)) * cos(DegToRad(angles_.x));
    forward_.y = sin(DegToRad(angles_.x));
    forward_.z = sin(DegToRad(angles_.y)) * cos(DegToRad(angles_.x));

    right_ = Vec3::UP().Cross(forward_).Normalized();
}

//------------------------------------------------------------------------------
void Camera::UpdateMatrics()
{
    if (projectionType_ == ProjectionType::Orthographic)
    {
        projection_ = MakeOrthographicProjection(-horizontalExtent_, horizontalExtent_, -horizontalExtent_ / g_Render->GetAspect(), horizontalExtent_ / g_Render->GetAspect(), near_, far_);
    }
    else
    {
        projection_ = MakePerspectiveProjection(DegToRad(fovy_), g_Render->GetAspect(), near_, far_);
    }

    toCamera_ = MakeLookAt(pos_, pos_ + forward_);
}

//------------------------------------------------------------------------------
void Camera::Update()
{
    UpdateMatrics();
}

//------------------------------------------------------------------------------
const Mat44& Camera::ToCamera() const
{
    return toCamera_;
}

//------------------------------------------------------------------------------
const Mat44& Camera::ToProjection() const
{
    return projection_;
}

//------------------------------------------------------------------------------
const Vec3& Camera::Position() const
{
    return pos_;
}

//------------------------------------------------------------------------------
void Camera::SetPosition(const Vec3& pos)
{
    pos_ = pos;
}

//------------------------------------------------------------------------------
void Camera::SetPosition(const Vec2& pos)
{
    pos_ = Vec3{ pos.x, pos.y, pos_.z };
}

//------------------------------------------------------------------------------
Box2D Camera::GetOrthoFrustum() const
{
    const Vec2 extents = Vec2(horizontalExtent_, horizontalExtent_ / g_Render->GetAspect());
    const Box2D frustum = MakeBox2DMinMax(
        Vec2(pos_.x - extents.x, pos_.y - extents.y),
        Vec2(pos_.x + extents.x, pos_.y + extents.y)
    );
    return frustum;
}

//------------------------------------------------------------------------------
float Camera::GetHorizontalExtent() const
{
    return horizontalExtent_;
}

//------------------------------------------------------------------------------
void Camera::SetHorizontalExtent(float extent)
{
    horizontalExtent_ = extent;
}

}
