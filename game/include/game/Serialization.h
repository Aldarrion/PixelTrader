#pragma once

#include "Config.h"

#include "containers/Array.h"
#include "containers/Hash.h"

#include "math/hs_Math.h"

#include "common/Enums.h"

#include <cstdlib>

#include <unordered_map>

struct cJSON;

namespace hs
{

//------------------------------------------------------------------------------
enum class PropertyType
{
    Int,
    Float,
    Vec2,
    Vec3,
    String,
};

//------------------------------------------------------------------------------
struct PropertyValue
{
    PropertyType Type;
    union
    {
        int I;
        float F;
        Vec2 V2;
        Vec3 V3;
        char* Str;
    };

    PropertyValue() = default;

    PropertyValue(PropertyType type, int i)
        : Type(type), I(i)
    {
    }

    PropertyValue(PropertyType type, float f)
        : Type(type), F(f)
    {
    }

    PropertyValue(PropertyType type, Vec2 v2)
        : Type(type), V2(v2)
    {
    }

    PropertyValue(PropertyType type, const Vec3& v3)
        : Type(type), V3(v3)
    {
    }

    PropertyValue(PropertyType type, char* s)
        : Type(type), Str(s)
    {
    }
};

//------------------------------------------------------------------------------
struct PropertyDefinition
{
    PropertyType type_;
    const char* name_;

    PropertyDefinition(PropertyType type, const char* name)
        : type_(type)
        , name_(name)
    {
    }
};

//------------------------------------------------------------------------------
struct ContainerDef
{
    const char* name_;
    Array<PropertyDefinition> props_;

    uint GetIdx(const char* prop) const
    {
        for (uint i = 0; i < props_.Count(); ++i)
        {
            if (strcmp(props_[i].name_, prop) == 0)
            {
                return i;
            }
        }

        return (uint)-1;
    }
};

struct DefBase;
//------------------------------------------------------------------------------
struct PropertyContainer
{
    struct PropertyPair
    {
        uint Idx;
        PropertyValue Value;

        PropertyPair() = default;

        ~PropertyPair()
        {
            if (Value.Type == PropertyType::String)
                free(Value.Str);
        }
    };

    Array<PropertyPair> Properties;
    const DefBase* Def;

    //------------------------------------------------------------------------------
    const PropertyValue& GetValue(uint idx) const
    {
        for (uint i = 0; i < Properties.Count(); ++i)
        {
            if (Properties[i].Idx == idx)
            {
                return Properties[i].Value;
            }
        }

        hs_assert(!"Property at given index not found");
        static auto empty = PropertyValue{};
        return empty;
    };

    //------------------------------------------------------------------------------
    void Remove(uint idx)
    {
        for (uint i = 0; i < Properties.Count(); ++i)
        {
            if (Properties[i].Idx == idx)
            {
                Properties.Remove(i);
                return;
            }
        }
    }

    //------------------------------------------------------------------------------
    void Insert(const PropertyPair& property)
    {
        uint i = 0;
        for (; i < Properties.Count(); ++i)
        {
            if (Properties[i].Idx > property.Idx)
            {
                break;
            }
        }

        Properties.Insert(i, property);
    }
};

//------------------------------------------------------------------------------
struct DefBase
{
    ContainerDef LatestDef;

    void Init()
    {
        LatestDef = GetDef(GetLatestVersion());
    }

    const ContainerDef& GetLatestDef() const
    {
        return LatestDef;
    }

    virtual ContainerDef GetDef(uint version) const = 0;
    virtual void Upgrade(const ContainerDef& def, PropertyContainer& container, uint& version) const = 0;
    virtual uint GetLatestVersion() const = 0;
};

//------------------------------------------------------------------------------
struct CameraDef : DefBase
{
    //------------------------------------------------------------------------------
    static constexpr const char* NAME = "CameraDef";

    //------------------------------------------------------------------------------
    uint GetLatestVersion() const override
    {
        return 1;
    }

    //------------------------------------------------------------------------------
    ContainerDef GetDef(uint version) const override
    {
        ContainerDef def;
        switch (version)
        {
            case 0:
            {
                def.name_ = NAME;
                def.props_.Add(PropertyDefinition{ PropertyType::Vec3, "Position" });
                def.props_.Add(PropertyDefinition{ PropertyType::Float, "Pitch" });
                def.props_.Add(PropertyDefinition{ PropertyType::Float, "Yaw" });
                break;
            }
            case 1:
            {
                def.name_ = NAME;
                def.props_.Add(PropertyDefinition{ PropertyType::Vec3, "Position" });
                def.props_.Add(PropertyDefinition{ PropertyType::Vec2, "Angles" });
                break;
            }
            default:
            {
                hs_assert(false);
                break;
            }
        }

        return def;
    }

    //------------------------------------------------------------------------------
    void Upgrade(const ContainerDef& def, PropertyContainer& container, uint& version) const override
    {
        switch(version)
        {
            case 0:
            {
                const uint yawIdx = def.GetIdx("Yaw");
                const uint pitchIdx = def.GetIdx("Pitch");

                const float yaw = container.GetValue(yawIdx).F;
                const float pitch = container.GetValue(pitchIdx).F;

                container.Remove(yawIdx);
                container.Remove(pitchIdx);

                PropertyContainer::PropertyPair newProp;
                newProp.Idx = 1;
                newProp.Value.V2 = Vec2{ pitch, yaw };
                newProp.Value.Type = PropertyType::Vec2;

                container.Insert(newProp);

                version++;
                return;
            }
            default:
            {
                version = GetLatestVersion();
                return;
            }
        }
    }

    static constexpr uint POSITION = 0;
    static constexpr uint ANGLES = 1;
};




//------------------------------------------------------------------------------
class SerializationManager
{
public:
    RESULT Init();
    RESULT LoadConfig(const char* fileName, PropertyContainer& container);
    RESULT SaveConfig(const char* fileName, const PropertyContainer& container);
    const DefBase* GetDef(const char* name) const;

private:
    std::unordered_map<const char*, DefBase*, StrHash<const char*>, StrCmpEq<const char*>> defs_;

    RESULT FillObject(cJSON* json, PropertyContainer& container);
};

////------------------------------------------------------------------------------
//class Camera
//{
//    Vec3 Position;
//    Vec2 Angles;
//
//    //------------------------------------------------------------------------------
//    void Init(const PropertyContainer& props)
//    {
//        Position = props.GetValue(CameraDef::POSITION).V3;
//        Angles = props.GetValue(CameraDef::ANGLES).V2;
//    }
//};

}
