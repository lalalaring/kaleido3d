#include "Public/ShaderCompiler.h"

#if K3DPLATFORM_OS_WIN && ENABLE_D3D12_BUILD
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <d3d12shader.h>
#include "DXCompiler.h"
#pragma comment(lib, "d3dcompiler.lib")

namespace k3d {

	using namespace k3d::shc;

	NGFXShaderDataType D3DSigTypeConvert(const D3D12_SIGNATURE_PARAMETER_DESC& desc)
	{
		NGFXShaderDataType result = NGFXShaderDataType::NGFX_SHADER_VAR_UNKNOWN;
		switch (desc.ComponentType)
		{
		case D3D_REGISTER_COMPONENT_SINT32:
		{
			if (desc.Mask == 1)
			{
				result = NGFXShaderDataType::NGFX_SHADER_VAR_INT; break;
			}
			else if (desc.Mask <= 3)
			{
				result = NGFXShaderDataType::NGFX_SHADER_VAR_INT2; break;
			}
			else if (desc.Mask <= 7)
			{
				result = NGFXShaderDataType::NGFX_SHADER_VAR_INT3; break;
			}
			else if (desc.Mask <= 15)
			{
				result = NGFXShaderDataType::NGFX_SHADER_VAR_INT4; break;
			}
		}
		break;

		case D3D_REGISTER_COMPONENT_UINT32:
		{
			if (desc.Mask == 1)
			{
				result = NGFXShaderDataType::NGFX_SHADER_VAR_UINT; break;
			}
			else if (desc.Mask <= 3)
			{
				result = NGFXShaderDataType::NGFX_SHADER_VAR_UINT2; break;
			}
			else if (desc.Mask <= 7)
			{
				result = NGFXShaderDataType::NGFX_SHADER_VAR_UINT3; break;
			}
			else if (desc.Mask <= 15)
			{
				result = NGFXShaderDataType::NGFX_SHADER_VAR_UINT4; break;
			}
		}
		break;

		case D3D_REGISTER_COMPONENT_FLOAT32:
		{
			if (desc.Mask == 1)
			{
				result = NGFXShaderDataType::NGFX_SHADER_VAR_FLOAT; break;
			}
			else if (desc.Mask <= 3)
			{
				result = NGFXShaderDataType::NGFX_SHADER_VAR_FLOAT2; break;
			}
			else if (desc.Mask <= 7)
			{
				result = NGFXShaderDataType::NGFX_SHADER_VAR_FLOAT3; break;
			}
			else if (desc.Mask <= 15)
			{
				result = NGFXShaderDataType::NGFX_SHADER_VAR_FLOAT4; break;
			}
		}
		break;

		}
		return result;
	}

	const char * GetSMModel(NGFXShaderType const& type)
	{
		switch (type)
		{
		case NGFX_SHADER_TYPE_VERTEX:
			return "vs_5_1";
		case NGFX_SHADER_TYPE_FRAGMENT:
			return "ps_5_1";
		case NGFX_SHADER_TYPE_COMPUTE:
			return "cs_5_1";
		case NGFX_SHADER_TYPE_HULL:
			return "hs_5_1";
		case NGFX_SHADER_TYPE_GEOMETRY:
			return "gs_5_1";
		default:
			return "";
		}
	}

	NGFXShaderCompileResult DXCompiler::Compile(String const & src, NGFXShaderDesc const & inOp, k3d::ShaderBundle & bundle)
	{
		if (inOp.Format == NGFX_SHADER_FORMAT_TEXT && inOp.Lang == NGFX_SHADER_LANG_HLSL && inOp.Profile == NGFX_SHADER_PROFILE_MODERN)
		{

#if defined(_DEBUG)
			// Enable better shader debugging with the graphics debugging tools.
			UINT dwShaderFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			UINT dwShaderFlags = 0;
#endif
			ComPtr<ID3DBlob> ShaderBlob, ErrorBlob;
			HRESULT Hr = ::D3DCompile(
				src.CStr(), src.Length(), /**SrcData, SrcDataLen***/
				nullptr, nullptr, nullptr, /**SrcName, Defines, Includes**/
				inOp.EntryFunction.CStr(), GetSMModel(inOp.Stage),
				dwShaderFlags, 0,
				ShaderBlob.GetAddressOf(), ErrorBlob.GetAddressOf());
			bundle.Desc = inOp;
			bundle.Desc.Format = NGFX_SHADER_FORMAT_BYTE_CODE;
			bundle.RawData = { ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize() };

			ComPtr<ID3D12ShaderReflection> reflection;
			HRESULT refHr = D3DReflect(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(), IID_PPV_ARGS(reflection.GetAddressOf()));
			D3D12_SHADER_DESC desc;
			if (SUCCEEDED(refHr))
			{
				refHr = reflection->GetDesc(&desc);
				printf("Shader Comipler : (%s)\n", desc.Creator);
				uint32 constBufferCount = desc.ConstantBuffers;
				for (uint32 i = 0; i < constBufferCount; i++)
				{
					auto cb = reflection->GetConstantBufferByIndex(i);
					D3D12_SHADER_BUFFER_DESC sbd = {};
					cb->GetDesc(&sbd);
					printf("ConstBuffer -- VarName:(%s) VarSize:(%d) VarNum:(%d)\n", sbd.Name, sbd.Size, sbd.Variables);
					for (uint32 vid = 0; vid < sbd.Variables; vid++)
					{
						//					auto var = reflection->Get
					}
				}

				uint32 bindResCount = desc.BoundResources;
				for (uint32 i = 0; i < bindResCount; i++)
				{
					D3D12_SHADER_INPUT_BIND_DESC sibd = {};
					reflection->GetResourceBindingDesc(i, &sibd);
					printf("BindResource -- VarName:(%s) VarBindPoint:(%d) VarBindCount:(%d) VarReg:(%d)\n", sibd.Name, sibd.BindPoint, sibd.BindCount, sibd.Type);
				}

				uint32 inputParamCount = desc.InputParameters;
				for (uint32 i = 0; i < inputParamCount; i++)
				{
					D3D12_SIGNATURE_PARAMETER_DESC spd = {};
					reflection->GetInputParameterDesc(i, &spd);
					//auto dt = D3DSigTypeConvert(spd);
					printf("Input Param -- SemName:(%s) SemId:(%d) Type:(%d)\n", spd.SemanticName, spd.SemanticIndex, spd.ComponentType);
				}
			}
			else {
				return NGFX_SHADER_COMPILE_FAILED;
			}
		}
		return NGFX_SHADER_COMPILE_OK;
	}

	const char * DXCompiler::GetVersion()
	{
		static char cCompilerVer[64] = { 0 };
		snprintf(cCompilerVer, 64, "Microsoft D3DCompiler %d", D3D_COMPILER_VERSION);
		return cCompilerVer;
	}
	
}
#endif
