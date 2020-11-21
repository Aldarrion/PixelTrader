#pragma once

#include "Config.h"

#include "containers/Array.h"

#include "common/Types.h"
#include "common/Util.h"

#include <algorithm> // For std::sort

namespace hs
{

using Entity_t = uint;

template<class T>
struct TypeInfo;

static constexpr uint ID_BAD{ (uint)-1 };

namespace internal
{
//------------------------------------------------------------------------------
template<class TWithoutCvRef>
struct TypeInfoHelper
{
    inline static uint typeId_{ ID_BAD };
};
}

using TypeCtor_t = void (*)(void* dst);
using TypeDtor_t = void (*)(void* dst);
using TypeCopyCtor_t = void (*)(void* dst, const void* src);
using TypeMoveCtor_t = void (*)(void* dst, void* src);

//------------------------------------------------------------------------------
struct TypeDetails
{
    TypeCtor_t ctor_;
    TypeDtor_t dtor_;
    TypeCopyCtor_t copyCtor_;
    TypeMoveCtor_t moveCtor_;
    uint alignment_;
    uint size_;
    bool isTrivial_;
};

//------------------------------------------------------------------------------
struct TypeInfoDb
{
    template<class T>
    friend struct TypeInfo;

    //------------------------------------------------------------------------------
    static const TypeDetails* GetDetails(uint type)
    {
        return details_[type];
    }

private:
    inline static uint lastTypeId_{ 0 };
    inline static Array<const TypeDetails*> details_;
};

//------------------------------------------------------------------------------
template<class T>
void TypeCtor(void* dst)
{
    new (dst) T;
}

//------------------------------------------------------------------------------
template<class T>
void TypeDtor(void* dst)
{
    static_cast<T*>(dst)->~T();
}

//------------------------------------------------------------------------------
template<class T>
void TypeCopyCtor(void* dst, const void* src)
{
    new (dst) T(*static_cast<const T*>(src));
}

//------------------------------------------------------------------------------
template<class T>
void TypeMoveCtor(void* dst, void* src)
{
    new (dst) T(std::move(*static_cast<T*>(src)));
}

//------------------------------------------------------------------------------
template<class T>
struct TypeInfo
{
    static_assert(std::is_standard_layout_v<T>);

    //------------------------------------------------------------------------------
    static uint TypeId()
    {
        const auto id = internal::TypeInfoHelper<RemoveCvRef_t<T>>::typeId_;
        hs_assert(id != ID_BAD && "Type is not registered by TypeInfo<T>::InitTypeId()");
        return id;
    }

    //------------------------------------------------------------------------------
    static void InitTypeId()
    {
        internal::TypeInfoHelper<RemoveCvRef_t<T>>::typeId_ = TypeInfoDb::lastTypeId_++;
        details_.alignment_ = alignof(T);
        details_.size_ = sizeof(T);
        details_.isTrivial_ = std::is_trivial_v<T>;
        details_.ctor_ = TypeCtor<T>;
        details_.dtor_ = TypeDtor<T>;
        details_.copyCtor_ = TypeCopyCtor<T>;
        details_.moveCtor_ = TypeMoveCtor<T>;
        TypeInfoDb::details_.Add(&details_);

        hs_assert(TypeInfoDb::details_.Count() == TypeInfoDb::lastTypeId_);
    }

    inline static TypeDetails details_;
};

class EcsWorld;

//------------------------------------------------------------------------------
class Archetype // Table?
{
public:
    using Type_t = Array<uint>;
    using Column_t = void*;

    //------------------------------------------------------------------------------
    Archetype(EcsWorld* world, const Type_t& type)
        : world_(world)
    {
        type_ = type;
        rowCapacity_ = 8;

        for (int i = 0; i < type_.Count(); ++i)
        {
            // TODO alignment
            const TypeDetails* details = TypeInfoDb::GetDetails(type_[i]);
            Column_t column = malloc(rowCapacity_ * details->size_);
            if (!details->isTrivial_)
            {
                for (uint rowI = 0; rowI < rowCapacity_; ++rowI)
                {
                    details->ctor_((byte*)column + rowI * details->size_);
                }
            }
            columns_.Add(column);
        }
    }

    //------------------------------------------------------------------------------
    ~Archetype()
    {
        for (int i = 0; i < type_.Count(); ++i)
        {
            const TypeDetails* details = TypeInfoDb::GetDetails(type_[i]);
            if (!details->isTrivial_)
            {
                for (uint rowI = 0; rowI < rowCapacity_; ++rowI)
                {
                    details->dtor_((byte*)columns_[i] + rowI * details->size_);
                }
            }
            free(columns_[i]);
        }
    }


    //------------------------------------------------------------------------------
    Archetype(Archetype&& other) = default;

    //------------------------------------------------------------------------------
    Archetype& operator=(Archetype&& other) = default;

    //------------------------------------------------------------------------------
    Archetype(const Archetype&) = delete;

    //------------------------------------------------------------------------------
    Archetype& operator=(const Archetype&) = delete;

    //------------------------------------------------------------------------------
    const Type_t& GetType() const
    {
        return type_;
    }

    //------------------------------------------------------------------------------
    uint FindComponent(uint componentTypeId) const
    {
        for (int i = 0; i < type_.Count(); ++i)
        {
            if (type_[i] == componentTypeId)
                return i;
        }

        return ID_BAD;
    }

    //------------------------------------------------------------------------------
    template<class TComponent>
    uint FindComponent() const
    {
        auto componentTypeId = TypeInfo<TComponent>::TypeId();
        return FindComponent(componentTypeId);
    }

    //------------------------------------------------------------------------------
    template<class... TComponent>
    bool HasComponents() const
    {
        return ((FindComponent<TComponent>() != ID_BAD) && ...);
    }

    //------------------------------------------------------------------------------
    bool IsType(const Type_t& otherType) const
    {
        if (type_.Count() != otherType.Count())
            return false;

        for (int i = 0; i < type_.Count(); ++i)
        {
            if (type_[i] != otherType[i])
                return false;
        }

        return true;
    }

    //------------------------------------------------------------------------------
    template<class TComponent>
    TComponent& GetComponent(uint row, uint column)
    {
        auto size = TypeInfo<TComponent>::details_.size_;
        auto& result = *reinterpret_cast<TComponent*>((byte*)columns_[column] + row * size);
        return result;
    }

    //------------------------------------------------------------------------------
    void SetComponent(uint rowIdx, uint componentTypeId, void* value)
    {
        hs_assert(componentTypeId != ID_BAD);

        const auto details = TypeInfoDb::GetDetails(componentTypeId);
        auto size = details->size_;

        auto componentIdx = FindComponent(componentTypeId);

        Column_t column = columns_[componentIdx];

        void* dst = (byte*)column + rowIdx * size;
        if (details->isTrivial_)
        {
            memcpy(dst, value, size);
        }
        else
        {
            details->copyCtor_(dst, value);
        }
    }

    //------------------------------------------------------------------------------
    template<class TComponent, class... TRest>
    void SetComponents(uint rowIdx, const TComponent& value, const TRest... rest)
    {
        SetComponent(rowIdx, value);
        SetComponents(rowIdx, rest...);
    }

    //------------------------------------------------------------------------------
    template<class TComponent>
    void SetComponents(uint rowIdx, const TComponent& value)
    {
        SetComponent(rowIdx, value);
    }

    //------------------------------------------------------------------------------
    uint AddEntity(Entity_t eid)
    {
        EnsureCapacity();

        Element entityElement = GetElement(rowCount_, 0);
        memcpy(entityElement.data_, &eid, sizeof(eid));

        for (int i = 1; i < columns_.Count(); ++i)
        {
            Element e = GetElement(rowCount_, i);
            if (e.details_->isTrivial_)
            {
                memset(e.data_, 0, e.details_->size_);
            }
            else
            {
                e.details_->dtor_(e.data_);
                e.details_->ctor_(e.data_);
            }
        }

        return rowCount_++;
    }

    //------------------------------------------------------------------------------
    void RemoveRow(uint row);

    //------------------------------------------------------------------------------
    int TryGetIterators(const Type_t& canonicalType, Span<const uint> permutation, void** arr, Span<const uint> avoidTypes) const
    {
        if (canonicalType.Count() > type_.Count())
            return 0;

        int colI = 0;
        for (int i = 0; i < type_.Count(); ++i)
        {
            for (int avoidI = 0; avoidI < avoidTypes.Count(); ++avoidI)
            {
                if (avoidTypes[avoidI] == type_[i])
                    return 0;
            }

            if (colI < permutation.Count() && canonicalType[colI] == type_[i])
            {
                auto finalIdx = permutation[colI++];
                arr[finalIdx] = columns_[i];
            }
        }

        if (colI != permutation.Count())
            return 0;

        return rowCount_;
    }

    //------------------------------------------------------------------------------
    struct Element
    {
        void* data_;
        const TypeDetails* details_;
    };

    //------------------------------------------------------------------------------
    Element GetElement(uint row, uint column)
    {
        const TypeDetails* details = TypeInfoDb::GetDetails(type_[column]);
        Element element;
        element.data_ = (byte*)columns_[column] + row * details->size_;
        element.details_ = details;
        return element;
    }

private:
    EcsWorld* world_;
    Type_t type_;
    Array<Column_t> columns_;
    uint rowCount_{};
    uint rowCapacity_;

    //------------------------------------------------------------------------------
    uint GetEntityId(uint row)
    {
        auto element = GetElement(row, 0);
        return *(Entity_t*)element.data_;
    }

    //------------------------------------------------------------------------------
    void EnsureCapacity()
    {
        if (rowCount_ < rowCapacity_)
            return;

        hs_assert(rowCount_ == rowCapacity_);

        rowCapacity_ *= 2;
        for (int i = 0; i < columns_.Count(); ++i)
        {
            // TODO alignment
            const TypeDetails* details = TypeInfoDb::GetDetails(type_[i]);
            Column_t newColumn{};
            if (details->isTrivial_)
            {
                newColumn = realloc(columns_[i], rowCapacity_ * details->size_);
            }
            else
            {
                newColumn = malloc(rowCapacity_ * details->size_);

                for (uint rowI = 0; rowI < rowCapacity_; ++rowI)
                {
                    void* dst = (byte*)newColumn + rowI * details->size_;
                    void* src = (byte*)columns_[i] + rowI * details->size_;
                    details->moveCtor_(dst, src);
                    details->dtor_(src);
                }
                free(columns_[i]);
            }
            hs_assert(newColumn);
            columns_[i] = newColumn;
        }
    }

    //------------------------------------------------------------------------------
    void SwapRow(uint a, uint b)
    {
        for (int i = 0; i < columns_.Count(); ++i)
        {
            const TypeDetails* details = TypeInfoDb::GetDetails(type_[i]);
            auto size = details->size_;

            byte* tmp = HS_ALLOCA(byte, size);
            byte* aPtr = (byte*)columns_[i] + size * a;
            byte* bPtr = (byte*)columns_[i] + size * b;

            if (details->isTrivial_)
            {
                memcpy(tmp, aPtr, size);
                memcpy(aPtr, bPtr, size);
                memcpy(bPtr, tmp, size);
            }
            else
            {
                details->moveCtor_(tmp, aPtr);
                details->moveCtor_(aPtr, bPtr);
                details->moveCtor_(bPtr, tmp);
                details->dtor_(tmp);
            }
        }
    }

    //------------------------------------------------------------------------------
    template<class TComponent>
    void SetComponent(uint rowIdx, const TComponent& value)
    {
        auto componentId = FindComponent<TComponent>();
        hs_assert(componentId != ID_BAD);
        hs_assert(rowIdx < rowCount_);

        Column_t column = columns_[componentId];
        static_cast<TComponent*>(column)[rowIdx] = value;
    }
};

//------------------------------------------------------------------------------
// Class that has all the types and entities
class EcsWorld
{
    friend class Archetype;

public:
    //------------------------------------------------------------------------------
    EcsWorld()
    {
        Archetype emptyArchetype(this, { 0 });
        archetypes_.Add(std::move(emptyArchetype));
    }

    //------------------------------------------------------------------------------
    Entity_t CreateEntity()
    {
        Entity_t id{};

        if (denseUsedCount_ == dense_.Count())
        {
            id = denseUsedCount_;
            dense_.Add(id);
            sparse_.Add(id);
        }
        else
        {
            hs_assert(denseUsedCount_ < dense_.Count());
            id = dense_[denseUsedCount_];
            sparse_[id] = denseUsedCount_;
        }
        ++denseUsedCount_;

        EntityRecord record;
        record.archetype_= 0;
        record.rowIndex_ = archetypes_[0].AddEntity(id);

        records_.Add(record);

        return id;
    }

    //------------------------------------------------------------------------------
    template<class... TComponents>
    Entity_t CreateEntity(TComponents... components)
    {
        auto e = CreateEntity();
        SetComponents(e, components...);
        return e;
    }

    //------------------------------------------------------------------------------
    void DeleteEntity(Entity_t entity)
    {
        EntityDeleteOperation deleteOp(this, entity);
        if (IsIterating())
        {
            deleteOp.Execute();
        }
        else
        {
            deferredDeletions_.Add(std::move(deleteOp));
        }
    }

    //------------------------------------------------------------------------------
    template<class TComponent>
    TComponent& GetComponent(Entity_t entity)
    {
        auto dense = sparse_[entity];
        EntityRecord& record = records_[dense];
        Archetype* arch = &archetypes_[record.archetype_];

        auto columnIdx = arch->FindComponent<TComponent>();
        hs_assert(columnIdx != ID_BAD);

        auto& component = arch->GetComponent<TComponent>(record.rowIndex_, columnIdx);
        return component;
    }

    //------------------------------------------------------------------------------
    template<class... TComponent>
    void SetComponents(Entity_t entity, const TComponent&... components)
    {
        // Find entity
        auto dense = sparse_[entity];
        EntityRecord& record = records_[dense];
        Archetype* originalArch = &archetypes_[record.archetype_];

        if (originalArch->HasComponents<TComponent...>())
        {
            originalArch->SetComponents(record.rowIndex_, components...);
        }
        else
        {
            // TODO get rid of this allocation
            auto type = originalArch->GetType();
            AddComponentsToType<TComponent...>(type);

            uint archetypeIdx = ID_BAD;

            for (int i = 0; i < archetypes_.Count(); ++i)
            {
                if (archetypes_[i].IsType(type))
                {
                    archetypeIdx = i;
                    break;
                }
            }

            if (archetypeIdx == ID_BAD)
            {
                Archetype newArchetype(this, type);
                archetypes_.Add(std::move(newArchetype));
                archetypeIdx = (uint)archetypes_.Count() - 1;
            }

            hs_assert(archetypeIdx != ID_BAD);

            // Copy components one by one from old to the new originalArch
            Archetype* newArch = &archetypes_[archetypeIdx];

            EntityRecord newRecord;
            newRecord.archetype_ = archetypeIdx;
            newRecord.rowIndex_ = newArch->AddEntity(entity);

            const auto& oldType = originalArch->GetType();

            // TODO skip components in ...components
            for (int i = 0; i < oldType.Count(); ++i)
            {
                void* oldValue = originalArch->GetElement(record.rowIndex_, i).data_;
                newArch->SetComponent(newRecord.rowIndex_, oldType[i], oldValue); // TODO use MoveComponent here since it will be no longer valid in the old archetype
            }

            newArch->SetComponents(newRecord.rowIndex_, components...);

            originalArch->RemoveRow(record.rowIndex_);

            record = newRecord;
        }
    }

    //------------------------------------------------------------------------------
    void GetEntities(uint*& begin, uint& count)
    {
        begin = dense_.Data();
        count = denseUsedCount_;
    }

    //------------------------------------------------------------------------------
    template<class... TComponent>
    struct Except
    {
    };

    //------------------------------------------------------------------------------
    template<class... TComponents>
    struct Iter
    {
        //------------------------------------------------------------------------------
        explicit Iter(EcsWorld* world) : world_(world) {}

        //------------------------------------------------------------------------------
        template<class... TAvoidComponents, class TFun>
        void EachExcept(TFun fun)
        {
            IterScope iterScope(world_);
            static constexpr int COMP_COUNT = sizeof...(TComponents);
            auto seq = std::make_index_sequence<COMP_COUNT>();

            uint avoidTypes[]{ TypeInfo<TAvoidComponents>::TypeId()... };

            Archetype::Type_t type{ TypeInfo<TComponents>::TypeId()... };
            Archetype::Type_t canonicalType = type;
            std::sort(canonicalType.begin(), canonicalType.end());

            uint permutation[COMP_COUNT];
            for (int i = 0; i < COMP_COUNT; ++i)
            {
                permutation[i] = type.IndexOf(canonicalType[i]);
            }

            for (int archI = 0; archI < world_->archetypes_.Count(); ++archI)
            {
                void* arr[COMP_COUNT]{};
                if (int rowCount = world_->archetypes_[archI].TryGetIterators(canonicalType, MakeSpan(permutation), arr, MakeSpan(avoidTypes));
                    rowCount)
                {
                    for (int rowI = 0; rowI < rowCount; ++rowI)
                    {
                        CallHelper(arr, rowI, fun, seq);
                    }
                }
            }
        }

        //------------------------------------------------------------------------------
        template<class TFun>
        void Each(TFun fun)
        {
            IterScope iterScope(world_);
            static constexpr int COMP_COUNT = sizeof...(TComponents);
            auto seq = std::make_index_sequence<COMP_COUNT>();

            Archetype::Type_t type{ TypeInfo<TComponents>::TypeId()... };
            Archetype::Type_t canonicalType = type;
            std::sort(canonicalType.begin(), canonicalType.end());

            uint permutation[COMP_COUNT];
            for (int i = 0; i < COMP_COUNT; ++i)
            {
                permutation[i] = (uint)type.IndexOf(canonicalType[i]);
            }

            for (int archI = 0; archI < world_->archetypes_.Count(); ++archI)
            {
                void* arr[COMP_COUNT]{};
                if (int rowCount = world_->archetypes_[archI].TryGetIterators(canonicalType, MakeSpan(permutation), arr, Span<const uint>());
                    rowCount)
                {
                    for (int rowI = 0; rowI < rowCount; ++rowI)
                    {
                        CallHelper(arr, rowI, fun, seq);
                    }
                }
            }
        }

    private:
        EcsWorld* world_;

        //------------------------------------------------------------------------------
        template<class TFun, unsigned long long... Seq>
        void CallHelper(void** arr, int row, TFun fun, std::index_sequence<Seq...>)
        {
            fun(((TComponents*)arr[Seq])[row]...);
        }

        //------------------------------------------------------------------------------
        // RAII structure for automatic handling of when iteration starts and ends.
        struct IterScope
        {
            //------------------------------------------------------------------------------
            [[nodiscard]]
            IterScope(EcsWorld* world) : world_(world)
            {
                ++world_->iteratingDepth_;
            }

            //------------------------------------------------------------------------------
            ~IterScope()
            {
                hs_assert(world_->IsIterating());
                --world_->iteratingDepth_;

                if (!world_->IsIterating())
                    world_->OnIterationEnd();
            }

        private:
            EcsWorld* world_;
        };
    };

private:
    struct EntityDeleteOperation;
    struct EntityRecord
    {
        uint archetype_;
        uint rowIndex_;
    };

    Array<Entity_t>     sparse_;
    Array<uint>         dense_;
    Array<EntityRecord> records_;
    Array<Archetype>    archetypes_;

    Array<EntityDeleteOperation> deferredDeletions_;

    uint                denseUsedCount_{};
    uint                iteratingDepth_{};

    //------------------------------------------------------------------------------
    void SwapEntity(uint denseIdxA, uint denseIdxB)
    {
        Swap(dense_[denseIdxA], dense_[denseIdxB]);
        Swap(records_[denseIdxA], records_[denseIdxB]);

        sparse_[dense_[denseIdxA]] = denseIdxA;
        sparse_[dense_[denseIdxB]] = denseIdxB;
    }

    //------------------------------------------------------------------------------
    template<class TComponent>
    void AddComponentToType(Archetype::Type_t& type)
    {
        auto typeId = TypeInfo<TComponent>::TypeId();
        // TODO binary search? But we will change the system to bitfields anyway
        for (int i = 0; i < type.Count(); ++i)
        {
            hs_assert(type[i] != typeId && "TypeId already present in type");
            if (type[i] > typeId)
            {
                type.Insert(i, typeId);
                return;
            }
        }

        type.Add(typeId);
    }

    //------------------------------------------------------------------------------
    template<class TComponent>
    bool AddComponentsToTypeHelper(Archetype::Type_t& type)
    {
        AddComponentToType<TComponent>(type);
        return true;
    }

    //------------------------------------------------------------------------------
    template<class... TComponent>
    void AddComponentsToType(Archetype::Type_t& type)
    {
        (AddComponentsToTypeHelper<TComponent>(type) && ...);
    }

    //------------------------------------------------------------------------------
    void UpdateRecord(Entity_t eid, uint rowIdx)
    {
        auto denseIdx = sparse_[eid];
        records_[denseIdx].rowIndex_ = rowIdx;
    }

    //------------------------------------------------------------------------------
    bool IsIterating()
    {
        return iteratingDepth_ > 0;
    }

    //------------------------------------------------------------------------------
    void OnIterationEnd()
    {
        for (int i = 0; i < deferredDeletions_.Count(); ++i)
        {
            deferredDeletions_[i].Execute();
        }

        deferredDeletions_.Clear();
    }

    //------------------------------------------------------------------------------
    struct EntityDeleteOperation
    {
        //------------------------------------------------------------------------------
        EntityDeleteOperation(EcsWorld* world, Entity_t entity)
            : world_(world)
            , entity_(entity)
        {
        }

        //------------------------------------------------------------------------------
        void Execute()
        {
            hs_assert(world_->denseUsedCount_ > 0);
            auto denseIdx = world_->sparse_[entity_];
            auto lastDense = world_->denseUsedCount_ - 1;
            hs_assert(denseIdx <= world_->denseUsedCount_);

            const auto& record = world_->records_[denseIdx];
            world_->archetypes_[record.archetype_].RemoveRow(record.rowIndex_);

            if (denseIdx < lastDense)
            {
                world_->SwapEntity(denseIdx, lastDense);
            }

            // Remove last entity
            --world_->denseUsedCount_;
            world_->records_.RemoveLast();
        }

    private:
        EcsWorld* world_;
        Entity_t entity_;
    };
};

//------------------------------------------------------------------------------
inline void Archetype::RemoveRow(uint row)
{
    hs_assert(row < rowCount_);

    const uint lastRowIdx = rowCount_ - 1;
    if (row < lastRowIdx)
    {
        auto eid = GetEntityId(lastRowIdx);
        world_->UpdateRecord(eid, row);
        SwapRow(row, lastRowIdx);
    }

    --rowCount_;
}


}
