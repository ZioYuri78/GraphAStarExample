// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "HexGridLibPluginBPLibrary.h"
#include "HexGridLibPlugin.h"
#include "HexGridActor.h"

DEFINE_LOG_CATEGORY(LogHGLP);

UHexGridLibPluginBPLibrary::UHexGridLibPluginBPLibrary(const FObjectInitializer &ObjectInitializer)
	: Super(ObjectInitializer)
{

}


FHCubeCoord UHexGridLibPluginBPLibrary::GetDirection(int32 Dir)
{
	check(Dir < HDirections.Directions.Num());
	return HDirections.Directions[Dir];
}


FHCubeCoord UHexGridLibPluginBPLibrary::GetDiagonal(int32 Dia)
{
	check(Dia < HDiagonals.Diagonals.Num());
	return HDiagonals.Diagonals[Dia];
}


FVector UHexGridLibPluginBPLibrary::HexToWorld(const FHTileLayout &TileLayout, const FHCubeCoord &H)
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

	FVector Result = FVector(x + TileLayout.Origin.X, y + TileLayout.Origin.Y, TileLayout.Origin.Z);

	return Result;
}


FHCubeCoord UHexGridLibPluginBPLibrary::WorldToHex(const FHTileLayout &TileLayout, const FVector &Location)
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


FVector UHexGridLibPluginBPLibrary::SnapToGrid(const FHTileLayout &TileLayout, const FVector &Location)
{
	float TempZ{ Location.Z };
	FVector Result{ HexToWorld(TileLayout, WorldToHex(TileLayout, Location)) };
	Result.Z = TempZ;
	return Result;
}


FHCubeCoord UHexGridLibPluginBPLibrary::HexRound(const FHFractional &F)
{
	int32 q = int32(FMath::RoundToDouble(F.QRS.X));
	int32 r = int32(FMath::RoundToDouble(F.QRS.Y));
	int32 s = int32(FMath::RoundToDouble(F.QRS.Z));

	float q_diff = FMath::Abs(q - F.QRS.X);
	float r_diff = FMath::Abs(r - F.QRS.Y);
	float s_diff = FMath::Abs(s - F.QRS.Z);

	if (q_diff > r_diff &&q_diff > s_diff)
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


void UHexGridLibPluginBPLibrary::HexSingleRing(const FHCubeCoord &Center, const int32 Radius, TArray<FHCubeCoord> &OutRing)
{
	OutRing.Empty();
	FHCubeCoord Start{ Center + (HDirections.Directions[4] * Radius) };

	for (int32 i{ 0 }; i < 6; ++i)
	{
		for (int32 j{ 0 }; j < Radius; ++j)
		{
			OutRing.Add(Start);
			Start = Start + HDirections.Directions[i];
		}
	}
}


void UHexGridLibPluginBPLibrary::HexLinedraw(const FHCubeCoord &A, const FHCubeCoord &B, TArray<FHCubeCoord> &OutLine)
{
	OutLine.Empty();
	int32 N{ HexDistance(A, B) };
	FHFractional ANudge{ A.QRS.X + static_cast<float>(1e-6), A.QRS.Y + static_cast<float>(1e-6), A.QRS.Z - static_cast<float>(2e-6) };
	FHFractional BNudge{ B.QRS.X + static_cast<float>(1e-6), B.QRS.Y + static_cast<float>(1e-6), B.QRS.Z - static_cast<float>(2e-6) };

	float Step{ 1.0f / FMath::Max(N, 1) };

	for (int32 i{ 0 }; i <= N; ++i)
	{
		OutLine.Add(HexRound(HexLerp(ANudge, BNudge, Step * i)));
	}
}


void UHexGridLibPluginBPLibrary::HexRange(const FHCubeCoord &Center, const int32 Range, TArray<FHCubeCoord> &OutRange)
{
	OutRange.Empty();
	for (int32 Q{ -Range }; Q <= Range; ++Q)
	{
		for (int32 R{ FMath::Max(-Range, -Q - Range) }; R <= FMath::Min(Range, -Q + Range); ++R)
		{
			int32 S{ -Q - R };
			OutRange.Add(Center + FHCubeCoord(FIntVector(Q, R, S)));
		}
	}
}


void UHexGridLibPluginBPLibrary::HexIntersectRanges(const FHCubeCoord &CenterA, const int32 RangeA, const FHCubeCoord &CenterB, const int32 RangeB, TArray<FHCubeCoord> &OutIntersection)
{
	OutIntersection.Empty();

	int32 Qmin{ FMath::Max(CenterA.QRS.X - RangeA, CenterB.QRS.X - RangeB) };
	int32 QMax{ FMath::Min(CenterA.QRS.X + RangeA, CenterB.QRS.X + RangeB) };

	int32 Rmin{ FMath::Max(CenterA.QRS.Y - RangeA, CenterB.QRS.Y - RangeB) };
	int32 RMax{ FMath::Min(CenterA.QRS.Y + RangeA, CenterB.QRS.Y + RangeB) };

	int32 Smin{ FMath::Max(CenterA.QRS.Z - RangeA, CenterB.QRS.Z - RangeB) };
	int32 SMax{ FMath::Min(CenterA.QRS.Z + RangeA, CenterB.QRS.Z + RangeB) };

	for (int32 Q{ Qmin }; Q <= QMax; ++Q)
	{
		for (int32 R{ FMath::Max(Rmin, -Q - SMax) }; R <= FMath::Min(RMax, -Q - Smin); ++R)
		{
			int32 S{ -Q - R };
			FHCubeCoord H{ FIntVector(Q, R, S) };
			OutIntersection.Add(H);
		}
	}
}


bool UHexGridLibPluginBPLibrary::HexGetTileAtCoord(const AHexGridActor *Grid, const FHCubeCoord &Coord, FHTile &Tile)
{	
	if (!Grid)
	{
		UE_LOG(LogHGLP, Warning, TEXT("AHexGridActor *Grid is nullptr!"));
		return false;
	}

	int32 Radius{
		FMath::Max3(
			FMath::Abs(Coord.QRS.X),
			FMath::Abs(Coord.QRS.Y),
			FMath::Abs(Coord.QRS.Z)
		)
	};

	
	int32 LastIdx{ Grid->GridTiles.Num() - 1 };
	int32 StartIdx{};
	int32 EndIdx{};

	if (Coord.QRS.X < 0)
	{
		StartIdx = 0;
		EndIdx = LastIdx / 2;
	}
	else if (Coord.QRS.X > 0)
	{
		StartIdx = (LastIdx / 2) + 1;
		EndIdx = LastIdx;
	}
	else
	{
		StartIdx = (LastIdx / 2) - Radius;
		EndIdx = (LastIdx / 2) + Radius;
	}


	for (int32 i{ StartIdx }; i < EndIdx; ++i)
	{
		if (Grid->GridTiles[i].GridCoordinates == Coord)
		{
			Tile = Grid->GridTiles[i];
			return true;
		}
	}

	return false;
}


bool UHexGridLibPluginBPLibrary::HexagonalGrid(AHexGridActor *Grid, const FOnAddTileDelegate &OnAddTile)
{
	SCOPE_CYCLE_COUNTER(STAT_CreateGrid);

	if (!Grid)
	{
		UE_LOG(LogHGLP, Warning, TEXT("AHexGridActor *Grid is nullptr!"));
		return false;
	}

	// https://www.unrealengine.com/en-US/blog/optimizing-tarray-usage-for-performance
	// preallocate array memory
	// R1 = 1 + 6*1
	// R2 = 1 + 6*1 + 6*2
	// R3 = 1 + 6*1 + 6*2 + 6*3
	// R4 = 1 + 6*1 + 6*2 + 6*3 + 6*4
	// R5 = .......
	int32 Size{ 1 };
	for (int32 i{ 1 }; i <= Grid->Radius; ++i)
	{
		Size += 6 * i;
	}
	Grid->GridTiles.Empty();
	Grid->GridTiles.Reserve(Size);

	Grid->GridShape = EGridShape::HEXAGON;
	
	FHTile TempTile{};

	for (int32 Q{ -Grid->Radius }; Q <= Grid->Radius; ++Q)
	{
		// Calculate R1
		int32 R1{ FMath::Max(-Grid->Radius, -Q - Grid->Radius) };

		// Calculate R2
		int32 R2{ FMath::Min(Grid->Radius, -Q + Grid->Radius) };

		for (int32 R{ R1 }; R <= R2; ++R)
		{
			FHCubeCoord TempCoord{ FIntVector(Q, R, -Q - R) };
			
			TempTile.TileLayout = Grid->TileLayout;
			TempTile.GridCoordinates = TempCoord;
			TempTile.WorldCoordinates = HexToWorld(Grid->TileLayout, TempCoord);
			Grid->GridTiles.Add(TempTile);

			OnAddTile.ExecuteIfBound(TempTile);
		}
	}
	
	return true;
}


