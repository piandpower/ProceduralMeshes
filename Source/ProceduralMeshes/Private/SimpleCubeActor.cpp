// Copyright 2016, Sigurdur Gunnarsson. All Rights Reserved. 
// Example cube

#include "ProceduralMeshesPrivatePCH.h"
#include "SimpleCubeActor.h"

ASimpleCubeActor::ASimpleCubeActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	MeshComponent = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("ProceduralMesh"));
	MeshComponent->bShouldSerializeMeshData = false;
	MeshComponent->SetupAttachment(RootComponent);
}

#if WITH_EDITOR  
void ASimpleCubeActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	GenerateMesh();
}
#endif // WITH_EDITOR

void ASimpleCubeActor::BeginPlay()
{
	Super::BeginPlay();
	GenerateMesh();
}

void ASimpleCubeActor::SetupMeshBuffers()
{
	int32 VertexCount = 6 * 4; // 6 sides on a cube, 4 verts each
	Vertices.AddUninitialized(VertexCount);
	Triangles.AddUninitialized(6 * 2 * 3); // 2x triangles per cube side, 3 verts each
}

void ASimpleCubeActor::GenerateMesh()
{
	// The number of vertices or polygons wont change at runtime, so we'll just allocate the arrays once
	if (!bHaveBuffersBeenInitialized)
	{
		SetupMeshBuffers();
		bHaveBuffersBeenInitialized = true;
	}

	FBox BoundingBox = FBox(-Size / 2.0f, Size / 2.0f);
	GenerateCube(Vertices, Triangles, Size);
	
	MeshComponent->ClearAllMeshSections();
	MeshComponent->CreateMeshSection(0, Vertices, Triangles, BoundingBox, false, EUpdateFrequency::Infrequent);
	MeshComponent->SetMaterial(0, Material);
}

void ASimpleCubeActor::GenerateCube(TArray<FRuntimeMeshVertexSimple>& Vertices, TArray<int32>& Triangles, FVector Size)
{
	// NOTE: Unreal uses an upper-left origin UV
	// NOTE: This sample uses a simple UV mapping scheme where each face is the same
	// NOTE: For a normal facing towards us, be build the polygon CCW in the order 0-1-2 then 0-2-3 to complete the quad.
	// Remember in Unreal, X is forwards, Y is to your right and Z is up.

	// Calculate a half offset so we get correct center of object
	float OffsetX = Size.X / 2.0f;
	float OffsetY = Size.Y / 2.0f;
	float OffsetZ = Size.Z / 2.0f;

	// Define the 8 corners of the cube
	FVector p0 = FVector(OffsetX,  OffsetY, -OffsetZ);
	FVector p1 = FVector(OffsetX, -OffsetY, -OffsetZ);
	FVector p2 = FVector(OffsetX, -OffsetY,  OffsetZ);
	FVector p3 = FVector(OffsetX,  OffsetY,  OffsetZ);
	FVector p4 = FVector(-OffsetX, OffsetY, -OffsetZ);
	FVector p5 = FVector(-OffsetX, -OffsetY, -OffsetZ);
	FVector p6 = FVector(-OffsetX, -OffsetY, OffsetZ);
	FVector p7 = FVector(-OffsetX, OffsetY, OffsetZ);

	// Now we create 6x faces, 4 vertices each
	int32 VertexOffset = 0;
	int32 TriangleOffset = 0;
	FPackedNormal Normal = FPackedNormal::ZeroNormal;
	FPackedNormal Tangent = FPackedNormal::ZeroNormal;

 	// Front (+X) face: 0-1-2-3
	Normal = FVector(1, 0, 0);
	Tangent = FVector(0, 1, 0);
	BuildQuad(Vertices, Triangles, p0, p1, p2, p3, VertexOffset, TriangleOffset, Normal, Tangent);

 	// Back (-X) face: 5-4-7-6
	Normal = FVector(-1, 0, 0);
	Tangent = FVector(0, -1, 0);
	BuildQuad(Vertices, Triangles, p5, p4, p7, p6, VertexOffset, TriangleOffset, Normal, Tangent);

 	// Left (-Y) face: 1-5-6-2
	Normal = FVector(0, -1, 0);
	Tangent = FVector(1, 0, 0);
	BuildQuad(Vertices, Triangles, p1, p5, p6, p2, VertexOffset, TriangleOffset, Normal, Tangent);

 	// Right (+Y) face: 4-0-3-7
	Normal = FVector(0, 1, 0);
	Tangent = FVector(-1, 0, 0);
	BuildQuad(Vertices, Triangles, p4, p0, p3, p7, VertexOffset, TriangleOffset, Normal, Tangent);

 	// Top (+Z) face: 6-7-3-2
	Normal = FVector(0, 0, 1);
	Tangent = FVector(0, 1, 0);
	BuildQuad(Vertices, Triangles, p6, p7, p3, p2, VertexOffset, TriangleOffset, Normal, Tangent);

 	// Bottom (-Z) face: 1-0-4-5
	Normal = FVector(0, 0, -1);
	Tangent = FVector(0, -1, 0);
	BuildQuad(Vertices, Triangles, p1, p0, p4, p5, VertexOffset, TriangleOffset, Normal, Tangent);
}

void ASimpleCubeActor::BuildQuad(TArray<FRuntimeMeshVertexSimple>& Vertices, TArray<int32>& Triangles, FVector BottomLeft, FVector BottomRight, FVector TopRight, FVector TopLeft, int32& VertexOffset, int32& TriangleOffset, FPackedNormal Normal, FPackedNormal Tangent)
{
	int32 Index1 = VertexOffset++;
	int32 Index2 = VertexOffset++;
	int32 Index3 = VertexOffset++;
	int32 Index4 = VertexOffset++;
	Vertices[Index1].Position = BottomLeft;
	Vertices[Index2].Position = BottomRight;
	Vertices[Index3].Position = TopRight;
	Vertices[Index4].Position = TopLeft;
	Vertices[Index1].UV0 = FVector2D(0.0f, 1.0f);
	Vertices[Index2].UV0 = FVector2D(1.0f, 1.0f);
	Vertices[Index3].UV0 = FVector2D(1.0f, 0.0f);
	Vertices[Index4].UV0 = FVector2D(0.0f, 0.0f);
	Triangles[TriangleOffset++] = Index1;
	Triangles[TriangleOffset++] = Index2;
	Triangles[TriangleOffset++] = Index3;
	Triangles[TriangleOffset++] = Index1;
	Triangles[TriangleOffset++] = Index3;
	Triangles[TriangleOffset++] = Index4;
	// On a cube side, all the vertex normals face the same way
	Vertices[Index1].Normal = Vertices[Index2].Normal = Vertices[Index3].Normal = Vertices[Index4].Normal = Normal;
	Vertices[Index1].Tangent = Vertices[Index2].Tangent = Vertices[Index3].Tangent = Vertices[Index4].Tangent = Tangent;
}