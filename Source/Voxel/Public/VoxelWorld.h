#pragma once
#include "VoxelChunk.h"
#include "VoxelWorld.generated.h"

UCLASS()
class AVoxelWorld : public AActor
{
	GENERATED_BODY()
public:
	AVoxelWorld();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	TSubclassOf<AActor> VoxelChunkActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	float ChunkWorldSize = 6400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	float VoxelWorldSize = 100.0f;

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	UVoxelChunk* GetOrCreateChunkByID(const FIntVector& ChunkID);

	UFUNCTION(BlueprintPure, Category = "Voxel")
	FIntVector WorldLocationToChunkID(FVector WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Voxel", meta = (DisplayName = "Sculpt In World (Symmetrical)"))
	void SculptInWorld_Symmetrical(UVoxelChunk* TargetChunk, UVoxelBrush* WorldSpaceBrush);
	
protected:
	UPROPERTY()
	TMap<FIntVector, UVoxelChunk*> Chunks;

private:
	float GetBrushRadius(UVoxelBrush* Brush) const;
};
