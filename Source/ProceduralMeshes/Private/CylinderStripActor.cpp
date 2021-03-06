// Copyright 2016, Sigurdur Gunnarsson. All Rights Reserved. 
// Example cylinder strip mesh

#include "ProceduralMeshesPrivatePCH.h"
#include "CylinderStripActor.h"

ACylinderStripActor::ACylinderStripActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	MeshComponent = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("ProceduralMesh"));
	MeshComponent->bShouldSerializeMeshData = false;
	MeshComponent->SetupAttachment(RootComponent);
}

#if WITH_EDITOR  
void ACylinderStripActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	PreCacheCrossSection();
	
	// We need to re-construct the buffers since values can be changed in editor
	Vertices.Empty();
	Triangles.Empty();
	SetupMeshBuffers();

	GenerateMesh();
}
#endif // WITH_EDITOR

void ACylinderStripActor::BeginPlay()
{
	Super::BeginPlay();
	PreCacheCrossSection();
	GenerateMesh();
}

void ACylinderStripActor::SetupMeshBuffers()
{
	int32 TotalNumberOfVerticesPerSection = RadialSegmentCount * 4; // 4 verts per face 
	int32 TotalNumberOfTrianglesPerSection = TotalNumberOfVerticesPerSection + 2 * RadialSegmentCount;
	int32 NumberOfSections = LinePoints.Num() - 1;

	Vertices.AddUninitialized(TotalNumberOfVerticesPerSection * NumberOfSections);
	Triangles.AddUninitialized(TotalNumberOfTrianglesPerSection * NumberOfSections);
}

void ACylinderStripActor::GenerateMesh()
{
	if (LinePoints.Num() < 2)
	{
		MeshComponent->ClearAllMeshSections();
		return;
	}

	// The number of vertices or polygons wont change at runtime, so we'll just allocate the arrays once
	if (!bHaveBuffersBeenInitialized)
	{
		SetupMeshBuffers();
		bHaveBuffersBeenInitialized = true;
	}

	// Now lets loop through all the defined points and create a cylinder between each two defined points
	int32 NumberOfSections = LinePoints.Num() - 1;
	int32 VertexIndex = 0;
	int32 TriangleIndex = 0;

	for (int32 i = 0; i < NumberOfSections; i++)
	{
		GenerateCylinder(Vertices, Triangles, LinePoints[i], LinePoints[i + 1], Radius, RadialSegmentCount, VertexIndex, TriangleIndex, bSmoothNormals);
	}

	MeshComponent->ClearAllMeshSections();
	MeshComponent->CreateMeshSection(0, Vertices, Triangles, GetBounds(), false, EUpdateFrequency::Infrequent);
	MeshComponent->SetMaterial(0, Material);
}

FBox ACylinderStripActor::GetBounds()
{
	FVector2D RangeX = FVector2D::ZeroVector;
	FVector2D RangeY = FVector2D::ZeroVector;
	FVector2D RangeZ = FVector2D::ZeroVector;

	for (FVector& Position : LinePoints)
	{
		if (Position.X < RangeX.X)
		{
			RangeX.X = Position.X;
		}
		else if (Position.X >= RangeX.Y)
		{
			RangeX.Y = Position.X;
		}

		if (Position.Y < RangeY.X)
		{
			RangeY.X = Position.Y;
		}
		else if (Position.Y >= RangeY.Y)
		{
			RangeY.Y = Position.Y;
		}

		if (Position.Z < RangeZ.X)
		{
			RangeZ.X = Position.Z;
		}
		else if (Position.Z >= RangeZ.Y)
		{
			RangeZ.Y = Position.Z;
		}
	}

	RangeX.X -= Radius;
	RangeX.Y += Radius;
	RangeY.X -= Radius;
	RangeY.Y += Radius;
	RangeZ.X -= Radius;
	RangeZ.Y += Radius;
	
	return FBox(FVector(RangeX.X, RangeY.X, RangeZ.X), FVector(RangeX.Y, RangeY.Y, RangeZ.Y));
}

FVector ACylinderStripActor::RotatePointAroundPivot(FVector InPoint, FVector InPivot, FVector InAngles)
{
	FVector direction = InPoint - InPivot; // get point direction relative to pivot
	direction = FQuat::MakeFromEuler(InAngles) * direction; // rotate it
	return direction + InPivot; // calculate rotated point
}

void ACylinderStripActor::PreCacheCrossSection()
{
	if (LastCachedCrossSectionCount == RadialSegmentCount)
	{
		return;
	}

	// Generate a cross-section for use in cylinder generation
	const float AngleBetweenQuads = (2.0f / (float)(RadialSegmentCount)) * PI;
	CachedCrossSectionPoints.Empty();

	// Pre-calculate cross section points of a circle, two more than needed
	for (int32 PointIndex = 0; PointIndex < (RadialSegmentCount + 2); PointIndex++)
	{
		float Angle = (float)PointIndex * AngleBetweenQuads;
		CachedCrossSectionPoints.Add(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0));
	}

	LastCachedCrossSectionCount = RadialSegmentCount;
}

void ACylinderStripActor::GenerateCylinder(TArray<FRuntimeMeshVertexSimple>& Vertices, TArray<int32>& Triangles, FVector StartPoint, FVector EndPoint, float InWidth, int32 InCrossSectionCount, int32& VertexIndex, int32& TriangleIndex, bool bInSmoothNormals/* = true*/)
{
	// Make a cylinder section
	const float AngleBetweenQuads = (2.0f / (float)(InCrossSectionCount)) * PI;
	const float UMapPerQuad = 1.0f / (float)InCrossSectionCount;

	FVector StartOffset = StartPoint - FVector(0, 0, 0);
	FVector Offset = EndPoint - StartPoint;

	// Find angle between vectors
	FVector LineDirection = (StartPoint - EndPoint);
	LineDirection.Normalize();
	FVector RotationAngle = LineDirection.Rotation().Add(90.f, 0.f, 0.f).Euler();

	// Start by building up vertices that make up the cylinder sides
	for (int32 QuadIndex = 0; QuadIndex < InCrossSectionCount; QuadIndex++)
	{
		float Angle = (float)QuadIndex * AngleBetweenQuads;
		float NextAngle = (float)(QuadIndex + 1) * AngleBetweenQuads;

		// Set up the vertices
		FVector p0 = (CachedCrossSectionPoints[QuadIndex] * InWidth) + StartOffset;
		p0 = RotatePointAroundPivot(p0, StartPoint, RotationAngle);
		FVector p1 = CachedCrossSectionPoints[QuadIndex + 1] * InWidth + StartOffset;
		p1 = RotatePointAroundPivot(p1, StartPoint, RotationAngle);
		FVector p2 = p1 + Offset;
		FVector p3 = p0 + Offset;

		// Set up the quad triangles
		int32 VertIndex1 = VertexIndex++;
		int32 VertIndex2 = VertexIndex++;
		int32 VertIndex3 = VertexIndex++;
		int32 VertIndex4 = VertexIndex++;

		Vertices[VertIndex1].Position = p0;
		Vertices[VertIndex2].Position = p1;
		Vertices[VertIndex3].Position = p2;
		Vertices[VertIndex4].Position = p3;

		// Now create two triangles from those four vertices
		// The order of these (clockwise/counter-clockwise) dictates which way the normal will face. 
		Triangles[TriangleIndex++] = VertIndex4;
		Triangles[TriangleIndex++] = VertIndex3;
		Triangles[TriangleIndex++] = VertIndex1;

		Triangles[TriangleIndex++] = VertIndex3;
		Triangles[TriangleIndex++] = VertIndex2;
		Triangles[TriangleIndex++] = VertIndex1;

		// UVs.  Note that Unreal UV origin (0,0) is top left
		Vertices[VertIndex1].UV0 = FVector2D(1.0f - (UMapPerQuad * QuadIndex), 1.0f);
		Vertices[VertIndex2].UV0 = FVector2D(1.0f - (UMapPerQuad * (QuadIndex + 1)), 1.0f);
		Vertices[VertIndex3].UV0 = FVector2D(1.0f - (UMapPerQuad * (QuadIndex + 1)), 0.0f);
		Vertices[VertIndex4].UV0 = FVector2D(1.0f - (UMapPerQuad * QuadIndex), 0.0f);

		// Normals
		FVector NormalCurrent = FVector::CrossProduct(Vertices[VertIndex1].Position - Vertices[VertIndex3].Position, Vertices[VertIndex2].Position - Vertices[VertIndex3].Position).GetSafeNormal();

		if (bInSmoothNormals)
		{
			// To smooth normals we give the vertices different values than the polygon they belong to.
			// GPUs know how to interpolate between those.
			// I do this here as an average between normals of two adjacent polygons
			float NextNextAngle = (float)(QuadIndex + 2) * AngleBetweenQuads;
			FVector p4 = (CachedCrossSectionPoints[QuadIndex + 2] * InWidth) + StartOffset;
			p4 = RotatePointAroundPivot(p4, StartPoint, RotationAngle);

			// p1 to p4 to p2
			FVector NormalNext = FVector::CrossProduct(p1 - p2, p4 - p2).GetSafeNormal();
			FVector AverageNormalRight = (NormalCurrent + NormalNext) / 2;
			AverageNormalRight = AverageNormalRight.GetSafeNormal();

			float PreviousAngle = (float)(QuadIndex - 1) * AngleBetweenQuads;
			FVector pMinus1 = FVector(FMath::Cos(PreviousAngle) * InWidth, FMath::Sin(PreviousAngle) * InWidth, 0.f) + StartOffset;
			pMinus1 = RotatePointAroundPivot(pMinus1, StartPoint, RotationAngle);

			// p0 to p3 to pMinus1
			FVector NormalPrevious = FVector::CrossProduct(p0 - pMinus1, p3 - pMinus1).GetSafeNormal();
			FVector AverageNormalLeft = (NormalCurrent + NormalPrevious) / 2;
			AverageNormalLeft = AverageNormalLeft.GetSafeNormal();

			Vertices[VertIndex1].Normal = FPackedNormal(AverageNormalLeft);
			Vertices[VertIndex2].Normal = FPackedNormal(AverageNormalRight);
			Vertices[VertIndex3].Normal = FPackedNormal(AverageNormalRight);
			Vertices[VertIndex4].Normal = FPackedNormal(AverageNormalLeft);
		}
		else
		{
			// If not smoothing we just set the vertex normal to the same normal as the polygon they belong to
			Vertices[VertIndex1].Normal = Vertices[VertIndex2].Normal = Vertices[VertIndex3].Normal = Vertices[VertIndex4].Normal = FPackedNormal(NormalCurrent);
		}

		// Tangents (perpendicular to the surface)
		FVector SurfaceTangent = p0 - p1;
		SurfaceTangent = SurfaceTangent.GetSafeNormal();
		Vertices[VertIndex1].Tangent = Vertices[VertIndex2].Tangent = Vertices[VertIndex3].Tangent = Vertices[VertIndex4].Tangent = FPackedNormal(SurfaceTangent);
	}
}
