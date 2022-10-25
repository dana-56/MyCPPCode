#pragma once
#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "EditorModeManager.h"

class CopyFoliage :
    public TSharedFromThis<CopyFoliage>
{
public:
    //TSharedRef<class SDockTab> CreateLayout();
    FReply Copy();
    FReply Generate();
    FReply Snap();

private:
    UWorld* World;
    UClass* AddedLevelStreamingClass;
    AActor* FoliageRootActor;
    
    const FString CopyFoliageTo = "/Game/DanaTest/Foliage_Instance/Map_Copy_Foliage";
    const FString CopyFoliageIn = "/Game/DanaTest/Foliage_Instance/Map_Copy_Foliage_In";
    const FString GenerateFoliageMapPath = "/Game/DanaTest/Foliage_Instance/Map_Gegerate_Foliage";
    const FString MeshPath = "/Game/DanaTest/Foliage_Instance/Meshes/";
    const FString ShadowPath = "/Game/DanaTest/Foliage_Instance/Meshes/";

    void CopyForliage();
    void GenerateFoliage();
    AActor* GetFoliageRootActor(ULevel* NewFoliageLevel);
    UInstancedStaticMeshComponent* GetInstancedSMComponent(AActor* FoliageRoot, ULevel* NewFoliageLevel, UStaticMesh* Mesh,ECollisionEnabled::Type CollisionType);
    TMap<UStaticMesh*, TArray<FTransform>> GetFoliageData(AInstancedFoliageActor* IFA);
    TMap<FString, TArray<FTransform>> CheckFoliageData(TMap<FString, TArray<FTransform>> FoliageData);
    TMap<FString, TArray<FTransform>> SliceFoliageData(FString MeshName, TArray<FTransform> TransformList);
    ULevelStreaming* LoadMap(const FString MapPath);
    ULevelStreaming* GetMap(const FString MapPath);
    ULevelStreaming* GetGenerateFoliageMap(const FString MapPath);
    float GetLandHeight(FVector2D Point);
    void AddInstances(AActor* FoliageRoot,ULevel* NewFoliageLevel,UStaticMesh* Mesh, TArray<FTransform> Transforms,bool IsShadow = false);
    void SnapFoliageInstances();
};

