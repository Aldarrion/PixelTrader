#pragma once

#include "common/Types.h"
#include "common/hs_Assert.h"

#include <math.h>

namespace hs
{

static constexpr float VKR_PI = 3.14159265359f;
static constexpr float VKR_TAU = 2 * VKR_PI;

//------------------------------------------------------------------------------
inline constexpr uint Max(uint a, uint b)
{
    return a > b ? a : b;
}

//------------------------------------------------------------------------------
inline constexpr uint Align(uint x, uint align)
{
    hs_assert(align);
    return ((x + align - 1) / align) * align;
}

//------------------------------------------------------------------------------
inline constexpr float DegToRad(float deg)
{
    return (deg * VKR_PI) / 180.0f;
}

//------------------------------------------------------------------------------
inline constexpr float RadToDeg(float rad)
{
    return (rad * 180.0f) / VKR_PI;
}

//------------------------------------------------------------------------------
struct Vec4
{
    union
    {
        float v[4];
        struct
        {
            float x, y, z, w;
        };
    };
};

//------------------------------------------------------------------------------
struct Vec3
{
    union
    {
        float v[3];
        struct
        {
            float x, y, z;
        };
    };

    //------------------------------------------------------------------------------
    static constexpr Vec3 UP()
    {
        return Vec3{ 0, 1, 0 };
    }

    //------------------------------------------------------------------------------
    static constexpr Vec3 FORWARD()
    {
        return Vec3{ 0, 0, 1 };
    }

    //------------------------------------------------------------------------------
    static constexpr Vec3 RIGHT()
    {
        return Vec3{ 1, 0, 0 };
    }

    //------------------------------------------------------------------------------
    constexpr float Dot(const Vec3& b) const
    {
        return
            x * b.x
            + y * b.y
            + z * b.z;
    }

    //------------------------------------------------------------------------------
    constexpr Vec3 Cross(const Vec3& b) const
    {
        return Vec3{
            y * b.z - z * b.y,
            z * b.x - x * b.z,
            x * b.y - y * b.x
        };
    }

    //------------------------------------------------------------------------------
    constexpr float LengthSqr() const
    {
        return Dot(*this);
    }

    //------------------------------------------------------------------------------
    float Length() const
    {
        return sqrt(LengthSqr());
    }

    //------------------------------------------------------------------------------
    void Normalize()
    {
        float lenRec = 1.0f / Length();
        x *= lenRec;
        y *= lenRec;
        z *= lenRec;
    }

    //------------------------------------------------------------------------------
    Vec3 Normalized() const
    {
        float lenRec = 1.0f / Length();
        return Vec3{ x * lenRec, y * lenRec, z * lenRec };
    }

    //------------------------------------------------------------------------------
    Vec3& operator+=(const Vec3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    //------------------------------------------------------------------------------
    Vec3& operator-=(const Vec3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    //------------------------------------------------------------------------------
    Vec3& operator*=(float t)
    {
        x *= t;
        y *= t;
        z *= t;
        return *this;
    }

    //------------------------------------------------------------------------------
    Vec4 ToVec4() const
    {
        return Vec4{ x, y, z, 1 };
    }
};

//------------------------------------------------------------------------------
inline Vec3 operator+(const Vec3& a, const Vec3& b)
{
    return Vec3{ a.x + b.x, a.y + b.y, a.z + b.z };
}

//------------------------------------------------------------------------------
inline Vec3 operator-(const Vec3& a)
{
    return Vec3{ -a.x, -a.y, -a.z };
}

//------------------------------------------------------------------------------
inline Vec3 operator-(const Vec3& a, const Vec3& b)
{
    return a + (-b);
}

//------------------------------------------------------------------------------
inline Vec3 operator*(const Vec3& v, float t)
{
    return Vec3{ v.x * t, v.y * t, v.z * t };
}

//------------------------------------------------------------------------------
inline Vec3 operator*(float t, const Vec3& v)
{
    return v * t;
}

//------------------------------------------------------------------------------
inline Vec3 operator/(const Vec3& v, float t)
{
    return v * (1 / t);
}

//------------------------------------------------------------------------------
// Vector 2
//------------------------------------------------------------------------------
struct Vec2
{
    union
    {
        float v[2];
        struct
        {
            float x, y;
        };
    };

    //------------------------------------------------------------------------------
    Vec2& operator+=(Vec2 other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    //------------------------------------------------------------------------------
    Vec2& operator-=(Vec2 other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    //------------------------------------------------------------------------------
    Vec2& operator*=(float t)
    {
        x *= t;
        y *= t;
        return *this;
    }

    //------------------------------------------------------------------------------
    Vec2& operator/=(float t)
    {
        float r = (1 / t);
        return operator*=(r);
    }
};

//------------------------------------------------------------------------------
inline Vec2 operator*(float f, Vec2 v)
{
    return Vec2{ v.x * f, v.y * f };
}

//------------------------------------------------------------------------------
inline Vec2 operator*(Vec2 v, float f)
{
    return f * v;
}

//------------------------------------------------------------------------------
inline Vec2 operator/(Vec2 v, float f)
{
    return (1 / f) * v;
}

//------------------------------------------------------------------------------
inline Vec2 operator+(Vec2 a, Vec2 b)
{
    return Vec2{ a.x + b.x, a.y + b.y };
}

//------------------------------------------------------------------------------
inline Vec2 operator-(Vec2 a)
{
    return Vec2{ -a.x , -a.y };
}

//------------------------------------------------------------------------------
inline Vec2 operator-(Vec2 a, Vec2 b)
{
    return a + (-b);
}

//------------------------------------------------------------------------------
inline bool operator==(Vec2 a, Vec2 b)
{
    return a.x == b.x && a.y == b.y;
}

//------------------------------------------------------------------------------
inline bool operator!=(Vec2 a, Vec2 b)
{
    return !(a == b);
}

//------------------------------------------------------------------------------
// Matrix is saved as row major, meaning that the vector at m[0] is the first
// row of the matrix, m[1] is the second row etc.
// The first column is given by Vec4(m[0][0], m[1][0], m[2][0], m[3][0])
// This means that the internal indexing the same as the mathematical
// convention where mat[i][j] gives ith row and jth column. For that we have
// operator().
//------------------------------------------------------------------------------
struct Mat44
{
    static Mat44 Identity()
    {
        return Mat44
        (
            Vec4{ 1, 0, 0, 0 },
            Vec4{ 0, 1, 0, 0 },
            Vec4{ 0, 0, 1, 0 },
            Vec4{ 0, 0, 0, 1 }
        );
    }

    union
    {
        float m[4][4];
        struct
        {
            Vec4 a;
            Vec4 b;
            Vec4 c;
            Vec4 pos;
        };
    };

    //------------------------------------------------------------------------------
    Mat44() = default;
    //------------------------------------------------------------------------------
    Mat44(
        float _11, float _12, float _13, float _14,
        float _21, float _22, float _23, float _24,
        float _31, float _32, float _33, float _34,
        float _41, float _42, float _43, float _44
    )
    {
        m[0][0] = _11; m[0][1] = _12; m[0][2] = _13; m[0][3] = _14;
        m[1][0] = _21; m[1][1] = _22; m[1][2] = _23; m[1][3] = _24;
        m[2][0] = _31; m[2][1] = _32; m[2][2] = _33; m[2][3] = _34;
        m[3][0] = _41; m[3][1] = _42; m[3][2] = _43; m[3][3] = _44;
    }

    //------------------------------------------------------------------------------
    Mat44(const Vec4& a, const Vec4& b, const Vec4& c, const Vec4& d)
    {
        m[0][0] = a.x; m[0][1] = a.y; m[0][2] = a.z; m[0][3] = a.w;
        m[1][0] = b.x; m[1][1] = b.y; m[1][2] = b.z; m[1][3] = b.w;
        m[2][0] = c.x; m[2][1] = c.y; m[2][2] = c.z; m[2][3] = c.w;
        m[3][0] = d.x; m[3][1] = d.y; m[3][2] = d.z; m[3][3] = d.w;
    }

    //------------------------------------------------------------------------------
    const Vec4& GetPosition() const
    {
        return *reinterpret_cast<const Vec4*>(m[3]);
    }

    //------------------------------------------------------------------------------
    constexpr float operator()(int i, int j) const
    {
        return m[i][j];
    }

    //------------------------------------------------------------------------------
    constexpr float& operator()(int i, int j)
    {
        return m[i][j];
    }

    //------------------------------------------------------------------------------
    void SetPosition(const Vec3& position)
    {
        pos = Vec4{ position.x, position.y, position.z, 1 };
    }
};

//------------------------------------------------------------------------------
inline Mat44 operator*(const Mat44& a, const Mat44& b)
{
    Mat44 out;

    out(0, 0) = a(0, 0) * b(0, 0) + a(0, 1) * b(1, 0) + a(0, 2) * b(2, 0) + a(0, 3) * b(3, 0);
    out(1, 0) = a(1, 0) * b(0, 0) + a(1, 1) * b(1, 0) + a(1, 2) * b(2, 0) + a(1, 3) * b(3, 0);
    out(2, 0) = a(2, 0) * b(0, 0) + a(2, 1) * b(1, 0) + a(2, 2) * b(2, 0) + a(2, 3) * b(3, 0);
    out(3, 0) = a(3, 0) * b(0, 0) + a(3, 1) * b(1, 0) + a(3, 2) * b(2, 0) + a(3, 3) * b(3, 0);

    out(0, 1) = a(0, 0) * b(0, 1) + a(0, 1) * b(1, 1) + a(0, 2) * b(2, 1) + a(0, 3) * b(3, 1);
    out(1, 1) = a(1, 0) * b(0, 1) + a(1, 1) * b(1, 1) + a(1, 2) * b(2, 1) + a(1, 3) * b(3, 1);
    out(2, 1) = a(2, 0) * b(0, 1) + a(2, 1) * b(1, 1) + a(2, 2) * b(2, 1) + a(2, 3) * b(3, 1);
    out(3, 1) = a(3, 0) * b(0, 1) + a(3, 1) * b(1, 1) + a(3, 2) * b(2, 1) + a(3, 3) * b(3, 1);

    out(0, 2) = a(0, 0) * b(0, 2) + a(0, 1) * b(1, 2) + a(0, 2) * b(2, 2) + a(0, 3) * b(3, 2);
    out(1, 2) = a(1, 0) * b(0, 2) + a(1, 1) * b(1, 2) + a(1, 2) * b(2, 2) + a(1, 3) * b(3, 2);
    out(2, 2) = a(2, 0) * b(0, 2) + a(2, 1) * b(1, 2) + a(2, 2) * b(2, 2) + a(2, 3) * b(3, 2);
    out(3, 2) = a(3, 0) * b(0, 2) + a(3, 1) * b(1, 2) + a(3, 2) * b(2, 2) + a(3, 3) * b(3, 2);

    out(0, 3) = a(0, 0) * b(0, 3) + a(0, 1) * b(1, 3) + a(0, 2) * b(2, 3) + a(0, 3) * b(3, 3);
    out(1, 3) = a(1, 0) * b(0, 3) + a(1, 1) * b(1, 3) + a(1, 2) * b(2, 3) + a(1, 3) * b(3, 3);
    out(2, 3) = a(2, 0) * b(0, 3) + a(2, 1) * b(1, 3) + a(2, 2) * b(2, 3) + a(2, 3) * b(3, 3);
    out(3, 3) = a(3, 0) * b(0, 3) + a(3, 1) * b(1, 3) + a(3, 2) * b(2, 3) + a(3, 3) * b(3, 3);

    return out;
}

//------------------------------------------------------------------------------
inline Vec4 operator*(const Vec4& v, const Mat44& m)
{
    Vec4 out;

    out.x = v.x * m(0, 0) + v.y * m(1, 0) + v.z * m(2, 0) + v.w * m(3, 0);
    out.y = v.x * m(0, 1) + v.y * m(1, 1) + v.z * m(2, 1) + v.w * m(3, 1);
    out.z = v.x * m(0, 2) + v.y * m(1, 2) + v.z * m(2, 2) + v.w * m(3, 2);
    out.w = v.x * m(0, 3) + v.y * m(1, 3) + v.z * m(2, 3) + v.w * m(3, 3);

    return out;
}

//------------------------------------------------------------------------------
// Multiply vector by matrix from the left, useful for column major matrix
//inline Vec4 operator*(const Mat44& m, const Vec4& v)
//{
//    Vec4 out;
//
//    out.x = v.x * m(0, 0) + v.y * m(0, 1) + v.z * m(0, 2) + v.w * m(0, 3);
//    out.y = v.x * m(1, 0) + v.y * m(1, 1) + v.z * m(1, 2) + v.w * m(1, 3);
//    out.z = v.x * m(2, 0) + v.y * m(2, 1) + v.z * m(2, 2) + v.w * m(2, 3);
//    out.w = v.x * m(3, 0) + v.y * m(3, 1) + v.z * m(3, 2) + v.w * m(3, 3);
//
//    return out;
//}

//------------------------------------------------------------------------------
inline Mat44 MakeOrthographicProjection(float l, float r, float t, float b, float n, float f)
{
    return Mat44(
        2 / (r - l),            0,                    0,                0,
        0,                      2 / (b - t),          0,                0,
        0,                      0,                    1 / (f - n),      0,
        -(r + l) / (r - l),     -(b + t) / (b - t),   -n / (f - n),     1
    );
}

//------------------------------------------------------------------------------
//! s = aspect ratio (width / height)
inline Mat44 MakePerspectiveProjection(float fovy, float s, float n, float f)
{
    const float g = 1.0f / tan(fovy * 0.5f);
    const float k = f / (f - n);
    return Mat44(
        g / s,  0,      0,      0,
        0,      g,      0,      0,
        0,      0,      k,      1,
        0,      0,      -n * k, 0
    );
}

//------------------------------------------------------------------------------
inline Mat44 MakeLookAt(const Vec3& pos, const Vec3& target)
{
    const Vec3 forward = (target - pos).Normalized();
    const Vec3 right = Vec3::UP().Cross(forward).Normalized();
    // No need to normalize, forward and right are perpendicular to each other
    const Vec3 up = forward.Cross(right);

    // Matrix transforms from world space to camera space. Therefore, we return
    // the inverse of the camera matrix - negate the translation and multiply by
    // transposition of the 3x3 rotation matrix.
    // We first need to translate the vector to -pos and then rotate it, which 
    // would be done by two matrices, here we have the result of multiplication of
    // those two matrices
    return Mat44(
        Vec4{ right.x, up.x, forward.x, 0 },
        Vec4{ right.y, up.y, forward.y, 0 },
        Vec4{ right.z, up.z, forward.z, 0 },
        Vec4{ right.Dot(-pos), up.Dot(-pos), forward.Dot(-pos), 1 }
    );
}

//------------------------------------------------------------------------------
inline float ToLinear(float srgb)
{
    if (srgb <= 0.04045)
        return srgb / 12.92;
    else
        return pow((srgb + 0.055) / 1.055, 2.4);
}

//------------------------------------------------------------------------------
inline float ToSrgb(float linear)
{
    if (linear <= 0.0031308)
        return linear * 12.92;
    else
        return 1.055 * pow(linear, 1.0 / 2.4) - 0.055;
}

//------------------------------------------------------------------------------
struct Color
{
public:
    union
    {
        float c[4];
        struct
        {
            float r, g, b, a;
        };
    };

    static Color ToLinear(const Color& srgb)
    {
        return Color{ ::hs::ToLinear(srgb.r), ::hs::ToLinear(srgb.g), ::hs::ToLinear(srgb.b), srgb.a };
    }

    static Color ToSrgb(const Color& linear)
    {
        return Color{ ::hs::ToSrgb(linear.r), ::hs::ToSrgb(linear.g), ::hs::ToSrgb(linear.b), linear.a };
    }
};

}
