#pragma once

#include "MeshMaterialShader.h"


class MESHGRASS_API FMGGrassScatterShaderVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FMGGrassScatterShaderVS, MeshMaterial);
	
public:
	FMGGrassScatterShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
	}

	FMGGrassScatterShaderVS() = default;

	
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters);
};


class MESHGRASS_API FMGGrassScatterShaderPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FMGGrassScatterShaderPS, MeshMaterial);
public:
	FMGGrassScatterShaderPS() = default;

	FMGGrassScatterShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
	}
	
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters);
};