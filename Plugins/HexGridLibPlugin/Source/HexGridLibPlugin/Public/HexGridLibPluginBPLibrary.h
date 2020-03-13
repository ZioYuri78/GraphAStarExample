// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HGLPTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HexGridLibPluginBPLibrary.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHGLP, Log, All);
DECLARE_STATS_GROUP(TEXT("HEXGRID_STATS"), STATGROUP_HEXGRID, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("CreateGrid(..)"), STAT_CreateGrid, STATGROUP_HEXGRID);

/* Delegate used in the CreateGrid function, executed if bound on each inner loop step. */
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAddTileDelegate, UPARAM(ref) FHTile &, Tile);

UCLASS()
class HEXGRIDLIBPLUGIN_API UHexGridLibPluginBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/**
	 * Compare two Cube coordinate.
	 * @see https://www.redblobgames.com/grids/hexagons/implementation.html#hex-equality
	*/
	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FORCEINLINE bool HexEqual(const FHCubeCoord &A, const FHCubeCoord &B)
	{
		return A == B;
	}

	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FORCEINLINE bool HexNotEqual(const FHCubeCoord &A, const FHCubeCoord &B)
	{
		return !HexEqual(A, B);
	}

	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FORCEINLINE FHCubeCoord HexAdd(const FHCubeCoord &A, const FHCubeCoord &B)
	{
		return A + B;
	}

	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FORCEINLINE FHCubeCoord HexSubstract(const FHCubeCoord &A, const FHCubeCoord &B)
	{
		return A - B;
	}

	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FORCEINLINE FHCubeCoord HexMultiply(const FHCubeCoord &H, int32 k)
	{
		return H * k;
	}

	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FORCEINLINE int32 HexDistance(const FHCubeCoord &A, const FHCubeCoord &B)
	{
		return FMath::Max3(FMath::Abs(A.QRS.X - B.QRS.X),
						   FMath::Abs(A.QRS.Y - B.QRS.Y),
						   FMath::Abs(A.QRS.Z - B.QRS.Z));
	}

	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FORCEINLINE FHCubeCoord HexRotateR60(const FHCubeCoord &Center, const FHCubeCoord &H)
	{
		FHCubeCoord PfromC{ H - Center };
		PfromC.QRS = FIntVector(-PfromC.QRS.Z, -PfromC.QRS.Y, -PfromC.QRS.X);
		PfromC.QRS = FIntVector(PfromC.QRS.Y, PfromC.QRS.X, PfromC.QRS.Z);

		return PfromC + Center;
	}

	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FORCEINLINE FHCubeCoord HexRotateL60(const FHCubeCoord &Center, const FHCubeCoord &H)
	{
		FHCubeCoord PfromC{ H - Center };
		PfromC.QRS = FIntVector(-PfromC.QRS.Y, -PfromC.QRS.X, -PfromC.QRS.Z);
		PfromC.QRS = FIntVector(PfromC.QRS.Z, PfromC.QRS.Y, PfromC.QRS.X);

		return PfromC + Center;
	}

	/**
	 * Return the neighbor Cube coordinate in the provided direction.
	 * @see https://www.redblobgames.com/grids/hexagons/#neighbors
	 */
	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FORCEINLINE FHCubeCoord GetNeighbor(const FHCubeCoord &H, const FHCubeCoord &Dir)
	{
		return H + Dir;
	}

	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FORCEINLINE FHFractional HexLerp(const FHFractional &A, const FHFractional &B, float t)
	{
		return FHFractional(FVector(FMath::Lerp(A.QRS.X, B.QRS.X, t),
							FMath::Lerp(A.QRS.Y, B.QRS.Y, t),
							FMath::Lerp(A.QRS.Z, B.QRS.Z, t)));
	}



	/**
	 * Return one of the six cube directions.
	 * @see https://www.redblobgames.com/grids/hexagons/#neighbors
	 *
	 *	IDX	|Flat			|Pointy
	 *	----|---------------|---------
	 *	0	|Top			|Top Left
	 *	1	|Top Right		|Top Right
	 *	2	|Bottom Right	|Right
	 *	3	|Bottom			|Bottom Right
	 *	4	|Bottom Left	|Bottom Left
	 *	5	|Top Left		|Left
	*/
	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FHCubeCoord GetDirection(int32 Dir);

	/*
	IDX	|Flat			|Pointy
	----|---------------|--------------
	0	|Top Right		|Top
	1	|Right			|Top Right
	2	|Bottom Right	|Bottom Right
	3	|Bottom Left	|Bottom
	4	|Left			|Bottom Left
	5	|Top Left		|Top Left
	*/
	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FHCubeCoord GetDiagonal(int32 Dia);


	/**
	 * Convert coordinates from Cube space to World space.
	 * @see https://www.redblobgames.com/grids/hexagons/#hex-to-pixel
	*/
	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FVector HexToWorld(const FHTileLayout &TileLayout, const FHCubeCoord &H);

	/**
	 * Convert coordinates from World space to Cube space.
	 * @see https://www.redblobgames.com/grids/hexagons/#pixel-to-hex
	*/
	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FHCubeCoord WorldToHex(const FHTileLayout &TileLayout, const FVector &Location);

	/** Snap a World coordinate to the Grid space. */
	UFUNCTION(BlueprintCallable, Category = "GraphAStarExample|HexGrid")
	static FVector SnapToGrid(const FHTileLayout &TileLayout, const FVector &Location);

	/**
	 * Round from floating-point cube coordinate to integer cube coordinate.
	 * @see https://www.redblobgames.com/grids/hexagons/#rounding
	*/
	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static FHCubeCoord HexRound(const FHFractional &F);

	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static void HexSingleRing(const FHCubeCoord &Center, const int32 Radius, TArray<FHCubeCoord> &OutRing);

	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static void HexLinedraw(const FHCubeCoord &A, const FHCubeCoord &B, TArray<FHCubeCoord> &OutLine);

	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin")
	static void HexRange(const FHCubeCoord &Center, const int32 Range, TArray<FHCubeCoord> &OutRange);

	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin", meta = (AutoCreateRefTerm = "CenterA,CenterB"))
	static void HexIntersectRanges(const FHCubeCoord &CenterA, const int32 RangeA, const FHCubeCoord &CenterB, const int32 RangeB, TArray<FHCubeCoord> &OutIntersection);

	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin", meta = (WorldContext = "WorldContextObject"))
	static bool HexGetTileAtCoord(const class AHexGridActor *Grid, const FHCubeCoord &Coord, FHTile &Tile);


	/**
	 * Create a new grid Hexagonal shaped grid
	 * @param Grid					AHexGridActor pointer.
	 * @param OnAddTileDelegate		This parameter is optional, if bound is executed at each step.
	 * @see https://www.redblobgames.com/grids/hexagons/implementation.html#map-shapes
	*/
	UFUNCTION(BlueprintCallable, Category = "Hex Grids Plugin", meta = (AutoCreateRefTerm = "OnAddTile"))
	static bool HexagonalGrid(class AHexGridActor *Grid, const FOnAddTileDelegate &OnAddTile);

	
};