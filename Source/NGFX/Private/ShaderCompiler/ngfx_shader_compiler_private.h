#ifndef __ngfx_shader_compiler_private__
#define __ngfx_shader_compiler_private__

#include "ngfx_shader_compiler.h"

namespace ngfx
{
    struct IShaderCompiler
    {
        virtual ~IShaderCompiler() {}

        virtual Result Compile(const char* Source, const char* File, const char* EntryPoint, 
            ShaderProfile Profile, ShaderStageBit ShaderType, ShaderOptimizeLevel OptLevel,
            IIncluder* pIncluder, const ShaderDefinition* Definitions,
            IBlob** ShaderOutput, IBlob** Error) = 0;
    };

    class GlslangCompiler : public IShaderCompiler
    {
    public:
        enum Output
        {
            CO_SPIRV,
            CO_GLSL,
            CO_ESSL,
            CO_MSL, // Metal Source
        };

        GlslangCompiler(Output O);
        virtual ~GlslangCompiler() override;

        virtual Result Compile(const char* Source, const char* File, const char* EntryPoint,
            ShaderProfile Profile, ShaderStageBit ShaderType, ShaderOptimizeLevel OptLevel,
            IIncluder* pIncluder, const ShaderDefinition* Definitions,
            IBlob** ShaderOutput, IBlob** Error) override;

    private:
        Output m_OutputType;
    };

    class DXBCCompiler : public IShaderCompiler
    {
    public:
        DXBCCompiler();
        virtual ~DXBCCompiler() override;

        virtual Result Compile(const char* Source, const char* File, const char* EntryPoint,
            ShaderProfile Profile, ShaderStageBit ShaderType, ShaderOptimizeLevel OptLevel,
            IIncluder* pIncluder, const ShaderDefinition* Definitions,
            IBlob** ShaderOutput, IBlob** Error) override;

    private:
    };
}

#endif