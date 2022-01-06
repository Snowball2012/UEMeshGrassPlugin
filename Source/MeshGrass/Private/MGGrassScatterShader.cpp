#include "MGGrassScatterShader.h"

IMPLEMENT_SHADER_TYPE(,FMGGrassScatterShaderVS,TEXT("/Plugin/MeshGrass/MeshGrass.usf"),TEXT("GrassScatterVS"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(,FMGGrassScatterShaderPS,TEXT("/Plugin/MeshGrass/MeshGrass.usf"),TEXT("GrassScatterPS"),SF_Pixel);

bool FMGGrassScatterShaderVS::ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
{
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
		!IsConsolePlatform(Parameters.Platform);
}

bool FMGGrassScatterShaderPS::ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
{
	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
		!IsConsolePlatform(Parameters.Platform);
}
