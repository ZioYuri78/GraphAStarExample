// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavMesh/RecastNavMesh.h"
#include "GraphAStarNavMesh.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGraphAStarExample_NavMesh, Log, All);

DECLARE_CYCLE_STAT(TEXT("Hex Grid A* Pathfinding"), STAT_Navigation_HGASPathfinding, STATGROUP_Navigation);

/**
 * TQueryFilter (FindPath's parameter) filter class is what decides which graph edges can be used and at what cost.
 */
struct FGridPathFilter
{
	FGridPathFilter(const AGraphAStarNavMesh &InNavMeshRef) : NavMeshRef(InNavMeshRef) {}

	/**
	 * Used as GetHeuristicCost's multiplier
	 */
	float GetHeuristicScale() const;

	/**
	 * Estimate of cost from StartNodeRef to EndNodeRef
	 */
	float GetHeuristicCost(const int32 StartNodeRef, const int32 EndNodeRef) const;

	/**
	 * Real cost of traveling from StartNodeRef directly to EndNodeRef
	 */
	float GetTraversalCost(const int32 StartNodeRef, const int32 EndNodeRef) const;

	/**
	 * Whether traversing given edge is allowed
	 */
	bool IsTraversalAllowed(const int32 NodeA, const int32 NodeB) const;

	/**
	 * Whether to accept solutions that do not reach the goal
	 */
	bool WantsPartialSolution() const;

protected:

	/**
	 * A reference to our NavMesh
	 */
	const AGraphAStarNavMesh &NavMeshRef;
};


// We inherit this struct because we need a custom GetCost/GetLength
struct FHexNavMeshPath : public FNavMeshPath
{
	FORCEINLINE
	virtual float GetCostFromIndex(int32 PathPointIndex) const override
	{
		return CurrentPathCost;
	}

	FORCEINLINE
	virtual float GetLengthFromPosition(FVector SegmentStart, uint32 NextPathPointIndex) const override
	{
		// We exclude the starting point so -1	
		return PathPoints.Num() - 1;
	}

	float CurrentPathCost{ 0 };
};

/**
 * 
 */
UCLASS()
class GRAPHASTAREXAMPLE_API AGraphAStarNavMesh : public ARecastNavMesh
{
	GENERATED_BODY()

public:

	/**
	 * the function is static for a reason, (wiki copy-paste->) 
	 * comments in the code explain it's for performance reasons: Epic are concerned 
	 * that if a lot of agents call the pathfinder in the same frame the virtual call overhead will accumulate and take too long, 
	 * so instead the function is declared static and stored in the FindPathImplementation function pointer.
     * Which means you need to manually set the function pointer in your new navigation class constructor 
	 * (or in some other function like we do here in SetHexGrid().
	 */
	static FPathFindingResult FindPath(const FNavAgentProperties &AgentProperties, const FPathFindingQuery &Query);
	
	/* Set a pointer to an hexagonal grid, it can be nullptr */
	UFUNCTION(BlueprintCallable, Category = "GraphAStarExample|NavMesh")
	void SetHexGrid(class AHexGrid *HGrid);

	//////////////////////////////////////////////////////////////////////////
	/**
	 * Generic graph A* implementation
	 * TGraph holds graph representation.Needs to implement functions :
	*/

	/* Type used as identification of nodes in the graph */
	typedef int32 FNodeRef;

	/* Returns number of neighbors that the graph node identified with NodeRef has */
	int32 GetNeighbourCount(FNodeRef NodeRef) const;

	/* Returns whether given node identification is correct */
	bool IsValidRef(FNodeRef NodeRef) const;

	/* Returns neighbor ref */
	FNodeRef GetNeighbour(const FNodeRef NodeRef, const int32 NeiIndex) const;
	//////////////////////////////////////////////////////////////////////////


	/* Just a pointer to an hexagonal grid actor */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GraphAStarExample|NavMesh")
	class AHexGrid *HexGrid;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GraphAStarExample|NavMesh")
	float PathPointZOffset{0.f};
};

