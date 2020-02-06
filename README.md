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
Welcome all to my example project about how to use the Unreal Engine 4 generic graph A* implementation with hexagonal grids, the intent of this project is to guide you on what you need to setup the basics for navigation on hexagonal grids, this is not a complete tutorial and doesn't cover topics like avoidance on hexagonal grid movement but is more like a guideline.

### Classes and Structs you need to know
Let me start with a list of the classes you have to know and explore in the engine
1. ANavigationData
2. ARecastNavMesh
3. FGraphAStar
3. AAIController
4. UPathFollowingComponent

#### ANavigationData
Represents abstract Navigation Data (sub-classed as NavMesh, NavGraph, etc).
Used as a common interface for all navigation types handled by NavigationSystem.

Here you will find a lot of interesting stuff but the most important for us is the FindPathImplementation class member, this is a function pointer defined as FFindPathPtr (typedef).
```
typedef FPathFindingResult (*FFindPathPtr)(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);
FFindPathPtr FindPathImplementation;
```
So the ANavigationData::FindPath function return this function pointer that will be in charge of run the operations.
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

the function is static for a reason, (wiki copy-paste->)comments in the code explain it’s for performance reasons: Epic are concerned that if a lot of agents call the pathfinder in the same frame the virtual call overhead will accumulate and take too long, so instead the function is declared static and stored in the FindPathImplementation function pointer. 
Which means you need to manually set the function pointer in your new navigation class constructor (or in some other function like i did in my example). 
  

