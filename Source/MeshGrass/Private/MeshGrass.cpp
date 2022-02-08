#include "MeshGrass.h"

#include "EngineModule.h"
#include "Interfaces/IPluginManager.h"
#include "MeshMaterialShader.h"
#include "MeshPassProcessor.h"
#include "MeshPassProcessor.inl"

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

	void ClearViewMeshArrays() { FMeshElementCollector::ClearViewMeshArrays(); }

	void AddViewMeshArrays(
		FSceneView* InView, 
		TArray<FMeshBatchAndRelevance,SceneRenderingAllocator>* ViewMeshes,
		FSimpleElementCollector* ViewSimpleElementCollector, 
		FGPUScenePrimitiveCollector* InDynamicPrimitiveCollector,
		ERHIFeatureLevel::Type InFeatureLevel,
		FGlobalDynamicIndexBuffer* InDynamicIndexBuffer,
		FGlobalDynamicVertexBuffer* InDynamicVertexBuffer,
		FGlobalDynamicReadBuffer* InDynamicReadBuffer)
	{
		FMeshElementCollector::AddViewMeshArrays(InView, ViewMeshes, ViewSimpleElementCollector,
			InDynamicPrimitiveCollector, InFeatureLevel, InDynamicIndexBuffer, InDynamicVertexBuffer,
			InDynamicReadBuffer);
	}

	void SetPrimitive(const FPrimitiveSceneProxy* InPrimitiveSceneProxy, FHitProxyId DefaultHitProxyId)
	{
		FMeshElementCollector::SetPrimitive(InPrimitiveSceneProxy, DefaultHitProxyId);
	}
	
};

FGlobalDynamicIndexBuffer UMGMeshGrass::DynamicIndexBuffer;
FGlobalDynamicVertexBuffer UMGMeshGrass::DynamicVertexBuffer;
TGlobalResource<FGlobalDynamicReadBuffer> UMGMeshGrass::DynamicReadBuffer;

class FMGScatterGrassMeshPassProcessor : public FMeshPassProcessor
{
private:
	FMeshPassProcessorRenderState DrawRenderState;
	
public:
	FMGScatterGrassMeshPassProcessor(
		const FScene* InScene,
		const FSceneView* InView,
		FMeshPassDrawListContext* InDrawListContext
	)
		: FMeshPassProcessor(InScene, InView->GetFeatureLevel(), InView, InDrawListContext)
	{
		DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
		DrawRenderState.SetBlendState(TStaticBlendState<>::GetRHI());
	}

	virtual void AddMeshBatch(const FMeshBatch& RESTRICT MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId = -1) override final
	{
		const FMaterialRenderProxy* FallbackMaterialRenderProxyPtr = nullptr;
		const FMaterial& Material = MeshBatch.MaterialRenderProxy->GetMaterialWithFallback(FeatureLevel, FallbackMaterialRenderProxyPtr);
		const FMaterialRenderProxy& MaterialRenderProxy = FallbackMaterialRenderProxyPtr ? *FallbackMaterialRenderProxyPtr : *MeshBatch.MaterialRenderProxy;

		if (MeshBatch.bUseForMaterial
			&& (!PrimitiveSceneProxy || PrimitiveSceneProxy->ShouldRenderInMainPass()))
		{
			Process(MeshBatch, BatchElementMask, StaticMeshId, PrimitiveSceneProxy, MaterialRenderProxy, Material);
		}
	}

private:
	void Process(
		const FMeshBatch& MeshBatch,
		uint64 BatchElementMask,
		int32 StaticMeshId,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		const FMaterialRenderProxy& RESTRICT MaterialRenderProxy,
		const FMaterial& RESTRICT MaterialResource)
	{
		const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;

		TMeshProcessorShaders<
			FMGGrassScatterShaderVS,
			FMGGrassScatterShaderPS> Shaders;

		Shaders.VertexShader = MaterialResource.GetShader<FMGGrassScatterShaderVS>(VertexFactory->GetType());
		Shaders.PixelShader = MaterialResource.GetShader<FMGGrassScatterShaderPS>(VertexFactory->GetType());
		const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(MeshBatch);
		ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(MeshBatch, MaterialResource, OverrideSettings);
		ERasterizerCullMode MeshCullMode = CM_None;

		FMeshMaterialShaderElementData ShaderElementData;
		ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, false);
		
		FMeshDrawCommandSortKey SortKey {};

		BuildMeshDrawCommands(
			MeshBatch,
			BatchElementMask,
			PrimitiveSceneProxy,
			MaterialRenderProxy,
			MaterialResource,
			DrawRenderState,
			Shaders,
			MeshFillMode,
			MeshCullMode,
			SortKey,
			EMeshPassFeatures::Default,
			ShaderElementData);
	}
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
		// Init RT
		FTextureRenderTargetResource* RTRes = RT->GetRenderTargetResource();
		TRefCountPtr<IPooledRenderTarget> RTPooled = CreateRenderTarget(RTRes->GetTextureRenderTarget2DResource()->GetTextureRHI(), TEXT("GrassScatterRT"));
		
		// Create view
		FEngineShowFlags ViewShowFlags(ESFIM_Editor);
		// Force GetDynamicMeshElements to collect meshes in this view
		ViewShowFlags.SetLighting(false);
		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(RTRes, &SceneProxy->GetScene(),
				ViewShowFlags).SetWorldTimes(
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

		// Gather mesh batches		
		TArray<FMeshBatchAndRelevance,SceneRenderingAllocator> OutDynamicMeshElements;
		
		// Simple elements not supported in mesh grass passes
		FSimpleElementCollector DynamicSubjectSimpleElements;

		FMemMark Mark(FMemStack::Get());
		FMGMeshCollector Collector;
		Collector.ClearViewMeshArrays();

		// Not sure if const_cast is legal here...
		Collector.AddViewMeshArrays(const_cast<FSceneView*>(View), &OutDynamicMeshElements, &DynamicSubjectSimpleElements,
			nullptr, GMaxRHIFeatureLevel, &DynamicIndexBuffer, &DynamicVertexBuffer, &DynamicReadBuffer);

		// We dont need a hit proxy here
		Collector.SetPrimitive(SceneProxy, FHitProxyId());
		SceneProxy->GetDynamicMeshElements(ViewFamily.Views, ViewFamily, ~0x0u, Collector);
		
		FRDGBuilder GraphBuilder(RHICmdList);
		auto* PassParameters = GraphBuilder.AllocParameters<FMGGrassScatterPassParameters>();

		// Render targets
		FRDGTextureRef RTRef = GraphBuilder.RegisterExternalTexture(RTPooled, TEXT("GrassScatterRDG"));
		PassParameters->RenderTargets[0] = FRenderTargetBinding(RTRef, ERenderTargetLoadAction::EClear);

		// View
		PassParameters->View = View->ViewUniformBuffer;

		if (OutDynamicMeshElements.Num() > 0)
		{
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ScatterMeshGrass"),
				PassParameters,
				ERDGPassFlags::Raster, [SceneProxy, View, &OutDynamicMeshElements](FRHICommandListImmediate& RHIPassCmdList)
				{
					DrawDynamicMeshPass(*View, RHIPassCmdList, [&](FDynamicPassMeshDrawListContext* DynamicMeshPassContext)
					{
						FMGScatterGrassMeshPassProcessor MeshProcessor(
							View->Family->Scene->GetRenderScene(),
							View,
							DynamicMeshPassContext);
					
						MeshProcessor.AddMeshBatch(*OutDynamicMeshElements[0].Mesh, ~0ull, OutDynamicMeshElements[0].PrimitiveSceneProxy);
					});
				});
		}
		GraphBuilder.SetTextureAccessFinal(RTRef, ERHIAccess::SRVGraphics);
		// Global dynamic buffers need to be committed before rendering.
		DynamicIndexBuffer.Commit();
		DynamicVertexBuffer.Commit();
		DynamicReadBuffer.Commit();
		GraphBuilder.Execute();
	});	
	
	return true;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMeshGrassModule, MeshGrass)