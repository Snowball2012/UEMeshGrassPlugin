#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"

#include "MeshGrass.generated.h"

class FMeshGrassModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

UCLASS()
class UMGMeshGrass : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(Category="MeshGrass")
	static bool RenderComponentToGrassMap(UPrimitiveComponent* Comp, UTextureRenderTarget2D* RT);
};