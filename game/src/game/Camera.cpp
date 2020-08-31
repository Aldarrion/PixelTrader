#include "Camera.h"

#include "Input.h"
#include "Render.h"
#include "Serialization.h"

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
void Camera::Update()
{
    bool isMoveMode = g_Input->GetState(VK_RBUTTON);
    if (isMoveMode)
    {
        g_Input->SetMouseMode(MouseMode::Relative);
        Vec2 mouseDelta = g_Input->GetMouseDelta();
        angles_.x -= mouseDelta.y * 0.1f;
        angles_.y -= mouseDelta.x * 0.1f;
        
        if (mouseDelta != Vec2{})
        {
            UpdateCameraVectors();
        }
    }
    else
    {
        g_Input->SetMouseMode(MouseMode::Absolute);
    }

    if (isMoveMode)
    {
        if (g_Input->GetState('W'))
        {
            pos_ += forward_ * speed_ * g_Render->GetDTime();
        }
        else if (g_Input->GetState('S'))
        {
            pos_ -= forward_ * speed_ * g_Render->GetDTime();
        }
    
        if (g_Input->GetState('D'))
        {
            pos_ += right_ * speed_ * g_Render->GetDTime();
        }
        else if (g_Input->GetState('A'))
        {
            pos_ -= right_ * speed_ * g_Render->GetDTime();
        }

        if (g_Input->GetState('Q'))
        {
            pos_ += Vec3::UP() * speed_ * g_Render->GetDTime();
        }
        else if (g_Input->GetState('E'))
        {
            pos_ -= Vec3::UP() * speed_ * g_Render->GetDTime();
        }
    }

    float extent = 20;
    //projection_ = MakeOrthographicProjection(-extent, extent, -extent / g_Render->GetAspect(), extent / g_Render->GetAspect(), 0.1f, 1000);
    projection_ = MakePerspectiveProjection(DegToRad(fovy_), g_Render->GetAspect(), near_, far_);

    toCamera_ = MakeLookAt(pos_, pos_ + forward_);
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

}