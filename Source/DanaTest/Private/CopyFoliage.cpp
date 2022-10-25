#include "CopyFoliage.h"
#include "EditorLevelUtils.h"
#include "Editor.h"
#include "LevelUtils.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Factories/WorldFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#include "Foliage/Public/InstancedFoliageActor.h"
#include "CoreUObject/Public/UObject/UObjectGlobals.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Templates/Casts.h"
#include "Engine/StaticMesh.h"
#include "RawMesh/Public/RawMesh.h"
#include "StaticMeshResources.h"
//#include "Landscape/Classes/LandscapeHeightfieldCollisionComponent.h"
#include "LandscapeHeightfieldCollisionComponent.h"



#define LOCTEXT_NAMESPACE "CopyFoliage"


FReply CopyFoliage::Copy()
{
	UE_LOG(LogTemp, Log, TEXT("Dana Copy click!!!"));
	World = GEditor->GetEditorWorldContext().World();
	AddedLevelStreamingClass = ULevelStreamingDynamic::StaticClass();
	if (World)
	{
		CopyForliage();
		//EditorLevelUtils::CreateNewStreamingLevelForWorld(*World, AddedLevelStreamingClass, CopyFoliageTo, false, nullptr);
	}
	return FReply::Handled();

}

FReply CopyFoliage::Generate()
{
	UE_LOG(LogTemp, Log, TEXT("Dana Generate Click!!!"));
	World = GEditor->GetEditorWorldContext().World();
	AddedLevelStreamingClass = ULevelStreamingDynamic::StaticClass();
	if (World)
	{
		GenerateFoliage();
	}
	return FReply::Handled();

}

FReply CopyFoliage::Snap()
{
	UE_LOG(LogTemp, Log, TEXT("Dana Snap Click!!!"));
	World = GEditor->GetEditorWorldContext().World();
	if (World)
	{
		SnapFoliageInstances();
	}
	return FReply::Handled();

}

ULevelStreaming* CopyFoliage::LoadMap(const FString MapPath)
{
	ULevelStreaming* NewStreamingLevel = FLevelUtils::FindStreamingLevel(World, *MapPath);
	bool bIsPersistentLevel = (World->PersistentLevel->GetOutermost()->GetName() == FString(MapPath));
	if (!bIsPersistentLevel && !NewStreamingLevel)
	{
		ULevel* NewLevel = nullptr;
		NewStreamingLevel = EditorLevelUtils::AddLevelToWorld(World, *MapPath, AddedLevelStreamingClass);
		if (NewStreamingLevel != nullptr)
		{
			NewLevel = NewStreamingLevel->GetLoadedLevel();
		}
		if (NewLevel != nullptr)
		{
			World->BroadcastLevelsChanged();
			FEditorDelegates::RefreshLevelBrowser.Broadcast();
		}
		else
		{
			NewStreamingLevel = nullptr;
		}
				
	}
	return NewStreamingLevel;
}

ULevelStreaming* CopyFoliage::GetMap(const FString MapPath)
{	
	ULevelStreaming* NewStreamingLevel = LoadMap(MapPath);
	if (NewStreamingLevel  == nullptr)
	{
		NewStreamingLevel = EditorLevelUtils::CreateNewStreamingLevelForWorld(*World, AddedLevelStreamingClass, MapPath, false, nullptr);
	}		
	return NewStreamingLevel;
}



void CopyFoliage::CopyForliage()
{	
	ULevelStreaming* NewStreamingLevel = LoadMap(CopyFoliageTo);
	if (NewStreamingLevel == nullptr)
	{
		return;
	}
	ULevel* NewLevel = NewStreamingLevel->GetLoadedLevel();
	if (NewLevel != nullptr)
	{
		const int32 NumLevels = World->GetNumLevels();
		for (int32 LevelIdx = 0; LevelIdx < NumLevels; ++LevelIdx)
		{
			ULevel* Level = World->GetLevel(LevelIdx);
			if (Level != nullptr)
			{
				AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(Level, /*bCreateIfNone*/ false);
				if (IFA != nullptr)
				{
					IFA->MoveAllInstancesToLevel(NewLevel);
				}
			}
		}
	}
}


void CopyFoliage::SnapFoliageInstances()
{
	ULevelStreaming* NewStreamingLevel = LoadMap(CopyFoliageTo);
	if (NewStreamingLevel == nullptr)
	{
		return;
	}
	ULevel* NewLevel = NewStreamingLevel->GetLoadedLevel();//World->GetCurrentLevel();
	if (NewLevel != nullptr)
	{
		AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(NewLevel, /*bCreateIfNone*/ false);
		FTransform Pos = FTransform(FVector(0, 0, 0));
		if (IFA != nullptr)
		{
			
			for (auto& Pair : IFA->FoliageInfos)
			{
				UFoliageType* Settings = Pair.Key;
				FFoliageInfo& MeshInfo = *Pair.Value;
				FVector TraceVector = FVector(0.f, 0.f, 1.f) * 51200.f + 10.f;

				TArray<int32> InstancesToRemove;
				TSet<UHierarchicalInstancedStaticMeshComponent*> AffectedFoliageComponents;

				bool bIsMeshInfoDirty = false;
				for (int InstanceIndex = 0; InstanceIndex < MeshInfo.Instances.Num(); ++InstanceIndex)
				{
					FFoliageInstance& Instance = MeshInfo.Instances[InstanceIndex];
					// Test location should remove any Z offset
					FVector TestLocation = FMath::Abs(Instance.ZOffset) > KINDA_SMALL_NUMBER
						? (FVector)Instance.GetInstanceWorldTransform().TransformPosition(FVector(0, 0, -Instance.ZOffset))
						: Instance.Location;
					FVector Start = TestLocation + TraceVector;
					FVector End = TestLocation - TraceVector;
					TArray<FHitResult> Results;
					check(World);
					// Editor specific landscape heightfield uses ECC_Visibility collision channel
					World->LineTraceMultiByObjectType(Results, Start, End, FCollisionObjectQueryParams(ECollisionChannel::ECC_Visibility), FCollisionQueryParams(SCENE_QUERY_STAT(FoliageSnapToLandscape), true));
					
					bool bFoundHit = false;
					for (const FHitResult& Hit : Results)
					{
						if (Hit.Component != nullptr && Hit.Component->GetClass() == ULandscapeHeightfieldCollisionComponent::StaticClass())
						{
							bFoundHit = true;
							if ((TestLocation - Hit.Location).SizeSquared() > KINDA_SMALL_NUMBER)
							{
								IFA->Modify();

								bIsMeshInfoDirty = true;

								// Remove instance location from the hash. Do not need to update ComponentHash as we re-add below.
								MeshInfo.InstanceHash->RemoveInstance(Instance.Location, InstanceIndex);

								// Update the instance editor data
								Instance.Location = Hit.Location;

								if (Instance.Flags & FOLIAGE_AlignToNormal)
								{
									// Remove previous alignment and align to new normal.
									Instance.Rotation = Instance.PreAlignRotation;
									Instance.AlignToNormal(Hit.Normal, Settings->AlignMaxAngle);
								}

								// Reapply the Z offset in local space
								if (FMath::Abs(Instance.ZOffset) > KINDA_SMALL_NUMBER)
								{
									Instance.Location = Instance.GetInstanceWorldTransform().TransformPosition(FVector(0, 0, Instance.ZOffset));
								}

								// Todo: add do validation with other parameters such as max/min height etc.

								MeshInfo.SetInstanceWorldTransform(InstanceIndex, Instance.GetInstanceWorldTransform(), false);
								// Re-add the new instance location to the hash
								MeshInfo.InstanceHash->InsertInstance(Instance.Location, InstanceIndex);
							}
							break;
						}
					}
					if (!bFoundHit)
					{
						// Couldn't find new spot - remove instance
						InstancesToRemove.Add(InstanceIndex);
						bIsMeshInfoDirty = true;
					}

					if (bIsMeshInfoDirty && (MeshInfo.GetComponent() != nullptr))
					{
						AffectedFoliageComponents.Add(MeshInfo.GetComponent());
					}
				}				
			
				// Remove any unused instances
				MeshInfo.RemoveInstances(IFA, InstancesToRemove, true);

				for (UHierarchicalInstancedStaticMeshComponent* FoliageComp : AffectedFoliageComponents)
				{
					FoliageComp->InvalidateLightingCache();
				}
			}
		}
	}
}

TMap<UStaticMesh*, TArray<FTransform>> CopyFoliage::GetFoliageData(AInstancedFoliageActor* IFA)
{
	TMap<UFoliageType*, FFoliageInfo*>InstanceFoliageTypes = IFA->GetAllInstancesFoliageType();
	TMap<UStaticMesh*, TArray<FTransform>> FoliageData;
	for (auto& Pair : InstanceFoliageTypes)
	{
		UStaticMesh* TargetMesh;
		TArray<FTransform> InstancesData;
		if (Pair.Key != nullptr && Pair.Key->GetSource() != nullptr)
		{
			FString MeshName = Pair.Key->GetSource()->GetName();
			UStaticMesh* Mesh = Cast<UStaticMesh>(Pair.Key->GetSource());
			if (Mesh)
			{
				int Num = Mesh->GetRenderData()->LODResources.Num();
				if (Num > 0)
				{			
					int index = Num - 1;
					int32 MaterialIndex = -1;
					const TArray<FStaticMeshSourceModel>& SourceModels = Mesh->GetSourceModels();	

					FRawMesh RawMesh;
					SourceModels[index].LoadRawMesh(RawMesh);
					
					FString PackageName = MeshPath + MeshName;
					UPackage* MeshPackage = CreatePackage(nullptr, *PackageName);
					TargetMesh = NewObject<UStaticMesh>(MeshPackage, FName(*MeshName), RF_Public | RF_Standalone);
					TargetMesh->PreEditChange(nullptr);
					FStaticMeshSourceModel& SrcModel = TargetMesh->AddSourceModel();
					SrcModel.SaveRawMesh(RawMesh);				

					
					auto Sections = Mesh->GetRenderData()->LODResources[index].Sections;					
					if (Sections.Num() > 0)
					{
						MaterialIndex = Sections[0].MaterialIndex;
					}
					if (MaterialIndex != -1)
					{
						UMaterialInterface* InstanceMaterial = Mesh->GetMaterial(MaterialIndex);
						if (InstanceMaterial)
						{
							TargetMesh->StaticMaterials.Add(InstanceMaterial);
						}						
					}
					TargetMesh->Build();
					FAssetRegistryModule::AssetCreated(TargetMesh);
				}				
			}
		}
		if (Pair.Value != nullptr && Pair.Value->Instances.Num() > 0)
		{
			for (auto& data : Pair.Value->Instances)
			{
				InstancesData.Add(data.GetInstanceWorldTransform());
			}
		}
		FoliageData.Add(TargetMesh, InstancesData);
	}
	return FoliageData;
}

AActor* CopyFoliage::GetFoliageRootActor(ULevel* NewFoliageLevel)
{	
	FTransform Pos = FTransform(FVector(0, 0, 0));
	FActorSpawnParameters RootSpawnInfo;
	RootSpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	RootSpawnInfo.OverrideLevel = NewFoliageLevel;
	AActor* FoliageRoot = World->SpawnActor(AInstancedFoliageActor::StaticClass(), &Pos, RootSpawnInfo);
	return FoliageRoot;
}

UInstancedStaticMeshComponent* CopyFoliage::GetInstancedSMComponent(AActor* FoliageRoot, ULevel* NewFoliageLevel, UStaticMesh* Mesh, ECollisionEnabled::Type CollisionType)
{	
	UInstancedStaticMeshComponent* InstanceMeshComponent = NewObject<UInstancedStaticMeshComponent>(FoliageRoot, UInstancedStaticMeshComponent::StaticClass(), NAME_None, RF_Transactional);
	if (InstanceMeshComponent != nullptr)
	{
		if (FoliageRoot->GetRootComponent())
		{
			InstanceMeshComponent->AttachToComponent(FoliageRoot->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		}
		else
		{
			FoliageRoot->SetRootComponent(InstanceMeshComponent);
		}
		InstanceMeshComponent->AttachToComponent(FoliageRoot->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		FoliageRoot->RemoveOwnedComponent(InstanceMeshComponent);
		InstanceMeshComponent->CreationMethod = EComponentCreationMethod::Instance;
		FoliageRoot->AddOwnedComponent(InstanceMeshComponent);
		
		InstanceMeshComponent->SetCollisionEnabled(CollisionType);
		InstanceMeshComponent->SetStaticMesh(Mesh);
		InstanceMeshComponent->SetMobility(EComponentMobility::Static);
	}
	return InstanceMeshComponent;
}

void CopyFoliage::AddInstances(AActor* FoliageRoot, ULevel* NewFoliageLevel, UStaticMesh* Mesh, TArray<FTransform> Transforms, bool IsShadow)
{
	if (Transforms.Num() > 0 && FoliageRoot)
	{
		UInstancedStaticMeshComponent* InstanceMeshComponent = GetInstancedSMComponent(FoliageRoot,NewFoliageLevel, Mesh, ECollisionEnabled::NoCollision);
		if (InstanceMeshComponent != nullptr)
		{
			if (IsShadow)
			{
				for (int i = 0; i < Transforms.Num(); i++)
				{
					auto TreePos = Transforms[i].GetTranslation();
					FVector ShadowPos = FVector(TreePos.X, TreePos.Y, TreePos.Z + 100.0);
					FVector ShadowScale = Transforms[i].GetScale3D() * FVector(2500, 2000, 0);
					FTransform ShadowTransform = FTransform(Transforms[i].GetRotation(), ShadowPos, ShadowScale);
					InstanceMeshComponent->AddInstanceWorldSpace(ShadowTransform);
				}
			}
			else
			{
				for (int i = 0; i < Transforms.Num(); i++)
				{
					InstanceMeshComponent->AddInstanceWorldSpace(Transforms[i]);
				}
			}
			InstanceMeshComponent->RegisterComponent();
		}
	}
}

float CopyFoliage::GetLandHeight(FVector2D Point)
{
	if (World)
	{
		FVector StartPos{ Point.X, Point.Y, 1000 };
		FVector EndPos{ Point.X, Point.Y, 0 };

		FHitResult HitResult;
		World->LineTraceSingleByObjectType(OUT HitResult,StartPos,EndPos,FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic),FCollisionQueryParams());
		if (HitResult.GetActor())
		{
			return HitResult.ImpactPoint.Z;
		}
	}
	return 0;
}

ULevelStreaming* CopyFoliage::GetGenerateFoliageMap(const FString MapPath) 
{
	/*ULevelStreaming* NewStreamingLevel = FLevelUtils::FindStreamingLevel(World, *MapPath);
	if (NewStreamingLevel)
	{
		World->RemoveStreamingLevel(NewStreamingLevel);
	}*/
	ULevelStreaming* NewStreamingLevel = EditorLevelUtils::CreateNewStreamingLevelForWorld(*World, AddedLevelStreamingClass, MapPath, false, nullptr);
	return NewStreamingLevel;
}

void CopyFoliage::GenerateFoliage()
{
	ULevelStreaming* FoliageStreaming = LoadMap(CopyFoliageTo);
	ULevelStreaming* NewFoliageStreaming = LoadMap(GenerateFoliageMapPath); 
	TMap<UStaticMesh*, TArray<FTransform>> FoliageData;
	if (FoliageStreaming != nullptr && NewFoliageStreaming != nullptr)
	{
		ULevel* FoliageLevel = FoliageStreaming->GetLoadedLevel();
		ULevel* NewFoliageLevel = NewFoliageStreaming->GetLoadedLevel();
		
		if (FoliageLevel && NewFoliageLevel )
		{
			AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(FoliageLevel, /*bCreateIfNone*/ false);
			AActor* FoliageRoot = GetFoliageRootActor(NewFoliageLevel);
			if (IFA && FoliageRoot)
			{
				FoliageData = GetFoliageData(IFA);
				for (auto& data : FoliageData)
				{
					//FString path = ShadowPath + "SM_Cube_Tree";
					//UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *path);
					if (data.Key)
					{
						AddInstances(FoliageRoot,NewFoliageLevel, data.Key, data.Value);
					}
				}
			}
			NewFoliageLevel->MarkPackageDirty();
		}

	}	
}


#undef LOCTEXT_NAMESPACE








//TSharedRef<SDockTab> CopyFoliage::CreateLayout()
//{
//	return SNew(SDockTab)
//		.TabRole(ETabRole::NomadTab)
//		.Label(LOCTEXT("CopyFoliageTab", "Copy Foliage Tab"))
//		.ShouldAutosize(true)
//		[
//			SNew(SVerticalBox)
//			+
//		SVerticalBox::Slot()
//		.Padding(0, 16, 0, 0)
//		.AutoHeight()
//		[
//			SNew(SButton)
//			.ForegroundColor(FLinearColor::Black)
//			.Text(LOCTEXT("Copy", "Copy"))
//			.OnClicked(this, &CopyFoliage::Copy)
//			.HAlign(HAlign_Center)
//		]			
//		];
//}




//ULevel* persistentLevel = World->PersistentLevel;
//ULevelStreaming* streaming =  FLevelUtils::FindStreamingLevel(persistentLevel);
//UClass* uclass = persistentLevel->GetClass();
//// Editor modes cannot be active when any level saving occurs.
//GLevelEditorModeTools().DeactivateAllModes();

//UWorld* WorldToAddLevelTo = World;
//UWorld* NewLevelWorld = nullptr;
//bool bNewWorldSaved = false;
//FString NewPackageName = "";

//// Create a new world
//UWorldFactory* Factory = NewObject<UWorldFactory>();
//Factory->WorldType = EWorldType::Inactive;
//UPackage* Pkg = CreatePackage(NULL);
//FName WorldName(TEXT("Untitled"));
//EObjectFlags Flags = RF_Public | RF_Standalone;
//NewLevelWorld = CastChecked<UWorld>(Factory->FactoryCreateNew(UWorld::StaticClass(), Pkg, WorldName, Flags, NULL, GWarn));
//if (NewLevelWorld)
//{
//	FAssetRegistryModule::AssetCreated(NewLevelWorld);
//}

//// Save the new world to disk.
//bNewWorldSaved = FEditorFileUtils::SaveLevel(NewLevelWorld->PersistentLevel, DefaultFilename);
//if (bNewWorldSaved)
//{
//	NewPackageName = NewLevelWorld->GetOutermost()->GetName();
//}

