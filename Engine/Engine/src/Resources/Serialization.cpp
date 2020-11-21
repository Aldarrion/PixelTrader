#include "Resources/Serialization.h"

#include "common/Logging.h"

#include "cjson/cJSON.h"

#include "platform/hs_Windows.h"

namespace hs
{

#define LOG_AND_FAIL(msg, ...)      do { Log(LogLevel::Error, msg, __VA_ARGS__);\
                                    return R_FAIL; } while(false)

//------------------------------------------------------------------------------
RESULT SerializationManager::FillObject(cJSON* json, PropertyContainer& container)
{
    const cJSON* defJson = cJSON_GetObjectItemCaseSensitive(json, "Def");
    if (!cJSON_IsString(defJson))
        LOG_AND_FAIL("JSON: Invalid Def");

    const cJSON* versionJson = cJSON_GetObjectItemCaseSensitive(json, "Version");
    if (!cJSON_IsNumber(versionJson))
        LOG_AND_FAIL("JSON: Invalid Version");

    auto def = defs_.find(defJson->valuestring);
    if (def == defs_.end())
        LOG_AND_FAIL("JSON: Def %s not found", defJson->valuestring);

    uint version = static_cast<uint>(versionJson->valueint);
    ContainerDef containerDef = def->second->GetDef(version);

    for (int i = 0; i < containerDef.props_.Count(); ++i)
    {
        const PropertyDefinition& propDef = containerDef.props_[i];
        const cJSON* propJson = cJSON_GetObjectItemCaseSensitive(json, propDef.name_);
        
        PropertyContainer::PropertyPair prop;
        prop.Idx = i;
        prop.Value.Type = propDef.type_;
        
        switch (propDef.type_) 
        {
            case PropertyType::Int:
            {
                if (!cJSON_IsNumber(propJson))
                    LOG_AND_FAIL("JSON: Invalid value of property %s", propDef.name_);
                prop.Value.I = propJson->valueint;
                break;
            }
            case PropertyType::Float:
            {
                if (!cJSON_IsNumber(propJson))
                    LOG_AND_FAIL("JSON: Invalid value of property %s", propDef.name_);
                prop.Value.F = (float)propJson->valuedouble;
                break;
            }
            case PropertyType::Vec2:
            {
                if (!cJSON_IsArray(propJson))
                    LOG_AND_FAIL("JSON: Invalid value of property %s", propDef.name_);

                cJSON* element;
                int elementIdx = 0;
                cJSON_ArrayForEach(element, propJson)
                {
                    if (elementIdx > 1)
                        LOG_AND_FAIL("JSON: Invalid value of property %s", propDef.name_);
                    prop.Value.V2.v[elementIdx++] = (float)element->valuedouble;
                }

                if (elementIdx != 2)
                    LOG_AND_FAIL("JSON: Invalid value of property %s", propDef.name_);

                break;
            }
            case PropertyType::Vec3:
            {
                if (!cJSON_IsArray(propJson))
                    LOG_AND_FAIL("JSON: Invalid value of property %s", propDef.name_);

                cJSON* element;
                int elementIdx = 0;
                cJSON_ArrayForEach(element, propJson)
                {
                    if (elementIdx > 2)
                        LOG_AND_FAIL("JSON: Invalid value of property %s", propDef.name_);
                    prop.Value.V2.v[elementIdx++] = (float)element->valuedouble;
                }

                if (elementIdx != 3)
                    LOG_AND_FAIL("JSON: Invalid value of property %s", propDef.name_);

                break;
            }
            case PropertyType::String:
            {
                if (!cJSON_IsString(propJson))
                    LOG_AND_FAIL("JSON: Invalid value of property %s", propDef.name_);

                size_t len = strlen(propJson->valuestring);
                prop.Value.Str = (char*)malloc(len);
                strcpy(prop.Value.Str, propJson->valuestring);

                break;
            }
            default:
            {
                LOG_AND_FAIL("Unknown property type");
            }
        }
        container.Insert(prop);
    }

    // Always ensure we use the latest version
    const uint latestVersion = def->second->GetLatestVersion();
    if (version == latestVersion)
        return R_OK;

    while(true)
    {
        def->second->Upgrade(containerDef, container, version);
        if (version == latestVersion)
            break;
        containerDef = def->second->GetDef(version);
    }

    container.Def = def->second;

    return R_OK;
}

//------------------------------------------------------------------------------
RESULT SerializationManager::LoadConfig(const char* fileName, PropertyContainer& container)
{
    FILE* f = fopen(fileName, "r");
    if (!f)
        LOG_AND_FAIL("Failed to open config %s", fileName);

    fseek(f , 0 , SEEK_END);
    auto size = ftell(f);
    rewind(f);

    char* buffer = (char*)malloc(size);
    if (!buffer)
    {
        free(buffer);
        fclose(f);
        LOG_AND_FAIL("Failed to alloc space for config data");
    }

    auto readRes = fread(buffer, 1, size, f);
    auto eof = feof(f);
    if (readRes != size && !eof)
    {
        free(buffer);
        fclose(f);
        LOG_AND_FAIL("Failed to read config file %s, error %d", fileName, ferror(f));
    }
    fclose(f);

    // Json parsing
    cJSON* root = cJSON_Parse(buffer);
    free(buffer);
    if (!root)
    {
        const char* jsonError = cJSON_GetErrorPtr();
        LOG_AND_FAIL("Failed to parse config file %s, error %s", fileName, jsonError);
    }

    // Json parsed, fill the container
    if (FAILED(FillObject(root, container)))
    {
        cJSON_Delete(root);
        LOG_AND_FAIL("Failed to decode config file %s", fileName);
    }

    cJSON_Delete(root);
    return R_OK;
}

//------------------------------------------------------------------------------
RESULT SerializationManager::SaveConfig(const char* fileName, const PropertyContainer& container)
{
    FILE* f = fopen(fileName, "w");
    if (!f)
        LOG_AND_FAIL("Failed to open config %s for writing", fileName);

    cJSON* root = cJSON_CreateObject();

    //-----------------------------
    // Serialize object starts here
    const ContainerDef& containerDef = container.Def->GetLatestDef();
    
    cJSON_AddStringToObject(root, "Def", containerDef.name_);
    cJSON_AddNumberToObject(root, "Version", container.Def->GetLatestVersion());
    
    for (int i = 0; i < containerDef.props_.Count(); ++i)
    {
        PropertyValue prop = container.GetValue(i);
        const char* name = containerDef.props_[i].name_;

        switch (prop.Type)
        {
            case PropertyType::Int:
            {
                cJSON_AddNumberToObject(root, name, prop.I);
                break;
            }
            case PropertyType::Float:
            {
                cJSON_AddNumberToObject(root, name, prop.F);
                break;
            }
            case PropertyType::Vec2:
            {
                cJSON* arr = cJSON_AddArrayToObject(root, name);
                
                cJSON_AddItemToArray(arr, cJSON_CreateNumber(prop.V2.x));
                cJSON_AddItemToArray(arr, cJSON_CreateNumber(prop.V2.y));
                break;
            }
            case PropertyType::Vec3:
            {
                cJSON* arr = cJSON_AddArrayToObject(root, name);
                
                cJSON_AddItemToArray(arr, cJSON_CreateNumber(prop.V3.x));
                cJSON_AddItemToArray(arr, cJSON_CreateNumber(prop.V3.y));
                cJSON_AddItemToArray(arr, cJSON_CreateNumber(prop.V3.z));
                break;
            }
            case PropertyType::String:
            {
                cJSON_AddStringToObject(root, name, prop.Str);
                break;
            }
        }
    }

    const char* serialized = cJSON_Print(root);
    if (!serialized)
    {
        fclose(f);
        cJSON_Delete(root);
        LOG_AND_FAIL("Failed to print JSON, error: %s", cJSON_GetErrorPtr());
    }

    cJSON_Delete(root);

    int writeResult = fputs(serialized, f);
    if (writeResult == EOF)
    {
        fclose(f);
        delete serialized;
        LOG_AND_FAIL("Failed to write config to file, error: %d", ferror(f));
    }

    fclose(f);
    delete serialized;
    return R_OK;
}

//------------------------------------------------------------------------------
RESULT SerializationManager::Init()
{
    defs_.emplace("CameraDef", new CameraDef());

    for (const auto& def : defs_)
        def.second->Init();

    return R_OK;
}

//------------------------------------------------------------------------------
const DefBase* SerializationManager::GetDef(const char* name) const
{
    auto def = defs_.find(name);
    hs_assert(def != defs_.end());
    return def->second;
}

#undef LOG_AND_FAIL

}

