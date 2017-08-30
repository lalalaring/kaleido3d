#ifndef __ngfx_shader_compiler_20180511__
#define __ngfx_shader_compiler_20180511__
#pragma once

#if _WIN32
#if defined(NGFX_SHADER_COMPILER_BUILD)
#if defined(NGFX_SHADER_COMPILER_BUILD_SHARED_LIB)
#define NGFX_SHADER_COMPILER_API __declspec(dllexport)
#else
#define NGFX_SHADER_COMPILER_API __declspec(dllimport)
#endif
#else
#define NGFX_SHADER_COMPILER_API     
#endif
#else
#define NGFX_SHADER_COMPILER_API __attribute__((visibility("default"))) 
#endif

#include "ngfx_shader.h"

#if __cplusplus
extern "C" {
#endif



#if __cplusplus
}

namespace ngfx {

    // Includer
    // DeviceLimit
    // DeviceAvailability
    // Defines
    // Optimize

    enum class IncludeType : uint8_t
    {
        Local,
        System,
    };

    enum class ShaderOptimizeLevel : uint8_t
    {

    };

    struct IIncluder 
    {
        virtual Result Open(IncludeType Type,
            const char* SrcFile, const char* RequestFile, 
            void** ppData, uint64_t* pBytes, 
            void* pUserData) = 0;
        virtual void Close(void *pData) = 0;
    };

    struct IBlob : public RefCounted<true>
    {
        virtual ~IBlob() {}

        virtual const void* GetData() = 0;
        virtual uint64_t    GetSize() = 0;
    };

    struct ShaderDefinition
    {
        const char* Name;
        Nullable const char* Definition;
    };

    NGFX_SHADER_COMPILER_API Result Compile(Backend Be,
        const char* source, const char* file, const char* EntryPoint,
        ShaderProfile Profile, ShaderStageBit ShaderStage, ShaderOptimizeLevel OptLevel,
        IIncluder* pIncluder, const ShaderDefinition* pDefinition,
        IBlob** ShaderOutput, IBlob** Error);

}
#endif

#endif