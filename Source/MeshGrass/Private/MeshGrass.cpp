#include "MeshGrass.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FMeshGrassModule"

void FMeshGrassModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("MeshGrass"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/MeshGrass"), PluginShaderDir);
}

void FMeshGrassModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMeshGrassModule, MeshGrass)