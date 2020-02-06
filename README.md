# Hexagonal grid pathfinding in Unreal Engine 4

## Prerequisites
* [A good knowledge of C++.](https://www.learncpp.com/)
* Red Blob Games articles about hexagonal grid.
  * [Hexagonal grid reference.](https://www.redblobgames.com/grids/hexagons/)
  * [Hexagonal grid implementation guide.](https://www.redblobgames.com/grids/hexagons/implementation.html)
  
## These readings will let you better understand my example project
* [Epic wiki article about replacing the pathfinder.](https://wiki.unrealengine.com/Replacing_The_Pathfinder)
* [Neatly replacing NavMesh with A* in UE4 by Chris Russel.](https://crussel.net/2016/06/05/neatly-replacing-navmesh-with-a-in-ue4/)
* [Epic wiki article about custom Path Following Component.](https://wiki.unrealengine.com/AI_Navigation_in_C%2B%2B,_Customize_Path_Following_Every_Tick)

## The GraphAStarExample project
Welcome all to my example project about how to use the Unreal Engine 4 generic graph A* implementation with hexagonal grids, the intent of this project is to guide you on what you need to setup the basics for navigation on hexagonal grids, this is not a complete tutorial but more like a guideline and doesn't cover topics like avoidance. 

### Classes and Structs you need to know
Let me start with a list of the classes you have to know and explore in the engine
1. ANavigationData
2. ARecastNavMesh
3. FGraphAStar
3. AAIController (optional)
4. UPathFollowingComponent (optional)

#### ANavigationData
Represents abstract Navigation Data (sub-classed as NavMesh, NavGraph, etc).
Used as a common interface for all navigation types handled by NavigationSystem.

Here you will find a lot of interesting stuff but the most important for us is the FindPathImplementation class member, this is a function pointer defined as FFindPathPtr (typedef).
```
typedef FPathFindingResult (*FFindPathPtr)(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);
FFindPathPtr FindPathImplementation;
```
So the ANavigationData::FindPath function call this function pointer that will be in charge of run the operations.
```
/** 
 *	Synchronously looks for a path from @StartLocation to @EndLocation for agent with properties @AgentProperties. 
 *  NavMesh actor appropriate for specified FNavAgentProperties will be found automatically
 *	@param ResultPath results are put here
 *	@return true if path has been found, false otherwise
 *
 *	@note don't make this function virtual! Look at implementation details and its comments for more info.
 */
FORCEINLINE FPathFindingResult FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query) const
{
	  check(FindPathImplementation);
		// this awkward implementation avoids virtual call overhead - it's possible this function will be called a lot
		return (*FindPathImplementation)(AgentProperties, Query);
}
```
Take a look at the note "don't make this function virtual!", we will come back on it in a while.

#### ARecastNavMesh
This class inherit from ANavigationData and extend his functionality, everytime you place a NavMeshBoundsVolume in the map an object of this class is created in the map, yes, is the RecastNavMesh-Default object!

This is the class we have to inherit from!

Let's look how this class implement the ANavigationData::FindPath function, we already know that this function is not virtual, so how we can implement it? We have the FindPathImplementation pointer!

In the header you can see the ARecastNavMesh::FindPath declaration.

` static FPathFindingResult FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);`

the function is static for a reason, (wiki copy-paste->)* *comments in the code explain it’s for performance reasons: Epic are concerned that if a lot of agents call the pathfinder in the same frame the virtual call overhead will accumulate and take too long, so instead the function is declared static and stored in the FindPathImplementation function pointer. 
Which means you need to manually set the function pointer in your new navigation class constructor* * (or in some other function like i did in my example). 

This is the function we will implement and pass to the FindPathImplementation pointer in our inherited class!

#### FGraphAStar
Finally we are in the core class (ok ok, is a struct) of our example, the FGraphAstar is the Unreal Engine 4 generic implementation of the A* algorithm.

If you open the GraphAStar.h file in UE4 you will find in the comments an explanation on how to use it, let's look:
> Generic graph A* implementation

>TGraph holds graph representation. Needs to implement functions:

`int32 GetNeighbourCount(FNodeRef NodeRef) const;`
> - returns number of neighbours that the graph node identified with NodeRef has	

`bool IsValidRef(FNodeRef NodeRef) const;`
> - returns whether given node identyfication is correct

`FNodeRef GetNeighbour(const FNodeRef NodeRef, const int32 NeighbourIndex) const;`
> - returns neighbour ref

> it also needs to specify node type
`FNodeRef		- type used as identification of nodes in the graph`

> TQueryFilter (FindPath's parameter) filter class is what decides which graph edges can be used and at what cost. It needs to implement following functions:

`float GetHeuristicScale() const;`
> - used as GetHeuristicCost's multiplier

`float GetHeuristicCost(const FNodeRef StartNodeRef, const FNodeRef EndNodeRef) const;`
> - estimate of cost from StartNodeRef to EndNodeRef

`float GetTraversalCost(const FNodeRef StartNodeRef, const FNodeRef EndNodeRef) const;`
> - real cost of traveling from StartNodeRef directly to EndNodeRef

`bool IsTraversalAllowed(const FNodeRef NodeA, const FNodeRef NodeB) const;`
> - whether traversing given edge is allowed

`bool WantsPartialSolution() const;`
> - whether to accept solutions that do not reach the goal

So we don't have to create a class (ok ok, is a struct) from FGrapAStar but we have to implement the above functions, typedefs and structs in the class that will be in charge of run the pathfinder.

In the engine code there is a good example on how to do it, go and look the NavLocalGridData.h file.
In our example we will implement the code requested by FGraphAStar in our ARecastNavMesh inherited class, we will talk about it in a moment.

For now this is all for the foundamental class we need, at the end the only class we really will use is the ARecastNavMesh class.

#### AAIController (optional)
In our example we will go further a little bit (in the Example B) and we will use a custom PathFollowingComponent but to use it we have to 
