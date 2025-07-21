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
	FIntVector WorldLocationToChunkID(const FVector& WorldLocation) const;
	
protected:
	UPROPERTY()
	TMap<FIntVector, UVoxelChunk*> Chunks;
};
