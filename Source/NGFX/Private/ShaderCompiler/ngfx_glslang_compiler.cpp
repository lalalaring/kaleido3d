#include "ngfx_shader_compiler_private.h"
#include <SPIRV/GlslangToSpv.h>
#include <spirv_msl.hpp>
#include <spirv_glsl.hpp>
namespace ngfx {
    using namespace glslang;

    class GLSlangIncluder : public TShader::Includer
    {
    public:
        GLSlangIncluder(IIncluder * pIncluder) : m_Includer(pIncluder) {}

        virtual IncludeResult* includeSystem(const char* headerName,
            const char* includerName, size_t /*inclusionDepth*/) override 
        {
            if (!m_Includer) {
                return nullptr;
            }
            char* pContent = nullptr;
            uint64_t length = 0;
            Result ret = m_Includer->Open(IncludeType::System, headerName, includerName, (void**)&pContent, &length, nullptr);        
            return ret == Result::Ok ? 
                new IncludeResult(headerName, pContent, length, pContent) 
                : nullptr;
        }

        // For the "local"-only aspect of a "" include. Should not search in the
        // "system" paths, because on returning a failure, the parser will
        // call includeSystem() to look in the "system" locations.
        IncludeResult* includeLocal(const char* headerName, const char* includerName,
            size_t /*inclusionDepth*/) override {
            if (!m_Includer) {
                return nullptr;
            }
            char* pContent = nullptr;
            uint64_t length = 0;
            Result ret = m_Includer->Open(IncludeType::Local, headerName, includerName, (void**)&pContent, &length, nullptr);
            return ret == Result::Ok ?
                new IncludeResult(headerName, pContent, length, pContent)
                : nullptr;
        }

        void releaseInclude(IncludeResult* result) override
        {
            if (!m_Includer)
                return;
            m_Includer->Close(result->userData);
            delete result;
        }
        
        ~GLSlangIncluder() override {}

    private:
        IIncluder* m_Includer;
    };

    class SpirvBlob : public IBlob
    {
    public:
        SpirvBlob(std::vector<uint32_t> && InSpirv);
        ~SpirvBlob() override {}
        const void* GetData() override;
        uint64_t    GetSize() override;
    private:
        std::vector<uint32_t> m_Spirv;
    };

    class StringBlob : public IBlob
    {
    public:
        StringBlob(std::string && InStr) : m_Str(std::forward<std::string>(InStr)) {}
        ~StringBlob() override {}
        const void* GetData() override { return m_Str.data(); }
        uint64_t    GetSize() override { return m_Str.length(); }
    private:
        std::string m_Str;
    };

    EShLanguage Convert(ShaderStageBit Shader)
    {
        switch (Shader)
        {
        case ShaderStageBit::Vertex:
            return EShLangVertex;
        case ShaderStageBit::Fragment:
            return EShLangFragment;
        case ShaderStageBit::Compute:
            return EShLangCompute;
        case ShaderStageBit::Geometry:
            return EShLangGeometry;
        case ShaderStageBit::TessailationEval:
            return EShLangTessEvaluation;
        case ShaderStageBit::TessailationControl:
            return EShLangTessControl;
        }
        return EShLangCount;
    }

    GlslangCompiler::GlslangCompiler(GlslangCompiler::Output O)
        : m_OutputType(O)
    {
    }

    GlslangCompiler::~GlslangCompiler()
    {
    }

    class GlslangScope
    {
    public:
        GlslangScope()
        {
            ShInitialize();
        }
        ~GlslangScope()
        {
            ShFinalize();
        }
    };


    const TBuiltInResource DefaultTBuiltInResource = {
        /* .MaxLights = */ 32,
        /* .MaxClipPlanes = */ 6,
        /* .MaxTextureUnits = */ 32,
        /* .MaxTextureCoords = */ 32,
        /* .MaxVertexAttribs = */ 64,
        /* .MaxVertexUniformComponents = */ 4096,
        /* .MaxVaryingFloats = */ 64,
        /* .MaxVertexTextureImageUnits = */ 32,
        /* .MaxCombinedTextureImageUnits = */ 80,
        /* .MaxTextureImageUnits = */ 32,
        /* .MaxFragmentUniformComponents = */ 4096,
        /* .MaxDrawBuffers = */ 32,
        /* .MaxVertexUniformVectors = */ 128,
        /* .MaxVaryingVectors = */ 8,
        /* .MaxFragmentUniformVectors = */ 16,
        /* .MaxVertexOutputVectors = */ 16,
        /* .MaxFragmentInputVectors = */ 15,
        /* .MinProgramTexelOffset = */ -8,
        /* .MaxProgramTexelOffset = */ 7,
        /* .MaxClipDistances = */ 8,
        /* .MaxComputeWorkGroupCountX = */ 65535,
        /* .MaxComputeWorkGroupCountY = */ 65535,
        /* .MaxComputeWorkGroupCountZ = */ 65535,
        /* .MaxComputeWorkGroupSizeX = */ 1024,
        /* .MaxComputeWorkGroupSizeY = */ 1024,
        /* .MaxComputeWorkGroupSizeZ = */ 64,
        /* .MaxComputeUniformComponents = */ 1024,
        /* .MaxComputeTextureImageUnits = */ 16,
        /* .MaxComputeImageUniforms = */ 8,
        /* .MaxComputeAtomicCounters = */ 8,
        /* .MaxComputeAtomicCounterBuffers = */ 1,
        /* .MaxVaryingComponents = */ 60,
        /* .MaxVertexOutputComponents = */ 64,
        /* .MaxGeometryInputComponents = */ 64,
        /* .MaxGeometryOutputComponents = */ 128,
        /* .MaxFragmentInputComponents = */ 128,
        /* .MaxImageUnits = */ 8,
        /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
        /* .MaxCombinedShaderOutputResources = */ 8,
        /* .MaxImageSamples = */ 0,
        /* .MaxVertexImageUniforms = */ 0,
        /* .MaxTessControlImageUniforms = */ 0,
        /* .MaxTessEvaluationImageUniforms = */ 0,
        /* .MaxGeometryImageUniforms = */ 0,
        /* .MaxFragmentImageUniforms = */ 8,
        /* .MaxCombinedImageUniforms = */ 8,
        /* .MaxGeometryTextureImageUnits = */ 16,
        /* .MaxGeometryOutputVertices = */ 256,
        /* .MaxGeometryTotalOutputComponents = */ 1024,
        /* .MaxGeometryUniformComponents = */ 1024,
        /* .MaxGeometryVaryingComponents = */ 64,
        /* .MaxTessControlInputComponents = */ 128,
        /* .MaxTessControlOutputComponents = */ 128,
        /* .MaxTessControlTextureImageUnits = */ 16,
        /* .MaxTessControlUniformComponents = */ 1024,
        /* .MaxTessControlTotalOutputComponents = */ 4096,
        /* .MaxTessEvaluationInputComponents = */ 128,
        /* .MaxTessEvaluationOutputComponents = */ 128,
        /* .MaxTessEvaluationTextureImageUnits = */ 16,
        /* .MaxTessEvaluationUniformComponents = */ 1024,
        /* .MaxTessPatchComponents = */ 120,
        /* .MaxPatchVertices = */ 32,
        /* .MaxTessGenLevel = */ 64,
        /* .MaxViewports = */ 16,
        /* .MaxVertexAtomicCounters = */ 0,
        /* .MaxTessControlAtomicCounters = */ 0,
        /* .MaxTessEvaluationAtomicCounters = */ 0,
        /* .MaxGeometryAtomicCounters = */ 0,
        /* .MaxFragmentAtomicCounters = */ 8,
        /* .MaxCombinedAtomicCounters = */ 8,
        /* .MaxAtomicCounterBindings = */ 1,
        /* .MaxVertexAtomicCounterBuffers = */ 0,
        /* .MaxTessControlAtomicCounterBuffers = */ 0,
        /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
        /* .MaxGeometryAtomicCounterBuffers = */ 0,
        /* .MaxFragmentAtomicCounterBuffers = */ 1,
        /* .MaxCombinedAtomicCounterBuffers = */ 1,
        /* .MaxAtomicCounterBufferSize = */ 16384,
        /* .MaxTransformFeedbackBuffers = */ 4,
        /* .MaxTransformFeedbackInterleavedComponents = */ 64,
        /* .MaxCullDistances = */ 8,
        /* .MaxCombinedClipAndCullDistances = */ 8,
        /* .MaxSamples = */ 4,
        /* .limits = */{
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    } };

    Result GlslangCompiler::Compile(const char* Source, const char* File, const char* EntryPoint,
        ShaderProfile Profile, ShaderStageBit ShaderType, ShaderOptimizeLevel OptLevel,
        IIncluder* pIncluder, const ShaderDefinition* Definitions,
        IBlob** ShaderOutput, IBlob** Error)
    {
        GlslangScope scope;
        GLSlangIncluder includer(pIncluder);
        EShMessages message = EShMessages(EShMsgVulkanRules | EShMsgSpvRules | EShMsgReadHlsl);
        EShLanguage language = Convert(ShaderType);
        auto shader = std::make_unique< TShader >(language);
        int len = (int)strlen(Source);
        shader->setStringsWithLengthsAndNames(&Source, &len, &File, 1);
        shader->setEntryPoint(EntryPoint); 
        //shader->setEnvInput(glslang::EShSourceHlsl);
        std::string defines;
        if (Definitions) {
            const ShaderDefinition* defs = Definitions;
            while (defs) {
                if (defs->Name) {
                    defines += std::string("#define ") + defs->Name;
                } else {
                    break;
                }
                if (defs->Definition) {
                    defines += std::string("=") + defs->Name;
                }
                defines += "\n";
                defs++;
            }
        }
        shader->setPreamble(defines.c_str());
        if (!shader->parse(&DefaultTBuiltInResource, 100, false,
            message, includer)) {
            if (Error) {
                std::string info = shader->getInfoLog();
                info += "\n";
                info += shader->getInfoDebugLog();
                *Error = new StringBlob(std::move(info));
            }
            return Result::Failed;
        } else {
            auto program = std::make_unique< TProgram >();
            program->addShader(shader.get());
            if (!program->link(message)) {
                std::string info = program->getInfoLog();
                info += "\n";
                info += program->getInfoDebugLog();
                *Error = new StringBlob(std::move(info));
                return Result::Failed;
            } else {
                if(program->buildReflection());
                std::vector<unsigned int> spirv;
                GlslangToSpv(*program->getIntermediate(language), spirv);
                if (ShaderOutput) {
                    switch (m_OutputType) {
                    case CO_SPIRV: {
                        *ShaderOutput = new SpirvBlob(std::move(spirv));
                        break;
                    }
                    case CO_GLSL: // SPIRV->GLSL
                    {
                        auto glsl_compiler = std::make_unique<spirv_cross::CompilerGLSL>(spirv.data(), spirv.size());
                        spirv_cross::CompilerGLSL::Options options;
                        options.version = 450;
                        options.es = false;
                        options.vertex.fixup_clipspace = false;
                        glsl_compiler->set_options(options);
                        *ShaderOutput = new StringBlob(std::move(glsl_compiler->compile()));
                        break;
                    }
                    case CO_ESSL: // SPIRV->ESSL
                    {
                        auto glsl_compiler = std::make_unique<spirv_cross::CompilerGLSL>(spirv.data(), spirv.size());
                        spirv_cross::CompilerGLSL::Options options;
                        options.version = 310;
                        options.es = true;
                        options.vertex.fixup_clipspace = false;
                        glsl_compiler->set_options(options);
                        *ShaderOutput = new StringBlob(std::move(glsl_compiler->compile()));
                        break;
                    }
                    case CO_MSL: // SPIRV->MSL
                    {
                        auto msl_compiler = std::make_unique<spirv_cross::CompilerMSL>(spirv.data(), spirv.size());
                        spirv_cross::CompilerMSL::Options options;
                        msl_compiler->set_options(options);
                        *ShaderOutput = new StringBlob(std::move(msl_compiler->compile()));
                        break;
                    }
                    }
                }
                return Result::Ok;
            }
        }
        return Result::Failed;
    }

    SpirvBlob::SpirvBlob(std::vector<uint32_t> && InSpirv)
        : m_Spirv(std::forward<std::vector<uint32_t>>(InSpirv))
    {
    }
    const void* SpirvBlob::GetData()
    {
        return m_Spirv.data();
    }
    uint64_t SpirvBlob::GetSize()
    {
        return m_Spirv.size() * sizeof(uint32_t);
    }
}