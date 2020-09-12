#include "render/Render.h"

#include "render/Allocator.h"
#include "render/Shader.h"
#include "render/Material.h"
#include "render/Texture.h"
#include "render/ShaderManager.h"
#include "render/VertexBuffer.h"
#include "render/DynamicUniformBuffer.h"
#include "render/hs_Vulkan.h"

#include "game/DrawCanvas.h"
#include "game/TileRenderer.h"
#include "game/DebugShapeRenderer.h"

#include "game/Serialization.h"
#include "input/Input.h"

#include "common/Logging.h"
#include "common/hs_Assert.h"

#include "vulkan/vulkan_win32.h"

#include <malloc.h>
#include <cstdio>
#include <cfloat>


//------------------------------------------------------------------------------
static constexpr const char* CAMERA_CFG = "configs/Camera.json";

//------------------------------------------------------------------------------
const char* ResultToString(VkResult result)
{
    switch(result)
    {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_INCOMPATIBLE_VERSION_KHR: return "VK_ERROR_INCOMPATIBLE_VERSION_KHR";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_NOT_PERMITTED_EXT: return "VK_ERROR_NOT_PERMITTED_EXT";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT: return "VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT";
        //case VK_ERROR_OUT_OF_POOL_MEMORY_KHR: return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
        //case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR: return "VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR";
        //case VK_ERROR_FRAGMENTATION_EXT: return "VK_ERROR_FRAGMENTATION_EXT";
        //case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT: return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";
        //case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR";
        //case VK_RESULT_BEGIN_RANGE: return "VK_RESULT_BEGIN_RANGE";
        //case VK_RESULT_END_RANGE: return "VK_RESULT_END_RANGE";
        //case VK_RESULT_RANGE_SIZE: return "VK_RESULT_RANGE_SIZE";
        default: return "UNKNOWN CODE";
    }
}

//------------------------------------------------------------------------------
bool CheckResult(VkResult result, const char* file, int line, const char* fun)
{
    if (result != VK_SUCCESS)
    {
        hs::Log(hs::LogLevel::Error, "Operation failed, %s(%d), %s, error: %d %s", file, line, fun, result, ResultToString(result));
        hs_assert(false);
        return false;
    }
    return true;
}

#if HS_DEBUG
    //------------------------------------------------------------------------------
    VkResult CreateDebugReportCallbackEXT(
        VkInstance instance,
        const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugReportCallbackEXT* pCallback)
    {
        auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
        if (func)
            return func(instance, pCreateInfo, pAllocator, pCallback);
        else
            return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    /*
    void DestroyDebugReportCallbackEXT(
        VkInstance instance, 
        VkDebugReportCallbackEXT callback, 
        const VkAllocationCallbacks* pAllocator
    );*/
#endif

namespace hs
{

//------------------------------------------------------------------------------
struct BindingUBO
{
    uint SRV[SRV_SLOT_COUNT]{};
};

#if HS_DEBUG
    //------------------------------------------------------------------------------
    void QueueFamiliesToString(uint bits, char* str)
    {
        str[0] = 0;
        char* start = str;
        if (bits & VK_QUEUE_GRAPHICS_BIT)
            start += sprintf(start, "VK_QUEUE_GRAPHICS_BIT ");
        if (bits & VK_QUEUE_COMPUTE_BIT)
            start += sprintf(start, "VK_QUEUE_COMPUTE_BIT ");
        if (bits & VK_QUEUE_TRANSFER_BIT)
            start += sprintf(start, "VK_QUEUE_TRANSFER_BIT ");
        if (bits & VK_QUEUE_SPARSE_BINDING_BIT)
            start += sprintf(start, "VK_QUEUE_SPARSE_BINDING_BIT ");
        if (bits & VK_QUEUE_PROTECTED_BIT)
            start += sprintf(start, "VK_QUEUE_PROTECTED_BIT ");
    }
#endif

//------------------------------------------------------------------------------
VkPhysicalDeviceFeatures CreateRequiredFeatures()
{
    VkPhysicalDeviceFeatures features{};

    features.samplerAnisotropy = VK_TRUE;
    features.fillModeNonSolid = VK_TRUE;

    return features;
}

//------------------------------------------------------------------------------
Render* g_Render;

//------------------------------------------------------------------------------
RESULT CreateRender(uint width, uint height)
{
    g_Render = new Render();

    g_Render->width_ = width;
    g_Render->height_ = height;

    return R_OK;
}

//------------------------------------------------------------------------------
void DestroyRender()
{
    g_Render->Free();
    delete g_Render;
}

//------------------------------------------------------------------------------
constexpr const char* IGNORED_MSGS[] = {
    "UNASSIGNED-BestPractices-vkCreateDevice-deprecated-extension",
};

//------------------------------------------------------------------------------
constexpr const char* VALIDATION_WARNING = "Validation Warning:";

//------------------------------------------------------------------------------
VkBool32 ValidationCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData)
{
    for (uint i = 0; i < hs_arr_len(IGNORED_MSGS); ++i)
    {
        if (strstr(msg, IGNORED_MSGS[i]) != NULL)
            return VK_FALSE;
    }

    auto logLevel = LogLevel::Error;
    if (strncmp(msg, VALIDATION_WARNING, strlen(VALIDATION_WARNING)) == 0)
        logLevel = LogLevel::Warning;

    Log(logLevel, "%s", msg);

    // Return true only when we want to test the VL themselves
    return VK_FALSE;
}

//------------------------------------------------------------------------------
RESULT Render::ReloadShaders()
{
    hs_assert(shaderManager_);

    for (auto pl : pipelineCache_)
        destroyPipelines_[currentBBIdx_].Add(pl.second);

    pipelineCache_.clear();

    return shaderManager_->ReloadShaders();
}

//------------------------------------------------------------------------------
RESULT Render::WaitForFence(VkFence fence)
{
    VkResult fenceVal = vkGetFenceStatus(vkDevice_, fence);
    if (fenceVal == VK_ERROR_DEVICE_LOST)
    {
        if (VKR_FAILED(fenceVal))
            return R_FAIL;
    }

    if (fenceVal == VK_NOT_READY)
    {
        if (VKR_FAILED(vkWaitForFences(vkDevice_, 1, &fence, VK_TRUE, (uint64)-1)))
            return R_FAIL;
    }

    if (VKR_FAILED(vkResetFences(vkDevice_, 1, &fence)))
        return R_FAIL;

    return R_OK;
}

//------------------------------------------------------------------------------
PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT;

//------------------------------------------------------------------------------
VkResult SetDiagName(VkDevice device, uint64 object, VkObjectType type, const char* name)
{
    VkDebugUtilsObjectNameInfoEXT nameInfo{};
    nameInfo.sType          = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType     = type;
    nameInfo.objectHandle   = object;
    nameInfo.pObjectName    = name;

    return pfnSetDebugUtilsObjectNameEXT(device, &nameInfo);
}

void Render::TransitionBarrier(
    VkImage img, VkImageSubresourceRange subresource,
    VkAccessFlags accessBefore, VkAccessFlags accessAfter,
    VkImageLayout layoutBefore, VkImageLayout layoutAfter,
    VkPipelineStageFlags stageBefore, VkPipelineStageFlags stageAfter)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask       = accessBefore;
    barrier.dstAccessMask       = accessAfter;
    barrier.oldLayout           = layoutBefore;
    barrier.newLayout           = layoutAfter;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = img;
    barrier.subresourceRange    = subresource;

    vkCmdPipelineBarrier(
        g_Render->CmdBuff(),
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

//------------------------------------------------------------------------------
RESULT Render::InitWin32(HWND hwnd, HINSTANCE hinst)
{
    hwnd_ = hwnd;

    //-----------------------
    // Create Vulkan instance
    uint apiVersion;
    if (VKR_FAILED(vkEnumerateInstanceVersion(&apiVersion)))
        return R_FAIL;

    if (apiVersion < VK_VERSION)
    {
        //Log(LogLevel::Error, "Vulkan version too low, %d, expected %d", apiVersion, VK_VERSION);
        //return R_FAIL;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName    = "VkRenderer";
    appInfo.applicationVersion  = VK_MAKE_VERSION(0, 0, 1);
    appInfo.pEngineName         = "VkRenderer";
    appInfo.engineVersion       = VK_MAKE_VERSION(0, 0, 1);
    appInfo.apiVersion          = VK_VERSION;

    VkInstanceCreateInfo instInfo{};
    instInfo.sType              = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo   = &appInfo;

    #if HS_DEBUG
        const char* validationLayers[] = {
            "VK_LAYER_LUNARG_standard_validation",
        };

        instInfo.enabledLayerCount      = (uint)hs_arr_len(validationLayers);
        instInfo.ppEnabledLayerNames    = validationLayers;
    #endif

    const char* instanceExt[] = {
        "VK_KHR_win32_surface",
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };

    instInfo.enabledExtensionCount      = hs_arr_len(instanceExt);
    instInfo.ppEnabledExtensionNames    = instanceExt;

    if (VKR_FAILED(vkCreateInstance(&instInfo, nullptr, &vkInstance_)))
        return R_FAIL;

    #if HS_DEBUG
        VkDebugReportCallbackCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        createInfo.pfnCallback = ValidationCallback;

        if (VKR_FAILED(CreateDebugReportCallbackEXT(vkInstance_, &createInfo, nullptr, &debugReportCallback_)))
            return R_FAIL;
    #endif

    //-----------------------
    // Create surface
    VkWin32SurfaceCreateInfoKHR winSurfaceInfo{};
    winSurfaceInfo.sType        = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    winSurfaceInfo.hinstance    = hinst;
    winSurfaceInfo.hwnd         = hwnd_;

    if (VKR_FAILED(vkCreateWin32SurfaceKHR(vkInstance_, &winSurfaceInfo, nullptr, &vkSurface_)))
        return R_FAIL;

    //-----------------------
    // Find physical device
    uint physicalDeviceCount = 32;
    static VkPhysicalDevice physicalDevices[32];
    if (VKR_FAILED(vkEnumeratePhysicalDevices(vkInstance_, &physicalDeviceCount, physicalDevices)))
        return R_FAIL;

    hs_assert(physicalDeviceCount > 0 && physicalDeviceCount < hs_arr_len(physicalDevices));

    int bestDevice = 0;
    // TODO Actually find the best device and check capabilities here
    vkPhysicalDevice_ = physicalDevices[0];

    vkGetPhysicalDeviceProperties(vkPhysicalDevice_, &vkPhysicalDeviceProperties_);

    //-----------------------
    // Queues
    uint queueCount;
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice_, &queueCount, nullptr);

    auto queueProps = (VkQueueFamilyProperties*) VKR_ALLOCA(queueCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice_, &queueCount, queueProps);

    uint directQueueBits = (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    DBG_LOG("Enuerating device queue families, %d found", queueCount);

    for (uint i = 0; i < queueCount; ++i)
    {
        VkBool32 presentSupport{};
        presentSupport = vkGetPhysicalDeviceWin32PresentationSupportKHR(vkPhysicalDevice_, i);
        //VKR_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice_, i, vkSurface_, &presentSupport));
        
        #if HS_DEBUG
            static char buff[512];
            QueueFamiliesToString(queueProps[i].queueFlags, buff);
            Log(LogLevel::Info, "\t%d: count %d, present %d, bits %s", i, queueProps[i].queueCount, presentSupport, buff);
        #endif

        if (directQueueFamilyIdx_ == VKR_INVALID 
            && (queueProps[i].queueFlags & directQueueBits) == directQueueBits
            && queueProps[i].queueCount > 0
            && presentSupport)
        {
            directQueueFamilyIdx_ = i;
        }
    }

    hs_assert(directQueueFamilyIdx_ != VKR_INVALID);

    VkDeviceQueueCreateInfo queues[1]{};
    float prioritites[1]{ 1.0f };
    queues[0].sType             = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queues[0].queueFamilyIndex  = directQueueFamilyIdx_;
    queues[0].queueCount        = 1;
    queues[0].pQueuePriorities  = prioritites;


    //-----------------------
    // Create Vulkan device
    VkPhysicalDeviceFeatures features{};
    vkGetPhysicalDeviceFeatures(vkPhysicalDevice_, &features);
    // TODO check if the device has all required featues

    VkPhysicalDeviceFeatures deviceFeatures = CreateRequiredFeatures();

    const char* deviceExt[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
    };

    VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
    indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    indexingFeatures.descriptorBindingSampledImageUpdateAfterBind   = VK_TRUE;
    indexingFeatures.descriptorBindingPartiallyBound                = VK_TRUE;
    indexingFeatures.descriptorBindingVariableDescriptorCount       = VK_TRUE;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext                    = &indexingFeatures;
    deviceInfo.queueCreateInfoCount     = 1;
    deviceInfo.pQueueCreateInfos        = queues;
    deviceInfo.enabledLayerCount        = instInfo.enabledLayerCount;
    deviceInfo.ppEnabledLayerNames      = instInfo.ppEnabledLayerNames;
    deviceInfo.enabledExtensionCount    = hs_arr_len(deviceExt);
    deviceInfo.ppEnabledExtensionNames  = deviceExt;
    deviceInfo.pEnabledFeatures         = &deviceFeatures;
        
    if (VKR_FAILED(vkCreateDevice(vkPhysicalDevice_, &deviceInfo, nullptr, &vkDevice_)))
        return R_FAIL;

    pfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(vkDevice_, "vkSetDebugUtilsObjectNameEXT");

    vkGetDeviceQueue(vkDevice_, directQueueFamilyIdx_, 0, &vkDirectQueue_);

    //-----------------------
    // Allocator
    {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.instance          = vkInstance_;
        allocatorInfo.physicalDevice    = vkPhysicalDevice_;
        allocatorInfo.device            = vkDevice_;

        if (VKR_FAILED(vmaCreateAllocator(&allocatorInfo, &allocator_)))
            return R_FAIL;
    }

    //-----------------------
    // Create swapchain
    VkBool32 presentSupport{};
    if (VKR_FAILED(vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice_, directQueueFamilyIdx_, vkSurface_, &presentSupport)))
        return R_FAIL;

    if (!presentSupport)
    {
        Log(LogLevel::Error, "Physical device & queue does not support present");
        return R_FAIL;
    }

    uint surfaceFmtCount;
    if (VKR_FAILED(vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice_, vkSurface_, &surfaceFmtCount, nullptr)))
        return R_FAIL;
    auto formats = (VkSurfaceFormatKHR*)VKR_ALLOCA(surfaceFmtCount * sizeof(VkSurfaceFormatKHR));
    if (VKR_FAILED(vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice_, vkSurface_, &surfaceFmtCount, formats)))
        return R_FAIL;

    bool formatFound = false;
    for (uint i = 0; !formatFound && i < surfaceFmtCount; ++i)
    {
        if (formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            formatFound = true;
    }

    if (!formatFound)
    {
        Log(LogLevel::Error, "SRGB nonlinear + B8G8R8A8_SRGB surface format not supported");
        return R_FAIL;
    }

    uint presentModeCount{};
    if (VKR_FAILED(vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice_, vkSurface_, &presentModeCount, nullptr)))
        return R_FAIL;
    auto presentModes = (VkPresentModeKHR*)VKR_ALLOCA(presentModeCount * sizeof(VkPresentModeKHR));
    if (VKR_FAILED(vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice_, vkSurface_, &presentModeCount, presentModes)))
        return R_FAIL;

    bool modeFound = false;
    for (uint i = 0; !modeFound && i < presentModeCount; ++i)
    {
        if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
            modeFound = true;
    }

    if (!modeFound)
    {
        Log(LogLevel::Error, "VK_PRESENT_MODE_IMMEDIATE_KHR not supported");
        return R_FAIL;
    }

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice_, vkSurface_, &capabilities);

    VkSwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.sType             = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface           = vkSurface_;
    swapchainInfo.minImageCount     = 2;
    swapchainInfo.imageFormat       = swapChainFormat_ = VK_FORMAT_B8G8R8A8_SRGB;
    swapchainInfo.imageColorSpace   = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchainInfo.imageExtent       = VkExtent2D{ width_, height_ };
    swapchainInfo.imageArrayLayers  = 1; // Non-stereoscopic view
    swapchainInfo.imageUsage        = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // VK_IMAGE_USAGE_TRANSFER_DST_BIT if not drawing directly to images
    swapchainInfo.imageSharingMode  = VK_SHARING_MODE_EXCLUSIVE; // We assume the same queue will draw and present
    swapchainInfo.preTransform      = capabilities.currentTransform;
    swapchainInfo.compositeAlpha    = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode       = VK_PRESENT_MODE_IMMEDIATE_KHR; //VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    swapchainInfo.clipped           = VK_FALSE; // Just to be safe VK_TRUE if we know we will never read the buffers back

    if (VKR_FAILED(vkCreateSwapchainKHR(vkDevice_, &swapchainInfo, nullptr, &vkSwapchain_)))
        return R_FAIL;

    //-----------------------
    // Create fences
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < BB_IMG_COUNT; ++i)
    {
        if (VKR_FAILED(vkCreateFence(vkDevice_, &fenceInfo, nullptr, &directQueueFences_[i])))
            return R_FAIL;
    }

    fenceInfo.flags = 0;
    if (VKR_FAILED(vkCreateFence(vkDevice_, &fenceInfo, nullptr, &nextImageFence_)))
        return R_FAIL;

    //-----------------------
    // Create semaphores
    VkSemaphoreCreateInfo semaphoreCreate{};
    semaphoreCreate.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (int i = 0; i < BB_IMG_COUNT; ++i)
    {
        if (VKR_FAILED(vkCreateSemaphore(vkDevice_, &semaphoreCreate, nullptr, &submitSemaphores_[i])))
            return R_FAIL;
    }

    //-----------------------
    // Get current back buffer
    uint swapchainImageCount{};
    if (VKR_FAILED(vkGetSwapchainImagesKHR(vkDevice_, vkSwapchain_, &swapchainImageCount, nullptr)))
        return R_FAIL;

    hs_assert(swapchainImageCount < 10);
    DBG_LOG("Swapchain image count: %d", swapchainImageCount);

    if (VKR_FAILED(vkGetSwapchainImagesKHR(vkDevice_, vkSwapchain_, &swapchainImageCount, bbImages_)))
        return R_FAIL;

    if (VKR_FAILED(vkAcquireNextImageKHR(vkDevice_, vkSwapchain_, (uint64)-1, VK_NULL_HANDLE, nextImageFence_, &currentBBIdx_)))
        return R_FAIL;

    if (FAILED(WaitForFence(nextImageFence_)))
        return R_FAIL;

    // All backbuffers are ready except for the current one we will use right away
    if (VKR_FAILED(vkResetFences(vkDevice_, 1, &directQueueFences_[currentBBIdx_])))
        return R_FAIL;

    //-----------------------
    // Create swapchain views
    for (int i = 0; i < BB_IMG_COUNT; ++i)
    {
        VkImageSubresourceRange subresource{};
        subresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource.baseMipLevel    = 0;
        subresource.levelCount      = 1;
        subresource.baseArrayLayer  = 0;
        subresource.layerCount      = 1;

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image             = bbImages_[i];
        imageViewInfo.viewType          = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format            = swapChainFormat_;
        imageViewInfo.components        = VkComponentMapping { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        imageViewInfo.subresourceRange  = subresource;
        
        if (VKR_FAILED(vkCreateImageView(vkDevice_, &imageViewInfo, nullptr, &bbViews_[i])))
            return R_FAIL;
        
        if (VKR_FAILED(SetDiagName(vkDevice_, (uint64)bbImages_[i], VK_OBJECT_TYPE_IMAGE, "BackBuffer")))
            return R_FAIL;

        VkImageCreateInfo depthImageInfo{};
        depthImageInfo.sType            = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        depthImageInfo.imageType        = VK_IMAGE_TYPE_2D;
        depthImageInfo.format           = VK_FORMAT_D24_UNORM_S8_UINT;
        depthImageInfo.extent           = VkExtent3D{ width_, height_, 1 };
        depthImageInfo.mipLevels        = 1;
        depthImageInfo.arrayLayers      = 1;
        depthImageInfo.samples          = VK_SAMPLE_COUNT_1_BIT;
        depthImageInfo.tiling           = VK_IMAGE_TILING_OPTIMAL;
        depthImageInfo.usage            = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depthImageInfo.sharingMode      = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo depthAllocInfo{};
        depthAllocInfo.usage            = VMA_MEMORY_USAGE_GPU_ONLY;

        if (VKR_FAILED(vmaCreateImage(allocator_, &depthImageInfo, &depthAllocInfo, &depthImages_[i], &depthMemory_[i], nullptr)))
            return R_FAIL;

        VkImageViewCreateInfo depthViewInfo{};
        depthViewInfo.sType               = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthViewInfo.image               = depthImages_[i];
        depthViewInfo.viewType            = VK_IMAGE_VIEW_TYPE_2D;
        depthViewInfo.format              = VK_FORMAT_D24_UNORM_S8_UINT;
        
        depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthViewInfo.subresourceRange.baseMipLevel = 0;
        depthViewInfo.subresourceRange.levelCount = 1;
        depthViewInfo.subresourceRange.baseArrayLayer = 0;
        depthViewInfo.subresourceRange.layerCount = 1;

        if (VKR_FAILED(vkCreateImageView(vkDevice_, &depthViewInfo, nullptr, &depthViews_[i])))
            return R_FAIL;
    }

    //-----------------------
    // Create command pools and buffers
    VkCommandPoolCreateInfo directPoolInfo{};
    directPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    directPoolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    directPoolInfo.queueFamilyIndex = directQueueFamilyIdx_;

    if (VKR_FAILED(vkCreateCommandPool(vkDevice_, &directPoolInfo, nullptr, &directCmdPool_)))
        return R_FAIL;

    VkCommandBufferAllocateInfo cmdBufferInfo{};
    cmdBufferInfo.sType                 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferInfo.commandPool           = directCmdPool_;
    cmdBufferInfo.level                 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferInfo.commandBufferCount    = BB_IMG_COUNT;

    if (VKR_FAILED(vkAllocateCommandBuffers(vkDevice_, &cmdBufferInfo, directCmdBuffers_)))
        return R_FAIL;

    for (int i = 0; i < BB_IMG_COUNT; ++i)
    {
        if (VKR_FAILED(SetDiagName(vkDevice_, (uint64)directCmdBuffers_[i], VK_OBJECT_TYPE_COMMAND_BUFFER, "DirectCmdBuffer")))
            return R_FAIL;
    }

    //-----------------------
    // Pipeline layout
    // Basic point
    VkSamplerCreateInfo pointClampInfo{};
    pointClampInfo.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    pointClampInfo.magFilter     = VK_FILTER_NEAREST;
    pointClampInfo.minFilter     = VK_FILTER_NEAREST;
    pointClampInfo.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    pointClampInfo.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    pointClampInfo.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    pointClampInfo.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    pointClampInfo.maxLod        = FLT_MAX;

    VkSampler pointClamp{};
    if (VKR_FAILED(vkCreateSampler(vkDevice_, &pointClampInfo, nullptr, &pointClamp)))
        return R_FAIL;

    VkSampler immutableSamplers[IMMUTABLE_SAMPLER_COUNT] = {
        pointClamp
    };

    // Skybox sampler
    VkSamplerCreateInfo skyboxSampInfo{};
    skyboxSampInfo.sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    skyboxSampInfo.magFilter     = VK_FILTER_LINEAR;
    skyboxSampInfo.minFilter     = VK_FILTER_LINEAR;
    skyboxSampInfo.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    skyboxSampInfo.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    skyboxSampInfo.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    skyboxSampInfo.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    skyboxSampInfo.maxLod        = FLT_MAX;

    VkSampler skyboxSampler{};
    if (VKR_FAILED(vkCreateSampler(vkDevice_, &skyboxSampInfo, nullptr, &skyboxSampler)))
        return R_FAIL;
    
    VkDescriptorSetLayoutBinding bindings[2]{};
    bindings[0].binding             = 0;
    bindings[0].descriptorType      = VK_DESCRIPTOR_TYPE_SAMPLER;
    bindings[0].descriptorCount     = IMMUTABLE_SAMPLER_COUNT;
    bindings[0].stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers  = immutableSamplers;

    bindings[1].binding             = 32;
    bindings[1].descriptorType      = VK_DESCRIPTOR_TYPE_SAMPLER;
    bindings[1].descriptorCount     = 1;
    bindings[1].stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers  = &skyboxSampler;

    VkDescriptorSetLayoutCreateInfo dsLayoutInfo{};
    dsLayoutInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsLayoutInfo.bindingCount   = hs_arr_len(bindings);
    dsLayoutInfo.pBindings      = bindings;

    if (VKR_FAILED(vkCreateDescriptorSetLayout(vkDevice_, &dsLayoutInfo, nullptr, &fsSamplerLayout_)))
        return R_FAIL;

    // Bindless SRV
    VkDescriptorSetLayoutBinding srvBindings[1]{};
    srvBindings[0].binding             = 0;
    srvBindings[0].descriptorType      = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    srvBindings[0].descriptorCount     = 500'000; // Minimum limit
    srvBindings[0].stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorBindingFlags bindingFlags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
    bindingFlagsInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlagsInfo.bindingCount   = 1;
    bindingFlagsInfo.pBindingFlags  = &bindingFlags;

    VkDescriptorSetLayoutCreateInfo bindlessSrv{};
    bindlessSrv.sType           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    bindlessSrv.pNext           = &bindingFlagsInfo;
    bindlessSrv.flags           = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
    bindlessSrv.bindingCount    = hs_arr_len(srvBindings);
    bindlessSrv.pBindings       = srvBindings;
    
    if (VKR_FAILED(vkCreateDescriptorSetLayout(vkDevice_, &bindlessSrv, nullptr, &bindlessTexturesLayout_)))
        return R_FAIL;

    // UBO
    VkDescriptorSetLayoutBinding uboBindings[DYNAMIC_UBO_COUNT + 1]{};
    uboBindings[0].binding             = 0;
    uboBindings[0].descriptorType      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    uboBindings[0].descriptorCount     = 1;
    uboBindings[0].stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

    uboBindings[1] = uboBindings[0];
    uboBindings[1].binding             = 1;

    VkDescriptorSetLayoutCreateInfo dynamicUbo{};
    dynamicUbo.sType           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dynamicUbo.bindingCount    = hs_arr_len(uboBindings);
    dynamicUbo.pBindings       = uboBindings;

    if (VKR_FAILED(vkCreateDescriptorSetLayout(vkDevice_, &dynamicUbo, nullptr, &dynamicUBOLayout_)))
        return R_FAIL;

    VkDescriptorSetLayout descLayouts[] = {
        fsSamplerLayout_,
        bindlessTexturesLayout_,
        dynamicUBOLayout_,
    };

    VkPipelineLayoutCreateInfo plLayoutInfo{};
    plLayoutInfo.setLayoutCount = hs_arr_len(descLayouts);
    plLayoutInfo.pSetLayouts    = descLayouts;

    plLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (VKR_FAILED(vkCreatePipelineLayout(vkDevice_, &plLayoutInfo, nullptr, &pipelineLayout_)))
        return R_FAIL;

    //-----------------------
    // Descriptors
    {
        VkDescriptorPoolSize poolSizes[1]{};
        poolSizes[0].type              = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSizes[0].descriptorCount   = 500'000;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags          = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
        poolInfo.maxSets        = 1;
        poolInfo.poolSizeCount  = hs_arr_len(poolSizes);
        poolInfo.pPoolSizes     = poolSizes;
    
        if (VKR_FAILED(vkCreateDescriptorPool(vkDevice_, &poolInfo, nullptr, &bindlessPool_)))
            return R_FAIL;

        VkDescriptorSetAllocateInfo dsAllocInfo{};
        dsAllocInfo.sType               = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAllocInfo.descriptorPool      = bindlessPool_;
        dsAllocInfo.descriptorSetCount  = 1;
        dsAllocInfo.pSetLayouts         = &bindlessTexturesLayout_;

        if (VKR_FAILED(vkAllocateDescriptorSets(vkDevice_, &dsAllocInfo, &bindlessSet_)))
            return R_FAIL;
    }

    // Static samplers
    {
        VkDescriptorPoolSize immutableSamplerSizes[1]{};
        immutableSamplerSizes[0].type              = VK_DESCRIPTOR_TYPE_SAMPLER;
        immutableSamplerSizes[0].descriptorCount   = 1;

        VkDescriptorPoolCreateInfo immutableSampler{};
        immutableSampler.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        immutableSampler.maxSets        = 1;
        immutableSampler.poolSizeCount  = hs_arr_len(immutableSamplerSizes);
        immutableSampler.pPoolSizes     = immutableSamplerSizes;
    
        if (VKR_FAILED(vkCreateDescriptorPool(vkDevice_, &immutableSampler, nullptr, &immutableSamplerPool_)))
            return R_FAIL;

        VkDescriptorSetAllocateInfo immutSamplerAllocInfo{};
        immutSamplerAllocInfo.sType               = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        immutSamplerAllocInfo.descriptorPool      = immutableSamplerPool_;
        immutSamplerAllocInfo.descriptorSetCount  = 1;
        immutSamplerAllocInfo.pSetLayouts         = &fsSamplerLayout_;

        if (VKR_FAILED(vkAllocateDescriptorSets(vkDevice_, &immutSamplerAllocInfo, &immutableSamplerSet_)))
            return R_FAIL;
    }

    // Dynamic UBO
    {
        VkDescriptorPoolSize dynUboSizes[1]{};
        dynUboSizes[0].type              = VK_DESCRIPTOR_TYPE_SAMPLER;
        dynUboSizes[0].descriptorCount   = DYNAMIC_UBO_COUNT + 1;

        VkDescriptorPoolCreateInfo dynamicUbo{};
        dynamicUbo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        dynamicUbo.maxSets        = 1024;
        dynamicUbo.poolSizeCount  = hs_arr_len(dynUboSizes);
        dynamicUbo.pPoolSizes     = dynUboSizes;

        for (int i = 0; i < BB_IMG_COUNT; ++i)
        {
            if (VKR_FAILED(vkCreateDescriptorPool(vkDevice_, &dynamicUbo, nullptr, &dynamicUBODPool_[i])))
                return R_FAIL;
        }
    }

    //-----------------------
    // Caches
    uboCache_ = MakeUnique<DynamicUBOCache>();
    if (FAILED(uboCache_->Init()))
        return R_FAIL;

    vbCache_ = MakeUnique<VertexBufferCache>();
    if (FAILED(vbCache_->Init()))
        return R_FAIL;

    //-----------------------
    // Init command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VKR_CHECK(vkBeginCommandBuffer(directCmdBuffers_[currentBBIdx_], &beginInfo));

    //-----------------------
    shaderManager_ = MakeUnique<ShaderManager>();
    if (shaderManager_->Init() != R_OK)
        return R_FAIL;

    //-----------------------
    // Material allocation
    //materials_.Add(new SkyboxMaterial());
    //materials_.Add(new TexturedTriangleMaterial());
    //materials_.Add(new ShapeMaterial());
    //materials_.Add(new PhongMaterial());
    //materials_.Add(new TileMaterial());
    
    for (int i = 0; i < materials_.Count(); ++i)
    {
        if (HS_FAILED(materials_[i]->Init()))
        {
            Log(LogLevel::Error, "Failed to init material");
            return R_FAIL;
        }
    }

    camera_.UpdateMatrics();

    // 
    serializationManager_ = MakeUnique<SerializationManager>();
    if (HS_FAILED(serializationManager_->Init()))
        return R_FAIL;

    //drawCanvas_ = MakeUnique<DrawCanvas>();
    if (drawCanvas_ && HS_FAILED(drawCanvas_->Init()))
        return R_FAIL;
    
    tileRenderer_ = MakeUnique<TileRenderer>();
    if (tileRenderer_ && HS_FAILED(tileRenderer_->Init()))
        return R_FAIL;

    debugShapeRenderer_ = MakeUnique<DebugShapeRenderer>();
    if (debugShapeRenderer_ && HS_FAILED(debugShapeRenderer_->Init()))
        return R_FAIL;

    state_.Reset();

    return R_OK;
}

//------------------------------------------------------------------------------
void Render::Free()
{
    FlushGpu(true);
}

//------------------------------------------------------------------------------
void Render::FlushGpu(bool wait)
{
    // TODO implement
}

//------------------------------------------------------------------------------
Render::PipelineKey Render::StateToPipelineKey(const RenderState& state)
{
    PipelineKey key{};

    if (state.shaders_[PS_VERT])
        key |= state.shaders_[PS_VERT]->id_;        // 16 bit
    if (state.shaders_[PS_FRAG])
        key |= state.shaders_[PS_FRAG]->id_ << 16;  // 16 bit

    key |= (uint64)state.vertexLayouts_[0] << 32;   // 10 bit

    key |= (uint64)state.primitiveTopology_ << 42;  // 4 bit

    return key;
}

//------------------------------------------------------------------------------
RESULT Render::PrepareForDraw()
{
    #pragma region Pipeline
    //-------------------
    // Create Pipeline
    VkPipelineShaderStageCreateInfo stages[2]{};
    uint numStages = 0;
    if (state_.shaders_[PS_VERT])
    {
        ++numStages;
        stages[0].sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage     = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module    = state_.shaders_[PS_VERT]->vkShader_;
        stages[0].pName     = "main";
    }
    
    if (state_.shaders_[PS_FRAG])
    {
        ++numStages;
        stages[1].sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage     = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module    = state_.shaders_[PS_FRAG]->vkShader_;
        stages[1].pName     = "main";
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    {
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    }

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    {
        inputAssembly.sType                     = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology                  = (VkPrimitiveTopology)state_.primitiveTopology_;
        inputAssembly.primitiveRestartEnable    = VK_FALSE;
    }

    VkPipelineViewportStateCreateInfo viewportState{};
    VkViewport viewport{};
    VkRect2D scissor{};
    {
        viewport.x          = 0.0f;
        viewport.y          = (float)height_;
        viewport.width      = (float)width_;
        viewport.height     = -(float)height_;
        viewport.minDepth   = 0.0f;
        viewport.maxDepth   = 1.0f;

        scissor.offset = { 0, 0 };
        scissor.extent = VkExtent2D{ width_, height_ };

        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports    = &viewport;
        viewportState.scissorCount  = 1;
        viewportState.pScissors     = &scissor;
    }

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    {
        rasterizer.sType                    = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable         = VK_FALSE; // Is geometry beyond near-far clamped to near far? (if not, it's discarded)
        rasterizer.rasterizerDiscardEnable  = VK_FALSE; // If true everything is discarded.
        rasterizer.polygonMode              = VK_POLYGON_MODE_FILL; // Using any other mode requires enabling GPU feature
        //rasterizer.polygonMode              = VK_POLYGON_MODE_LINE;
        rasterizer.lineWidth                = 1.0f;
        rasterizer.cullMode                 = VK_CULL_MODE_NONE;
        rasterizer.frontFace                = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }

    VkPipelineMultisampleStateCreateInfo multisampling{};
    {
        multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    }

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    {
        depthStencil.sType              = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable    = (state_.depthState_ & DS_TEST) ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable   = (state_.depthState_ & DS_WRITE) ? VK_TRUE : VK_FALSE;;
        depthStencil.depthCompareOp     = VK_COMPARE_OP_LESS;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    {
        colorBlendAttachment.blendEnable            = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor    = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor    = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp           = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor    = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.alphaBlendOp           = VK_BLEND_OP_MAX;
        colorBlendAttachment.colorWriteMask         = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.attachmentCount   = 1;
        colorBlending.pAttachments      = &colorBlendAttachment;
    }

    VkGraphicsPipelineCreateInfo plInfo{};
    plInfo.sType                = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    plInfo.stageCount           = numStages;
    plInfo.pStages              = stages;
    if (state_.vertexLayouts_[0] != RenderState::INVALID_HANDLE)
    {
        plInfo.pVertexInputState    = &vertexLayouts_[state_.vertexLayouts_[0]];
    }
    else
    {
        plInfo.pVertexInputState    = &vertexInputInfo;
    }
    plInfo.pInputAssemblyState  = &inputAssembly;
    plInfo.pViewportState       = &viewportState;
    plInfo.pRasterizationState  = &rasterizer;
    plInfo.pMultisampleState    = &multisampling;
    plInfo.pDepthStencilState   = &depthStencil;
    plInfo.pColorBlendState     = &colorBlending;
    plInfo.layout               = pipelineLayout_;
    plInfo.renderPass           = renderPass_[currentBBIdx_];

    VkPipeline pipeline{};

    PipelineKey plKey = StateToPipelineKey(state_);
    auto cachedPl = pipelineCache_.find(plKey);
    if (cachedPl != pipelineCache_.end())
    {
        pipeline = cachedPl->second;
    }
    else
    {
        VKR_CHECK(vkCreateGraphicsPipelines(vkDevice_, VK_NULL_HANDLE, 1, &plInfo, nullptr, &pipeline));
        pipelineCache_.emplace(plKey, pipeline);
    }
    // End Create Pipeline
    //-------------------
    #pragma endregion

    //-------------------
    // Descriptors
    VkDescriptorSetAllocateInfo dsAllocInfo{};
    dsAllocInfo.sType               = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsAllocInfo.descriptorPool      = dynamicUBODPool_[currentBBIdx_];
    dsAllocInfo.descriptorSetCount  = 1;
    dsAllocInfo.pSetLayouts         = &dynamicUBOLayout_;

    if (VKR_FAILED(vkAllocateDescriptorSets(vkDevice_, &dsAllocInfo, &state_.uboDescSet_)))
        return R_FAIL;

    if (state_.fsDirtyTextures_ || true) // TODO handle the case where no textures are dirty but a UBO descriptor set is needed
    {
        void* mapped;
        state_.bindlessUBO_ = uboCache_->BeginAlloc(sizeof(BindingUBO), &mapped);
        
        auto ubo = (BindingUBO*)mapped;
        for (uint i = 0; i < SRV_SLOT_COUNT; ++i)
        {
            ubo->SRV[i] = state_.fsTextures_[i];
        }
        uboCache_->EndAlloc();
    }

    // Copy descriptors
    VkDescriptorBufferInfo buffInfo[DYNAMIC_UBO_COUNT + 1]{};
    buffInfo[0].buffer = state_.bindlessUBO_.buffer_;
    buffInfo[0].offset = 0;
    buffInfo[0].range  = state_.bindlessUBO_.size_;

    uint dynOffsets[DYNAMIC_UBO_COUNT + 1]{};
    dynOffsets[0] = state_.bindlessUBO_.dynOffset_;
    
    VkWriteDescriptorSet UBOWrites[2]{};
    UBOWrites[0].sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    UBOWrites[0].dstSet             = state_.uboDescSet_;
    UBOWrites[0].dstBinding         = 0;
    UBOWrites[0].dstArrayElement    = 0;
    UBOWrites[0].descriptorCount    = 1;
    UBOWrites[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    UBOWrites[0].pBufferInfo        = buffInfo;

    uint writeCount = 1;

    if (state_.dynamicUBOs_[0].buffer_)
    {
        buffInfo[1].buffer = state_.dynamicUBOs_[0].buffer_;
        buffInfo[1].offset = 0;
        buffInfo[1].range  = state_.dynamicUBOs_[0].size_;

        dynOffsets[1] = state_.dynamicUBOs_[0].dynOffset_;

        ++writeCount;
        UBOWrites[1].sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        UBOWrites[1].dstSet             = state_.uboDescSet_;
        UBOWrites[1].dstBinding         = 1;
        UBOWrites[1].dstArrayElement    = 0;
        UBOWrites[1].descriptorCount    = 1;
        UBOWrites[1].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        UBOWrites[1].pBufferInfo        = &buffInfo[1];
    }


    vkUpdateDescriptorSets(vkDevice_, writeCount, UBOWrites, 0, nullptr);

    VkDescriptorSet descSets[] = {
        immutableSamplerSet_,
        bindlessSet_,
        state_.uboDescSet_,
    };

    vkCmdBindDescriptorSets(CmdBuff(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, hs_arr_len(descSets), descSets, hs_arr_len(dynOffsets), dynOffsets);

    // Vertex buffers
    if (state_.vertexBuffers_[0])
    {
        vkCmdBindVertexBuffers(CmdBuff(), 0, RenderState::MAX_VERT_BUFF, state_.vertexBuffers_, state_.vbOffsets_);
    }

    vkCmdBindPipeline(CmdBuff(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    return R_OK;
}

//------------------------------------------------------------------------------
RESULT Render::AfterDraw()
{
    state_.Reset();

    return R_OK;
}

//------------------------------------------------------------------------------
void Render::Draw(uint vertexCount, uint firstVertex)
{
    if (PrepareForDraw() == R_OK)
        vkCmdDraw(directCmdBuffers_[currentBBIdx_], vertexCount, 1, firstVertex, 0);

    AfterDraw();
}

//------------------------------------------------------------------------------
void Render::Update(float dTime)
{
    //-------------------
    // Create render pass
    VkRenderPass renderPass{};
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format          = swapChainFormat_;
        colorAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp         = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment   = 0;
        colorAttachmentRef.layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        //VkAttachmentDescription depthAttachment{};
        //depthAttachment.format          = VK_FORMAT_D24_UNORM_S8_UINT;
        //depthAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
        //depthAttachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
        //depthAttachment.storeOp         = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        //depthAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        //depthAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        //depthAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
        //depthAttachment.finalLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment   = 1;
        depthAttachmentRef.layout       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &colorAttachmentRef;
        //subpass.pDepthStencilAttachment = &depthAttachmentRef;

        /*VkSubpassDependency dependency{};
        dependency.srcSubpass       = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass       = 0;
        dependency.srcStageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask    = 0;
        dependency.dstStageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;*/

        VkAttachmentDescription attachments[] = { colorAttachment/*, depthAttachment*/ };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = hs_arr_len(attachments);
        renderPassInfo.pAttachments = attachments;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        //renderPassInfo.dependencyCount = 1;
        //renderPassInfo.pDependencies = &dependency;

        VKR_CHECK(vkCreateRenderPass(vkDevice_, &renderPassInfo, nullptr, &renderPass));

        if (renderPass_[currentBBIdx_])
            vkDestroyRenderPass(vkDevice_, renderPass_[currentBBIdx_], nullptr);
        renderPass_[currentBBIdx_] = renderPass;
    }

    //-------------------
    // Create framebuffer
    VkFramebuffer frameBuffer{};
    {
        VkImageView viewAttachments[] = { bbViews_[currentBBIdx_]/*, depthViews_[currentBBIdx_]*/ };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = renderPass;
        framebufferInfo.attachmentCount = hs_arr_len(viewAttachments);
        framebufferInfo.pAttachments    = viewAttachments;
        framebufferInfo.width           = width_;
        framebufferInfo.height          = height_;
        framebufferInfo.layers          = 1;
        
        VKR_CHECK(vkCreateFramebuffer(vkDevice_, &framebufferInfo, nullptr, &frameBuffer));
        
        if (frameBuffer_[currentBBIdx_])
            vkDestroyFramebuffer(vkDevice_, frameBuffer_[currentBBIdx_], nullptr);
        frameBuffer_[currentBBIdx_] = frameBuffer;
    }

    const Color clearColor = Color::ToLinear(Color{ 0.72f, 0.74f, 0.98f, 1.0f });

    VkClearValue clearVal[2] = {};
    clearVal[0].color = VkClearColorValue { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
    clearVal[1].depthStencil =  VkClearDepthStencilValue { 1, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass      = renderPass;
    renderPassBeginInfo.framebuffer     = frameBuffer;
    renderPassBeginInfo.renderArea      = VkRect2D { VkOffset2D { 0, 0 }, VkExtent2D { width_, height_ } };
    renderPassBeginInfo.clearValueCount = hs_arr_len(clearVal);
    renderPassBeginInfo.pClearValues    = clearVal;

    vkCmdBeginRenderPass(directCmdBuffers_[currentBBIdx_], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Before frame start
    //-------------------

    for (int i = 0; i < materials_.Count(); ++i)
    {
        materials_[i]->Draw();
    }

    if (drawCanvas_)
        drawCanvas_->Draw();

    if (tileRenderer_)
        tileRenderer_->Draw();

    if (debugShapeRenderer_)
        debugShapeRenderer_->Draw();

    //-------------------
    // Submit and Present

    vkCmdEndRenderPass(directCmdBuffers_[currentBBIdx_]);

    vkEndCommandBuffer(directCmdBuffers_[currentBBIdx_]);

    VkSubmitInfo submit{};
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &directCmdBuffers_[currentBBIdx_];
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &submitSemaphores_[currentBBIdx_];

    VKR_CHECK(vkQueueSubmit(vkDirectQueue_, 1, &submit, directQueueFences_[currentBBIdx_]));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount      = 1;
    presentInfo.pSwapchains         = &vkSwapchain_;
    presentInfo.pImageIndices       = &currentBBIdx_;
    presentInfo.waitSemaphoreCount  = 1;
    presentInfo.pWaitSemaphores     = &submitSemaphores_[currentBBIdx_];

    VKR_CHECK(vkQueuePresentKHR(vkDirectQueue_, &presentInfo));

    VKR_CHECK(vkAcquireNextImageKHR(vkDevice_, vkSwapchain_, (uint64)-1, nullptr, nextImageFence_, &currentBBIdx_));

    WaitForFence(nextImageFence_);
    WaitForFence(directQueueFences_[currentBBIdx_]);

    vkResetDescriptorPool(vkDevice_, dynamicUBODPool_[currentBBIdx_], 0);

    // Reset kept alive objects
    for (int i = 0; i < destroyPipelines_[currentBBIdx_].Count(); ++i)
        vkDestroyPipeline(vkDevice_, destroyPipelines_[currentBBIdx_][i], nullptr);
    destroyPipelines_[currentBBIdx_].Clear();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VKR_CHECK(vkBeginCommandBuffer(directCmdBuffers_[currentBBIdx_], &beginInfo));

    ++frame_;
}

//------------------------------------------------------------------------------
VkDevice Render::GetDevice() const
{
    return vkDevice_;
}

//------------------------------------------------------------------------------
VmaAllocator Render::GetAllocator() const
{
    return allocator_;
}

//------------------------------------------------------------------------------
ShaderManager* Render::GetShaderManager() const
{
    return shaderManager_.Get();
}

//------------------------------------------------------------------------------
VkCommandBuffer Render::CmdBuff() const
{
    return directCmdBuffers_[currentBBIdx_];
}

//------------------------------------------------------------------------------
uint64 Render::GetCurrentFrame() const
{
    return frame_;
}

//------------------------------------------------------------------------------
uint64 Render::GetSafeFrame() const
{
    return frame_ + 2;
}

//------------------------------------------------------------------------------
const VkPhysicalDeviceProperties& Render::GetPhysDevProps() const
{
    return vkPhysicalDeviceProperties_;
}

//------------------------------------------------------------------------------
DynamicUBOCache* Render::GetUBOCache() const
{
    return uboCache_.Get();
}

//------------------------------------------------------------------------------
VertexBufferCache* Render::GetVertexCache() const
{
    return vbCache_.Get();
}

//------------------------------------------------------------------------------
uint Render::GetWidth() const
{
    return width_;
}

//------------------------------------------------------------------------------
uint Render::GetHeight() const
{
    return height_;
}

//------------------------------------------------------------------------------
float Render::GetAspect() const
{
    return 1.0f * width_ / height_;
}

//------------------------------------------------------------------------------
void Render::ResetState()
{
    state_.Reset();
}

//------------------------------------------------------------------------------
void Render::SetTexture(uint slot, Texture* texture)
{
    uint texIdx = texture->GetBindlessIndex();
    if (state_.fsTextures_[slot] == texIdx)
        return;

    state_.fsTextures_[slot] = texIdx;
    state_.fsDirtyTextures_ |= (uint64) 1 << slot;
}

//------------------------------------------------------------------------------
void Render::SetVertexBuffer(uint slot, const VertexBufferEntry& entry)
{
    hs_assert(slot < RenderState::MAX_VERT_BUFF);

    state_.vertexBuffers_[slot] = entry.buffer_;
    state_.vbOffsets_[slot] = entry.offset_;
}

//------------------------------------------------------------------------------
void Render::SetPrimitiveTopology(VkrPrimitiveTopology primitiveTopology)
{
    state_.primitiveTopology_ = primitiveTopology;
}

//------------------------------------------------------------------------------
void Render::SetVertexLayout(uint slot, uint layoutHandle)
{
    state_.vertexLayouts_[slot] = layoutHandle;
}

//------------------------------------------------------------------------------
void Render::SetDynamicUbo(uint slot, const DynamicUBOEntry& entry)
{
    hs_assert(slot < DYNAMIC_UBO_COUNT + 1);

    state_.dynamicUBOs_[slot - 1] = entry;
}

//------------------------------------------------------------------------------
void Render::SetDepthState(uint state)
{
    state_.depthState_ = state;
}

//------------------------------------------------------------------------------
uint Render::AddBindlessTexture(VkImageView view)
{
    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imgInfo.imageView   = view;

    VkWriteDescriptorSet texWrite{};
    texWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    texWrite.dstSet           = bindlessSet_;
    texWrite.dstBinding       = 0;
    texWrite.dstArrayElement  = lastFreeBindlessIndex_++;
    texWrite.descriptorCount  = 1;
    texWrite.descriptorType   = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    texWrite.pImageInfo       = &imgInfo;

    vkUpdateDescriptorSets(vkDevice_, 1, &texWrite, 0, nullptr);

    return texWrite.dstArrayElement;
}

//------------------------------------------------------------------------------
void RenderState::Reset()
{
    for (int i = 0; i < PS_COUNT; ++i)
        shaders_[i] = {};

    for (uint i = 0; i < SRV_SLOT_COUNT; ++i)
        fsTextures_[i] = RenderState::INVALID_DESC;

    for (int i = 0; i < DYNAMIC_UBO_COUNT; ++i)
        dynamicUBOs_[i] = {};

    fsDirtyTextures_ = 0;

    for (int i = 0; i < MAX_VERT_BUFF; ++i)
    {
        vertexBuffers_[i] = {};
        vbOffsets_[i] = {};
        vertexLayouts_[i] = INVALID_HANDLE;
    }

    primitiveTopology_ = VkrPrimitiveTopology::TRIANGLE_LIST;
    
    cullMode_ = VkrCullMode::Back;
    depthState_ = DS_TEST | DS_WRITE;
}

//------------------------------------------------------------------------------
uint Render::GetOrCreateVertexLayout(VkPipelineVertexInputStateCreateInfo info)
{
    for (int i = 0; i < vertexLayouts_.Count(); ++i)
    {
        if (memcmp(&vertexLayouts_[i], &info, sizeof(VkPipelineVertexInputStateCreateInfo)) == 0)
            return i;
    }

    vertexLayouts_.Add(info);
    return vertexLayouts_.Count() - 1;
}

//------------------------------------------------------------------------------
const Camera& Render::GetCamera() const
{
    return camera_;
}

//------------------------------------------------------------------------------
Camera& Render::GetCamera()
{
    return camera_;
}

//------------------------------------------------------------------------------
TileRenderer* Render::GetTileRenderer() const
{
    return tileRenderer_.Get();
}

//------------------------------------------------------------------------------
DebugShapeRenderer* Render::GetDebugShapeRenderer() const
{
    return debugShapeRenderer_.Get();
}

}
