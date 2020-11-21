#pragma once

#include "Config.h"

#include "game/Camera.h"

#include "render/VertexBufferEntry.h"
#include "render/DynamicUniformBufferEntry.h"
#include "render/VkTypes.h"

#include "containers/Hash.h"
#include "containers/Array.h"

#include "math/hs_Math.h"

#include "common/Pointers.h"
#include "common/Enums.h"
#include "common/Types.h"

#include "platform/hs_Windows.h"

#include <unordered_map> // TODO use custom hashmap

//------------------------------------------------------------------------------
bool CheckResult(VkResult result, const char* file, int line, const char* fun);

#define VKR_SUCCEED(x) CheckResult(x, __FILE__, __LINE__, #x)
#define VKR_CHECK(x) VKR_SUCCEED(x)
#define VKR_FAILED(x) !VKR_SUCCEED(x)

//------------------------------------------------------------------------------
namespace hs
{

//------------------------------------------------------------------------------
static constexpr uint SRV_SLOT_COUNT = 8;
static constexpr uint IMMUTABLE_SAMPLER_COUNT = 1;
static constexpr uint DYNAMIC_UBO_COUNT = 2;

//------------------------------------------------------------------------------
extern class Render* g_Render;

//------------------------------------------------------------------------------
RESULT CreateRender(uint width, uint height);
void DestroyRender();

//------------------------------------------------------------------------------
VkResult SetDiagName(VkDevice device, uint64 object, VkObjectType type, const char* name);

//------------------------------------------------------------------------------
class Shader;
class Material;
class Texture;
class ShaderManager;
class VertexBuffer;
class DynamicUBOCache;
class VertexBufferCache;

class DrawCanvas;
class SpriteRenderer;
class DebugShapeRenderer;

class SerializationManager;

//------------------------------------------------------------------------------
enum DepthState
{
    DS_TEST = 1,
    DS_WRITE = 2,
};

//------------------------------------------------------------------------------
struct RenderState
{
    static constexpr uint MAX_CONST_BUFF = 1;
    static constexpr uint MAX_VERT_BUFF = 1;
    static constexpr uint INVALID_DESC = (uint)-1;
    static constexpr uint INVALID_HANDLE = (uint)-1;

    Shader*                 shaders_[PS_COUNT]{};
    uint                    fsTextures_[SRV_SLOT_COUNT]{};
    DynamicUBOEntry         dynamicUBOs_[DYNAMIC_UBO_COUNT]{};

    DynamicUBOEntry         bindlessUBO_{};

    VkBuffer                vertexBuffers_[MAX_VERT_BUFF];
    VkDeviceSize            vbOffsets_[MAX_VERT_BUFF];
    VkDescriptorSet         uboDescSet_{};

    uint                    vertexLayouts_[MAX_VERT_BUFF];

    VkrPrimitiveTopology    primitiveTopology_{};
    VkrCullMode             cullMode_{};

    uint                    depthState_{ DS_TEST | DS_WRITE};

    void Reset();
};

//------------------------------------------------------------------------------
class Render
{
    friend RESULT CreateRender(uint width, uint height);
    friend void DestroyRender();

public:
    RESULT OnWindowResized(uint width, uint height);
    RESULT ReloadShaders();
    RESULT InitWin32(HWND hwnd, HINSTANCE hinst);
    RESULT InitImgui();

    void ClearPipelineCache();

    // Setting state
    template<PipelineStage stage>
    void SetShader(Shader* shader)
    {
        state_.shaders_[stage] = shader;
    }
    void SetTexture(uint slot, Texture* texture);
    void SetVertexBuffer(uint slot, const VertexBufferEntry& entry);
    void SetVertexLayout(uint slot, uint layoutHandle);
    void SetPrimitiveTopology(VkrPrimitiveTopology primitiveTopology);
    void SetDynamicUbo(uint slot, const DynamicUBOEntry& entry);
    void SetDepthState(uint state);

    // Drawing
    void Draw(uint vertexCount, uint firstVertex);

    void Update(float dTime);

    VkDevice GetDevice() const;
    VmaAllocator GetAllocator() const;
    ShaderManager* GetShaderManager() const;
    VkCommandBuffer CmdBuff() const;
    uint64 GetCurrentFrame() const;
    uint64 GetSafeFrame() const;

    void DestroyLater(VkBuffer buffer, VmaAllocation allocation);

    void TransitionBarrier(
        VkImage img, VkImageSubresourceRange subresource,
        VkAccessFlags accessBefore, VkAccessFlags accessAfter,
        VkImageLayout layoutBefore, VkImageLayout layoutAfter,
        VkPipelineStageFlags stageBefore, VkPipelineStageFlags stageAfter
    );

    uint AddBindlessTexture(VkImageView view);

    const VkPhysicalDeviceProperties& GetPhysDevProps() const;

    DynamicUBOCache* GetUBOCache() const;
    VertexBufferCache* GetVertexCache() const;

    uint GetWidth() const;
    uint GetHeight() const;
    float GetAspect() const;
    Vec2 GetDimensions() const;

    void ResetState();

    //----------------------
    // Vertex layout manager
    uint GetOrCreateVertexLayout(VkPipelineVertexInputStateCreateInfo info);

    //----------------------
    // Camera
    const Camera& GetCamera() const;
    Camera& GetCamera();

    // Sprite renderer
    SpriteRenderer* GetSpriteRenderer() const;

    // Debug shape renderer
    DebugShapeRenderer* GetDebugShapeRenderer() const;

private:
    static constexpr auto VK_VERSION = VK_API_VERSION_1_1;
    static constexpr uint VKR_INVALID = -1;

    static constexpr uint BB_IMG_COUNT = 2;

    // Win32
    HINSTANCE   hinst_;
    HWND        hwnd_;

    uint                width_{};
    uint                height_{};

    VkSurfaceCapabilitiesKHR vkSurfaceCapabilities_;

    // Core Vulkan
    VkInstance          vkInstance_{};
    VkPhysicalDevice    vkPhysicalDevice_{};
    VkDevice            vkDevice_{};

    VkPhysicalDeviceProperties vkPhysicalDeviceProperties_{};

    // Debug
    #if HS_DEBUG
        VkDebugReportCallbackEXT debugReportCallback_{};
    #endif

    // Swapchain
    VkSurfaceKHR        vkSurface_{};
    VkSwapchainKHR      vkSwapchain_{};
    uint                currentBBIdx_{};
    VkImage             bbImages_[BB_IMG_COUNT]{};
    VkImageView         bbViews_[BB_IMG_COUNT]{};
    VkFormat            swapChainFormat_{};

    VkImage             depthImages_[BB_IMG_COUNT]{};
    VmaAllocation       depthMemory_[BB_IMG_COUNT]{};
    VkImageView         depthViews_[BB_IMG_COUNT]{};

    uint64              frame_{};

    // Synchronization
    #if defined(VKR_USE_TIMELINE_SEMAPHORES)
        VkSemaphore         directQueueSemaphore_{};
        uint64              semaphoreValues[BB_IMG_COUNT]{ 1, 1 };
    #endif

    VkFence             directQueueFences_[BB_IMG_COUNT]{};
    VkFence             nextImageFence_;

    VkSemaphore         submitSemaphores_[BB_IMG_COUNT]{};

    // Queues
    uint                directQueueFamilyIdx_{ VKR_INVALID };
    VkQueue             vkDirectQueue_{};

    // Command buffers
    VkCommandPool       directCmdPool_{};
    VkCommandBuffer     directCmdBuffers_[BB_IMG_COUNT]{};

    VkRenderPass        mainRenderPass_{};
    VkFramebuffer       mainFrameBuffer_[BB_IMG_COUNT]{};

    // Descriptors
    VkDescriptorPool    bindlessPool_{};
    VkDescriptorSet     bindlessSet_{};
    uint                lastFreeBindlessIndex_{};

    VkDescriptorPool    immutableSamplerPool_{};
    VkDescriptorSet     immutableSamplerSet_{};

    VkDescriptorPool    dynamicUBODPool_[BB_IMG_COUNT]{};

    // Imgui
    // TODO(pavel): Rework this, how big descriptor pool does Imgui need? Can we use one of ours?
    VkDescriptorPool    imguiDescriptorPool_;

    // Pipelines
    /*struct PipelineKey
    {
        uint64 k[1];
    };*/
    using PipelineKey = uint64;
    std::unordered_map<PipelineKey, VkPipeline, FibonacciHash<PipelineKey>> pipelineCache_;

    // Caches
    UniquePtr<DynamicUBOCache>    uboCache_;
    UniquePtr<VertexBufferCache>  vbCache_;

    // Allocator
    VmaAllocator        allocator_;

    // Keep alive objects
    struct BufferToRelease
    {
        VkBuffer        buffer_;
        VmaAllocation   allocation_;
    };
    Array<VkPipeline>       destroyPipelines_[BB_IMG_COUNT];
    Array<BufferToRelease>  destroyBuffers_[BB_IMG_COUNT];

    // Shaders
    UniquePtr<ShaderManager>    shaderManager_{};
    VkDescriptorSetLayout       fsSamplerLayout_{};
    VkDescriptorSetLayout       bindlessTexturesLayout_{};
    VkDescriptorSetLayout       dynamicUBOLayout_{};
    VkPipelineLayout            pipelineLayout_{};

    RenderState state_;

    Array<UniquePtr<Material>>  materials_;
    UniquePtr<DrawCanvas>       drawCanvas_;

    UniquePtr<SpriteRenderer>       spriteRenderer_;
    UniquePtr<DebugShapeRenderer>   debugShapeRenderer_;

    RESULT CreateSurface();
    RESULT CreateSwapchain();
    RESULT CreateMainRenderPass();
    RESULT CreateMainFrameBuffer();

    void DestroySurface();
    void DestroySwapchain();
    void DestroyMainRenderPass();
    void DestroyMainFrameBuffer();

    //----------------------
    // Vertex layout manager
    Array<VkPipelineVertexInputStateCreateInfo> vertexLayouts_;

    static PipelineKey StateToPipelineKey(const RenderState& state);

    RESULT PrepareForDraw();
    void AfterDraw();

    RESULT WaitForFence(VkFence fence);

    void Free();

    template<bool present, bool wait>
    void FlushGpu();

    //----------------------
    // Serialization
    UniquePtr<SerializationManager> serializationManager_;

    //----------------------
    // Camera
    Camera camera_;
};



}
