#include "VoxelChunk.h"

#include "VoxelGenerator.h"
#include "MarchingCubes/MeshBuilder.h"
#include "MarchingCubes/MeshData.h"
#include "VoxelStats.h"
#include "DynamicMesh/MeshAttributeUtil.h"

UVoxelChunk::UVoxelChunk()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UVoxelChunk::BeginPlay()
{
	Super::BeginPlay();
	MeshComponent = NewObject<UDynamicMeshComponent>(GetOwner());
	MeshComponent->SetupAttachment(GetOwner()->GetRootComponent());
	MeshComponent->RegisterComponent();
	MeshComponent->SetMaterial(0, Material);
	MeshComponent->SetComplexAsSimpleCollisionEnabled(true, true);
	MeshComponent->bUseAsyncCooking = true;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetGenerateOverlapEvents(true);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Data = new FVoxel[Size * Size * Size];

	FVoxelGenerator::Clear(Data, Size);
	// Generate();
	// Update();
}

void UVoxelChunk::BeginDestroy()
{
	Super::BeginDestroy();
	delete Data;
	Data = nullptr;
}

void UVoxelChunk::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UVoxelChunk::SetSize(int NewSize)
{
	Size = NewSize;
	delete Data;
	Data = new FVoxel[Size * Size * Size];
}

void UVoxelChunk::Sculpt(UVoxelBrush* VoxelBrush)
{
	// FVoxelGenerator::Sculpt(Data, Size, VoxelBrush);
	// FVoxelGenerator::Sculpt(Data, Size, VoxelBrush, GetOwner()->GetActorLocation());
	
	const FVector BrushWorldLocation = VoxelBrush->Location;
	const FVector ChunkWorldOrigin = GetOwner()->GetActorLocation() / 100.0f;
	const FVector BrushLocalLocation = BrushWorldLocation - ChunkWorldOrigin;

	UVoxelBrush* LocalSpaceBrush = NewObject<UVoxelBrush>();
	LocalSpaceBrush->Shape = VoxelBrush->Shape;
	LocalSpaceBrush->Strength = VoxelBrush->Strength;
	LocalSpaceBrush->Location = BrushLocalLocation;

	FVoxelGenerator::Sculpt(Data, Size, LocalSpaceBrush);
}

void UVoxelChunk::Paint(UVoxelBrush* VoxelBrush, int MaterialId)
{
	FVoxelGenerator::Paint(Data, Size, VoxelBrush, MaterialId);
}

void UVoxelChunk::Generate() const
{
	const double StartTime = FPlatformTime::Seconds();
	FVoxelGenerator::Generate(GetOwner()->GetActorLocation(), Size, Data);
	StatsRef.GenerateTime = (FPlatformTime::Seconds() - StartTime) * 1000;
}

void UVoxelChunk::Update() const
{
	const double StartTime = FPlatformTime::Seconds();
	FMCMeshBuilder MeshBuilder;
	const FMCMesh MeshData = MeshBuilder.Build(Data, Size - 1);
	StatsRef.VertexCount = MeshData.Vertices.Num();
	StatsRef.TriangleCount = MeshData.Triangles.Num();

	TArray<int32> Indices;
	FDynamicMesh3* Mesh = MeshComponent->GetMesh();
	Mesh->Clear();
	Mesh->EnableVertexNormals(FVector3f());
	Mesh->EnableVertexColors(FVector4f());

	Mesh->EnableAttributes();
	Mesh->Attributes()->EnablePrimaryColors();
	const auto ColorOverlay = Mesh->Attributes()->PrimaryColors();

	const FVector ChunkWorldOrigin = GetOwner()->GetActorLocation();

	for (int i = 0; i < MeshData.Vertices.Num(); i++)
	{
		int Id = Mesh->AppendVertex(MeshData.Vertices[i]);
		Indices.Add(Id);
		Mesh->SetVertexNormal(Id, FVector3f(MeshData.Normals[i]));
		Mesh->SetVertexColor(Id, FVector4f(MeshData.Colors[i]));
		ColorOverlay->AppendElement(MeshData.Colors[i]);
	}

	for (int i = 0; i < MeshData.Triangles.Num(); i += 3)
	{
		const int T0 = Indices[MeshData.Triangles[i]];
		const int T1 = Indices[MeshData.Triangles[i + 1]];
		const int T2 = Indices[MeshData.Triangles[i + 2]];
		const int Id = Mesh->AppendTriangle(T0, T1, T2);
		ColorOverlay->SetTriangle(Id, UE::Geometry::FIndex3i(T0, T1, T2));
	}

	MeshComponent->NotifyMeshUpdated();
	MeshComponent->UpdateCollision(false);

	StatsRef.UpdateTime = (FPlatformTime::Seconds() - StartTime) * 1000;
}

void UVoxelChunk::GodFinger_DigHoleAtCenter()
{
	// ===================================================================
	// 阶段一：用最直接的方式修改数据
	// ===================================================================
	
	// 1. 计算出数据数组最中心的索引。这里只使用纯粹的局部索引。
	const int CenterX = Size / 2;
	const int CenterY = Size / 2;
	const int CenterZ = Size / 2;

	// 这是作者在FMCMeshBuilder里使用的GetIndex的C++等价实现。
	// 确保我们使用的索引计算方式和构建器是一致的。
	const int CenterIndex = (CenterZ * Size * Size) + (CenterY * Size) + CenterX;
	
	// 2. 一个绝对必要的安全检查
	if (CenterIndex < 0 || CenterIndex >= (Size * Size * Size))
	{
		UE_LOG(LogTemp, Error, TEXT("!!!!!!!! GodFinger FAILED: Index %d is OUT OF BOUNDS for Size %d !!!!!"), CenterIndex, Size);
		return;
	}
	
	// 3. 直接、粗暴地修改这个中心点的数据。
	//    将密度从默认的 1.0 (实心) 改为 -1.0 (空心)。
	//    这个巨大的密度差，Marching Cubes算法不可能看不见。
	Data[CenterIndex].Density = -1.0f;

	// ===================================================================
	// 阶段二：用最详细的日志轰炸来确认我们的状态
	// ===================================================================

	// 打印这个Chunk所属Actor的信息
	AActor* MyOwner = GetOwner();
	if (!MyOwner)
	{
		UE_LOG(LogTemp, Error, TEXT("!!!!!!!! GodFinger FAILED: This Chunk has NO OWNER !!!!!"));
		return;
	}

	// 打印这个MeshComponent的信息
	if (!MeshComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("!!!!!!!! GodFinger FAILED: MeshComponent is NULL !!!!!"));
		return;
	}

	// 获取并打印出所有重要的变换信息
	FVector ChunkLocation = MyOwner->GetActorLocation();
	FTransform MeshComponentWorldTransform = MeshComponent->GetComponentTransform();
	bool bIsAttached = MeshComponent->GetAttachParent() != nullptr;

	UE_LOG(LogTemp, Warning, TEXT("================ GOD FINGER REPORT ================"));
	UE_LOG(LogTemp, Warning, TEXT(">> Actor Location: %s"), *ChunkLocation.ToString());
	UE_LOG(LogTemp, Warning, TEXT(">> MeshComponent is Attached: %s"), bIsAttached ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Warning, TEXT(">> MeshComponent World Location: %s"), *MeshComponentWorldTransform.GetLocation().ToString());
	UE_LOG(LogTemp, Warning, TEXT(">> Modified Data at LOCAL index (%d, %d, %d)"), CenterX, CenterY, CenterZ);
	UE_LOG(LogTemp, Warning, TEXT("==================================================="));

	// ===================================================================
	// 阶段三：强制更新
	// ===================================================================
	Update();
}
