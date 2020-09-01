#include "game/Camera.h"

#include "input/Input.h"
#include "render/Render.h"
#include "game/Serialization.h"
#include "game/Game.h"

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
        projection_ = MakeOrthographicProjection(-extent_, extent_, -extent_ / g_Render->GetAspect(), extent_ / g_Render->GetAspect(), near_, far_);
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

}