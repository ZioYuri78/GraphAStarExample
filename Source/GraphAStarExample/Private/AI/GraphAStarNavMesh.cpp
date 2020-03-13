// Fill out your copyright notice in the Description page of Project Settings.


#include "GraphAStarNavMesh.h"
#include "HexGrid/HexGrid.h"
#include "AIModule/Public/GraphAStar.h"

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
		return NavMeshRef.HexGrid->GridTiles[EndNodeRef].Cost;
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
	if (NavMeshRef.HexGrid->GridTiles.IsValidIndex(NodeB))
	{
		return !NavMeshRef.HexGrid->GridTiles[NodeB].bIsBlocking;
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
	
	if (Self == NULL)
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

			// Reset the PathPoints array
			Result.Path->GetPathPoints().Reset();

			// The pathfinder need a starting and ending point, so we create two temporary
			// cube coordinates from the Query start and ending location
			FHCubeCoord StartCCoord{ GraphAStarNavMesh->HexGrid->WorldToHex(Query.StartLocation) };
			FHCubeCoord EndCCoord{ GraphAStarNavMesh->HexGrid->WorldToHex(Query.EndLocation) };
			
			// and than we search in the HexGrid CubeCoordinates array for the index of items 
			// equals to our temp coordinates.
			const int32 StartIdx{ GraphAStarNavMesh->HexGrid->GridCoordinates.IndexOfByKey(StartCCoord) };
			const int32 EndIdx{ GraphAStarNavMesh->HexGrid->GridCoordinates.IndexOfByKey(EndCCoord)};

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
					Result.Path->GetPathPoints().Add(FNavPathPoint(Query.StartLocation));

					// Let's traverse the PathIndices array and build the FNavPathPoints we 
					// need to add to the Path.
					for (const int32 &PathIndex : PathIndices)
					{
						// Get a temporary Cube Coordinate from our HexGrid
						FHCubeCoord GridCoord{ GraphAStarNavMesh->HexGrid->GridCoordinates[PathIndex] };

						// Create a temporary FNavPathPoint
						FNavPathPoint PathPoint{};

						// Because we can create HexGrid with only Cube Coordinates and no tiles
						// we look if the current index we are using is a valid index for the GridTiles array
						if (GraphAStarNavMesh->HexGrid->GridTiles.IsValidIndex(PathIndex))
						{
							// If the index is valid (so we have a HexGrid with tiles) we compute the Location
							// of the PathPoint, we use the World Space coordinates of the current Cube Coordinate
							// as a base location and we add an offset to the Z.
							// How to compute the Z axis of the path is up to you, this is only an example!
							PathPoint.Location = GraphAStarNavMesh->HexGrid->HexToWorld(GridCoord) +
								FVector(0.f, 0.f, GraphAStarNavMesh->HexGrid->GridTiles[PathIndex].Cost +
								GraphAStarNavMesh->PathPointZOffset);
						}
						else
						{
							// If the current PathIndex isn't a valid index for the GridTiles array
							// (so we assume our HexGrid is only a "logical" grid with only cube coordinates and no tiles)
							// we simply transform the coordinates from cube space to world space and pass it to the PathPoint
							PathPoint.Location = GraphAStarNavMesh->HexGrid->HexToWorld(GridCoord);
						}

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


void AGraphAStarNavMesh::SetHexGrid(AHexGrid *HGrid)
{
	if (HGrid)
	{
		// If the pointer is valid we will use our implementation of the FindPath function
		HexGrid = HGrid;
		FindPathImplementation = FindPath;
	}
	else
	{
		// If the pointer is not valid we will fallback to the default RecastNavMesh implementation
		// of the FindPath function (the standard navigation behavior)
		// You can also use FindPathImplementation = ARecastNavMesh::FindPath;
		// but i start from the assumption that we are inheriting from ARecastNavMesh
		HexGrid = nullptr;
		FindPathImplementation = Super::FindPath;
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
	return HexGrid->GridCoordinates.IsValidIndex(NodeRef);
}

AGraphAStarNavMesh::FNodeRef
AGraphAStarNavMesh::GetNeighbour(const FNodeRef NodeRef, const int32 NeiIndex) const
{
	FHCubeCoord Neigh{ HexGrid->GetNeighbor(HexGrid->GridCoordinates[NodeRef], HexGrid->GetDirection(NeiIndex)) };
	return HexGrid->GridCoordinates.IndexOfByKey(Neigh);
}
//////////////////////////////////////////////////////////////////////////
