#include <Core/CoreMinimal.h>
#include <spirv_cross/spirv_cross.hpp>

#if K3DCOMPILER_MSVC
#pragma warning(disable:4267)
#endif

#if K3DPLATFORM_OS_WIN
#define VK_USE_PLATFORM_WIN32_KHR 1
#elif K3DPLATFORM_OS_ANDROID
#define VK_USE_PLATFORM_ANDROID_KHR 1
#endif
#include <vulkan/vulkan.h>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "ngfx.h"
#include "vulkan_glslang.h"
#include <vector>
#include <list>
#include <set>

using namespace ngfx;
using namespace std;

using k3d::os::File;
using k3d::os::MemMapFile;

#define VULKAN_ALLOCATOR nullptr

static vector<const char*> RequiredLayers =
{
  "VK_LAYER_LUNARG_standard_validation"
};

static vector<const char*> RequiredInstanceExtensions =
{
  VK_KHR_SURFACE_EXTENSION_NAME,
  VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
  VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
};

static std::vector<const char *> RequiredDeviceExtensions =
{
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  VK_KHX_EXTERNAL_MEMORY_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
  VK_KHX_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
#endif
};

enum class Log
{
    Debug,
    Info,
    Warn,
    Error,
};

void LogPrint(Log const& i, const char* Tag, const char* Fmt, ...)
{
#if K3DPLATFORM_OS_WIN
    static char Buffer[1024] = { 0 };
    va_list va;
    va_start(va, Fmt);
    vsnprintf_s(Buffer, 1024, Fmt, va);
    va_end(va);
    OutputDebugStringA(Buffer);
#endif
}

const char * VulkanError(VkResult Result)
{
#define ERROR_STR(X) case VK_##X: return #X;
    switch (Result)
    {
        ERROR_STR(SUCCESS)
            ERROR_STR(NOT_READY)
            ERROR_STR(TIMEOUT)
            ERROR_STR(EVENT_SET)
            ERROR_STR(EVENT_RESET)
            ERROR_STR(INCOMPLETE)
            ERROR_STR(ERROR_OUT_OF_HOST_MEMORY)
            ERROR_STR(ERROR_OUT_OF_DEVICE_MEMORY)
            ERROR_STR(ERROR_INITIALIZATION_FAILED)
            ERROR_STR(ERROR_DEVICE_LOST)
            ERROR_STR(ERROR_LAYER_NOT_PRESENT)
            ERROR_STR(ERROR_EXTENSION_NOT_PRESENT)
            ERROR_STR(ERROR_MEMORY_MAP_FAILED)
            ERROR_STR(ERROR_FEATURE_NOT_PRESENT)
            ERROR_STR(ERROR_INCOMPATIBLE_DRIVER)
            ERROR_STR(ERROR_TOO_MANY_OBJECTS)
            ERROR_STR(ERROR_FORMAT_NOT_SUPPORTED)
            ERROR_STR(ERROR_FRAGMENTED_POOL)
            ERROR_STR(ERROR_SURFACE_LOST_KHR)
            ERROR_STR(ERROR_NATIVE_WINDOW_IN_USE_KHR)
            ERROR_STR(ERROR_OUT_OF_DATE_KHR)
            ERROR_STR(ERROR_INCOMPATIBLE_DISPLAY_KHR)
            ERROR_STR(ERROR_VALIDATION_FAILED_EXT)
            ERROR_STR(ERROR_OUT_OF_POOL_MEMORY_KHR)
            ERROR_STR(ERROR_INVALID_EXTERNAL_HANDLE_KHX)
    }
    return "Unknown";
#undef ERROR_STR
}

const char * StrVkFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_UNDEFINED: return "VK_FORMAT_UNDEFINED";
    case VK_FORMAT_R4G4_UNORM_PACK8: return "VK_FORMAT_R4G4_UNORM_PACK8";
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16: return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16: return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
    case VK_FORMAT_R5G6B5_UNORM_PACK16: return "VK_FORMAT_R5G6B5_UNORM_PACK16";
    case VK_FORMAT_B5G6R5_UNORM_PACK16: return "VK_FORMAT_B5G6R5_UNORM_PACK16";
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16: return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16: return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16: return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
    case VK_FORMAT_R8_UNORM: return "VK_FORMAT_R8_UNORM";
    case VK_FORMAT_R8_SNORM: return "VK_FORMAT_R8_SNORM";
    case VK_FORMAT_R8_USCALED: return "VK_FORMAT_R8_USCALED";
    case VK_FORMAT_R8_SSCALED: return "VK_FORMAT_R8_SSCALED";
    case VK_FORMAT_R8_UINT: return "VK_FORMAT_R8_UINT";
    case VK_FORMAT_R8_SINT: return "VK_FORMAT_R8_SINT";
    case VK_FORMAT_R8_SRGB: return "VK_FORMAT_R8_SRGB";
    case VK_FORMAT_R8G8_UNORM: return "VK_FORMAT_R8G8_UNORM";
    case VK_FORMAT_R8G8_SNORM: return "VK_FORMAT_R8G8_SNORM";
    case VK_FORMAT_R8G8_USCALED: return "VK_FORMAT_R8G8_USCALED";
    case VK_FORMAT_R8G8_SSCALED: return "VK_FORMAT_R8G8_SSCALED";
    case VK_FORMAT_R8G8_UINT: return "VK_FORMAT_R8G8_UINT";
    case VK_FORMAT_R8G8_SINT: return "VK_FORMAT_R8G8_SINT";
    case VK_FORMAT_R8G8_SRGB: return "VK_FORMAT_R8G8_SRGB";
    case VK_FORMAT_R8G8B8_UNORM: return "VK_FORMAT_R8G8B8_UNORM";
    case VK_FORMAT_R8G8B8_SNORM: return "VK_FORMAT_R8G8B8_SNORM";
    case VK_FORMAT_R8G8B8_USCALED: return "VK_FORMAT_R8G8B8_USCALED";
    case VK_FORMAT_R8G8B8_SSCALED: return "VK_FORMAT_R8G8B8_SSCALED";
    case VK_FORMAT_R8G8B8_UINT: return "VK_FORMAT_R8G8B8_UINT";
    case VK_FORMAT_R8G8B8_SINT: return "VK_FORMAT_R8G8B8_SINT";
    case VK_FORMAT_R8G8B8_SRGB: return "VK_FORMAT_R8G8B8_SRGB";
    case VK_FORMAT_B8G8R8_UNORM: return "VK_FORMAT_B8G8R8_UNORM";
    case VK_FORMAT_B8G8R8_SNORM: return "VK_FORMAT_B8G8R8_SNORM";
    case VK_FORMAT_B8G8R8_USCALED: return "VK_FORMAT_B8G8R8_USCALED";
    case VK_FORMAT_B8G8R8_SSCALED: return "VK_FORMAT_B8G8R8_SSCALED";
    case VK_FORMAT_B8G8R8_UINT: return "VK_FORMAT_B8G8R8_UINT";
    case VK_FORMAT_B8G8R8_SINT: return "VK_FORMAT_B8G8R8_SINT";
    case VK_FORMAT_B8G8R8_SRGB: return "VK_FORMAT_B8G8R8_SRGB";
    case VK_FORMAT_R8G8B8A8_UNORM: return "VK_FORMAT_R8G8B8A8_UNORM";
    case VK_FORMAT_R8G8B8A8_SNORM: return "VK_FORMAT_R8G8B8A8_SNORM";
    case VK_FORMAT_R8G8B8A8_USCALED: return "VK_FORMAT_R8G8B8A8_USCALED";
    case VK_FORMAT_R8G8B8A8_SSCALED: return "VK_FORMAT_R8G8B8A8_SSCALED";
    case VK_FORMAT_R8G8B8A8_UINT: return "VK_FORMAT_R8G8B8A8_UINT";
    case VK_FORMAT_R8G8B8A8_SINT: return "VK_FORMAT_R8G8B8A8_SINT";
    case VK_FORMAT_R8G8B8A8_SRGB: return "VK_FORMAT_R8G8B8A8_SRGB";
    case VK_FORMAT_B8G8R8A8_UNORM: return "VK_FORMAT_B8G8R8A8_UNORM";
    case VK_FORMAT_B8G8R8A8_SNORM: return "VK_FORMAT_B8G8R8A8_SNORM";
    case VK_FORMAT_B8G8R8A8_USCALED: return "VK_FORMAT_B8G8R8A8_USCALED";
    case VK_FORMAT_B8G8R8A8_SSCALED: return "VK_FORMAT_B8G8R8A8_SSCALED";
    case VK_FORMAT_B8G8R8A8_UINT: return "VK_FORMAT_B8G8R8A8_UINT";
    case VK_FORMAT_B8G8R8A8_SINT: return "VK_FORMAT_B8G8R8A8_SINT";
    case VK_FORMAT_B8G8R8A8_SRGB: return "VK_FORMAT_B8G8R8A8_SRGB";
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32: return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32: return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32: return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
    case VK_FORMAT_A8B8G8R8_UINT_PACK32: return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
    case VK_FORMAT_A8B8G8R8_SINT_PACK32: return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32: return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32: return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32: return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32: return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
    case VK_FORMAT_A2R10G10B10_UINT_PACK32: return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
    case VK_FORMAT_A2R10G10B10_SINT_PACK32: return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32: return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32: return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
    case VK_FORMAT_A2B10G10R10_UINT_PACK32: return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
    case VK_FORMAT_A2B10G10R10_SINT_PACK32: return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
    case VK_FORMAT_R16_UNORM: return "VK_FORMAT_R16_UNORM";
    case VK_FORMAT_R16_SNORM: return "VK_FORMAT_R16_SNORM";
    case VK_FORMAT_R16_USCALED: return "VK_FORMAT_R16_USCALED";
    case VK_FORMAT_R16_SSCALED: return "VK_FORMAT_R16_SSCALED";
    case VK_FORMAT_R16_UINT: return "VK_FORMAT_R16_UINT";
    case VK_FORMAT_R16_SINT: return "VK_FORMAT_R16_SINT";
    case VK_FORMAT_R16_SFLOAT: return "VK_FORMAT_R16_SFLOAT";
    case VK_FORMAT_R16G16_UNORM: return "VK_FORMAT_R16G16_UNORM";
    case VK_FORMAT_R16G16_SNORM: return "VK_FORMAT_R16G16_SNORM";
    case VK_FORMAT_R16G16_USCALED: return "VK_FORMAT_R16G16_USCALED";
    case VK_FORMAT_R16G16_SSCALED: return "VK_FORMAT_R16G16_SSCALED";
    case VK_FORMAT_R16G16_UINT: return "VK_FORMAT_R16G16_UINT";
    case VK_FORMAT_R16G16_SINT: return "VK_FORMAT_R16G16_SINT";
    case VK_FORMAT_R16G16_SFLOAT: return "VK_FORMAT_R16G16_SFLOAT";
    case VK_FORMAT_R16G16B16_UNORM: return "VK_FORMAT_R16G16B16_UNORM";
    case VK_FORMAT_R16G16B16_SNORM: return "VK_FORMAT_R16G16B16_SNORM";
    case VK_FORMAT_R16G16B16_USCALED: return "VK_FORMAT_R16G16B16_USCALED";
    case VK_FORMAT_R16G16B16_SSCALED: return "VK_FORMAT_R16G16B16_SSCALED";
    case VK_FORMAT_R16G16B16_UINT: return "VK_FORMAT_R16G16B16_UINT";
    case VK_FORMAT_R16G16B16_SINT: return "VK_FORMAT_R16G16B16_SINT";
    case VK_FORMAT_R16G16B16_SFLOAT: return "VK_FORMAT_R16G16B16_SFLOAT";
    case VK_FORMAT_R16G16B16A16_UNORM: return "VK_FORMAT_R16G16B16A16_UNORM";
    case VK_FORMAT_R16G16B16A16_SNORM: return "VK_FORMAT_R16G16B16A16_SNORM";
    case VK_FORMAT_R16G16B16A16_USCALED: return "VK_FORMAT_R16G16B16A16_USCALED";
    case VK_FORMAT_R16G16B16A16_SSCALED: return "VK_FORMAT_R16G16B16A16_SSCALED";
    case VK_FORMAT_R16G16B16A16_UINT: return "VK_FORMAT_R16G16B16A16_UINT";
    case VK_FORMAT_R16G16B16A16_SINT: return "VK_FORMAT_R16G16B16A16_SINT";
    case VK_FORMAT_R16G16B16A16_SFLOAT: return "VK_FORMAT_R16G16B16A16_SFLOAT";
    case VK_FORMAT_R32_UINT: return "VK_FORMAT_R32_UINT";
    case VK_FORMAT_R32_SINT: return "VK_FORMAT_R32_SINT";
    case VK_FORMAT_R32_SFLOAT: return "VK_FORMAT_R32_SFLOAT";
    case VK_FORMAT_R32G32_UINT: return "VK_FORMAT_R32G32_UINT";
    case VK_FORMAT_R32G32_SINT: return "VK_FORMAT_R32G32_SINT";
    case VK_FORMAT_R32G32_SFLOAT: return "VK_FORMAT_R32G32_SFLOAT";
    case VK_FORMAT_R32G32B32_UINT: return "VK_FORMAT_R32G32B32_UINT";
    case VK_FORMAT_R32G32B32_SINT: return "VK_FORMAT_R32G32B32_SINT";
    case VK_FORMAT_R32G32B32_SFLOAT: return "VK_FORMAT_R32G32B32_SFLOAT";
    case VK_FORMAT_R32G32B32A32_UINT: return "VK_FORMAT_R32G32B32A32_UINT";
    case VK_FORMAT_R32G32B32A32_SINT: return "VK_FORMAT_R32G32B32A32_SINT";
    case VK_FORMAT_R32G32B32A32_SFLOAT: return "VK_FORMAT_R32G32B32A32_SFLOAT";
    case VK_FORMAT_R64_UINT: return "VK_FORMAT_R64_UINT";
    case VK_FORMAT_R64_SINT: return "VK_FORMAT_R64_SINT";
    case VK_FORMAT_R64_SFLOAT: return "VK_FORMAT_R64_SFLOAT";
    case VK_FORMAT_R64G64_UINT: return "VK_FORMAT_R64G64_UINT";
    case VK_FORMAT_R64G64_SINT: return "VK_FORMAT_R64G64_SINT";
    case VK_FORMAT_R64G64_SFLOAT: return "VK_FORMAT_R64G64_SFLOAT";
    case VK_FORMAT_R64G64B64_UINT: return "VK_FORMAT_R64G64B64_UINT";
    case VK_FORMAT_R64G64B64_SINT: return "VK_FORMAT_R64G64B64_SINT";
    case VK_FORMAT_R64G64B64_SFLOAT: return "VK_FORMAT_R64G64B64_SFLOAT";
    case VK_FORMAT_R64G64B64A64_UINT: return "VK_FORMAT_R64G64B64A64_UINT";
    case VK_FORMAT_R64G64B64A64_SINT: return "VK_FORMAT_R64G64B64A64_SINT";
    case VK_FORMAT_R64G64B64A64_SFLOAT: return "VK_FORMAT_R64G64B64A64_SFLOAT";
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
    case VK_FORMAT_D16_UNORM: return "VK_FORMAT_D16_UNORM";
    case VK_FORMAT_X8_D24_UNORM_PACK32: return "VK_FORMAT_X8_D24_UNORM_PACK32";
    case VK_FORMAT_D32_SFLOAT: return "VK_FORMAT_D32_SFLOAT";
    case VK_FORMAT_S8_UINT: return "VK_FORMAT_S8_UINT";
    case VK_FORMAT_D16_UNORM_S8_UINT: return "VK_FORMAT_D16_UNORM_S8_UINT";
    case VK_FORMAT_D24_UNORM_S8_UINT: return "VK_FORMAT_D24_UNORM_S8_UINT";
    case VK_FORMAT_D32_SFLOAT_S8_UINT: return "VK_FORMAT_D32_SFLOAT_S8_UINT";
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK: return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK: return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
    case VK_FORMAT_BC2_UNORM_BLOCK: return "VK_FORMAT_BC2_UNORM_BLOCK";
    case VK_FORMAT_BC2_SRGB_BLOCK: return "VK_FORMAT_BC2_SRGB_BLOCK";
    case VK_FORMAT_BC3_UNORM_BLOCK: return "VK_FORMAT_BC3_UNORM_BLOCK";
    case VK_FORMAT_BC3_SRGB_BLOCK: return "VK_FORMAT_BC3_SRGB_BLOCK";
    case VK_FORMAT_BC4_UNORM_BLOCK: return "VK_FORMAT_BC4_UNORM_BLOCK";
    case VK_FORMAT_BC4_SNORM_BLOCK: return "VK_FORMAT_BC4_SNORM_BLOCK";
    case VK_FORMAT_BC5_UNORM_BLOCK: return "VK_FORMAT_BC5_UNORM_BLOCK";
    case VK_FORMAT_BC5_SNORM_BLOCK: return "VK_FORMAT_BC5_SNORM_BLOCK";
    case VK_FORMAT_BC6H_UFLOAT_BLOCK: return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
    case VK_FORMAT_BC6H_SFLOAT_BLOCK: return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
    case VK_FORMAT_BC7_UNORM_BLOCK: return "VK_FORMAT_BC7_UNORM_BLOCK";
    case VK_FORMAT_BC7_SRGB_BLOCK: return "VK_FORMAT_BC7_SRGB_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
    case VK_FORMAT_EAC_R11_UNORM_BLOCK: return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
    case VK_FORMAT_EAC_R11_SNORM_BLOCK: return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
    }
    return "Unknown VkFormat";
}

const char* DeviceType(VkPhysicalDeviceType DType)
{
    switch (DType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return "Discrete GPU";
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return "Integrated GPU";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return "Virtual GPU";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return "CPU";
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
    default:
        return "Unknown";
    }

}

#define CHECK(Ret) \
  if (Ret != VK_SUCCESS) \
  {\
    LogPrint(Log::Error, "CheckRet", "%s !!\n\tReturn Error: %s @line: %d @file: %s.\n", #Ret, VulkanError(Ret), __LINE__, __FILE__); \
  }

VkShaderStageFlagBits ConvertShaderTypeToVulkanEnum(ShaderType const& e) {
    switch (e) {
    case ShaderType::Vertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderType::Fragment:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderType::Compute:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    case ShaderType::Geometry:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    case ShaderType::TessailationEval:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case ShaderType::TessailationControl:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    }
    return VK_SHADER_STAGE_ALL;
}

VkVertexInputRate ConvertVertexInputRateToVulkanEnum(VertexInputRate const& e) {
    switch (e) {
    case VertexInputRate::PerVertex:
        return VK_VERTEX_INPUT_RATE_VERTEX;
    case VertexInputRate::PerInstance:
        return VK_VERTEX_INPUT_RATE_INSTANCE;
    }
    return VK_VERTEX_INPUT_RATE_MAX_ENUM;
}


VkShaderStageFlags ConvertShaderStageBit(ShaderStageBit visibility)
{
    EnumAsUint32<ShaderStageBit> Enum(visibility);
    VkShaderStageFlags Flag = 0;
    if (Enum & ShaderStageBit::Vertex)
    {
        Flag |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (Enum & ShaderStageBit::Compute)
    {
        Flag |= VK_SHADER_STAGE_COMPUTE_BIT;
    }
    if (Enum & ShaderStageBit::Fragment)
    {
        Flag |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if (Enum & ShaderStageBit::Geometry)
    {
        Flag |= VK_SHADER_STAGE_GEOMETRY_BIT;
    }
    return Flag;
}

VkFormat g_VertexFormatTable[] = {
  VK_FORMAT_R32_SFLOAT,
  VK_FORMAT_R32G32_SFLOAT,
  VK_FORMAT_R32G32B32_SFLOAT,
  VK_FORMAT_R32G32B32A32_SFLOAT };

VkLogicOp ConvertLogicOperationToVulkanEnum(LogicOperation const& e) {
    switch (e) {
    case LogicOperation::Clear:
        return VK_LOGIC_OP_CLEAR;
    case LogicOperation::And:
        return VK_LOGIC_OP_AND;
    case LogicOperation::Xor:
        return VK_LOGIC_OP_XOR;
    case LogicOperation::Or:
        return VK_LOGIC_OP_OR;
    case LogicOperation::Nor:
        return VK_LOGIC_OP_NOR;
    case LogicOperation::Invert:
        return VK_LOGIC_OP_INVERT;
    }
    return VK_LOGIC_OP_MAX_ENUM;
}

VkFormat ConvertPixelFormatToVulkanEnum(PixelFormat const& e) {
    switch (e) {
    case PixelFormat::RGBA16Uint:
        return VK_FORMAT_R16G16B16A16_UINT;
    case PixelFormat::RGBA32Float:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case PixelFormat::RGBA8UNorm:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case PixelFormat::RGBA8UNorm_sRGB:
        return VK_FORMAT_R8G8B8A8_SNORM;
    case PixelFormat::R11G11B10Float:
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case PixelFormat::D32Float:
        return VK_FORMAT_D32_SFLOAT;
    case PixelFormat::RGB32Float:
        return VK_FORMAT_R32G32B32_SFLOAT;
    }
    return VK_FORMAT_MAX_ENUM;
}
VkAttachmentLoadOp ConvertLoadActionToVulkanEnum(LoadAction const& e) {
    switch (e) {
    case LoadAction::Load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case LoadAction::Clear:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    case LoadAction::DontCare:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    }
    return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
}
VkAttachmentStoreOp ConvertStoreActionToVulkanEnum(StoreAction const& e) {
    switch (e) {
    case StoreAction::Store:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case StoreAction::DontCare:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
}
VkPrimitiveTopology ConvertPrimitiveTypeToVulkanEnum(PrimitiveType const& e) {
    switch (e) {
    case PrimitiveType::Points:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case PrimitiveType::Lines:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case PrimitiveType::Triangles:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case PrimitiveType::TriangleStrips:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    }
    return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
}
VkBlendOp ConvertBlendOperationToVulkanEnum(BlendOperation const& e) {
    switch (e) {
    case BlendOperation::Add:
        return VK_BLEND_OP_ADD;
    case BlendOperation::Sub:
        return VK_BLEND_OP_SUBTRACT;
    }
    return VK_BLEND_OP_MAX_ENUM;
}
VkBlendFactor ConvertBlendTypeToVulkanEnum(BlendType const& e) {
    switch (e) {
    case BlendType::Zero:
        return VK_BLEND_FACTOR_ZERO;
    case BlendType::One:
        return VK_BLEND_FACTOR_ONE;
    case BlendType::SrcColor:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case BlendType::DstColor:
        return VK_BLEND_FACTOR_DST_COLOR;
    case BlendType::SrcAlpha:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case BlendType::DstAlpha:
        return VK_BLEND_FACTOR_DST_ALPHA;
    }
    return VK_BLEND_FACTOR_MAX_ENUM;
}
VkStencilOp ConvertStencilOperationToVulkanEnum(StencilOperation const& e) {
    switch (e) {
    case StencilOperation::Keep:
        return VK_STENCIL_OP_KEEP;
    case StencilOperation::Zero:
        return VK_STENCIL_OP_ZERO;
    case StencilOperation::Replace:
        return VK_STENCIL_OP_REPLACE;
    case StencilOperation::Invert:
        return VK_STENCIL_OP_INVERT;
    case StencilOperation::Increment:
        return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case StencilOperation::Decrement:
        return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    }
    return VK_STENCIL_OP_MAX_ENUM;
}
VkCompareOp ConvertComparisonFunctionToVulkanEnum(ComparisonFunction const& e) {
    switch (e) {
    case ComparisonFunction::Never:
        return VK_COMPARE_OP_NEVER;
    case ComparisonFunction::Less:
        return VK_COMPARE_OP_LESS;
    case ComparisonFunction::Equal:
        return VK_COMPARE_OP_EQUAL;
    case ComparisonFunction::LessEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case ComparisonFunction::Greater:
        return VK_COMPARE_OP_GREATER;
    case ComparisonFunction::NotEqual:
        return VK_COMPARE_OP_NOT_EQUAL;
    case ComparisonFunction::GreaterEqual:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case ComparisonFunction::Always:
        return VK_COMPARE_OP_ALWAYS;
    }
    return VK_COMPARE_OP_MAX_ENUM;
}
VkPolygonMode ConvertFillModeToVulkanEnum(FillMode const& e) {
    switch (e) {
    case FillMode::Wire:
        return VK_POLYGON_MODE_LINE;
    case FillMode::Solid:
        return VK_POLYGON_MODE_FILL;
    }
    return VK_POLYGON_MODE_MAX_ENUM;
}

VkCullModeFlagBits ConvertCullModeToVulkanEnum(CullMode const& e) {
    switch (e) {
    case CullMode::None:
        return VK_CULL_MODE_NONE;
    case CullMode::Front:
        return VK_CULL_MODE_FRONT_BIT;
    case CullMode::Back:
        return VK_CULL_MODE_BACK_BIT;
    }
    return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
}
VkFilter ConvertFilterModeToVulkanEnum(FilterMode const& e) {
    switch (e) {
    case FilterMode::Point:
        return VK_FILTER_NEAREST;
    case FilterMode::Linear:
        return VK_FILTER_LINEAR;
    }
    return VK_FILTER_MAX_ENUM;
}
VkSamplerAddressMode ConvertAddressModeToVulkanEnum(AddressMode const& e) {
    switch (e) {
    case AddressMode::Wrap:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case AddressMode::Mirror:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case AddressMode::Clamp:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case AddressMode::Border:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case AddressMode::MirrorOnce:
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    }
    return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
}

VkImageViewType ConvertTextureDimensionToVulkanEnum(TextureDimension const& e) {
    switch (e) {
    case TextureDimension::Tex1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case TextureDimension::Tex2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case TextureDimension::Tex2DMS:
        return VK_IMAGE_VIEW_TYPE_2D;
    case TextureDimension::Tex2DArray:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case TextureDimension::Tex3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    case TextureDimension::Tex3DArray:
        return VK_IMAGE_VIEW_TYPE_3D;
    case TextureDimension::TexCube:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    }
    return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
}

VkSampleCountFlagBits ConvertMultiSampleFlagToVulkanEnum(MultiSampleFlag const& e) {
    switch (e) {
    case MultiSampleFlag::MS1x:
        return VK_SAMPLE_COUNT_1_BIT;
    case MultiSampleFlag::MS2x:
        return VK_SAMPLE_COUNT_2_BIT;
    case MultiSampleFlag::MS4x:
        return VK_SAMPLE_COUNT_4_BIT;
    case MultiSampleFlag::MS8x:
        return VK_SAMPLE_COUNT_8_BIT;
    case MultiSampleFlag::MS16x:
        return VK_SAMPLE_COUNT_16_BIT;
    }
    return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
}

using ByteCode = std::vector<uint32_t>;

namespace spirv_cross
{
    class Variable : public ngfx::Variable
    {
    public:
        const char *    Name() override { return name.c_str(); }
        ArgumentAccess  Access() override;
        VariableType *  Type() override { return type; }
        uint32_t        Index() override { return id; }
        bool            Active() override { return active; }
        ShaderType      GetStage() const override { return ShaderType::Compute; }

        std::string     name;
        VariableType*   type;
        uint32_t        id;
        bool            active;
    };

    class ArrayType : public ngfx::ArrayType
    {
    public:
        DataType      GetType() override { return DataType::Array; }

        uint32_t      Length() override { return length; }
        DataType      ElementType() override { return elementType; }
        uint32_t      Stride() override { return stride; }
        VariableType* Elements() override { return elementType_; }

        uint32_t      length;
        uint32_t      stride;
        DataType      elementType;
        VariableType* elementType_;
    };

    class StructType : public ngfx::StructType
    {
    public:
        DataType      GetType() override { return DataType::Struct; }

    };

    class PointerType : public ngfx::PointerType
    {
    public:
        DataType        GetType() override { return DataType::Pointer; }

        ArgumentAccess  Access() override { return access; }
        uint32_t        Alignment() override { return alignment; }
        uint32_t        DataSize() override { return dataSize; }
        DataType        ElementType() override { return dataType; }

        PointerType() = default;

        ArgumentAccess  access = ArgumentAccess::ReadOnly;
        uint32_t        alignment = 0;
        uint32_t        dataSize = 0;
        DataType        dataType = DataType::Pointer;
    };

    class TextureType : public ngfx::TextureReferType
    {
    public:
        TextureType() {}

        DataType          GetType() override { return DataType::Texture; }
        ArgumentAccess    Access() override { return access; }
        TextureDimension  TextureDim() override { return dimension; }
        DataType          TextureDataType() override { return dataType; }

        ArgumentAccess    access;
        DataType          dataType;
        TextureDimension  dimension;
    };

    class Reflection : public ngfx::PipelineReflection
    {
        using Variables = std::vector<Variable*>;
    public:
        Reflection(void* pData, uint32_t size)
        {
            m_Reflector = new spirv_cross::Compiler(reinterpret_cast<const uint32_t*>(pData), size / 4);
            DoReflect();
        }
        explicit Reflection(const ByteCode& bc)
        {
            m_Reflector = new spirv_cross::Compiler(bc);
            DoReflect();
        }
        ~Reflection()
        {
            if (m_Reflector)
            {
                delete m_Reflector;
                m_Reflector = nullptr;
            }
        }

        uint32_t VariableCount() const override
        {
            return m_Vars.size();
        }
        ngfx::Variable* VariableAt(uint32_t id) const override
        {
            return m_Vars[id];
        }
        ngfx::Result Merge(const ngfx::PipelineReflection * pReflection) override
        {
            return Result::Ok;
        }
    private:
        void DoReflect()
        {
            auto shRes = m_Reflector->get_shader_resources();
            auto active = m_Reflector->get_active_interface_variables();
            for (auto& res : shRes.uniform_buffers)
            {
                const k3d::U64 kBlockMask = (1ULL << spv::DecorationBlock) | (1ULL << spv::DecorationBufferBlock);
                auto&    typeInfo = m_Reflector->get_type(res.type_id);
                bool     isPushConstant = (spv::StorageClassPushConstant == m_Reflector->get_storage_class(res.id));
                bool     isBlock = (0 != (m_Reflector->get_decoration_mask(typeInfo.self) & kBlockMask));
                //k3d::U32 	 typeId = ((!isPushConstant && isBlock) ? res.type_id : res.id);

                //auto            bindingType = isPushConstant ? NGFX_SHADER_BIND_CONSTANTS : NGFX_SHADER_BIND_BLOCK;
                std::string     bindingName = m_Reflector->get_name(res.id);
                //k3d::U32			bindingNumber = m_Reflector->get_decoration(res.id, spv::DecorationBinding);
                //k3d::U32			bindingSet = m_Reflector->get_decoration(res.id, spv::DecorationDescriptorSet);
                //NGFXShaderType  bindingStage = shaderType;

                for (k3d::U32 index = 0; index < typeInfo.member_types.size(); ++index) {
                    std::string memberName = m_Reflector->get_member_name(res.type_id, index);
                    k3d::U32      memberId = typeInfo.member_types[index];
                    auto&		memberTypeInfo = m_Reflector->get_type(memberId);
                    k3d::U32      memberOffset = m_Reflector->get_member_decoration(res.type_id, index, spv::DecorationOffset);
                    k3d::U32      memberArraySize = memberTypeInfo.array.empty() ? 1 : memberTypeInfo.array[0];
                    //NGFXShaderDataType	memberDataType = spirTypeToGlslUniformDataType(memberTypeInfo);

                    std::string uniformName = bindingName + "." + memberName;
                    //outUniformLayout.AddUniform({ memberDataType, uniformName.c_str(), memberOffset, memberArraySize });
                }

                // Block size
                //size_t blockSize = backCompiler.get_declared_struct_size(typeInfo);

                //// Round up to next multiple of 16			
                //size_t paddedSize = (blockSize + 15) & (~15);
                //if ((blockSize != paddedSize) && (!isPushConstant)) {
                //	Log::Out(LogLevel::Info, "GLSLCompiler", "Padded uniform block " << bindingName << " from " << blockSize << " bytes to " << paddedSize << " bytes");
                //}

                // Use block size for push constants and padded size for UBOs
                //outUniformLayout->setBlockSizeBytes(bindingName, isPushConstant ? blockSize : paddedSize);
                //outUniformLayout->sortByOffset();

                Variable* var = new Variable;
                auto name = m_Reflector->get_name(res.id);
                var->name = name;
                if (name.empty())
                {
                    var->active = false;
                    var->type = nullptr;
                }
                else
                {
                    var->id = m_Reflector->get_decoration(res.id, spv::DecorationBinding);
                    auto type = m_Reflector->get_type(res.type_id);
                    m_Vars.push_back(var);
                }
            }

            for (auto& res : shRes.sampled_images)
            {
            }

            for (auto& res : shRes.storage_buffers)
            {
                Variable* var = new Variable;
                var->name = m_Reflector->get_name(res.id);
                var->id = m_Reflector->get_decoration(res.id, spv::DecorationBinding);
                auto type = m_Reflector->get_type(res.type_id);
                //uint32_t bindingSet = m_Reflector->get_decoration(res.id, spv::DecorationDescriptorSet);
                if (type.pointer)
                {
                    PointerType* pType = new PointerType;
                    auto b_type = m_Reflector->get_type(res.base_type_id);
                    switch (b_type.basetype)
                    {
                    case SPIRType::Struct:
                        // expand struct
                        auto r = m_Reflector->get_type(b_type.member_types[0]);
                        break;
                    }
                    var->type = pType;
                }
                else
                {
                    ArrayType* aType = new ArrayType;
                    var->active;
                    var->type = aType;
                }
                m_Vars.push_back(var);
            }

            for (auto& res : shRes.storage_images)
            {
                Variable* var = new Variable;
                var->name = m_Reflector->get_name(res.id);
                var->id = m_Reflector->get_decoration(res.id, spv::DecorationBinding);
                auto type = m_Reflector->get_type(res.type_id);
                TextureType* image = new TextureType;
                image->access = (ArgumentAccess)((uint32_t)ArgumentAccess::WriteOnly
                    | (uint32_t)ArgumentAccess::ReadOnly);
                switch (type.image.dim)
                {
                case spv::DimBuffer:
                case spv::Dim1D:
                    image->dimension = TextureDimension::Tex1D;
                    break;
                case spv::Dim2D:
                    image->dimension = TextureDimension::Tex2D;
                    break;
                case spv::Dim3D:
                    image->dimension = TextureDimension::Tex3D;
                    break;
                case spv::DimCube:
                    image->dimension = TextureDimension::TexCube;
                    break;
                }
                switch (type.image.format)
                {
                case spv::ImageFormatRgba32f:
                    image->dataType = DataType::Float4;
                    break;
                case spv::ImageFormatRg32f:
                    image->dataType = DataType::Float2;
                    break;
                case spv::ImageFormatR32f:
                    image->dataType = DataType::Float;
                    break;
                }
                switch (type.vecsize)
                {
                case 1:
                    image->dimension = TextureDimension::Tex1D;
                    break;
                }
                var->type = image;
                m_Vars.push_back(var);
            }

            for (auto& res : shRes.push_constant_buffers)
            {

            }
        }
    private:
        Compiler* m_Reflector;
        Variables m_Vars;
    };

    ArgumentAccess Variable::Access()
    {
        switch (type->GetType())
        {
        case DataType::Array:
            //static_cast<SPIRVArrayType*>(type)->
            break;
        case DataType::Struct:
            //static_cast<SPIRVStructType*>(type)->
            break;
        case DataType::Pointer:
            return static_cast<PointerType*>(type)->access;
        case DataType::Texture:
            return static_cast<TextureType*>(type)->access;
        }
        return ArgumentAccess::ReadOnly;
    }
}

using ByteCode = vector<uint32_t>;

class VulkanDevice;
class VulkanBindTableEncoder;
class BindTableAllocator
{
public:
    BindTableAllocator(VulkanDevice* pDevice);
    ~BindTableAllocator();

    Result Allocate(const VulkanBindTableEncoder* pLayout, BindTable** ppTable);
    void Free(BindTable* ppTable);

    VkDescriptorPool Handle;
    VulkanDevice* Device;

    vector<VkDescriptorPoolSize> Sizes;
};

class VulkanLibrary1;

class VulkanFunction1 : public ngfx::Function
{
public:
    VulkanFunction1(VulkanLibrary1* pLibrary, const char * name);
    ~VulkanFunction1() override;

    ShaderType Type() const override;
    const char * Name() const override;
    const ByteCode& GetByteCode() const;
    
    VulkanLibrary1* Library;
    VkPipelineShaderStageCreateInfo StageInfo;

    VkShaderModuleCreateInfo CreateInfo = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    };
private:
    string           EntryName;
    string           Source;
    ngfx::ShaderType ShaderType;
    VkShaderModule   ShaderModule = VK_NULL_HANDLE;
};

/**
 * Support Multi-Entry SPIRV
 */
class VulkanLibrary1 : public ngfx::Library
{
    friend class VulkanFunction1;
public:
    VulkanLibrary1(VulkanDevice* pDevice, const void *pBlobData, k3d::U64 Size);
    VulkanLibrary1(VulkanDevice* pDevice, const char *pFilePath);
    ~VulkanLibrary1() override;

    Result MakeFunction(const char * name, Function ** ppFunction) override;
protected:
    void Init(const void *pBlobData, k3d::U64 Size);

private:
    FunctionMap DataBlob;
    VulkanDevice* Device;
};

class VulkanCommandBuffer : public CommandBuffer
{
protected:
    class VulkanQueue* OwningRoot;
public:
    VkCommandBuffer Handle = VK_NULL_HANDLE;
    void Commit(Fence * pFence) override;
    Result CreateRenderCommandEncoder(Drawable * pDrawable, RenderPass * pRenderPass, RenderCommandEncoder **) override;
    Result CreateComputeCommandEncoder(ComputeCommandEncoder **) override;
    Result CreateParallelCommandEncoder(ParallelRenderCommandEncoder **) override;
    Result CreateCopyCommandEncoder(CopyCommandEncoder **) override;
    VulkanCommandBuffer(VulkanQueue* pQueue);
    ~VulkanCommandBuffer();
};

template <class T>
class TCmdEncoder : public T
{
public:
    using Super = TCmdEncoder<T>;

    TCmdEncoder(VulkanCommandBuffer* pCmd, VkPipelineBindPoint BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
    virtual ~TCmdEncoder();

    void Barrier(Resource * pResource) override;
    void SetPipeline(Pipeline* pPipelineState) override;
    void SetBindTable(BindTable * pBindTable) override;
    virtual void EndEncode() override;

    VkPipelineBindPoint CurrentBindingPoint;

    VulkanCommandBuffer* OwningCommand = nullptr;
};

class VulkanCopyEncoder : public TCmdEncoder<CopyCommandEncoder>
{
public:
    VulkanCopyEncoder(VulkanCommandBuffer * pCmd);
    ~VulkanCopyEncoder();
    void CopyTexture() override;
    void CopyBuffer(uint64_t srcOffset, uint64_t dstOffset, uint64_t size, Buffer * srcBuffer, Buffer * dstBuffer) override;
};

class VulkanComputeEncoder : public TCmdEncoder<ComputeCommandEncoder>
{
public:
    VulkanComputeEncoder(VulkanCommandBuffer* pCmd);
    ~VulkanComputeEncoder();
    void Dispatch(uint32_t x, uint32_t y, uint32_t z) override;
};

class VulkanRenderEncoder : public TCmdEncoder<RenderCommandEncoder>
{
public:
    VulkanRenderEncoder(VulkanCommandBuffer* pCmd);
    ~VulkanRenderEncoder();

    void SetScissorRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    void SetViewport(const Viewport * pViewport);
    void SetDepthBias(Float32 biasConst, Float32 biasClamp, Float32 biasSlope) override;
    void SetDepthBounds(Float32 minDepth, Float32 maxDepth) override;
    void SetStencilReference(StencilFaceRef face, uint32_t value) override;
    void SetBlendConsts(Float32x4 constant) override;
    void SetLineWidth(Float32 width) override;

    void SetIndexBuffer(Buffer * pIndexBuffer) override;
    void SetVertexBuffer(uint32_t slot, uint64_t offset, Buffer * pVertexBuffer) override;

    void DrawInstanced(const DrawInstancedDesc * drawParam) override;
    void DrawIndexedInstanced(const DrawIndexedInstancedDesc * drawParam) override;
    void DrawIndirect(Buffer * pIndirectBuffer, uint32_t offset, uint32_t drawCount, uint32_t stride) override;
    void Present(Drawable * pDrawable);
};

class VulkanQueue : public CommandQueue
{
public:
    VulkanQueue(class VulkanDevice* pDevice);
    ~VulkanQueue() override;
    Result CreateCommandBuffer(CommandBuffer ** ppComandBuffer) override;
    VulkanDevice* OwningRoot;
    bool IsSupport(CommandQueueType const&type) const;

    VkQueue Handle = VK_NULL_HANDLE;
    VkCommandPool Pool;

    uint32_t FamilyId = 0;
    uint32_t QueueId = 0;
};

class VulkanRenderPass : public RenderPass
{
public:
    VulkanRenderPass(VulkanDevice* pDevice, const RenderPassDesc* pDesc);
    ~VulkanRenderPass() override;

    Result MakeFrameBuffer(
        const TextureView ** pRenderTargetViews, int32_t viewCount, 
        int32_t width, int32_t height, int32_t layers, 
        FrameBuffer ** ppFramebuffer) override;
    Result MakeRenderPipeline(
        NotNull const RenderPipelineDesc * pPipelineDesc, 
        NotNull const PipelineLayout * pPipelineLayout, 
        Pipeline ** ppPipeline) override;

    VulkanDevice* Device;
    VkRenderPass Handle = VK_NULL_HANDLE;
};

template <class T>
struct PipelineTrait
{
};

template <>
struct PipelineTrait<RenderPipeline>
{
    using CreateInfo = VkGraphicsPipelineCreateInfo;
    static decltype(vkCreateGraphicsPipelines)* Create;
    static PipelineType Type;
};

decltype(vkCreateGraphicsPipelines)* PipelineTrait<RenderPipeline>::Create = &vkCreateGraphicsPipelines;
PipelineType PipelineTrait<RenderPipeline>::Type = PipelineType::Graphics;

template <>
struct PipelineTrait<ComputePipeline>
{
    using CreateInfo = VkComputePipelineCreateInfo;
    static decltype(vkCreateComputePipelines)* Create;
    static PipelineType Type;
};

decltype(vkCreateComputePipelines)* PipelineTrait<ComputePipeline>::Create = &vkCreateComputePipelines;
PipelineType PipelineTrait<ComputePipeline>::Type = PipelineType::Compute;

class VulkanPipelineLayout;

template <class T>
class TPipeline : public T
{
public:

    TPipeline(VulkanDevice * pDevice);
    virtual ~TPipeline() override;

    Result GetCache(uint64_t * pSize, void * pOutData) override
    {
        RetrieveCache(pOutData, pSize);
        return Result::Ok;
    }

    PipelineType Type() const override
    {
        return PipelineTrait<T>::Type;
    }

    VkPipeline              Handle = VK_NULL_HANDLE;
    VulkanPipelineLayout*   Layout = nullptr;

protected:
    using Super = typename TPipeline<T>;

    VulkanDevice* OwningDevice = nullptr;
    VkPipelineCache Cache = VK_NULL_HANDLE;

    typename PipelineTrait<T>::CreateInfo Info;

    VkResult CreatePipelineCache(void const* InData, k3d::U64 InSize)
    {
        VkPipelineCacheCreateInfo CacheInfo = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, nullptr, 0, InSize, InData };
        return vkCreatePipelineCache(OwningDevice->Handle, &CacheInfo, VULKAN_ALLOCATOR, &Cache);
    }

    VkResult CreatePipeline()
    {
        return PipelineTrait<T>::Create(OwningDevice->Handle, Cache, 1, &Info, VULKAN_ALLOCATOR, &Handle);
    }

    VkResult RetrieveCache(void* OutData, k3d::U64* OutSize)
    {
        if (Cache)
        {
            return vkGetPipelineCacheData(OwningDevice->Handle, Cache, OutSize, OutData);
        }
        //vkMergePipelineCaches(OwningDevice->Handle)
        return VK_RESULT_MAX_ENUM;
    }

private:
    friend class VulkanPipelineLibrary;
};

class VulkanPipelineLibrary : public PipelineLibrary
{
public:
    VulkanPipelineLibrary(VulkanDevice* pDevice, const void* InData, uint64_t Size);
    ~VulkanPipelineLibrary() override;

    void StorePipeline(const char * key, Pipeline * pPipeline) override;
    uint64_t GetSerializedSize() const override;
    void Serialize(void * pData, uint64_t Size) override;

    VkPipelineCache Handle;
    VulkanDevice* Device;
};

class VulkanRenderPipeline : public TPipeline<RenderPipeline>
{
public:
    VulkanRenderPipeline(VulkanDevice* pDevice, const RenderPipelineDesc* pDesc, RenderPass * pRenderPass, const PipelineLayout * pLayout);
    ~VulkanRenderPipeline() override;
    Result GetDesc(RenderPipelineDesc * pDesc) override;

protected:
    void InitShaderStages(const RenderPipelineDesc* pDesc);
    void InitRasterState(const RenderPipelineDesc* pDesc);
    void InitDepthStencilState(const RenderPipelineDesc* pDesc);
    void InitBlendState(const RenderPipelineDesc* pDesc);
    void InitInputState(const RenderPipelineDesc* pDesc);

private:
    using VertexAttribs = std::vector<VkVertexInputAttributeDescription>;
    using VertexBindings = std::vector<VkVertexInputBindingDescription>;
    using BlendAttachStates = std::vector<VkPipelineColorBlendAttachmentState>;
    /* shaders */
    std::vector<VkPipelineShaderStageCreateInfo> Stages;

    VertexAttribs                           VertexInputAttributes;
    VertexBindings                          VertexInputBindings;
    VkPipelineVertexInputStateCreateInfo    VertexInputState;

    VkPipelineInputAssemblyStateCreateInfo  InputAssemblyState;

    VkPipelineRasterizationStateCreateInfo  RasterizationState;
    VkPipelineMultisampleStateCreateInfo    MultisampleState;
    VkPipelineDepthStencilStateCreateInfo   DepthStencilState;

    BlendAttachStates                       BlendAttachState;
    VkPipelineColorBlendStateCreateInfo     BlendState;
    VkPipelineTessellationStateCreateInfo   TessellationState;
    /* Dynamic state */
    VkPipelineViewportStateCreateInfo       ViewportState;
    VkPipelineDynamicStateCreateInfo        DynamicState;
};

class VulkanComputePipeline : public TPipeline<ComputePipeline>
{
public:
    VulkanComputePipeline(VulkanDevice* pDevice, Function* pComputeFunc/*, PipelineLayout * pLayout*/);
    VulkanComputePipeline(VulkanDevice* pDevice, const Function* pComputeFunc, const PipelineLayout * pLayout);
    ~VulkanComputePipeline();
};

class VulkanTextureView;
// Correspond to VkFramebuffer And TextureViews
class VulkanDrawable : public FrameBuffer
{
public:
    VulkanDrawable(VulkanDevice* pDevice, const FrameBufferDesc* pDesc);
    VulkanDrawable(VulkanDevice* pDevice, const VulkanRenderPass * pRenderPass,
        const TextureView ** pRenderTargetViews, int32_t viewCount,
        int32_t width, int32_t height, int32_t layers);
    
    ~VulkanDrawable() override;

    Bool32 CompatWith(const RenderPass * pRenderPass) override { return false; }

    VulkanDevice* Device;
    VkFramebuffer Handle = VK_NULL_HANDLE;
    // transient
    std::vector<VulkanTextureView*> Attachments;
};

Result VulkanCommandBuffer::CreateRenderCommandEncoder(Drawable* pDrawable, RenderPass* pRenderPass, RenderCommandEncoder ** ppEncoder)
{
    *ppEncoder = OwningRoot->IsSupport(CommandQueueType::Graphics) ?
        new VulkanRenderEncoder(this) : nullptr;
    return Result::Ok;
}

Result VulkanCommandBuffer::CreateComputeCommandEncoder(ComputeCommandEncoder ** ppEncoder)
{
    *ppEncoder = OwningRoot->IsSupport(CommandQueueType::Compute) ?
        new VulkanComputeEncoder(this) : nullptr;
    return Result::Ok;
}

Result VulkanCommandBuffer::CreateParallelCommandEncoder(ParallelRenderCommandEncoder ** ppEncoder)
{
    return Result::Ok;
}

Result VulkanCommandBuffer::CreateCopyCommandEncoder(CopyCommandEncoder ** ppEncoder)
{
    *ppEncoder = OwningRoot->IsSupport(CommandQueueType::Copy) ?
        new VulkanCopyEncoder(this) : nullptr;
    return Result::Ok;
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanQueue* pQueue)
    : OwningRoot(pQueue)
{
    OwningRoot->AddInternalRef();

}

VulkanCommandBuffer::~VulkanCommandBuffer()
{

    OwningRoot->ReleaseInternal();
}

class VulkanSampler : public Sampler
{
protected:
    VulkanDevice* OwningDevice;
private:
    VkSamplerCreateInfo Info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
public:
    VulkanSampler(VulkanDevice* pDevice, const SamplerDesc* pDesc);
    ~VulkanSampler();
    Result GetDesc(SamplerDesc * desc) override;
    VkSampler Handle = VK_NULL_HANDLE;
};

template<class RHIObj>
struct ResTrait
{
};

template<>
struct ResTrait<Buffer>
{
    typedef VkBuffer Obj;
    typedef VkBufferView View;
    typedef VkBufferUsageFlags UsageFlags;
    typedef VkBufferCreateInfo CreateInfo;
    typedef VkBufferViewCreateInfo ViewCreateInfo;
    typedef VkDescriptorBufferInfo DescriptorInfo;
    typedef BufferDesc CreateDesc;
    static decltype(vkCreateBufferView)* CreateView;
    static decltype(vkDestroyBufferView)* DestroyView;
    static decltype(vkCreateBuffer)* Create;
    static decltype(vmaCreateBuffer)* vmaCreate;
    static decltype(vkDestroyBuffer)* Destroy;
    static decltype(vmaDestroyBuffer)* vmaDestroy;
    static decltype(vkGetBufferMemoryRequirements)* GetMemoryInfo;
    static decltype(vkBindBufferMemory)* BindMemory;
};

template<>
struct ResTrait<Texture>
{
    typedef VkImage Obj;
    typedef VkImageView View;
    typedef VkImageUsageFlags UsageFlags;
    typedef VkImageCreateInfo CreateInfo;
    typedef VkImageViewCreateInfo ViewCreateInfo;
    typedef VkDescriptorImageInfo DescriptorInfo;
    typedef TextureDesc CreateDesc;
    static decltype(vkCreateImageView)* CreateView;
    static decltype(vkDestroyImageView)* DestroyView;
    static decltype(vkCreateImage)* Create;
    static decltype(vmaCreateImage)* vmaCreate;
    static decltype(vkDestroyImage)* Destroy;
    static decltype(vmaDestroyImage)* vmaDestroy;
    static decltype(vkGetImageMemoryRequirements)* GetMemoryInfo;
    static decltype(vkBindImageMemory)* BindMemory;
};

// Buffer functors
decltype(vkCreateBufferView)* ResTrait<Buffer>::CreateView =
&vkCreateBufferView;
decltype(vkDestroyBufferView)* ResTrait<Buffer>::DestroyView =
&vkDestroyBufferView;
decltype(vkCreateBuffer)* ResTrait<Buffer>::Create = &vkCreateBuffer;
decltype(vmaCreateBuffer)* ResTrait<Buffer>::vmaCreate = &vmaCreateBuffer;
decltype(vkDestroyBuffer)* ResTrait<Buffer>::Destroy = &vkDestroyBuffer;
decltype(vmaDestroyBuffer)* ResTrait<Buffer>::vmaDestroy = &vmaDestroyBuffer;
decltype(vkGetBufferMemoryRequirements)* ResTrait<Buffer>::GetMemoryInfo =
&vkGetBufferMemoryRequirements;
decltype(vkBindBufferMemory)* ResTrait<Buffer>::BindMemory =
&vkBindBufferMemory;
// Texture
decltype(vkCreateImageView)* ResTrait<Texture>::CreateView = &vkCreateImageView;
decltype(vkDestroyImageView)* ResTrait<Texture>::DestroyView =
&vkDestroyImageView;
decltype(vkCreateImage)* ResTrait<Texture>::Create = &vkCreateImage;
decltype(vmaCreateImage)* ResTrait<Texture>::vmaCreate = &vmaCreateImage;
decltype(vkDestroyImage)* ResTrait<Texture>::Destroy = &vkDestroyImage;
decltype(vmaDestroyImage)* ResTrait<Texture>::vmaDestroy = &vmaDestroyImage;
decltype(vkGetImageMemoryRequirements)* ResTrait<Texture>::GetMemoryInfo =
&vkGetImageMemoryRequirements;
decltype(vkBindImageMemory)* ResTrait<Texture>::BindMemory = &vkBindImageMemory;


template<class TRHIResObj>
class TResource : public TRHIResObj
{
    friend class CommandBufferImpl;
public:
    using TObj = typename ResTrait<TRHIResObj>::Obj;
    using ResInfo = typename ResTrait<TRHIResObj>::CreateInfo;
    using Desc = typename ResTrait<TRHIResObj>::CreateDesc;
    using Super = TResource<TRHIResObj>;

    TResource(VulkanDevice* Device, const Desc* pDesc) : OwningDevice(Device) 
    {
        memcpy(&ResDesc, pDesc, sizeof(Desc));
    }

    virtual ~TResource();

    void * Map(uint64_t offset, uint64_t size) override;
    void UnMap() override;

    Result Create(ResInfo const& Info, StorageOption const& Option);
    TObj GetHandle() const { return Handle; }

    VulkanDevice*           OwningDevice;
    ResInfo                 Info = {};
protected:
    VkMappedMemoryRange     MappedMemoryRange = {};
    VmaMemoryRequirements   MemReq = {};
    Desc                    ResDesc = {};
    TObj                    Handle = VK_NULL_HANDLE;
};

class VulkanBuffer : public TResource<Buffer>
{
public:
    VulkanBuffer(VulkanDevice * pDevice, const BufferDesc* pDesc);
    ~VulkanBuffer();
    Result GetDesc(BufferDesc * pDesc);
    Result CreateView(const BufferViewDesc * pDesc, BufferView ** ppView) override;
};

class VulkanBufferView : public BufferView
{
public:
    VulkanBufferView(const BufferViewDesc * pDesc, VulkanBuffer* pBuffer);
    ~VulkanBufferView() override;
    VkBufferView Handle;
    VkBufferViewCreateInfo Info;
    VulkanBuffer* OwningBuffer;
    bool IsTexelBuffer;
    VkDescriptorType DesriptorType;
    VkBuffer GetRawBuffer() const { return OwningBuffer->GetHandle(); }
    friend class VulkanBuffer;
};

class VulkanSwapChainTexture : public Texture
{
    friend class VulkanSwapChain;
public:
    VulkanSwapChainTexture(class VulkanSwapChain* pSwapChain, VkImage Image, const SwapChainDesc* pDesc);
    ~VulkanSwapChainTexture() override;

    Result GetDesc(TextureDesc * pDesc) override { return Result::Ok; }
    Result CreateView(const TextureViewDesc * pDesc, TextureView ** ppView) override { return Result::Ok; }

    void * Map(uint64_t offset, uint64_t size) override { return nullptr; }
    void UnMap() override {}

    VkImage     Image;
    VkImageView ImageView;
    VulkanSwapChain* SwapChain;
};

class VulkanTexture : public TResource<Texture>
{
public:
    VulkanTexture(VulkanDevice* pDevice, const TextureDesc* pDesc);
    ~VulkanTexture();
    Result GetDesc(TextureDesc * pDesc) override;
    Result CreateView(const TextureViewDesc * pDesc, TextureView ** ppView) override;
};

class VulkanTextureView : public TextureView
{
public:
    VulkanTextureView(const TextureViewDesc * pDesc, VulkanTexture* pBuffer);
    ~VulkanTextureView() override;
    VkImageView Handle;
    VkImageViewCreateInfo Info;
    VulkanTexture* OwningTexture;
    friend class VulkanTexture;
};

class VulkanPipelineLayout : public PipelineLayout
{
public:
    VulkanPipelineLayout(VulkanDevice* pDevice, const PipelineLayoutDesc * pDesc);
    // for compute pipeline
    VulkanPipelineLayout(VulkanDevice* pDevice, const VulkanFunction1* pComputeShader);
    // for render pipeline
    VulkanPipelineLayout(VulkanDevice* pDevice,
        NotNull const VulkanFunction1* pVertexShader,
        NotNull const VulkanFunction1* pPixelShader,
        Nullable const VulkanFunction1* pGeometryShader = nullptr,
        Nullable const VulkanFunction1* pDomainShader = nullptr,
        Nullable const VulkanFunction1* pHullShader = nullptr);
    // deconstructor 
    ~VulkanPipelineLayout() override;

    // owning table layouts
    std::vector<BindTableEncoder*> OwningTableLayouts;

    VkPipelineLayout Handle = VK_NULL_HANDLE;
    VulkanDevice* Device = nullptr;

    friend class VulkanDevice;
private:
    void CreateWithComputeFunction(const VulkanFunction1* pCompute);
};

class VulkanBindTableEncoder : public BindTableEncoder
{
public:
    VulkanBindTableEncoder(VulkanDevice* pDevice, const ArgumentDesc * pArgumentDescs, int32_t argumentCount);
    ~VulkanBindTableEncoder() override;

    Result Allocate(BindTable ** ppBindTable) override;

    /*void AddBuffer(uint32_t slot, uint32_t count, ShaderStageBit visibility) override;
    void AddTexture(uint32_t slot, uint32_t count, ShaderStageBit visibility) override;
    void AddSampler(uint32_t slot, ShaderStageBit visibility) override;
    Result Initialize(BindTableLayout ** ppBindTableLayout) override;
*/
    VulkanDevice* OwningDevice = nullptr;
    VkDescriptorSetLayout Handle = VK_NULL_HANDLE;
    VkDescriptorSetLayoutCreateInfo Info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    std::vector<VkDescriptorSetLayoutBinding> DescriptorBindings;

    VkDescriptorPoolCreateInfo PoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    std::vector<VkDescriptorPoolSize> PoolSizes;
};

class VulkanBindTable : public BindTable
{
public:
    VulkanBindTable(VulkanDevice* pDevice, VkDescriptorSet Ds);
    ~VulkanBindTable() override;

    void SetSampler(uint32_t index, ShaderType shaderVis, Sampler * pSampler) override;
    void SetBuffer(uint32_t index, ShaderType shaderVis, BufferView * bufferView) override;
    void SetTexture(uint32_t index, ShaderType shaderVis, TextureView * textureView) override;

    VkDescriptorSet                   Handle = VK_NULL_HANDLE;
    std::vector<VkWriteDescriptorSet> WriteDescriptorSets;
    VulkanDevice*                     Device;
    VulkanPipelineLayout*             OwningLayout;

    friend class VulkanPipelineLayout;
    friend class BindTableAllocator;
};

class VulkanFence : public Fence
{
public:
    VulkanFence(VulkanDevice* pDevice);
    ~VulkanFence() override;
    void Wait() override;
    void Reset() override;
    VulkanDevice* OwningDevice = nullptr;
    VkFence Handle = VK_NULL_HANDLE;
};

namespace ngfx
{
    class VulkanFactory;
}

class VulkanDevice : public Device
{
public:
    VulkanDevice(class VulkanFactory* pFactory, VkPhysicalDevice PhysicalDevice);
    ~VulkanDevice() override;
    void GetDesc(DeviceDesc * pDesc) override;
    Result CreateCommandQueue(CommandQueueType queueType, CommandQueue ** pQueue) override;
    Result CreatePipelineLayout(const PipelineLayoutDesc * pPipelineLayoutDesc, PipelineLayout ** ppPipelineLayout) override; 
    Result MakeBindTableEncoder(NotNull const ArgumentDesc * pArgumentDescs, int32_t argumentCount, BindTableEncoder ** ppBindingTableLayout) override;
    Result CreateRenderPipeline(NotNull const RenderPipelineDesc * pPipelineDesc, Nullable RenderPass * pRenderPass, Pipeline ** pPipelineState, NotNull PipelineReflection ** ppReflection) override;
    Result CreateComputePipeline(NotNull Function * pComputeFunction, Pipeline ** pPipeline, NotNull PipelineReflection ** ppReflection) override;
    Result CreateComputePipeline(NotNull const Function * pComputeFunction, NotNull const PipelineLayout * ppPipelineLayout, NotNull Pipeline ** ppPipeline) override;
    Result CreatePipelineLibrary(const void * pData, uint64_t Size, PipelineLibrary ** ppPipelineLibrary) override;
    Result CreateLibrary(const CompileOption * compileOption, const void * pData, uint64_t Size, Library ** ppLibrary) override;
    Result CreateRenderPass(const RenderPassDesc * desc, RenderPass ** ppRenderpass) override;
    Result CreateSampler(const SamplerDesc* desc, Sampler ** pSampler) override;
    Result CreateBuffer(const BufferDesc* desc, Buffer ** pBuffer) override;
    Result CreateTexture(const TextureDesc * desc, Texture ** pTexture) override;
    Result CreateFence(Fence ** ppFence) override;
    void WaitIdle() override;
    bool SupportAsyncCompute() const { return IsSupportAsyncCompute; }

    VkDevice Handle = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties Prop;
    VkPhysicalDeviceFeatures Features;

    BindTableAllocator* TableAllocator;

private:

    struct QueueInfo
    {
        uint32_t Flags = 0;
        uint32_t Family = 0;
        uint32_t Count = 0;
    };

    VkPhysicalDevice Device = VK_NULL_HANDLE;
    VkPhysicalDeviceGroupPropertiesKHX DeviceGroupProperties;
    VmaAllocator MemoryAllocator = nullptr;
    std::vector<QueueInfo> QueueInfos;
    bool IsSupportAsyncCompute;

protected:
    VulkanFactory* OwningRoot;
    friend class VulkanFactory;
    friend class VulkanQueue;
    template<class T>
    friend class TResource;
};

VulkanQueue::VulkanQueue(VulkanDevice* pDevice)
    : OwningRoot(pDevice)
{
    OwningRoot->AddInternalRef();
    vkGetDeviceQueue(OwningRoot->Handle, 0, 0, &Handle);
    VkCommandPoolCreateInfo PoolInfo = {
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
    };
    PoolInfo.queueFamilyIndex = FamilyId;
    PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
    CHECK(vkCreateCommandPool(OwningRoot->Handle, &PoolInfo, VULKAN_ALLOCATOR, &Pool));
}

VulkanQueue::~VulkanQueue()
{
    vkDestroyCommandPool(OwningRoot->Handle, Pool, VULKAN_ALLOCATOR);
    OwningRoot->ReleaseInternal();
}

Result VulkanQueue::CreateCommandBuffer(CommandBuffer ** ppCmdBuffer)
{
    VkCommandBuffer CmdBuffer = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo AllocInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
      Pool,
      VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      1
    };
    CHECK(vkAllocateCommandBuffers(OwningRoot->Handle, &AllocInfo, &CmdBuffer));
    //
    vkFreeCommandBuffers(OwningRoot->Handle, Pool, 1, &CmdBuffer);
    return Result::Ok;
}

bool VulkanQueue::IsSupport(CommandQueueType const& type) const
{
    return false;
}

template<class TRHIResObj>
void * TResource<TRHIResObj>::Map(uint64_t offset, uint64_t size)
{
    MappedMemoryRange.offset = offset;
    MappedMemoryRange.size = size;
    void * pData = nullptr;
    vmaMapMemory(OwningDevice->MemoryAllocator, &MappedMemoryRange, &pData);
    return pData;
}

template<class TRHIResObj>
void TResource<TRHIResObj>::UnMap()
{
    vmaUnmapMemory(OwningDevice->MemoryAllocator, &MappedMemoryRange);
}

template<class TRHIResObj>
TResource<TRHIResObj>::~TResource()
{
    ResTrait<TRHIResObj>::vmaDestroy(OwningDevice->MemoryAllocator, Handle);
}

template<class TRHIResObj>
Result TResource<TRHIResObj>::Create(ResInfo const & _Info, StorageOption const & Option)
{
    switch (Option)
    {
    case StorageOption::Shared:
        MemReq.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        break;
    case StorageOption::Private:
        MemReq.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        break;
    case StorageOption::Managed:
        MemReq.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        break;
    }
    CHECK(ResTrait<TRHIResObj>::vmaCreate(OwningDevice->MemoryAllocator,
        &Info, &MemReq,
        &Handle, &MappedMemoryRange,
        VULKAN_ALLOCATOR));
    return Result::Ok;
}

VulkanBuffer::VulkanBuffer(VulkanDevice * pDevice, const BufferDesc* pDesc)
    : Super(pDevice, pDesc)
{
    Info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    Info.size = pDesc->size;
    if (pDesc->allowedViewBits & BufferViewBit::UnOrderedAccess)
    {
        Info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (pDesc->allowedViewBits & BufferViewBit::VertexBuffer)
    {
        Info.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (pDesc->allowedViewBits & BufferViewBit::ConstantBuffer)
    {
        Info.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if ((uint32_t)pDesc->option & (uint32_t)StorageOption::Private)
    {
        Info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    Info.flags = 0;
    Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    Super::Create(Info, pDesc->option);
}

VulkanBuffer::~VulkanBuffer()
{

}

Result VulkanBuffer::GetDesc(BufferDesc * pDesc)
{
    if (!pDesc)
        return Result::ParamError;
    memcpy(pDesc, &ResDesc, sizeof(ResDesc));
    return Result::Ok;
}

VulkanBufferView::VulkanBufferView(const BufferViewDesc * pDesc, VulkanBuffer * pBuffer)
    : OwningBuffer(pBuffer)
    , Info{ VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO }
{
    OwningBuffer->AddInternalRef();
    switch (pDesc->view)
    {
    case ResourceViewType::LinearBuffer: // Simple Buffer
        IsTexelBuffer = false;
        DesriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        break;
    case ResourceViewType::UnorderAccessBuffer: // Simple Buffer
        IsTexelBuffer = false;
        DesriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        break;
    case ResourceViewType::SampledTexture: // Uniform Texel Buffer
        IsTexelBuffer = true;
        DesriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        break;
    case ResourceViewType::UnorderAccessTexture: // Storage Texel Buffer
        IsTexelBuffer = true;
        DesriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        break;
    }
    Info.buffer = pBuffer->GetHandle();
    Info.format; pDesc->stride;
    Info.offset = pDesc->offset;
    Info.range = pDesc->size;
    if (IsTexelBuffer)
    {
        CHECK(vkCreateBufferView(OwningBuffer->OwningDevice->Handle, &Info, VULKAN_ALLOCATOR, &Handle));
    }
}

VulkanBufferView::~VulkanBufferView()
{
    if (Handle && IsTexelBuffer)
    {
        vkDestroyBufferView(OwningBuffer->OwningDevice->Handle, Handle, VULKAN_ALLOCATOR);
    }
    OwningBuffer->ReleaseInternal();
}

Result VulkanBuffer::CreateView(const BufferViewDesc * pDesc, BufferView ** ppView)
{
    if (!pDesc || !ppView)
        return Result::ParamError;
    *ppView = new VulkanBufferView(pDesc, this);
    return Result::Ok;
}

VulkanTexture::VulkanTexture(VulkanDevice * pDevice, const TextureDesc * pDesc)
    : Super(pDevice, pDesc)
{
    Info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    Info.flags = 0;
    // extents
    Info.format = ConvertPixelFormatToVulkanEnum(pDesc->format);
    Info.extent = { pDesc->width, pDesc->height, pDesc->depth };
    Info.arrayLayers = pDesc->layers;
    Info.mipLevels = pDesc->mipLevels;

    if (pDesc->width > 1)
    {
        Info.imageType = VK_IMAGE_TYPE_1D;
    }
    if (pDesc->height > 1)
    {
        Info.imageType = VK_IMAGE_TYPE_2D;
    }
    if (pDesc->depth > 1)
    {
        Info.imageType = VK_IMAGE_TYPE_3D;
    }
    Info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    Info.samples = ConvertMultiSampleFlagToVulkanEnum(pDesc->samples);

    auto usage = pDesc->allowedViewBits;
    if (usage & TextureViewBit::RenderTarget)
    {
        Info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        //Info.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        if (pDesc->option != StorageOption::Private)
        {
            LogPrint(Log::Error, "Texture", "When Texture used as RenderTarget, StorageOption should be PRIVATE!\n");
            assert(0);
        }
    }
    if (usage & TextureViewBit::DepthStencil)
    {
        Info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        //Info.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    if (usage & TextureViewBit::ShaderRead)
    {
        Info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (usage & TextureViewBit::ShaderWrite)
    {
        Info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        Info.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }

    if (pDesc->option == StorageOption::Managed ||
        pDesc->option == StorageOption::Shared)
    {
        Info.tiling = VK_IMAGE_TILING_LINEAR;
        Info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    }
    else if (pDesc->option == StorageOption::Private)
    {
        Info.tiling = VK_IMAGE_TILING_OPTIMAL;
    }

    Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    Super::Create(Info, pDesc->option);
}

VulkanTexture::~VulkanTexture()
{

}

class VulkanSwapChain : public SwapChain
{
public:
    void InitWithRenderPass(RenderPass * pRenderPass) override;
    Result GetTexture(Texture ** ppTexture, uint32_t index) override;
    Drawable * CurrentDrawable() override;
    Drawable * NextDrawable() override;
    uint32_t BufferCount() override;
    VulkanSwapChain(VulkanFactory* pFactory, void* pHandle, const SwapChainDesc* pDesc, VulkanQueue* pQueue);
    ~VulkanSwapChain() override;

    vector<VulkanDrawable*> SwapChainDrawables;
    VulkanDevice* GetDevice() const { return OwningDevice; }

private:
    VkSwapchainKHR Handle = VK_NULL_HANDLE;
    VkSurfaceKHR Surface = VK_NULL_HANDLE;
    VkSwapchainCreateInfoKHR CreateInfo;

protected:
    VulkanFactory* OwningRoot;
    VulkanDevice* OwningDevice;
};

namespace ngfx {

    class VulkanFactory : public Factory
    {
    public:
        bool Debug = false;
        bool PreferLinkedGpu = true;
        bool PreferCrossVendor = true;
        bool VRSupport = true;

        Result EnumDevice(uint32_t * count, Device ** ppDevice);
        Result CreateSwapchain(const SwapChainDesc * desc, CommandQueue * pCommandQueue, void * pWindow, SwapChain ** pSwapchain);

        friend NGFX_API Result CreateFactory(Factory ** ppFactory, bool debugEnabled)
        {
            *ppFactory = new VulkanFactory(debugEnabled);
            return Result::Ok;
        }

        VulkanFactory(bool debug) : Debug(debug)
        {
            uint32_t layerCount = 0;
            std::vector<VkLayerProperties> layers;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
            layers.resize(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
            LogPrint(Log::Info, "Factory", "DumpingLayers:\n");
            for (auto l : layers)
            {
                LogPrint(Log::Info, "Factory", "\t%s Desc:%s\n", l.layerName, l.description);
            }

            uint32_t layerExtPropCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &layerExtPropCount, nullptr);
            AvailableExtensions.resize(layerExtPropCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &layerExtPropCount, AvailableExtensions.data());
            LogPrint(Log::Info, "Factory", "Dumping Instance Extensions:\n");
            for (auto extProp : AvailableExtensions)
            {
                LogPrint(Log::Info, "Factory", "\t%s \n", extProp.extensionName);
            }

            VkApplicationInfo appInfo = {};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "test";
            appInfo.pEngineName = "test";
            appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 42);
            appInfo.engineVersion = 1;
            appInfo.applicationVersion = 0;

            VkInstanceCreateInfo instanceCreateInfo = {};
            instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instanceCreateInfo.pNext = NULL;
            instanceCreateInfo.pApplicationInfo = &appInfo;

            if (Debug) // Debug extension ??
            {
                instanceCreateInfo.enabledLayerCount = RequiredLayers.size();
                instanceCreateInfo.ppEnabledLayerNames = RequiredLayers.data();
            }

            if (PreferLinkedGpu)
            {
                RequiredInstanceExtensions.push_back(VK_KHX_DEVICE_GROUP_CREATION_EXTENSION_NAME);
                LogPrint(Log::Info, "Factory", "MultiGpu Extension Found: %s \n", VK_KHX_DEVICE_GROUP_CREATION_EXTENSION_NAME);
            }

            instanceCreateInfo.enabledExtensionCount = RequiredInstanceExtensions.size();
            instanceCreateInfo.ppEnabledExtensionNames = RequiredInstanceExtensions.data();
            CHECK(vkCreateInstance(&instanceCreateInfo, VULKAN_ALLOCATOR, &Handle));
        }
        ~VulkanFactory() override
        {
            vkDestroyInstance(Handle, VULKAN_ALLOCATOR);
        }

        std::vector<VkExtensionProperties> AvailableExtensions;

    private:
        friend class VulkanSwapChain;
        VkInstance Handle = VK_NULL_HANDLE;
    };

}


VulkanDevice::VulkanDevice(VulkanFactory* pFactory, VkPhysicalDevice PhysicalDevice)
    : OwningRoot(pFactory)
    , Device(PhysicalDevice)
    , TableAllocator(nullptr)
{
    OwningRoot->AddInternalRef();

    k3d::U32 queueCount = 0;
    std::vector<VkQueueFamilyProperties> QueueFamilyProps;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueCount, NULL);
    QueueFamilyProps.resize(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueCount, QueueFamilyProps.data());
    QueueInfos.resize(queueCount);
    vkGetPhysicalDeviceProperties(Device, &Prop);
    // Create Command Queue
    std::vector<VkDeviceQueueCreateInfo> DeviceQueueInfo;
    DeviceQueueInfo.resize(queueCount);
    std::vector<float*> QueuePriorities;
    // Async Compute & Transfer
    uint32_t GfxQueueId = 0;
    uint32_t CptQueueId = 0;
    for (k3d::U32 Id = 0; Id < queueCount; Id++)
    {
        QueueInfos[Id].Family = Id;
        QueueInfos[Id].Flags = QueueFamilyProps[Id].queueFlags;
        QueueInfos[Id].Count = QueueFamilyProps[Id].queueCount;
        if (QueueFamilyProps[Id].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            GfxQueueId = Id;
        }
        if (QueueFamilyProps[Id].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            CptQueueId = Id;
        }
        DeviceQueueInfo[Id].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        DeviceQueueInfo[Id].pNext = nullptr;
        DeviceQueueInfo[Id].flags = 0;
        DeviceQueueInfo[Id].queueFamilyIndex = Id;
        DeviceQueueInfo[Id].queueCount = QueueFamilyProps[Id].queueCount;
        float * Priorities = new float[QueueFamilyProps[Id].queueCount]{ 0 };
        DeviceQueueInfo[Id].pQueuePriorities = Priorities;
        QueuePriorities.push_back(Priorities);
    }
    IsSupportAsyncCompute = GfxQueueId != CptQueueId;

    uint32_t extCount = 0;
    std::vector<VkExtensionProperties> exts;
    vkEnumerateDeviceExtensionProperties(Device, nullptr, &extCount, nullptr);
    exts.resize(extCount);
    vkEnumerateDeviceExtensionProperties(Device, nullptr, &extCount, exts.data());
    std::set<std::string> deviceExtensions;
    LogPrint(Log::Info, "Device", "DumpVulkanDeviceExtensions:\n");
    for (auto ext : exts)
    {
        deviceExtensions.insert(ext.extensionName);
        LogPrint(Log::Info, "Device", "\t%s\n", ext.extensionName);
    }

    uint32_t layerCount = 0;
    std::vector<VkLayerProperties> layers;
    vkEnumerateDeviceLayerProperties(Device, &layerCount, nullptr);
    layers.resize(layerCount);
    vkEnumerateDeviceLayerProperties(Device, &layerCount, layers.data());

    vkGetPhysicalDeviceFeatures(Device, &Features);

    // extension diff
    VK_MAKE_VERSION(1, 0, 38);
    VK_MAKE_VERSION(1, 0, 46);
    VK_MAKE_VERSION(1, 0, 54);

    LogPrint(Log::Info, "Device", "Vendor: %s\n\tType: %s\n\tVulkan: %d.%d.%d\n\tAsyncCompute:%d\n\tMultiDraweIndirect: %d\n\tMultiViewport: %d\n",
        Prop.deviceName,
        DeviceType(Prop.deviceType),
        VK_VERSION_MAJOR(Prop.apiVersion),
        VK_VERSION_MINOR(Prop.apiVersion),
        VK_VERSION_PATCH(Prop.apiVersion), IsSupportAsyncCompute,
        Features.multiDrawIndirect, Features.multiViewport);

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = NULL;
    deviceCreateInfo.queueCreateInfoCount = DeviceQueueInfo.size();
    deviceCreateInfo.pQueueCreateInfos = DeviceQueueInfo.data();
    deviceCreateInfo.pEnabledFeatures = &Features;
    if (OwningRoot->Debug)
    {
        if (deviceExtensions.find(VK_EXT_DEBUG_MARKER_EXTENSION_NAME) != deviceExtensions.end())
        {
            RequiredDeviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        }
    }
    deviceCreateInfo.enabledLayerCount = RequiredLayers.size();
    deviceCreateInfo.ppEnabledLayerNames = RequiredLayers.data();

    // Multi GPU
    if (OwningRoot->PreferLinkedGpu)
    {
        const char* linkGpuExt = VK_KHX_DEVICE_GROUP_EXTENSION_NAME;
        bool linkedGpuSupported = true;
        if (deviceExtensions.find(linkGpuExt) != deviceExtensions.end())
        {
            LogPrint(Log::Info, "Device", "MultiGpuExtension found: %s\n", linkGpuExt);
            RequiredDeviceExtensions.push_back(linkGpuExt);
        }
        else
        {
            linkedGpuSupported = false;
        }
        LogPrint(Log::Info, "Device", "MultiGpu Availability: %d\n", linkedGpuSupported);

        VkDeviceGroupDeviceCreateInfoKHX deviceGroupCreateInfo = {};
        deviceGroupCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO_KHX;
        //deviceGroupCreateInfo.physicalDeviceCount = deviceGroupProperties.physicalDeviceCount;
        //deviceGroupCreateInfo.pPhysicalDevices = deviceGroupProperties.physicalDevices;
        deviceCreateInfo.pNext = &deviceGroupCreateInfo;
    }

    if (OwningRoot->PreferCrossVendor)
    {
        // external_memory on NVIDIA
        // external_semaphore 
    }

    if (OwningRoot->VRSupport)
    {
        // external_memory Oculus SDK
        // multi viewport
    }

    if (RequiredDeviceExtensions.size() > 0)
    {
        deviceCreateInfo.enabledExtensionCount = (uint32_t)RequiredDeviceExtensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = RequiredDeviceExtensions.data();
    }
    CHECK(vkCreateDevice(Device, &deviceCreateInfo, VULKAN_ALLOCATOR, &Handle));

    for (auto ptr : QueuePriorities)
    {
        delete[]ptr;
    }

    VmaAllocatorCreateInfo AllocCreateInfo = {
      Device, Handle, 0, 0,
      VULKAN_ALLOCATOR
    };
    vmaCreateAllocator(&AllocCreateInfo, &MemoryAllocator);
    TableAllocator = new BindTableAllocator(this);
}

VulkanDevice::~VulkanDevice()
{
    if (TableAllocator)
    {
        delete TableAllocator;
    }
    vmaDestroyAllocator(MemoryAllocator);
    vkDestroyDevice(Handle, VULKAN_ALLOCATOR);
    OwningRoot->ReleaseInternal();
}

void VulkanDevice::GetDesc(DeviceDesc * pDesc)
{
    //VkDebugMarkerObjectNameInfoEXT ext;
    //vkDebugMarkerSetObjectNameEXT(Handle, &ext);
}

Result VulkanDevice::CreateCommandQueue(CommandQueueType queueType, CommandQueue ** pQueue)
{
    VulkanQueue* pVkQueue = new VulkanQueue(this);
    switch (queueType)
    {
    case CommandQueueType::Graphics:
    {
        for (auto Info : QueueInfos)
        {
            if (Info.Flags & VK_QUEUE_GRAPHICS_BIT)
            {
                uint32_t qId = 0;
                vkGetDeviceQueue(Handle, Info.Family, qId, &pVkQueue->Handle);
                pVkQueue->FamilyId = Info.Family;
                pVkQueue->QueueId = qId;
                break;
            }
        }
        break;
    }
    case CommandQueueType::Compute:
    {
        for (auto Info : QueueInfos)
        {
            if (Info.Flags & VK_QUEUE_COMPUTE_BIT)
            {
                //uint32_t qId = 0;
                vkGetDeviceQueue(Handle, Info.Family, 0, &pVkQueue->Handle);
                pVkQueue->FamilyId = Info.Family;
                pVkQueue->QueueId = 0;
            }
        }
        break;
    }
    case CommandQueueType::Copy:
    {
        for (auto Info : QueueInfos)
        {
            if (Info.Flags & VK_QUEUE_TRANSFER_BIT)
            {
                //uint32_t qId = 0;
                vkGetDeviceQueue(Handle, Info.Family, 0, &pVkQueue->Handle);
                pVkQueue->FamilyId = Info.Family;
                pVkQueue->QueueId = 0;
            }
        }
        break;
    }
    }
    *pQueue = pVkQueue;
    return Result::Ok;
}

Result VulkanDevice::CreatePipelineLayout(const PipelineLayoutDesc * pPipelineLayoutDesc, PipelineLayout ** ppPipelineLayout)
{
    *ppPipelineLayout = new VulkanPipelineLayout(this, pPipelineLayoutDesc);
    return Result::Ok;
}

Result VulkanDevice::MakeBindTableEncoder(NotNull const ArgumentDesc * pArgumentDescs, int32_t argumentCount, BindTableEncoder ** ppBindingTableLayout)
{
    if (!pArgumentDescs || !argumentCount || !ppBindingTableLayout)
        return Result::ParamError;
    * ppBindingTableLayout = new VulkanBindTableEncoder(this, pArgumentDescs, argumentCount);
    return Result::Ok;
}

//Result VulkanDevice::CreateBindTable(NotNull const BindTableLayout * pBindTableLayout, BindTable ** ppBindingTable)
//{
//    return TableAllocator->Allocate(static_cast<const VulkanBindTableLayout*>(pBindTableLayout), ppBindingTable);
//}

void VulkanSwapChain::InitWithRenderPass(RenderPass * pRenderPass)
{
}

Result VulkanSwapChain::GetTexture(Texture ** ppTexture, uint32_t index)
{
    return Result::Ok;
}

Drawable * VulkanSwapChain::CurrentDrawable()
{
    return nullptr;
}

Drawable * VulkanSwapChain::NextDrawable()
{
    return nullptr;
}

uint32_t VulkanSwapChain::BufferCount()
{
    return 0U;
}

VulkanSwapChain::VulkanSwapChain(VulkanFactory* pFactory, void* pHandle, const SwapChainDesc* pDesc, VulkanQueue* pQueue)
    : OwningRoot(pFactory)
{
    OwningRoot->AddInternalRef();

    OwningDevice = pQueue->OwningRoot;
    OwningDevice->AddInternalRef();

    CreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    CreateInfo.imageArrayLayers = 1;
    CreateInfo.queueFamilyIndexCount = VK_SHARING_MODE_EXCLUSIVE;
    CreateInfo.oldSwapchain = VK_NULL_HANDLE;
    CreateInfo.clipped = VK_TRUE;
    CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    CreateInfo.imageFormat = ConvertPixelFormatToVulkanEnum(pDesc->pixelFormat);
    CreateInfo.imageExtent = { pDesc->width, pDesc->height };
    CreateInfo.minImageCount = pDesc->numColorBuffers;

#if K3DPLATFORM_OS_WIN
    VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo = {};
    SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    SurfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
    SurfaceCreateInfo.hwnd = (HWND)pHandle;
    vkCreateWin32SurfaceKHR(OwningRoot->Handle, &SurfaceCreateInfo, VULKAN_ALLOCATOR, &Surface);
#endif

    CreateInfo.surface = Surface;

    LogPrint(Log::Info, "SwapChain", "Chosen SwapChain Format %s.\n", StrVkFormat(CreateInfo.imageFormat));

    CHECK(vkCreateSwapchainKHR(OwningDevice->Handle, &CreateInfo, VULKAN_ALLOCATOR, &Handle));
    uint32_t ImageCount = 0;
    vkGetSwapchainImagesKHR(OwningDevice->Handle, Handle, &ImageCount, nullptr);
    vector<VkImage> Images(ImageCount);
    vkGetSwapchainImagesKHR(OwningDevice->Handle, Handle, &ImageCount, Images.data());
    // Initialize Image Views
    for (auto Image : Images)
    {
        new VulkanSwapChainTexture(this, Image, pDesc);
    }
    if (pDesc->hasDepthStencilTarget)
    {
        Ptr<Texture> depthStecilTexture;
        TextureDesc texDesc;
        texDesc.SetAllowedViewBits(TextureViewBit::DepthStencil).SetDepth(1)
            .SetWidth(CreateInfo.imageExtent.width).SetHeight(CreateInfo.imageExtent.height)
            .SetLayers(1).SetMipLevels(1)
            .SetFormat(pDesc->depthStencilFormat)
            .SetOption(StorageOption::Private);
        OwningDevice->CreateTexture(&texDesc, depthStecilTexture.GetAddressOf());

        Ptr<TextureView> depthStencilView;
        TextureViewDesc dsvDesc;
        dsvDesc.SetDimension(TextureDimension::Tex2D)
            .SetMipLevel(0)
            .SetView(ResourceViewType::SampledTexture)
            .SetState(ResourceState::Common);
        depthStecilTexture->CreateView(&dsvDesc, depthStencilView.GetAddressOf());

    }

    Ptr<FrameBuffer> presentFbo;
    //  FrameBufferDesc pFBoDesc;
    //  pFBoDesc.depthStencilAttachment = depthS
}

VulkanSwapChain::~VulkanSwapChain()
{
    vkDestroySwapchainKHR(OwningDevice->Handle, Handle, VULKAN_ALLOCATOR);
    OwningDevice->ReleaseInternal();
    vkDestroySurfaceKHR(OwningRoot->Handle, Surface, VULKAN_ALLOCATOR);
    OwningRoot->ReleaseInternal();
}

Result VulkanFactory::CreateSwapchain(const SwapChainDesc* desc, CommandQueue* pCommandQueue, void* pWindow, SwapChain** pSwapchain)
{
    *pSwapchain = new VulkanSwapChain(this, pWindow, desc, static_cast<VulkanQueue*>(pCommandQueue));
    return Result::Ok;
}

Result VulkanFactory::EnumDevice(uint32_t * count, Device ** ppDevice)
{
    if (!ppDevice)
    {
        vkEnumeratePhysicalDevices(Handle, count, nullptr);
    }
    else
    {
        if (count && *count > 0)
        {
            VkPhysicalDevice * PhysicalDevices = new VkPhysicalDevice[*count];
            vkEnumeratePhysicalDevices(Handle, count, PhysicalDevices);
            for (uint32_t i = 0; i < *count; i++)
            {
                ppDevice[i] = new VulkanDevice(this, PhysicalDevices[i]);
            }
            delete[] PhysicalDevices;
        }
        else
        {
            return Result::Failed;
        }
        //    vkEnumeratePhysicalDeviceGroupsKHX()
    }
    return Result::Ok;
}

Result VulkanTexture::GetDesc(TextureDesc * pDesc)
{
    if (!pDesc)
    {
        return Result::ParamError;
    }
    memcpy(pDesc, &ResDesc, sizeof(ResDesc));
    return Result::Ok;
}


VulkanTextureView::VulkanTextureView(const TextureViewDesc * pDesc, VulkanTexture * pTexture)
    : OwningTexture(pTexture)
    , Info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO }
{
    OwningTexture->AddInternalRef();
    Info.image = pTexture->GetHandle();
    Info.format = pTexture->Info.format;
    Info.viewType = ConvertTextureDimensionToVulkanEnum(pDesc->dimension);
    Info.components = { VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_G,VK_COMPONENT_SWIZZLE_B,VK_COMPONENT_SWIZZLE_A };
    TextureDesc texDesc;
    pTexture->GetDesc(&texDesc);
    Info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    if (texDesc.allowedViewBits & TextureViewBit::DepthStencil)
    {
        Info.subresourceRange.aspectMask |= (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    }
    if (texDesc.allowedViewBits & TextureViewBit::RenderTarget)
    {
        Info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
    }
    CHECK(vkCreateImageView(OwningTexture->OwningDevice->Handle, &Info, VULKAN_ALLOCATOR, &Handle));
}

VulkanTextureView::~VulkanTextureView()
{
    vkDestroyImageView(OwningTexture->OwningDevice->Handle, Handle, VULKAN_ALLOCATOR);
    OwningTexture->ReleaseInternal();
}

Result VulkanTexture::CreateView(const TextureViewDesc * pDesc, TextureView ** ppView)
{
    if (!pDesc || !ppView)
        return Result::ParamError;
    *ppView = new VulkanTextureView(pDesc, this);
    return Result::Ok;
}

Result VulkanDevice::CreateRenderPipeline(NotNull const RenderPipelineDesc * pPipelineDesc, Nullable RenderPass * pRenderPass, Pipeline ** ppPipelineState, NotNull PipelineReflection ** ppReflection)
{
    if (!pPipelineDesc || !pRenderPass || !ppPipelineState || !pPipelineDesc->vertexFunction)
        return Result::ParamError;
    *ppPipelineState = new VulkanRenderPipeline(this, pPipelineDesc, pRenderPass, nullptr);
    return Result::Ok;
}

Result VulkanDevice::CreateComputePipeline(NotNull Function * pComputeFunction, Pipeline ** ppPipeline, NotNull PipelineReflection ** ppReflection)
{
    if (!pComputeFunction || !ppPipeline)
        return Result::ParamError;
    *ppPipeline = new VulkanComputePipeline(this, pComputeFunction);
    return Result::Ok;
}

Result VulkanDevice::CreateComputePipeline(NotNull const Function * pComputeFunction, NotNull const PipelineLayout * pPipelineLayout, NotNull Pipeline ** ppPipeline)
{
    if (!pComputeFunction || !pPipelineLayout || !ppPipeline)
        return Result::ParamError;
    *ppPipeline = new VulkanComputePipeline(this, pComputeFunction, pPipelineLayout);
    return Result::Ok;
}

Result VulkanDevice::CreatePipelineLibrary(const void * pData, uint64_t Size, PipelineLibrary ** ppPipelineLibrary)
{
    if (!pData || Size <= 0 || !ppPipelineLibrary)
        return Result::ParamError;
    *ppPipelineLibrary = new VulkanPipelineLibrary(this, pData, Size);
    return Result::Ok;
}

Result VulkanDevice::CreateLibrary(const CompileOption * compileOption, const void * pData, uint64_t Size, Library ** ppLibrary)
{
    if (!pData || Size <= 0 || !ppLibrary)
        return Result::ParamError;
    *ppLibrary = new VulkanLibrary1(this, pData, Size);
    return Result::Ok;
}

VulkanRenderPass::VulkanRenderPass(VulkanDevice * pDevice, const RenderPassDesc * pDesc)
    : Device(pDevice)
{
    Device->AddInternalRef();

    VkRenderPassCreateInfo Info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };

    std::vector<VkAttachmentReference> colorRefers;
    std::vector<VkAttachmentDescription> attachments;

    Info.attachmentCount = pDesc->colorAttachmentsCount + pDesc->pDepthStencilAttachment ? 1 : 0;
    attachments.resize(Info.attachmentCount);
    memset(attachments.data(), 0, Info.attachmentCount * sizeof(VkAttachmentDescription));

    k3d::U32 colorId = 0;

    for (int i = 0; i < pDesc->colorAttachmentsCount; i++)
    {
        TextureDesc texDesc;
        pDesc->pColorAttachments[i].texture->GetDesc(&texDesc);
        attachments[i].format = ConvertPixelFormatToVulkanEnum(texDesc.format);
        attachments[i].loadOp = ConvertLoadActionToVulkanEnum(pDesc->pColorAttachments[i].loadAction);
        attachments[i].storeOp = ConvertStoreActionToVulkanEnum(pDesc->pColorAttachments[i].storeAction);
        attachments[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[i].samples = ConvertMultiSampleFlagToVulkanEnum(texDesc.samples);
        attachments[i].flags = 0;
        colorRefers.push_back({ colorId, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
        colorId++;
    }

    // Create default subpass
    VkSubpassDescription defaultSubpass = {
      0,                                //VkSubpassDescriptionFlags       flags;
      VK_PIPELINE_BIND_POINT_GRAPHICS,  //VkPipelineBindPoint             pipelineBindPoint;
      0,                                //uint32_t                        inputAttachmentCount;
      nullptr,                          //const VkAttachmentReference*    pInputAttachments;
      colorRefers.size(),               //uint32_t                        colorAttachmentCount;
      colorRefers.data(),               //const VkAttachmentReference*    pColorAttachments;
      nullptr,                          //const VkAttachmentReference*    pResolveAttachments;
                                        //const VkAttachmentReference*    pDepthStencilAttachment;
                                        //uint32_t                        preserveAttachmentCount;
                                        //const uint32_t*                 pPreserveAttachments;
    };

    VkAttachmentReference depthStencilRefer = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    if (pDesc->pDepthStencilAttachment)
    {
        TextureDesc texDesc;
        pDesc->pDepthStencilAttachment->texture->GetDesc(&texDesc);
        attachments[pDesc->colorAttachmentsCount].format = ConvertPixelFormatToVulkanEnum(texDesc.format);
        attachments[pDesc->colorAttachmentsCount].stencilLoadOp = ConvertLoadActionToVulkanEnum(pDesc->pDepthStencilAttachment->loadAction);
        attachments[pDesc->colorAttachmentsCount].stencilStoreOp = ConvertStoreActionToVulkanEnum(pDesc->pDepthStencilAttachment->storeAction);
        attachments[pDesc->colorAttachmentsCount].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[pDesc->colorAttachmentsCount].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        defaultSubpass.pDepthStencilAttachment = &depthStencilRefer;
    }

    Info.pAttachments = attachments.data();
    Info.subpassCount = 1;
    Info.pSubpasses = &defaultSubpass;

    VkSubpassDependency dependency = { 0 };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = 0;
    Info.dependencyCount = 1;
    Info.pDependencies = &dependency;

    CHECK(vkCreateRenderPass(Device->Handle, &Info, VULKAN_ALLOCATOR, &Handle));
}

VulkanRenderPass::~VulkanRenderPass()
{
    vkDestroyRenderPass(Device->Handle, Handle, VULKAN_ALLOCATOR);
    Device->ReleaseInternal();
}

Result VulkanRenderPass::MakeFrameBuffer(
    const TextureView ** pRenderTargetViews, int32_t viewCount, 
    int32_t width, int32_t height, int32_t layers, 
    FrameBuffer ** ppFramebuffer)
{
    if (!pRenderTargetViews || viewCount < 1 || width <= 0 || height <= 0 || layers <= 0 || !ppFramebuffer)
        return Result::ParamError;
    *ppFramebuffer = new VulkanDrawable(Device, this, pRenderTargetViews, viewCount, width, height, layers);
    return Result::Ok;
}

Result VulkanRenderPass::MakeRenderPipeline(NotNull const RenderPipelineDesc * pPipelineDesc, NotNull const PipelineLayout * pPipelineLayout, Pipeline ** ppPipeline)
{
    if (!pPipelineDesc || !pPipelineLayout || !ppPipeline)
        return Result::ParamError;
    *ppPipeline = new VulkanRenderPipeline(Device, pPipelineDesc, this, pPipelineLayout);
    return Result::Ok;
}

Result VulkanDevice::CreateRenderPass(const RenderPassDesc * desc, RenderPass ** ppRenderpass)
{
    if (!desc || !ppRenderpass)
        return Result::ParamError;
    *ppRenderpass = new VulkanRenderPass(this, desc);
    return Result::Ok;
}

Result VulkanDevice::CreateSampler(const SamplerDesc * desc, Sampler ** pSampler)
{
    *pSampler = new VulkanSampler(this, desc);
    return Result::Ok;
}

Result VulkanDevice::CreateBuffer(const BufferDesc * desc, Buffer ** pBuffer)
{
    if (!desc || !pBuffer)
        return Result::ParamError;
    *pBuffer = new VulkanBuffer(this, desc);
    return Result::Ok;
}

Result VulkanDevice::CreateTexture(const TextureDesc * desc, Texture ** pTexture)
{
    if (!desc || !pTexture)
        return Result::ParamError;
    *pTexture = new VulkanTexture(this, desc);
    return Result::Ok;
}

Result VulkanDevice::CreateFence(Fence ** ppFence)
{
    if (!ppFence)
        return Result::ParamError;
    *ppFence = new VulkanFence(this);
    return Result::Ok;
}

void VulkanDevice::WaitIdle()
{
    vkDeviceWaitIdle(Handle);
}

VulkanSampler::VulkanSampler(VulkanDevice* pDevice, const SamplerDesc* pDesc)
    : OwningDevice(pDevice)
{
    OwningDevice->AddInternalRef();
    Info.magFilter = ConvertFilterModeToVulkanEnum(pDesc->filter.magFilter);
    Info.minFilter = ConvertFilterModeToVulkanEnum(pDesc->filter.minFilter);
    Info.mipmapMode = pDesc->filter.mipMapFilter == FilterMode::Linear ?
        VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    Info.addressModeU = ConvertAddressModeToVulkanEnum(pDesc->U);
    Info.addressModeV = ConvertAddressModeToVulkanEnum(pDesc->V);
    Info.addressModeW = ConvertAddressModeToVulkanEnum(pDesc->W);
    Info.mipLodBias = pDesc->mipLodBias;
    Info.anisotropyEnable;
    Info.maxAnisotropy = pDesc->maxAnistropy;
    Info.compareEnable;
    Info.compareOp = ConvertComparisonFunctionToVulkanEnum(pDesc->comparisonFunc);
    Info.minLod = pDesc->minLod;
    Info.maxLod = pDesc->maxLod;
    Info.borderColor;
    Info.unnormalizedCoordinates;
    CHECK(vkCreateSampler(OwningDevice->Handle, &Info, VULKAN_ALLOCATOR, &Handle));
}

VulkanSampler::~VulkanSampler()
{
    if (Handle)
    {
        vkDestroySampler(OwningDevice->Handle, Handle, VULKAN_ALLOCATOR);
    }
    OwningDevice->ReleaseInternal();
}

Result VulkanSampler::GetDesc(SamplerDesc * desc)
{
    return Result::Ok;
}

VulkanComputePipeline::VulkanComputePipeline(VulkanDevice * pDevice, Function * pComputeFunc/*, PipelineLayout * pLayout*/)
    : Super(pDevice)
{
    Info = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr, 0 };

    Layout = new VulkanPipelineLayout(pDevice, static_cast<const VulkanFunction1*>(pComputeFunc));
    //VulkanPipelineLayout* layout = static_cast<VulkanPipelineLayout*>(pLayout);
    Info.layout = Layout->Handle;

    VulkanFunction1* computeFunction = static_cast<VulkanFunction1*>(pComputeFunc);
    Info.stage = computeFunction->StageInfo;
    CHECK(CreatePipelineCache(nullptr, 0));
    CHECK(CreatePipeline());
}

VulkanComputePipeline::VulkanComputePipeline(VulkanDevice* pDevice, const Function* pComputeFunc, const PipelineLayout * pLayout)
    : Super(pDevice)
{
    Info = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr, 0 };

    const VulkanPipelineLayout* layout = static_cast<const VulkanPipelineLayout*>(pLayout);
    Info.layout = static_cast<const VulkanPipelineLayout*>(pLayout)->Handle;

    const VulkanFunction1* computeFunction = static_cast<const VulkanFunction1*>(pComputeFunc);
    Info.stage = computeFunction->StageInfo;
    CHECK(CreatePipelineCache(nullptr, 0));
    CHECK(CreatePipeline());
}

VulkanComputePipeline::~VulkanComputePipeline()
{
}

VulkanRenderPipeline::VulkanRenderPipeline(VulkanDevice * pDevice, const RenderPipelineDesc * pDesc, RenderPass * pRenderPass, const PipelineLayout * pLayout)
    : VulkanRenderPipeline::Super(pDevice)
{
    Info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

    InitShaderStages(pDesc);

    Info.stageCount = Stages.size();
    Info.pStages = Stages.data();

    InitRasterState(pDesc);
    Info.pRasterizationState = &RasterizationState;
    Info.pMultisampleState = &MultisampleState;

    InitDepthStencilState(pDesc);
    Info.pDepthStencilState = &DepthStencilState;

    InitBlendState(pDesc);
    Info.pColorBlendState = &BlendState;

    InitInputState(pDesc);
    Info.pInputAssemblyState = &InputAssemblyState;
    Info.pVertexInputState = &VertexInputState;

    ViewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    Info.pViewportState = &ViewportState;

    /* dynamic state set up */
    DynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    Info.pDynamicState = &DynamicState;

    TessellationState = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
    Info.pTessellationState = &TessellationState;

    VulkanRenderPass* renderPass = static_cast<VulkanRenderPass*>(pRenderPass);
    const VulkanPipelineLayout* pipelineLayout = static_cast<const VulkanPipelineLayout*>(pLayout);
    assert(renderPass && pipelineLayout);

    Info.renderPass = renderPass->Handle;
    Info.layout = pipelineLayout->Handle;

    Info.subpass = 0;
    Info.basePipelineHandle = VK_NULL_HANDLE;
    Info.basePipelineIndex = 0;

    CHECK(CreatePipelineCache(nullptr, 0));
    CHECK(CreatePipeline());
}

void VulkanRenderPipeline::InitShaderStages(const RenderPipelineDesc* pDesc)
{
    if (pDesc->vertexFunction)
    {
        auto vertShader = static_cast<VulkanFunction1*>(pDesc->vertexFunction);
        Stages.push_back(vertShader->StageInfo);
    }
    if (pDesc->pixelFunction)
    {
        auto pixelShader = static_cast<VulkanFunction1*>(pDesc->pixelFunction);
        Stages.push_back(pixelShader->StageInfo);
    }
}

void VulkanRenderPipeline::InitRasterState(const RenderPipelineDesc * pDesc)
{
    auto Rs = pDesc->rasterState;
    RasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    RasterizationState.polygonMode = ConvertFillModeToVulkanEnum(Rs.fillMode);
    RasterizationState.cullMode = ConvertCullModeToVulkanEnum(Rs.cullMode);
    RasterizationState.frontFace = Rs.frontCCW ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    RasterizationState.rasterizerDiscardEnable = VK_TRUE;
    RasterizationState.depthBiasEnable = Rs.depthBias > 0 ? VK_TRUE : VK_FALSE;
    RasterizationState.depthClampEnable = Rs.depthClipEnable;
    RasterizationState.depthBiasClamp = Rs.depthBiasClamp;
    RasterizationState.depthBiasConstantFactor = Rs.depthBias;
    RasterizationState.depthBiasSlopeFactor = Rs.depthBiasSlope;
    RasterizationState.lineWidth = 1;

    MultisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    MultisampleState.alphaToCoverageEnable = pDesc->blendState.alphaToCoverageEnable;
}

void VulkanRenderPipeline::InitDepthStencilState(const RenderPipelineDesc* pDesc)
{
    auto Ds = pDesc->depthStencil;
    DepthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    DepthStencilState.depthTestEnable = Ds.depthEnable;
    DepthStencilState.stencilTestEnable = Ds.stencilEnable;
}

void VulkanRenderPipeline::InitBlendState(const RenderPipelineDesc* pDesc)
{
    auto Bs = pDesc->blendState;
    BlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    BlendAttachState.resize(pDesc->numRenderTargets);
    BlendState.attachmentCount = pDesc->numRenderTargets;
    for (uint32_t i = 0; i < pDesc->numRenderTargets; i++)
    {
        BlendAttachState[i].blendEnable = Bs.renderTargets[i].enable;
        BlendAttachState[i].alphaBlendOp = ConvertBlendOperationToVulkanEnum(Bs.renderTargets[i].alphaBlendOp);
        BlendAttachState[i].colorBlendOp = ConvertBlendOperationToVulkanEnum(Bs.renderTargets[i].colorBlendOp);
        BlendAttachState[i].colorWriteMask = Bs.renderTargets[i].colorWriteMask;
        BlendAttachState[i].srcAlphaBlendFactor = ConvertBlendTypeToVulkanEnum(Bs.renderTargets[i].srcAlphaBlend);
        BlendAttachState[i].dstAlphaBlendFactor = ConvertBlendTypeToVulkanEnum(Bs.renderTargets[i].destAlphaBlend);
        BlendAttachState[i].srcColorBlendFactor = ConvertBlendTypeToVulkanEnum(Bs.renderTargets[i].srcColorBlend);
        BlendAttachState[i].dstColorBlendFactor = ConvertBlendTypeToVulkanEnum(Bs.renderTargets[i].destColorBlend);
    }
    BlendState.pAttachments = BlendAttachState.data();
    BlendState.logicOpEnable = Bs.logicOpEnable;
    BlendState.logicOp = ConvertLogicOperationToVulkanEnum(Bs.logicOp);
}

void VulkanRenderPipeline::InitInputState(const RenderPipelineDesc * pDesc)
{
    auto Is = pDesc->inputState;
    assert(Is.attributeCount);
    InputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    InputAssemblyState.topology = ConvertPrimitiveTypeToVulkanEnum(pDesc->primitiveTopology);

    VertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    VertexInputAttributes.resize(Is.attributeCount);
    for (uint32_t i = 0; i < Is.attributeCount; i++)
    {
        VertexInputAttributes[i].binding = Is.pAttributes[i].slot;
        VertexInputAttributes[i].offset = Is.pAttributes[i].offset;
        VertexInputAttributes[i].format = g_VertexFormatTable[(uint32_t)Is.pAttributes[i].format];
        VertexInputAttributes[i].location = i;
    }
    VertexInputState.vertexAttributeDescriptionCount = VertexInputAttributes.size();
    VertexInputState.pVertexAttributeDescriptions = VertexInputAttributes.data();

    VertexInputBindings.resize(Is.layoutCount);
    for (uint32_t i = 0; i < Is.layoutCount; i++)
    {
        VertexInputBindings[i].binding = i;
        VertexInputBindings[i].stride = Is.pLayouts[i].stride;
        VertexInputBindings[i].inputRate = ConvertVertexInputRateToVulkanEnum(Is.pLayouts[i].rate);
    }
    VertexInputState.vertexBindingDescriptionCount = VertexInputBindings.size();
    VertexInputState.pVertexBindingDescriptions = VertexInputBindings.data();
}

VulkanRenderPipeline::~VulkanRenderPipeline()
{
}

Result VulkanRenderPipeline::GetDesc(RenderPipelineDesc * pDesc)
{
    return Result::Ok;
}

template<class T>
TPipeline<T>::TPipeline(VulkanDevice * pDevice)
    : OwningDevice(pDevice)
{
    OwningDevice->AddInternalRef();
}

template<class T>
TPipeline<T>::~TPipeline()
{
    if (Handle)
    {
        vkDestroyPipeline(OwningDevice->Handle, Handle, VULKAN_ALLOCATOR);
    }
    if (Cache)
    {
        vkDestroyPipelineCache(OwningDevice->Handle, Cache, VULKAN_ALLOCATOR);
    }
    OwningDevice->ReleaseInternal();
}

VulkanPipelineLibrary::VulkanPipelineLibrary(VulkanDevice * pDevice, const void * InData, uint64_t Size)
    : Device(pDevice), Handle(VK_NULL_HANDLE)
{
    Device->AddInternalRef();
    VkPipelineCacheCreateInfo Info = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, nullptr, 0, Size, InData };
    CHECK(vkCreatePipelineCache(Device->Handle, &Info, VULKAN_ALLOCATOR, &Handle));
}

VulkanPipelineLibrary::~VulkanPipelineLibrary()
{
    vkDestroyPipelineCache(Device->Handle, Handle, VULKAN_ALLOCATOR);
    Device->ReleaseInternal();
}

void VulkanPipelineLibrary::StorePipeline(const char * key, Pipeline * pPipeline)
{
    if (pPipeline)
    {
        switch (pPipeline->Type())
        {
        case PipelineType::Graphics:
        {
            auto Gfx = static_cast<VulkanRenderPipeline*>(pPipeline);
            if (Gfx->Cache)
                CHECK(vkMergePipelineCaches(Device->Handle, Handle, 1, &Gfx->Cache));
            break;
        }
        case PipelineType::Compute:
        {
            auto Cpt = static_cast<VulkanComputePipeline*>(pPipeline);
            if (Cpt->Cache)
                CHECK(vkMergePipelineCaches(Device->Handle, Handle, 1, &Cpt->Cache));
            break;
        }
        }
    }
}

uint64_t VulkanPipelineLibrary::GetSerializedSize() const
{
    uint64_t Size = 0;
    CHECK(vkGetPipelineCacheData(Device->Handle, Handle, &Size, nullptr));
    return Size;
}

void VulkanPipelineLibrary::Serialize(void * pData, uint64_t Size)
{
    CHECK(vkGetPipelineCacheData(Device->Handle, Handle, &Size, pData));
}

VkDescriptorType InferDescriptorType(DataType const& dType, EnumAsUint32< ArgumentAccess > const& access, TextureDimension const& Dim)
{
    switch (dType)
    {
    case DataType::Sampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case DataType::Texture:
    {
        if (Dim == TextureDimension::Buffer)
        {
            if (access & ArgumentAccess::WriteOnly)
                return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            else
                return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        }
        else
        {
            if (access == ArgumentAccess::ReadOnly)
                return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            else if (access & ArgumentAccess::WriteOnly)
                return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            else
                return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        }
    }
    case DataType::Array:
    case DataType::Pointer:
    {
        if (access == ArgumentAccess::ReadOnly)
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        else if (access & ArgumentAccess::WriteOnly)
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        else
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    }
    default:
        if (Dim == TextureDimension::Buffer)
        {
            if (access & ArgumentAccess::WriteOnly)
                return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            else
                return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        }
        else
        {
            if (access == ArgumentAccess::ReadOnly)
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            else
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        }
    }
}

VulkanBindTableEncoder::VulkanBindTableEncoder(VulkanDevice * pDevice, const ArgumentDesc * pArgumentDescs, int32_t argumentCount)
    : OwningDevice(pDevice)
{
    OwningDevice->AddInternalRef();
    DescriptorBindings.resize(argumentCount);
    for (int32_t i = 0; i < argumentCount; i++)
    {
        auto const& Desc = pArgumentDescs[i];
        auto & Binding = DescriptorBindings[i];
        Binding.binding = Desc.index;
        Binding.descriptorCount = 1;
        Binding.descriptorType = InferDescriptorType(Desc.dataType, Desc.access, Desc.textureDim);
        Binding.stageFlags = ConvertShaderStageBit(Desc.stage);
        Binding.pImmutableSamplers = NULL;
    }
    Info.pBindings = DescriptorBindings.data();
    Info.bindingCount = DescriptorBindings.size();
    CHECK(vkCreateDescriptorSetLayout(OwningDevice->Handle, &Info, VULKAN_ALLOCATOR, &Handle));
}

VulkanBindTableEncoder::~VulkanBindTableEncoder()
{
    if(Handle)
    {
      vkDestroyDescriptorSetLayout(OwningDevice->Handle, Handle, VULKAN_ALLOCATOR);
      Handle = VK_NULL_HANDLE;
    }
    OwningDevice->ReleaseInternal();
}

Result VulkanBindTableEncoder::Allocate(BindTable ** ppBindTable)
{
    return OwningDevice->TableAllocator->Allocate(this, ppBindTable);
}

//void VulkanBindTableEncoder::AddBuffer(uint32_t slot, uint32_t count, ShaderStageBit visibility)
//{
//    DescriptorBindings.push_back({ slot, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count, ConvertShaderStageBit(visibility), nullptr });
//}
//
//void VulkanBindTableEncoder::AddTexture(uint32_t slot, uint32_t count, ShaderStageBit visibility)
//{
//    DescriptorBindings.push_back({ slot, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, count, ConvertShaderStageBit(visibility), nullptr });
//}
//
//void VulkanBindTableEncoder::AddSampler(uint32_t slot, ShaderStageBit visibility)
//{
//    DescriptorBindings.push_back({ slot, VK_DESCRIPTOR_TYPE_SAMPLER, 1, ConvertShaderStageBit(visibility), nullptr });
//}

//VulkanBindTableLayout::VulkanBindTableLayout(VulkanBindTableEncoder * pInitializer)
//    : OwningInitializer(pInitializer)
//{
//    OwningInitializer->AddInternalRef();
//    CHECK(vkCreateDescriptorSetLayout(
//        OwningInitializer->OwningDevice->Handle,
//        &pInitializer->Info, VULKAN_ALLOCATOR, &Handle));
//}

//VulkanBindTableLayout::~VulkanBindTableLayout()
//{
//    vkDestroyDescriptorSetLayout(OwningInitializer->OwningDevice->Handle, Handle, VULKAN_ALLOCATOR);
//    OwningInitializer->ReleaseInternal();
//}

//Result VulkanBindTableEncoder::Initialize(BindTableLayout ** ppBindTableLayout)
//{
//    Info.bindingCount = DescriptorBindings.size();
//    Info.pBindings = DescriptorBindings.data();
//    *ppBindTableLayout = new VulkanBindTableLayout(this);
//    return Result::Ok;
//}

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice * pDevice, const ngfx::PipelineLayoutDesc * pDesc)
    : Device(pDevice)
{
    Device->AddInternalRef();
    assert(pDesc && pDesc->shaderLayoutCount);
    std::vector<VkDescriptorSetLayout> setLayouts;
    VkPipelineLayoutCreateInfo Info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    Info.setLayoutCount = pDesc->shaderLayoutCount;
    setLayouts.resize(Info.setLayoutCount);
    for (uint32_t i = 0; i < pDesc->shaderLayoutCount; i++) // error
    {
        setLayouts[i] = static_cast<const VulkanBindTableEncoder*>(pDesc->pTableEncoders + i)->Handle;
    }
    Info.pSetLayouts = setLayouts.data();
    CHECK(vkCreatePipelineLayout(Device->Handle, &Info, VULKAN_ALLOCATOR, &Handle));
}

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice * pDevice, const VulkanFunction1 * pComputeShader)
    : Device(pDevice)
{
    CreateWithComputeFunction(pComputeShader);
}

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice * pDevice,
    NotNull const VulkanFunction1 * pVertexShader,
    NotNull const VulkanFunction1 * pPixelShader,
    Nullable const VulkanFunction1 * pGeometryShader,
    Nullable const VulkanFunction1 * pDomainShader,
    Nullable const VulkanFunction1 * pHullShader)
    : Device(pDevice)
{

}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
    if (Handle)
    {
        vkDestroyPipelineLayout(Device->Handle, Handle, VULKAN_ALLOCATOR);
    }
    Device->ReleaseInternal();
}

void VulkanPipelineLayout::CreateWithComputeFunction(const VulkanFunction1 * pCompute)
{
    Ptr<PipelineReflection> Reflection(new spirv_cross::Reflection(pCompute->GetByteCode()));
    auto Count = Reflection->VariableCount();
    for (uint32_t i = 0; i < Count; i++)
    {
        auto Var = Reflection->VariableAt(i);
        auto Type = Var->Type();
        auto tType = Type->GetType();
    }
}

VulkanBindTable::VulkanBindTable(VulkanDevice* pDevice, VkDescriptorSet Ds)
    : Handle(Ds)
    , Device(pDevice)
{
    Device->AddInternalRef();
}

VulkanBindTable::~VulkanBindTable()
{
    Device->ReleaseInternal();
}

void VulkanBindTable::SetSampler(uint32_t index, ShaderType shaderVis, Sampler * pSampler)
{
    WriteDescriptorSets[index] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, Handle, index, 0 };
    WriteDescriptorSets[index].descriptorCount = 1;
    WriteDescriptorSets[index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    VkDescriptorImageInfo Info = { static_cast<VulkanSampler*>(pSampler)->Handle, VK_NULL_HANDLE };
    WriteDescriptorSets[index].pImageInfo = &Info;
    vkUpdateDescriptorSets(Device->Handle, static_cast<uint32_t>(WriteDescriptorSets.size()), WriteDescriptorSets.data(), 0, NULL);
}

void VulkanBindTable::SetBuffer(uint32_t index, ShaderType shaderVis, BufferView * bufferView)
{
    auto pBufferView = static_cast<VulkanBufferView*>(bufferView);
    auto& targetDesc = WriteDescriptorSets[index];
    targetDesc = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, Handle, index, 0 };
    targetDesc.descriptorCount = 1;
    targetDesc.descriptorType = pBufferView->DesriptorType;
    if (pBufferView->IsTexelBuffer)
    {
        targetDesc.pTexelBufferView = &pBufferView->Handle;
        vkUpdateDescriptorSets(Device->Handle, static_cast<uint32_t>(WriteDescriptorSets.size()), WriteDescriptorSets.data(), 0, NULL);
    }
    else
    {
        VkDescriptorBufferInfo bufferInfo = { pBufferView->GetRawBuffer(), pBufferView->Info.offset, pBufferView->Info.range };
        targetDesc.pBufferInfo = &bufferInfo;
        vkUpdateDescriptorSets(Device->Handle, static_cast<uint32_t>(WriteDescriptorSets.size()), WriteDescriptorSets.data(), 0, NULL);
    }
}

void VulkanBindTable::SetTexture(uint32_t index, ShaderType shaderVis, TextureView * textureView)
{
    WriteDescriptorSets[index] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, Handle, index, 0 };
    WriteDescriptorSets[index].descriptorCount = 1;
    WriteDescriptorSets[index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    /*VkDescriptorImageInfo Info = { VK_NULL_HANDLE, static_cast<VulkanTextureView*>(textureView)->Handle, VK_NULL_HANDLE };
    WriteDescriptorSets[index].pImageInfo = &Info;
    vkUpdateDescriptorSets(Device->Handle, static_cast<uint32_t>(WriteDescriptorSets.size()), WriteDescriptorSets.data(), 0, NULL);*/
}

VulkanFence::VulkanFence(VulkanDevice * pDevice)
    : OwningDevice(pDevice)
{
    OwningDevice->AddInternalRef();
    VkFenceCreateInfo Info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    vkCreateFence(OwningDevice->Handle, &Info, VULKAN_ALLOCATOR, &Handle);
}

VulkanFence::~VulkanFence()
{
    if (Handle)
    {
        vkDestroyFence(OwningDevice->Handle, Handle, VULKAN_ALLOCATOR);
    }
    OwningDevice->ReleaseInternal();
}

void VulkanFence::Wait()
{
}

void VulkanFence::Reset()
{
}

VulkanDrawable::VulkanDrawable(VulkanDevice* pDevice, const FrameBufferDesc* pDesc)
    : Device(pDevice)
{
    Device->AddInternalRef();
    VkFramebufferCreateInfo CreateInfo = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO
    };
    // How to process render pass ? Prebuild ?
    CreateInfo.renderPass;

    int attachmentNum = 0;
    for (auto attachment : pDesc->colorAttachments)
    {
        if (attachment)
        {
            attachmentNum++;
        }
    }

    if (pDesc->colorAttachments[0])
    {
        TextureDesc desc;
        pDesc->colorAttachments[0]->GetDesc(&desc);
        CreateInfo.width = desc.width;
        CreateInfo.height = desc.height;
    }
    if (pDesc->depthStencilAttachment)
    {
        attachmentNum++;
    }

    CreateInfo.attachmentCount = attachmentNum;
    CreateInfo.pAttachments;
    CreateInfo.layers = pDesc->layers;
    CreateInfo.flags;
    CHECK(vkCreateFramebuffer(Device->Handle, nullptr, VULKAN_ALLOCATOR, &Handle));
}

VulkanDrawable::VulkanDrawable(VulkanDevice * pDevice, const VulkanRenderPass * pRenderPass, const TextureView ** pRenderTargetViews, int32_t viewCount, int32_t width, int32_t height, int32_t layers)
    : Device(pDevice)
{
    Device->AddInternalRef();
    VkFramebufferCreateInfo CreateInfo = {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO
    };
    CreateInfo.renderPass = pRenderPass->Handle;
    CreateInfo.attachmentCount = viewCount;
    CreateInfo.width = width;
    CreateInfo.height = height;
    CreateInfo.layers = layers;

    std::vector<VkImageView> rawViews;
    rawViews.resize(viewCount);
    for (int i =0; i<viewCount; i++)
    {
        const TextureView* view = pRenderTargetViews[i];
        const VulkanTextureView* rawView = static_cast<const VulkanTextureView*>(view);
        assert(rawView);
        //Attachments.push_back(rawView);
        /*rawView->AddInternalRef();*/
        rawViews[i] = rawView->Handle;
    }
    CreateInfo.pAttachments = rawViews.data();
    CHECK(vkCreateFramebuffer(Device->Handle, &CreateInfo, VULKAN_ALLOCATOR, &Handle));
}

VulkanDrawable::~VulkanDrawable()
{
   /* for (auto* ptr : Attachments)
    {
        assert(ptr);
        ptr->ReleaseInternal();
    }*/
    vkDestroyFramebuffer(Device->Handle, Handle, VULKAN_ALLOCATOR);
    Device->ReleaseInternal();
}

void VulkanCommandBuffer::Commit(Fence * pFence)
{
    vkQueueSubmit(OwningRoot->Handle, 0, nullptr, static_cast<VulkanFence*>(pFence)->Handle);
}

template<class T>
TCmdEncoder<T>::TCmdEncoder(VulkanCommandBuffer* pCmd, VkPipelineBindPoint Point)
    : OwningCommand(pCmd)
    , CurrentBindingPoint(Point)
{
}

template<class T>
TCmdEncoder<T>::~TCmdEncoder()
{
}

template<class T>
void TCmdEncoder<T>::Barrier(Resource * pResource)
{
}

template<class T>
void TCmdEncoder<T>::SetPipeline(Pipeline * pPipelineState)
{
    if (pPipelineState)
    {
        switch (pPipelineState->Type())
        {
        case PipelineType::Compute:
        {
            VulkanComputePipeline* pipeline = static_cast<VulkanComputePipeline*>(pPipelineState);
            vkCmdBindPipeline(OwningCommand->Handle, CurrentBindingPoint, pipeline->Handle);
            break;
        }
        case PipelineType::Graphics:
        {
            VulkanRenderPipeline* pipeline = static_cast<VulkanRenderPipeline*>(pPipelineState);
            vkCmdBindPipeline(OwningCommand->Handle, CurrentBindingPoint, pipeline->Handle);
            break;
        }
        }
    }
}

template<class T>
void TCmdEncoder<T>::SetBindTable(BindTable * pBindTable)
{
    VulkanBindTable* BindTable = static_cast<VulkanBindTable*>(pBindTable);
    VkDescriptorSet sets[] = { BindTable->Handle };
    vkCmdBindDescriptorSets(OwningCommand->Handle, CurrentBindingPoint,
        BindTable->OwningLayout->Handle, 0, 1, sets, 0, nullptr);
}

template<class T>
void TCmdEncoder<T>::EndEncode()
{
    vkEndCommandBuffer(OwningCommand->Handle);
}

VulkanRenderEncoder::VulkanRenderEncoder(VulkanCommandBuffer * pCmd)
    : Super(pCmd)
{
}

VulkanRenderEncoder::~VulkanRenderEncoder()
{
}

void VulkanRenderEncoder::SetScissorRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    VkRect2D Rect2D = { {x, y}, {w, h} };
    vkCmdSetScissor(OwningCommand->Handle, 0, 1, &Rect2D);
}

void VulkanRenderEncoder::SetViewport(const Viewport * pViewport)
{
    VkViewport viewPort = { pViewport->left, pViewport->top, pViewport->width, pViewport->height, pViewport->minDepth, pViewport->maxDepth };
    vkCmdSetViewport(OwningCommand->Handle, 0, 1, &viewPort);
}

void VulkanRenderEncoder::SetDepthBias(Float32 biasConst, Float32 biasClamp, Float32 biasSlope)
{
    vkCmdSetDepthBias(OwningCommand->Handle, biasConst, biasClamp, biasSlope);
}

void VulkanRenderEncoder::SetDepthBounds(Float32 minDepth, Float32 maxDepth)
{
    vkCmdSetDepthBounds(OwningCommand->Handle, minDepth, maxDepth);
}

void VulkanRenderEncoder::SetStencilReference(StencilFaceRef face, uint32_t value)
{
    vkCmdSetStencilReference(OwningCommand->Handle, (VkStencilFaceFlags)face, value);
}

void VulkanRenderEncoder::SetBlendConsts(Float32x4 constant)
{
    float constants[4] = { constant.x, constant.y, constant.z, constant.w };
    vkCmdSetBlendConstants(OwningCommand->Handle, constants);
}

void VulkanRenderEncoder::SetLineWidth(Float32 width)
{
    vkCmdSetLineWidth(OwningCommand->Handle, width);
}

void VulkanRenderEncoder::SetIndexBuffer(Buffer * pIndexBuffer)
{
    VulkanBuffer* pBuffer = static_cast<VulkanBuffer*>(pIndexBuffer);
    vkCmdBindIndexBuffer(OwningCommand->Handle, pBuffer->GetHandle(), 0, VkIndexType::VK_INDEX_TYPE_UINT32);
}

void VulkanRenderEncoder::SetVertexBuffer(uint32_t slot, uint64_t offset, Buffer * pVertexBuffer)
{
    VulkanBuffer* pBuffer = static_cast<VulkanBuffer*>(pVertexBuffer);
    VkBuffer buffHandle = pBuffer->GetHandle();
    VkDeviceSize OffSets[] = { offset };
    vkCmdBindVertexBuffers(OwningCommand->Handle, slot, 1, &buffHandle, OffSets);
}

void VulkanRenderEncoder::DrawInstanced(const DrawInstancedDesc * drawParam)
{
    vkCmdDraw(OwningCommand->Handle,
        drawParam->vertexCountPerInstance,
        drawParam->instanceCount,
        drawParam->startVertexId,
        drawParam->startInstanceId);
}

void VulkanRenderEncoder::DrawIndexedInstanced(const DrawIndexedInstancedDesc * drawParam)
{
    vkCmdDrawIndexed(OwningCommand->Handle,
        drawParam->indexCountPerInstance,
        drawParam->instanceCount,
        drawParam->startId,
        drawParam->baseVertexId,
        drawParam->startInstanceId);
}

void VulkanRenderEncoder::DrawIndirect(Buffer * pIndirectBuffer, uint32_t offset, uint32_t drawCount, uint32_t stride)
{
    VulkanBuffer* pBuffer = static_cast<VulkanBuffer*>(pIndirectBuffer);
    vkCmdDrawIndirect(OwningCommand->Handle, pBuffer->GetHandle(), offset, drawCount, stride);
}

void VulkanRenderEncoder::Present(Drawable * pDrawable)
{
}

VulkanCopyEncoder::VulkanCopyEncoder(VulkanCommandBuffer * pCmd)
    : Super(pCmd)
{
}

VulkanCopyEncoder::~VulkanCopyEncoder()
{
}

void VulkanCopyEncoder::CopyTexture()
{
}

void VulkanCopyEncoder::CopyBuffer(uint64_t srcOffset, uint64_t dstOffset, uint64_t size, Buffer * srcBuffer, Buffer * dstBuffer)
{
    VkBufferCopy copy = { srcOffset, dstOffset, size };
    vkCmdCopyBuffer(OwningCommand->Handle,
        static_cast<VulkanBuffer*>(srcBuffer)->GetHandle(),
        static_cast<VulkanBuffer*>(dstBuffer)->GetHandle(),
        1, &copy);
}

VulkanComputeEncoder::VulkanComputeEncoder(VulkanCommandBuffer * pCmd)
    : Super(pCmd)
{
}

VulkanComputeEncoder::~VulkanComputeEncoder()
{
}

void VulkanComputeEncoder::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
    vkCmdDispatch(OwningCommand->Handle, x, y, z);
}

VulkanLibrary1::VulkanLibrary1(VulkanDevice * pDevice, const void * pBlobData, k3d::U64 Size)
    : Device(pDevice)
{
    Device->AddInternalRef();
    Init(pBlobData, Size);
}

VulkanLibrary1::VulkanLibrary1(VulkanDevice * pDevice, const char * pFilePath)
    : Device(pDevice)
{
    Device->AddInternalRef();
    MemMapFile MemFile;
    if (MemFile.Open(pFilePath, k3d::IOFlag::Read))
    {
        Init(MemFile.FileData(), MemFile.GetSize());
        MemFile.Close();
    }
}

void VulkanLibrary1::Init(const void * pBlobData, k3d::U64 Size)
{
    const char* pHead = reinterpret_cast<const char*>(pBlobData);
    // parse binary
    if (pHead[0] == 'V' && pHead[1] == 'K' && pHead[2] == 'B' && pHead[3] == 'C') // Binary Version
    {
        const uint32_t* pVersion = reinterpret_cast<const uint32_t*>(pHead + 4);
        LogPrint(Log::Info, "VulkanLibrary1", "Library Version: %d.\n", *pVersion);
        const uint32_t* pEntryCount = reinterpret_cast<const uint32_t*>(pHead + 8);
        if (*pEntryCount > 0)
        {
            int curOffset = 12;
            int szEntryInfos = *pEntryCount * sizeof(EntryInfo);
            int spirvOffset = curOffset + szEntryInfos;
            for (uint32_t i = 0; i < *pEntryCount; i++)
            {
                const EntryInfo* pInfo = reinterpret_cast<const EntryInfo*>(pHead + curOffset);
                DataBlob[pInfo->Name].Stage = (ngfx::ShaderType)pInfo->ShaderType;
                curOffset += sizeof(EntryInfo);
                DataBlob[pInfo->Name].ByteCodes.resize(pInfo->Size / sizeof(uint32_t));
                memcpy(DataBlob[pInfo->Name].ByteCodes.data(), pHead + spirvOffset + pInfo->OffSet, pInfo->Size);
            }
        }
    }
    else  // Source Text Version
    {
        std::string ErrorInfo;
        CompileFromSource(CompileOption(), pHead, DataBlob, ErrorInfo);
    }
}

VulkanLibrary1::~VulkanLibrary1()
{
    Device->ReleaseInternal();
}

VulkanFunction1::VulkanFunction1(VulkanLibrary1 * pLibrary, const char * name)
    : Library(pLibrary)
    , StageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO }
    , ShaderModule(VK_NULL_HANDLE)
    , EntryName(name)
{
    Library->AddInternalRef();
    Library->DataBlob[name].ByteCodes;
    ShaderType = Library->DataBlob[name].Stage;
    CreateInfo.codeSize = sizeof(uint32_t) * Library->DataBlob[name].ByteCodes.size();
    CreateInfo.pCode = Library->DataBlob[name].ByteCodes.data();
    CHECK(vkCreateShaderModule(Library->Device->Handle, &CreateInfo, VULKAN_ALLOCATOR, &ShaderModule));
    StageInfo.module = ShaderModule;
    StageInfo.stage = ConvertShaderTypeToVulkanEnum(Library->DataBlob[name].Stage);
    StageInfo.pName = EntryName.c_str();
}

VulkanFunction1::~VulkanFunction1()
{
    if (ShaderModule)
    {
        vkDestroyShaderModule(Library->Device->Handle, ShaderModule, VULKAN_ALLOCATOR);
    }
    Library->ReleaseInternal();
}

ShaderType VulkanFunction1::Type() const
{
    return ShaderType;
}

const char * VulkanFunction1::Name() const
{
    return EntryName.c_str();
}

const ByteCode& VulkanFunction1::GetByteCode() const
{
    return Library->DataBlob[Name()].ByteCodes;
}

Result VulkanLibrary1::MakeFunction(const char * name, Function ** ppFunction)
{
    if (DataBlob.find(name) != DataBlob.end())
    {
        *ppFunction = new VulkanFunction1(this, name);
        return Result::Ok;
    }
    return Result::Failed;
}

BindTableAllocator::BindTableAllocator(VulkanDevice * pDevice)
    : Device(pDevice)
{
    uint32_t MaxDescriptorSets = 16384;
    const VkPhysicalDeviceLimits& Limits = Device->Prop.limits;
    uint32_t Counts[] = {
      Limits.maxDescriptorSetSamplers,
      4096,
      Limits.maxDescriptorSetSampledImages,
      Limits.maxDescriptorSetStorageImages,
      512,
      512,
      Limits.maxDescriptorSetUniformBuffers,
      Limits.maxDescriptorSetStorageBuffers,
      Limits.maxDescriptorSetUniformBuffersDynamic,
      Limits.maxDescriptorSetStorageBuffersDynamic,
    };
    for (auto type : {
      VK_DESCRIPTOR_TYPE_SAMPLER,
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
      VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
      VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC })
    {
        Sizes.push_back({ type, Counts[type] });
    }
    VkDescriptorPoolCreateInfo PoolInfo;
    PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    PoolInfo.poolSizeCount = Sizes.size();
    PoolInfo.pPoolSizes = Sizes.data();
    PoolInfo.maxSets = MaxDescriptorSets;
    CHECK(vkCreateDescriptorPool(Device->Handle, &PoolInfo, VULKAN_ALLOCATOR, &Handle));
}

BindTableAllocator::~BindTableAllocator()
{
    vkDestroyDescriptorPool(Device->Handle, Handle, VULKAN_ALLOCATOR);
}

Result BindTableAllocator::Allocate(const VulkanBindTableEncoder * pLayout, BindTable ** ppTable)
{
    VkDescriptorSetAllocateInfo AllocInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    };
    AllocInfo.descriptorPool = Handle;
    AllocInfo.descriptorSetCount = 1;
    AllocInfo.pSetLayouts = &pLayout->Handle;
    VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
    auto Ret = vkAllocateDescriptorSets(Device->Handle, &AllocInfo, &DescriptorSet);
    // Track Usage
    if (DescriptorSet)
    {
        *ppTable = new VulkanBindTable(Device, DescriptorSet);
    }
    return Ret == VK_SUCCESS ?
        Result::Ok : Result::Failed;
}

void BindTableAllocator::Free(BindTable * ppTable)
{
    // Remove Usage
    vkFreeDescriptorSets(Device->Handle, Handle, 1, &(static_cast<VulkanBindTable*>(ppTable)->Handle));
}

VulkanSwapChainTexture::VulkanSwapChainTexture(VulkanSwapChain * pSwapChain, VkImage _Image, const SwapChainDesc* pDesc)
    : Image(_Image)
    , SwapChain(pSwapChain)
{
    VkImageViewCreateInfo Info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    Info.image = Image;
    Info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    Info.format = ConvertPixelFormatToVulkanEnum(pDesc->pixelFormat);
    Info.components = { VK_COMPONENT_SWIZZLE_R,VK_COMPONENT_SWIZZLE_G,VK_COMPONENT_SWIZZLE_B,VK_COMPONENT_SWIZZLE_A };
    Info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1 };
    CHECK(vkCreateImageView(SwapChain->GetDevice()->Handle, &Info, VULKAN_ALLOCATOR, &ImageView));
}

VulkanSwapChainTexture::~VulkanSwapChainTexture()
{
    vkDestroyImageView(SwapChain->GetDevice()->Handle, ImageView, VULKAN_ALLOCATOR);
}
