#include "MeshGrass.h"

#include "EngineModule.h"
#include "Interfaces/IPluginManager.h"
#include "MeshMaterialShader.h"
#include "MeshPassProcessor.h"

#define LOCTEXT_NAMESPACE "FMeshGrass"

BEGIN_SHADER_PARAMETER_STRUCT(FMGGrassScatterPassParameters, )
SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

class FMGGrassScatterShaderVS : public FMeshMaterialShader
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

class FMGGrassScatterShaderPS : public FMeshMaterialShader
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

class FMGMeshCollector : public FMeshElementCollector
{
public:
	FMGMeshCollector() : FMeshElementCollector(GMaxRHIFeatureLevel) {}
};

bool UMGMeshGrass::RenderComponentToGrassMap(UPrimitiveComponent* Comp, UTextureRenderTarget2D* RT)
{
	if (!ensure(Comp && RT))
		return false;

	FPrimitiveSceneProxy* SceneProxy = Comp->SceneProxy;
	if (!ensure(SceneProxy))
		return false;

	ENQUEUE_RENDER_COMMAND(RenderMeshGrassToRT)([SceneProxy, RT](auto& RHICmdList)
	{
		FRDGBuilder GraphBuilder(RHICmdList);
		auto* PassParameters = GraphBuilder.AllocParameters<FMGGrassScatterPassParameters>();

		// Render targets
		FTextureRenderTargetResource* RTRes = RT->GetRenderTargetResource();
		TRefCountPtr<IPooledRenderTarget> RTPooled = CreateRenderTarget(RTRes->GetTextureRenderTarget2DResource()->GetTextureRHI(), TEXT("GrassScatterRT"));
		FRDGTextureRef RTRef = GraphBuilder.RegisterExternalTexture(RTPooled, TEXT("GrassScatterRDG"));
		PassParameters->RenderTargets[0] = FRenderTargetBinding(RTRef, ERenderTargetLoadAction::EClear);

		// View
		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(RTRes, &SceneProxy->GetScene(),
				FEngineShowFlags(ESFIM_Editor)).SetWorldTimes(
					FApp::GetCurrentTime() - GStartTime,
					FApp::GetDeltaTime(),
					FApp::GetCurrentTime() - GStartTime));

		const auto& Bounds = SceneProxy->GetBounds();
		FSceneViewInitOptions ViewInitOptions;
		ViewInitOptions.SetViewRectangle(FIntRect(0,0, RTRes->GetSizeX(), RTRes->GetSizeY()));
		ViewInitOptions.ViewOrigin = FVector(Bounds.Origin);
		ViewInitOptions.ViewRotationMatrix = FLookAtMatrix(Bounds.Origin + Bounds.SphereRadius*FVector::UpVector, Bounds.Origin, FVector::YAxisVector);
		ViewInitOptions.ProjectionMatrix = FReversedZOrthoMatrix(Bounds.BoxExtent.X * 2, Bounds.BoxExtent.Y * 2, Bounds.BoxExtent.Z * 2, 0);
		ViewInitOptions.ViewFamily = &ViewFamily;
		
		GetRendererModule().CreateAndInitSingleView(RHICmdList, &ViewFamily, &ViewInitOptions);
		const FSceneView* View = ViewFamily.Views[0];
		PassParameters->View = View->ViewUniformBuffer;

		FMGMeshCollector Collector;
		SceneProxy->GetDynamicMeshElements(ViewFamily.Views, ViewFamily, ~0x0u, Collector);

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("ScatterMeshGrass"),
			PassParameters,
			ERDGPassFlags::Raster, [SceneProxy](FRHICommandListImmediate& RHIPassCmdList)
			{
				FMemMark Mark(FMemStack::Get());

				FDynamicMeshDrawCommandStorage DynamicMeshDrawCommandStorage;
				FMeshCommandOneFrameArray VisibleMeshDrawCommands;
				FGraphicsMinimalPipelineStateSet GraphicsMinimalPipelineStateSet;
				bool NeedsShaderInitialization = false;

			});
		
		GraphBuilder.SetTextureAccessFinal(RTRef, ERHIAccess::SRVGraphics);
		GraphBuilder.Execute();
	});	
	
	return true;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMeshGrassModule, MeshGrass)