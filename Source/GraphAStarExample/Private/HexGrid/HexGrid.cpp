// Fill out your copyright notice in the Description page of Project Settings.


#include "HexGrid.h"

DEFINE_LOG_CATEGORY(LogGraphAStarExample_HexGrid);

// Sets default values
AHexGrid::AHexGrid()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AHexGrid::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHexGrid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AHexGrid::CreateGrid(const FHTileLayout &TLayout, const int32 GridRadius, const FCreationStepDelegate &CreationStepDelegate)
{
	SCOPE_CYCLE_COUNTER(STAT_CreateGrid);


	// https://www.unrealengine.com/en-US/blog/optimizing-tarray-usage-for-performance
	// preallocate array memory
	// R1 = 1 + 6*1
	// R2 = 1 + 6*1 + 6*2
	// R3 = 1 + 6*1 + 6*2 + 6*3
	// R4 = 1 + 6*1 + 6*2 + 6*3 + 6*4
	// R5 = .......
	int32 Size{ 1 };
	for (int32 i{ 1 }; i <= Radius; ++i)
	{
		Size += 6 * i;
	}
	GridCoordinates.Reserve(Size);

	// Check if we provided a delegate, if yes we also reserve space in the GridTiles array.
	if (CreationStepDelegate.IsBound())
	{
		GridTiles.Reserve(Size);		
	}
	else
	{
		UE_LOG(LogGraphAStarExample_HexGrid, Warning, TEXT("AHexGrid::CreateGrid(...) CreationStepDelegate not bound!"));
	}

	TileLayout = TLayout;
	
	Radius = GridRadius;

	for (int32 Q{ -Radius }; Q <= Radius; ++Q)
	{
		// Calculate R1
		int32 R1{ FMath::Max(-Radius, -Q - Radius) };

		// Calculate R2
		int32 R2{ FMath::Min(Radius, -Q + Radius) };

		for (int32 R{ R1 }; R <= R2; ++R)
		{
			FHCubeCoord CCoord{ FIntVector(Q, R, -Q - R) };
			GridCoordinates.Add(CCoord);

			// If we provided a delegate execute it, with this we can make additional operations on each step of the loop,
			// in our example i use it in the blueprint to add a tile on each cube coordinate. 
			CreationStepDelegate.ExecuteIfBound(TileLayout, CCoord);
		}
	}
}


FVector AHexGrid::HexToWorld(const FHCubeCoord &H)
{
	// Set the layout orientation
	FHTileOrientation TileOrientation{};
	if (TileLayout.TileOrientation == EHTileOrientationFlag::FLAT)
	{
		TileOrientation = HFlatTopLayout;
	}
	else
	{
		TileOrientation = HPointyLayout;
	}

	float x = ((TileOrientation.f0 * H.QRS.X) + (TileOrientation.f1 * H.QRS.Y)) * TileLayout.TileSize;
	float y = ((TileOrientation.f2 * H.QRS.X) + (TileOrientation.f3 * H.QRS.Y)) * TileLayout.TileSize;

	return FVector(x + TileLayout.Origin.X, y + TileLayout.Origin.Y, TileLayout.Origin.Z);
}


FHCubeCoord AHexGrid::WorldToHex(const FVector &Location)
{
	// Set the layout orientation
	FHTileOrientation TileOrientation{};
	if (TileLayout.TileOrientation == EHTileOrientationFlag::FLAT)
	{
		TileOrientation = HFlatTopLayout;
	}
	else
	{
		TileOrientation = HPointyLayout;
	}

	FVector InternalLocation{ FVector((Location.X - TileLayout.Origin.X) / TileLayout.TileSize,
									  (Location.Y - TileLayout.Origin.Y) / TileLayout.TileSize,
									  (Location.Z - TileLayout.Origin.Z))	// Z is useless here.
	};

	float q = ((TileOrientation.b0 * InternalLocation.X) + (TileOrientation.b1 * InternalLocation.Y));
	float r = ((TileOrientation.b2 * InternalLocation.X) + (TileOrientation.b3 * InternalLocation.Y));
	
	FVector v{ (TileLayout.TileOrientation == EHTileOrientationFlag::FLAT) ? FVector(q, -q - r, r) : FVector(q, r, -q - r) };

	return HexRound(FHFractional(v));
}


FVector AHexGrid::SnapToGrid(const FVector &Location)
{
	float TempZ{ Location.Z };
	FVector Result{ HexToWorld(WorldToHex(Location)) };
	Result.Z = TempZ;
	return Result;
}


FHCubeCoord AHexGrid::HexRound(const FHFractional &F)
{
	int32 q{ int32(FMath::RoundToDouble(F.QRS.X)) };
	int32 r{ int32(FMath::RoundToDouble(F.QRS.Y)) };
	int32 s{ int32(FMath::RoundToDouble(F.QRS.Z)) };

	float q_diff{ FMath::Abs(q - F.QRS.X) };
	float r_diff{ FMath::Abs(r - F.QRS.Y) };
	float s_diff{ FMath::Abs(s - F.QRS.Z) };

	if ((q_diff > r_diff) && (q_diff > s_diff))
	{
		q = -r - s;
	}
	else if (r_diff > s_diff)
	{
		r = -q - s;
	}
	else
	{
		s = -q - r;
	}

	return FHCubeCoord{ FIntVector(q, r, s) };
}


bool AHexGrid::HexEqual(const FHCubeCoord &A, const FHCubeCoord &B)
{
	return A == B;
}

FHCubeCoord AHexGrid::GetDirection(int32 Dir)
{
	check(Dir < HDirections.Directions.Num());
	return HDirections.Directions[Dir];
}

FHCubeCoord AHexGrid::GetNeighbor(const FHCubeCoord &H, const FHCubeCoord &Dir)
{
	return H + Dir;
}

