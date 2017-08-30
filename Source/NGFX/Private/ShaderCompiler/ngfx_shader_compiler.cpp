#include "ngfx_shader_compiler_private.h"

#include <memory>

namespace ngfx {
    Result Compile(Backend Be,
        const char* Source, const char* File, const char* EntryPoint,
        ShaderProfile Profile, ShaderStageBit ShaderStage, ShaderOptimizeLevel OptLevel,
        IIncluder* pIncluder, const ShaderDefinition* pDefinition,
        IBlob** ShaderOutput, IBlob** Error)
    {
        std::unique_ptr<IShaderCompiler> pComp = nullptr;
        switch (Be) {
#if _WIN32
        case Backend::D3D11:
        case Backend::D3D12:
            pComp = std::make_unique< DXBCCompiler > ();
            break;
#endif
        case Backend::D3D12SM6:
            break;
        case Backend::Vulkan:
            pComp = std::make_unique< GlslangCompiler >(GlslangCompiler::CO_SPIRV);
            break;
        case Backend::OpenGLCore:
            pComp = std::make_unique< GlslangCompiler >(GlslangCompiler::CO_GLSL);
            break;
        case Backend::OpenGLES3:
            pComp = std::make_unique< GlslangCompiler >(GlslangCompiler::CO_ESSL);
            break;
        case Backend::Metal:
            pComp = std::make_unique< GlslangCompiler >(GlslangCompiler::CO_MSL);
            break;
        }
        if (pComp) {
            return pComp->Compile(Source, File, EntryPoint, Profile, ShaderStage, OptLevel, pIncluder, pDefinition, ShaderOutput, Error);
        }
        return Result::Failed;
    }
}
