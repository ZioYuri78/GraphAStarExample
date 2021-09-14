// Fill out your copyright notice in the Description page of Project Settings.


#include "GraphAStarNavMesh.h"
//#include "HexGrid/HexGrid.h"
#include "AIModule/Public/GraphAStar.h"
#include "DrawDebugHelpers.h"
#include "HexGridLibPlugin/Public/HexGridLibPluginBPLibrary.h"

typedef UHexGridLibPluginBPLibrary HGLP;

DEFINE_LOG_CATEGORY(LogGraphAStarExample_NavMesh)


//==== FGridPathFilter functions implementation ===
// In these functions we do not check if the HexGrid is valid because it must be!
// Remember, if the HexGrid is a nullptr we will never use this code
// but we fallback to the RecastNavMesh implementation of it.

float FGridPathFilter::GetHeuristicScale() const
{
	// For the sake of simplicity we just return 1.f
	return 1.0f;
}

float FGridPathFilter::GetHeuristicCost(const int32 StartNodeRef, const int32 EndNodeRef) const
{
	return GetTraversalCost(StartNodeRef, EndNodeRef);
}

float FGridPathFilter::GetTraversalCost(const int32 StartNodeRef, const int32 EndNodeRef) const
{
	// If EndNodeRef is a valid index of the GridTiles array we return the tile cost, 
	// if not we return 1 because the traversal cost need to be > 0 or the FGraphAStar will stop the execution
	// look at GraphAStar.h line 244: ensure(NewTraversalCost > 0);
	if (NavMeshRef.HexGrid->GridTiles.IsValidIndex(EndNodeRef))
	{
		return NavMeshRef.HexGrid->GridTiles[EndNodeRef].TileCost;
	}
	else
	{
		return 1.f;
	}
}

bool FGridPathFilter::IsTraversalAllowed(const int32 NodeA, const int32 NodeB) const
{
	// If NodeB is a valid index of the GridTiles array we return bIsBlocking, 
	// if not we assume we can traverse so we return true.
	// Here you can make a more complex operation like use a line trace to see
	// there is some obstacles (like an enemy), in our example we just use a simple implementation
	if (NavMeshRef.HexGrid->GridTiles.IsValidIndex(NodeA) && NavMeshRef.HexGrid->GridTiles.IsValidIndex(NodeB))
	{
		FVector NodeALocation{ NavMeshRef.HexGrid->GridTiles[NodeA].WorldCoordinates + FVector(0.f, 0.f, 100.f) };
		FVector NodeBLocation{ NavMeshRef.HexGrid->GridTiles[NodeB].WorldCoordinates + FVector(0.f, 0.f, 100.f) };
		FVector AB{ NodeBLocation - NodeALocation };
		FVector Dir;
		float Length;
		AB.ToDirectionAndLength(Dir, Length);

		bool bBlockingTile{ NavMeshRef.HexGrid->GridTiles[NodeB].bIsBlocking };
		bool bIsHit{};

		if (NavMeshRef.Avoidance)
		{
			FHitResult OutHit;
			bIsHit = NavMeshRef.GetWorld()->LineTraceSingleByChannel(OutHit, (NodeALocation + Dir * 50.f) + FVector(0.f, 0.f, 25.f), 
																	 NodeBLocation + FVector(0.f, 0.f, 25.f), ECC_Pawn);
		}
		
#if WITH_EDITOR
		// ----------- DEBUG ------------
		if (NavMeshRef.bDrawDebug)
		{
			if (bBlockingTile)
			{
				DrawDebugDirectionalArrow(NavMeshRef.GetWorld(), NodeALocation + (Dir * 50.f),
										  NodeBLocation - (Dir * 50.f), 25.f, FColor::Red, !NavMeshRef.bIsTemporary, NavMeshRef.DrawDebugLifetime);
			}
			else
			{
				DrawDebugDirectionalArrow(NavMeshRef.GetWorld(), NodeALocation + (Dir * 50.f),
										  NodeBLocation - (Dir * 50.f), 25.f, FColor::Green, !NavMeshRef.bIsTemporary, NavMeshRef.DrawDebugLifetime);
			}

			if (bIsHit)
			{
				DrawDebugDirectionalArrow(NavMeshRef.GetWorld(), NodeALocation + (Dir * 50.f)+ FVector(0.f, 0.f, 25.f),
										  NodeBLocation - (Dir * 50.f) + FVector(0.f, 0.f, 25.f), 25.f, FColor::Magenta, !NavMeshRef.bIsTemporary);
			}
		}
		// ------------------------------
#endif

		return !(bBlockingTile || bIsHit);
	}
	else
	{
		return true;
	}
	
}

bool FGridPathFilter::WantsPartialSolution() const
{
	// Just return true
	return true;
}

//==== END OF FGridPathFilter functions implementation ====


AGraphAStarNavMesh::AGraphAStarNavMesh()
{
	// Need it for EQS Test Pawn, you also need to manually set the HexGrid
	// in the GraphAStarRecast details panel.
#if WITH_EDITOR
	TestPathImplementation = TestPath;	
	FindPathImplementation = FindPath;
#endif
}


FPathFindingResult AGraphAStarNavMesh::FindPath(const FNavAgentProperties &AgentProperties, const FPathFindingQuery &Query)
{
	// =================================================================================================
	// The first part is the same of RecastNavMesh::FindPath implementation, the only difference is
	// the cast of ANavigationData to our class.

	SCOPE_CYCLE_COUNTER(STAT_Navigation_HGASPathfinding);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Pathfinding);

	// Because we are in a static function we don't have a "this" pointer and we can't access to class member variables like HexGrid
	// but luckily the FPathFindingQuery contain a pointer to the ANavigationData object.
	const ANavigationData *Self = Query.NavData.Get();
	check(Cast<const AGraphAStarNavMesh>(Self));

	// Now we can cast to our class, this will allow us to access the member variables (and functions).
	// NOTE: remember, our AGraphAStarNavMesh inherit from ARecastNavMesh that inherit from ANavigationData so we can do the cast.
	const AGraphAStarNavMesh *GraphAStarNavMesh{ Cast<const AGraphAStarNavMesh>(Self) };
	
	// This RecastNavMeshImpl check is probably useless here, we don't care because we don't
	// have if for our class and the parent class version isn't used here because we are in the
	// A* implementation.
	if (Self == NULL/* || GraphAStarNavMesh->GetRecastNavMeshImpl() == NULL*/)
	{
		return ENavigationQueryResult::Error;
	}

	// This struct contains the result of our search and the Path that the AI will follow
	FPathFindingResult Result(ENavigationQueryResult::Error);

	
	FNavigationPath *NavPath = Query.PathInstanceToFill.Get();
	FHexNavMeshPath *NavMeshPath = NavPath ? NavPath->CastPath<FHexNavMeshPath>() : nullptr;

	if (NavMeshPath)
	{
		Result.Path = Query.PathInstanceToFill;
		NavMeshPath->ResetForRepath();
	}
	else
	{
		Result.Path = Self->CreatePathInstance<FHexNavMeshPath>(Query);
		NavPath = Result.Path.Get();
		NavMeshPath = NavPath ? NavPath->CastPath<FHexNavMeshPath>() : nullptr;
	}

	const FNavigationQueryFilter *NavFilter = Query.QueryFilter.Get();
	if (NavMeshPath && NavFilter)
	{
		NavMeshPath->ApplyFlags(Query.NavDataFlags);

		const FVector AdjustedEndLocation = NavFilter->GetAdjustedEndLocation(Query.EndLocation);
		if ((Query.StartLocation - AdjustedEndLocation).IsNearlyZero() == true)
		{
			Result.Path->GetPathPoints().Reset();
			Result.Path->GetPathPoints().Add(FNavPathPoint(AdjustedEndLocation));
			Result.Result = ENavigationQueryResult::Success;
		}
		// ============ END OF SAME CODE AS RECASTNAVMESH ========================================================

		else
		{
			// ====================== BEGIN OF OUR CODE ===========================================================
#if WITH_EDITOR
			if (GraphAStarNavMesh->bDrawDebug)
			{
				FlushPersistentDebugLines(GraphAStarNavMesh->GetWorld());
			}
#endif

			// Reset the PathPoints array
			Result.Path->GetPathPoints().Reset();


			const FHTileLayout &TempLayout{ GraphAStarNavMesh->HexGrid->TileLayout };
			const AHexGridActor &TempGrid{ *GraphAStarNavMesh->HexGrid };

			// The pathfinder need a starting and ending point, so we create two temporary
			// cube coordinates from the Query start and ending location			
			FHCubeCoord StartCCoord{ HGLP::WorldToHex(TempLayout, Query.StartLocation) };
			FHCubeCoord EndCCoord{ HGLP::WorldToHex(TempLayout, Query.EndLocation) };
			
			// and than we search in the HexGrid CubeCoordinates array for the index of items 
			// equals to our temp coordinates.
			const int32 StartIdx{ TempGrid.GridTiles.IndexOfByPredicate([&](const FHTile &Tile) {return Tile.GridCoordinates == StartCCoord; }) };
			const int32 EndIdx{ TempGrid.GridTiles.IndexOfByPredicate([&](const FHTile &Tile) {return Tile.GridCoordinates == EndCCoord; }) };
			

			// We need the index because the FGraphAStar work with indexes!

			// Here we will store the path generated from the pathfinder
			TArray<int32> PathIndices;
			
			// Initialization of the pathfinder, as you can see we pass our GraphAStarNavMesh as parameter,
			// so internally it can use the functions we implemented.
			FGraphAStar<AGraphAStarNavMesh> Pathfinder(*GraphAStarNavMesh);

			// and run the A* algorithm, the FGraphAStar::FindPath function want a starting index, an ending index,
			// the FGridPathFilter which want our GraphAStarNavMesh as parameter and a reference to the array where
			// all the indices of our path will be stored
			EGraphAStarResult AStarResult{ Pathfinder.FindPath(StartIdx, EndIdx, FGridPathFilter(*GraphAStarNavMesh), PathIndices) };

			// The FGraphAStar::FindPath return a EGraphAStarResult enum, we need to assign the right
			// value to the FPathFindingResult (that is returned by AGraphAStarNavMesh::FindPath) based on this.
			// In the first three cases are simple and process the three "bad" results
			switch (AStarResult)
			{
				case GoalUnreachable:
					Result.Result = ENavigationQueryResult::Invalid;
					break;

				case InfiniteLoop:
					Result.Result = ENavigationQueryResult::Error;
					break;

				case SearchFail:
					Result.Result = ENavigationQueryResult::Fail;
					break;

				// The search was successful so let's process the computed path, remember, right now
				// we only have an array of indexes so we have to compute the path locations
				// from these indexes and pass them to the PathPoints array of the Path
				// that the AI will follow.
				case SearchSuccess:					

					// Search succeeded
					Result.Result = ENavigationQueryResult::Success;
					
					// PathIndices array computed by FGraphAStar will not contain the starting point, so
					// we need to add it manually the the Path::PathPoints array
					// UPDATE: this is a very lazy workaround to avoid the duplicated first node bug that
					// happen when we have a path longer than 7 tiles and also it let us move of only one tile.					
					if(PathIndices.Num() == 1) {
						Result.Path->GetPathPoints().Add(FNavPathPoint(Query.StartLocation));
					}

					// Let's traverse the PathIndices array and build the FNavPathPoints we 
					// need to add to the Path.
					for (const int32 &PathIndex : PathIndices)
					{						

						// Get a temporary Cube Coordinate from our HexGrid
						FHCubeCoord GridCoord = TempGrid.GridTiles[PathIndex].GridCoordinates;

						// Create a temporary FNavPathPoint
						FNavPathPoint PathPoint{};

						// Because we can create HexGrid with only Cube Coordinates and no tiles
						// we look if the current index we are using is a valid index for the GridTiles array
						//if (GraphAStarNavMesh->HexGrid->GridTiles.IsValidIndex(PathIndex))
						//{
							// If the index is valid (so we have a HexGrid with tiles) we compute the Location
							// of the PathPoint, we use the World Space coordinates of the current Cube Coordinate
							// as a base location and we add an offset to the Z axis based on the corresponding
							// Tile cost multiplied by a factor of 10. (NO MORE BECAUSE BROKEN THE PATH ON TILES WITH HIGH COST)
							// How to compute the Z axis of the path is up to you, this is only an example!							
							PathPoint.Location = HGLP::HexToWorld(TempLayout, GridCoord) +
								FVector(0.f, 0.f, TempGrid.GridTiles[PathIndex].TileCost +
										GraphAStarNavMesh->PathPointZOffset);

							// PathCost accumulator
							NavMeshPath->CurrentPathCost += TempGrid.GridTiles[PathIndex].TileCost;
						//}
						//else
						//{
							// If the current PathIndex isn't a valid index for the GridTiles array
							// (so we assume our HexGrid is only a "logical" grid with only cube coordinates and no tiles)
							// we simply transform the coordinates from cube space to world space and pass it to the PathPoint
							//PathPoint.Location = GraphAStarNavMesh->HexGrid->HexToWorld(GridCoord);

							// PathCost accumulator
							//NavMeshPath->CurrentPathCost++;
						//}

						// We finally add the computed PathPoint to the Path::PathPoints array
						Result.Path->GetPathPoints().Add(FNavPathPoint(PathPoint));
					}

					// We finished to create the Path so mark it as Ready.
					Result.Path->MarkReady();
					break;
			}
			// =========================== END OF OUR CODE ============================================================
		}
	}

	return Result;
}


bool AGraphAStarNavMesh::TestPath(const FNavAgentProperties &AgentProperties, const FPathFindingQuery &Query, int32 *NumVisitedNodes)
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_HGASTestPath);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Pathfinding);

	const ANavigationData *Self = Query.NavData.Get();
	check(Cast<const AGraphAStarNavMesh>(Self));

	const AGraphAStarNavMesh *GraphAStarNavMesh = (const AGraphAStarNavMesh *)Self;
	if (Self == NULL)
	{
		return false;
	}
	
	if (!GraphAStarNavMesh->HexGrid)
	{
		//UE_LOG(LogGraphAStarExample_NavMesh, Warning, TEXT("HexGrid is nullptr, we can't run TestPath"));
		return false;
	}

	const FHTileLayout &TempLayout{ GraphAStarNavMesh->HexGrid->TileLayout };
	const AHexGridActor &TempGrid{ *GraphAStarNavMesh->HexGrid };

	bool bPathExists = true;

	const FNavigationQueryFilter *NavFilter = Query.QueryFilter.Get();
	if (NavFilter)
	{
		const FVector AdjustedEndLocation = NavFilter->GetAdjustedEndLocation(Query.EndLocation);
		if ((Query.StartLocation - AdjustedEndLocation).IsNearlyZero() == false)
		{
			FHCubeCoord StartCCoord{ HGLP::WorldToHex(TempLayout, Query.StartLocation) };
			FHCubeCoord EndCCoord{ HGLP::WorldToHex(TempLayout, Query.EndLocation) };

			const int32 StartIdx{ TempGrid.GridTiles.IndexOfByPredicate([&](const FHTile &Tile) {return Tile.GridCoordinates == StartCCoord; }) };
			const int32 EndIdx{ TempGrid.GridTiles.IndexOfByPredicate([&](const FHTile &Tile) {return Tile.GridCoordinates == EndCCoord; }) };
			
			TArray<int32> PathIndices;			
			FGraphAStar<AGraphAStarNavMesh> Pathfinder(*GraphAStarNavMesh);			
			EGraphAStarResult AStarResult{ Pathfinder.FindPath(StartIdx, EndIdx, FGridPathFilter(*GraphAStarNavMesh), PathIndices) };
			switch (AStarResult)
			{
				case SearchSuccess:
					bPathExists = true;
					break;

				case GoalUnreachable:
				case InfiniteLoop:
				case SearchFail:
					bPathExists = false;
					break;
			}			
		}
	}

	return bPathExists;
}


void AGraphAStarNavMesh::SetHexGrid(const AHexGridActor *HGrid)
{
	if (HGrid)
	{
		// If the pointer is valid we will use our implementation of the FindPath function
		HexGrid = HGrid;
		FindPathImplementation = FindPath;
		TestPathImplementation = TestPath;
	}
	else
	{
		// If the pointer is not valid we will fallback to the default RecastNavMesh implementation
		// of the FindPath function (the standard navigation behavior)
		// You can also use FindPathImplementation = ARecastNavMesh::FindPath;
		// but i start from the assumption that we are inheriting from ARecastNavMesh
		HexGrid = nullptr;
		FindPathImplementation = Super::FindPath;
		TestPathImplementation = Super::TestPath;
	}
}


//////////////////////////////////////////////////////////////////////////
// FGraphAStar: TGraph
// Functions implementation for our FGraphAStar struct
int32 AGraphAStarNavMesh::GetNeighbourCount(FNodeRef NodeRef) const
{
	return 6;
}

bool AGraphAStarNavMesh::IsValidRef(FNodeRef NodeRef) const
{
	return HexGrid->GridTiles.IsValidIndex(NodeRef);
}

AGraphAStarNavMesh::FNodeRef
AGraphAStarNavMesh::GetNeighbour(const FNodeRef NodeRef, const int32 NeiIndex) const
{
	FHCubeCoord Neigh{ HGLP::GetNeighbor(HexGrid->GridTiles[NodeRef].GridCoordinates, HGLP::GetDirection(NeiIndex)) };

#if WITH_EDITOR
	if (bDrawDebug)
	{
		FHTileLayout TempLayout{ HexGrid->TileLayout };
		FVector NodeRefLocation{ HGLP::HexToWorld(TempLayout, HexGrid->GridTiles[NodeRef].GridCoordinates) + FVector(0.f, 1.f, 75.f) };
		FVector NeighLocation{ HGLP::HexToWorld(TempLayout, Neigh) + FVector(0.f, 0.f, 75.f) };
		FVector AB{ NeighLocation - NodeRefLocation };
		FVector Dir;
		float Length;
		AB.ToDirectionAndLength(Dir, Length);
		DrawDebugDirectionalArrow(GetWorld(), NodeRefLocation + Dir * 25.f, NeighLocation - Dir * 25.f, 25.f, FColor::White, !bIsTemporary, DrawDebugLifetime);
	}
#endif
	
	return HexGrid->GridTiles.IndexOfByPredicate([&](const FHTile &Tile) {return Tile.GridCoordinates == Neigh; });
}
//////////////////////////////////////////////////////////////////////////
