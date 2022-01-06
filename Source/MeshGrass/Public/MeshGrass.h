#pragma once

#include "CoreMinimal.h"

class FMeshGrassModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
