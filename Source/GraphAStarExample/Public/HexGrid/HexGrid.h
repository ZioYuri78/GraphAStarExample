// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HGTypes.h"
#include "HexGrid.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGraphAStarExample_HexGrid, Log, All);
DECLARE_STATS_GROUP(TEXT("HEXGRID_STATS"), STATGROUP_HEXGRID, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("CreateGrid(..)"), STAT_CreateGrid, STATGROUP_HEXGRID);

/*
	Just a rough implementation of a tile.
*/
USTRUCT(BlueprintType)
struct FHexTile
{
	GENERATED_USTRUCT_BODY()

	FHexTile() {};

	/* Coordinate in Cube space */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GraphAStarExample|HexGrid")
	FHCubeCoord CubeCoord;

	/* Coordinate in World space */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GraphAStarExample|HexGrid")
	FVector WorldPosition {
		FVector::ZeroVector
	};

	/* Cost of the tile, for a well execution of the GraphAStar pathfinder it need to have a value of at least 1 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GraphAStarExample|HexGrid", meta =(ClampMin = 1))
	float Cost{};

	/* Is this tile a blocking tile? For example a static obstacle. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GraphAStarExample|HexGrid")
	bool bIsBlocking{};


	friend bool operator==(const FHexTile &A, const FHexTile &B)
	{
		return (A.CubeCoord == B.CubeCoord) && (A.Cost == B.Cost) && (A.bIsBlocking == B.bIsBlocking);
	}

	friend bool operator!=(const FHexTile &A, const FHexTile &B)
	{
		return !(A == B);
	}

};


/* Delegate used in the CreateGrid function, executed if bound on each inner loop step. */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FCreationStepDelegate, const FHTileLayout &, TileLayout, const FHCubeCoord &, Coord);


/**
 * This is a very simple, rough and unoptimized implementation of an hexagonal grid,
 * i heavily suggest to read the awesome Red Blob Games article before look at it.
 * @see https://www.redblobgames.com/grids/hexagons/
 */
UCLASS()
class GRAPHASTAREXAMPLE_API AHexGrid : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHexGrid();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/**
	 * Create a new grid and fill the CubeCoordinates array.
	 * @param TLayout				Tile layout structure.
	 * @param GridRadius			Radius of the grid in tiles.
	 * @param CreationStepDelegate	This parameter is optional, if bound is executed at each step.
	 * @see https://www.redblobgames.com/grids/hexagons/implementation.html#map-shapes
	 */	
	UFUNCTION(BlueprintCallable, Category = "GraphAStarExample|HexGrid", meta = (AutoCreateRefTerm = "CreationStepDelegate"))
	void CreateGrid(const FHTileLayout &TLayout, const int32 GridRadius, const FCreationStepDelegate &CreationStepDelegate);

	/**
	 * Convert coordinates from Cube space to World space.
	 * @see https://www.redblobgames.com/grids/hexagons/#hex-to-pixel
	 */ 
	UFUNCTION(BlueprintCallable, Category = "GraphAStarExample|HexGrid")
	FVector HexToWorld(const FHCubeCoord &H);

	/**
	 * Convert coordinates from World space to Cube space.
	 * @see https://www.redblobgames.com/grids/hexagons/#pixel-to-hex
	 */ 
	UFUNCTION(BlueprintCallable, Category = "GraphAStarExample|HexGrid")
	FHCubeCoord WorldToHex(const FVector &Location);

	/** Snap a World coordinate to the Grid space. */
	UFUNCTION(BlueprintCallable, Category = "GraphAStarExample|HexGrid")
	FVector SnapToGrid(const FVector &Location);

	/** 
	 * Round from floating-point cube coordinate to integer cube coordinate. 
	 * @see https://www.redblobgames.com/grids/hexagons/#rounding
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphAStarExample|HexGrid")
	FHCubeCoord HexRound(const FHFractional &F);

	/**
	 * Compare two Cube coordinate. 
	 * @see https://www.redblobgames.com/grids/hexagons/implementation.html#hex-equality
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphAStarExample|HexGrid")
	bool HexEqual(const FHCubeCoord &A, const FHCubeCoord &B);

	/**
	 * Return one of the six cube directions.
	 * @see https://www.redblobgames.com/grids/hexagons/#neighbors
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphAStarExample|HexGrid")
	FHCubeCoord GetDirection(int32 Dir);

	/**
	 * Return the neighbor Cube coordinate in the provided direction. 
	 * @see https://www.redblobgames.com/grids/hexagons/#neighbors
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphAStarExample|HexGrid")
	FHCubeCoord GetNeighbor(const FHCubeCoord &H, const FHCubeCoord &Dir);

	/** Array of HexTiles, in our example we fill it in blueprint with the CreationStepDelegate. */
	UPROPERTY(BlueprintReadWrite, Category = "GraphAStarExample|HexGrid")
	TArray<FHexTile> GridTiles;

	/** Array of Cube coordinates that compose the grid. */
	UPROPERTY(BlueprintReadWrite, Category = "GraphAStarExample|HexGrid")
	TArray<FHCubeCoord> GridCoordinates{};

	/**
	 * Layout of the tile (i know is very misleading, please read the article)
	 * @see  https://www.redblobgames.com/grids/hexagons/implementation.html#layout
	 */ 
	UPROPERTY(BlueprintReadWrite, Category = "GraphAStarExample|HexGrid")
	FHTileLayout TileLayout {};

	/**
	 * Radius of the grid in "tiles", clamped [1, 25]
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphAStarExample|HexGrid")
	int32 Radius {};

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


private:

	FHDirections HDirections{};
};




