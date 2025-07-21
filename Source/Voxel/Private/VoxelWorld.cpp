#include "VoxelWorld.h"

#include "VoxelBrush/SphereShape.h"

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

FIntVector AVoxelWorld::WorldLocationToChunkID(FVector WorldLocation) const
{
	if (WorldLocation.Z == -0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("修正前位置: %s"), *WorldLocation.ToString());
		WorldLocation.Z = 0.0f;
		UE_LOG(LogTemp, Warning, TEXT("修正后位置： %s"), *WorldLocation.ToString());
	}
	const int32 X = FMath::FloorToInt(WorldLocation.X / ChunkWorldSize);
	const int32 Y = FMath::FloorToInt(WorldLocation.Y / ChunkWorldSize);
	const int32 Z = FMath::FloorToInt(WorldLocation.Z / ChunkWorldSize);
	return FIntVector(X, Y, Z);
}

void AVoxelWorld::SculptInWorld_Symmetrical(UVoxelChunk* TargetChunk, UVoxelBrush* WorldSpaceBrush)
{
	if (!TargetChunk || !WorldSpaceBrush)
	{
		return;
	}

	const float BrushRadius = GetBrushRadius(WorldSpaceBrush);
	if (BrushRadius <= 0.0f) return;

	FIntVector TargetChunkID = WorldLocationToChunkID(TargetChunk->GetOwner()->GetActorLocation());
	float ChunkVoxelSize = ChunkWorldSize / VoxelWorldSize;
	float TargetChunkLeftBound = TargetChunkID.Y * ChunkVoxelSize;
	float TargetChunkRightBound = TargetChunkLeftBound + ChunkVoxelSize;
	float TargetChunkBackBound = TargetChunkID.X * ChunkVoxelSize;
	float TargetChunkFrontBound = TargetChunkBackBound + ChunkVoxelSize;
	float TargetChunkBottomBound = TargetChunkID.Z * ChunkVoxelSize;
	float TargetChunkTopBound = TargetChunkBottomBound + ChunkVoxelSize;

	TMap<UVoxelChunk*, UVoxelBrush*> OpsMap;
	OpsMap.Add(TargetChunk, WorldSpaceBrush);
	
	FVector BrushWorldLocation = WorldSpaceBrush->Location;

	// 左
	if (BrushWorldLocation.Y - BrushRadius < TargetChunkLeftBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(0, -1, 0);
		FVector MirroredLocation = BrushWorldLocation - FVector(0, 2 * (BrushWorldLocation.Y - TargetChunkLeftBound), 0);
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	// 右
	if (BrushWorldLocation.Y + BrushRadius > TargetChunkRightBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(0, 1, 0);
		FVector MirroredLocation = BrushWorldLocation + FVector(0, 2 * (TargetChunkRightBound - BrushWorldLocation.Y), 0);
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	// 后
	if (BrushWorldLocation.X - BrushRadius < TargetChunkBackBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(-1, 0, 0);
		FVector MirroredLocation = BrushWorldLocation - FVector(2 * (BrushWorldLocation.X - TargetChunkBackBound), 0, 0);
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	// 前
	if (BrushWorldLocation.X + BrushRadius > TargetChunkFrontBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(1, 0, 0);
		FVector MirroredLocation = BrushWorldLocation + FVector(2 * (TargetChunkFrontBound - BrushWorldLocation.X), 0, 0);
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	// 下
	if (BrushWorldLocation.Z - BrushRadius < TargetChunkBottomBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(0, 0, -1);
		FVector MirroredLocation = BrushWorldLocation - FVector(0, 0, 2 * (BrushWorldLocation.Z - TargetChunkBottomBound));
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	// 上
	if (BrushWorldLocation.Z + BrushRadius > TargetChunkTopBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(0, 0, 1);
		FVector MirroredLocation = BrushWorldLocation + FVector(0, 0, 2 * (TargetChunkTopBound - BrushWorldLocation.Z));
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	// 左后
	if (BrushWorldLocation.Y - BrushRadius < TargetChunkLeftBound && BrushWorldLocation.X - BrushRadius < TargetChunkBackBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(-1, -1, 0);
		FVector MirroredLocation = BrushWorldLocation - FVector(2 * (BrushWorldLocation.X - TargetChunkBackBound), 2 * (BrushWorldLocation.Y - TargetChunkLeftBound), 0);
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	// 左前
	if (BrushWorldLocation.Y - BrushRadius < TargetChunkLeftBound && BrushWorldLocation.X + BrushRadius > TargetChunkFrontBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(1, -1, 0);
		FVector MirroredLocation = BrushWorldLocation - FVector(2 * (BrushWorldLocation.X - TargetChunkFrontBound), 2 * (BrushWorldLocation.Y - TargetChunkLeftBound), 0);
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	// 右后
	if (BrushWorldLocation.Y + BrushRadius > TargetChunkRightBound && BrushWorldLocation.X - BrushRadius < TargetChunkBackBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(-1, 1, 0);
		FVector MirroredLocation = BrushWorldLocation - FVector(2 * (BrushWorldLocation.X - TargetChunkBackBound), 2 * (BrushWorldLocation.Y - TargetChunkRightBound), 0);
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	// 右前
	if (BrushWorldLocation.Y + BrushRadius > TargetChunkRightBound && BrushWorldLocation.X + BrushRadius > TargetChunkFrontBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(1, 1, 0);
		FVector MirroredLocation = BrushWorldLocation - FVector(2 * (BrushWorldLocation.X - TargetChunkFrontBound), 2 * (BrushWorldLocation.Y - TargetChunkRightBound), 0);
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	// 下左
	if (BrushWorldLocation.Z - BrushRadius < TargetChunkBottomBound && BrushWorldLocation.Y - BrushRadius < TargetChunkLeftBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(0, -1, -1);
		FVector MirroredLocation = BrushWorldLocation - FVector(0, 2 * (BrushWorldLocation.Y - TargetChunkLeftBound), 2 * (BrushWorldLocation.Z - TargetChunkBottomBound));
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	// 下右
	if (BrushWorldLocation.Z - BrushRadius < TargetChunkBottomBound && BrushWorldLocation.Y + BrushRadius > TargetChunkRightBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(0, 1, -1);
		FVector MirroredLocation = BrushWorldLocation - FVector(0, 2 * (BrushWorldLocation.Y - TargetChunkRightBound), 2 * (BrushWorldLocation.Z - TargetChunkBottomBound));
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	// 上左
	if (BrushWorldLocation.Z + BrushRadius > TargetChunkTopBound && BrushWorldLocation.Y - BrushRadius < TargetChunkLeftBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(0, -1, 1);
		FVector MirroredLocation = BrushWorldLocation - FVector(0, 2 * (BrushWorldLocation.Y - TargetChunkLeftBound), 2 * (BrushWorldLocation.Z - TargetChunkTopBound));
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	// 上右
	if (BrushWorldLocation.Z + BrushRadius > TargetChunkTopBound && BrushWorldLocation.Y + BrushRadius > TargetChunkRightBound)
	{
		FIntVector NeighborID = TargetChunkID + FIntVector(0, 1, 1);
		FVector MirroredLocation = BrushWorldLocation - FVector(0, 2 * (BrushWorldLocation.Y - TargetChunkRightBound), 2 * (BrushWorldLocation.Z - TargetChunkTopBound));
		UVoxelChunk* NeighborChunk = GetOrCreateChunkByID(NeighborID);
		if (NeighborChunk)
		{
			OpsMap.Add(NeighborChunk, NewObject<UVoxelBrush>(this, WorldSpaceBrush->GetClass()));
			OpsMap[NeighborChunk]->Shape = WorldSpaceBrush->Shape;
			OpsMap[NeighborChunk]->Strength = WorldSpaceBrush->Strength;
			OpsMap[NeighborChunk]->Location = MirroredLocation;
		}
	}
	
	for (const TPair<UVoxelChunk*, UVoxelBrush*>& Op : OpsMap)
	{
		UVoxelChunk* ChunkToSculpt = Op.Key;
		UVoxelBrush* Brush = Op.Value;

		ChunkToSculpt->Sculpt(Brush);
		ChunkToSculpt->Update();
	}
}

float AVoxelWorld::GetBrushRadius(UVoxelBrush* Brush) const
{
	if (Brush && Brush->Shape)
	{
		if (USphereShape* Sphere = Cast<USphereShape>(Brush->Shape))
		{
			return Sphere->Radius;
		}
	}
	
	return 0.0f;
}
