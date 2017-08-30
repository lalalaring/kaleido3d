#include "ngfx_shader_compiler_private.h"
#if _WIN32
#include <wrl/client.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")
using Microsoft::WRL::ComPtr;
namespace ngfx {

    class DXIncluder : public ID3DInclude
    {
    public:
        DXIncluder(IIncluder * pInclude) : m_Includer(pInclude)
        {}

        ~DXIncluder() {}

        HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) override
        {
            return 1;
        }
        HRESULT Close(LPCVOID pData) override
        {
            return 1;
        }
    private:
        IIncluder* m_Includer;
    };

    DXBCCompiler::DXBCCompiler()
    {}

    DXBCCompiler::~DXBCCompiler()
    {}

    Result DXBCCompiler::Compile(const char * Source, const char * File, const char * EntryPoint, 
        ShaderProfile Profile, ShaderStageBit ShaderType, ShaderOptimizeLevel OptLevel, 
        IIncluder * pIncluder, const ShaderDefinition * Definitions, 
        IBlob ** ShaderOutput, IBlob ** Error)
    {
        DXIncluder includer(pIncluder);

        return Result::Failed;
    }

}
#endif
