#include "VoxelWorld.h"

AVoxelWorld::AVoxelWorld()
{
	PrimaryActorTick.bCanEverTick = false;
	
	USceneComponent* DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(DefaultSceneRoot);
}

UVoxelChunk* AVoxelWorld::GetOrCreateChunkByID(const FIntVector& ChunkID)
{
	if (UVoxelChunk** FoundChunk = Chunks.Find(ChunkID))
	{
		return *FoundChunk;
	}

	if (!VoxelChunkActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("VoxelWorld: VoxelChunkActorClass 未在蓝图中设置！"));
		return nullptr;
	}

	const FVector NewChunkLocation = FVector(ChunkID) * ChunkWorldSize;

	AActor* NewChunkActor = GetWorld()->SpawnActor<AActor>(VoxelChunkActorClass, NewChunkLocation, FRotator::ZeroRotator);
	if (!NewChunkActor)
	{
		UE_LOG(LogTemp, Error, TEXT("VoxelWorld: 生成 VoxelChunkActor 失败！"));
		return nullptr;
	}

	UVoxelChunk* NewChunk = NewChunkActor->FindComponentByClass<UVoxelChunk>();
	if (!NewChunk)
	{
		UE_LOG(LogTemp, Error, TEXT("VoxelWorld: 生成的Actor上没有找到 UVoxelChunk 组件！"));
		NewChunkActor->Destroy();
		return nullptr;
	}

	Chunks.Add(ChunkID, NewChunk);

	UE_LOG(LogTemp, Warning, TEXT("VoxelWorld: 创建新区块, ID: %s, 位置: %s"), *ChunkID.ToString(), *NewChunkLocation.ToString());

	return NewChunk;
}

FIntVector AVoxelWorld::WorldLocationToChunkID(const FVector& WorldLocation) const
{
	const int32 X = FMath::FloorToInt(WorldLocation.X / ChunkWorldSize);
	const int32 Y = FMath::FloorToInt(WorldLocation.Y / ChunkWorldSize);
	const int32 Z = FMath::FloorToInt(WorldLocation.Z / ChunkWorldSize);
	return FIntVector(X, Y, Z);
}
