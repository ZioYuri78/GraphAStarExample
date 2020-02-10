// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"
#include "GraphAstarNavMesh.h"
#include "HGPathFollowingComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorBumpDelegate, const FVector&, BumpLocation);

/**
 * We inherit from the UPathFollowingComponent and we override a bunch of functions
 * just to let you know this exist and it can been very powerful.
 */
UCLASS()
class GRAPHASTAREXAMPLE_API UHGPathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()
	
	

	/** called when moving agent collides with another actor */
	virtual void OnActorBump(AActor *SelfActor, AActor *OtherActor, FVector NormalImpulse, const FHitResult &Hit) override;

public:

	virtual void BeginPlay() override;

	/**
	 * Executed if a "Bump" happen, we bind this delegate on the activation of our Behavior Tree Service BTS_BindBump
	 */
	UPROPERTY(BlueprintAssignable, Category = "GraphAStarExample|PathFollowingComponent")
	FOnActorBumpDelegate OnActorBumped;

	/**
	 * Toggle our debug drawing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GraphAStarExample|PathFollowingComponent")
	bool DrawDebug{};

	/**
	 * The PathFollowingComponent has a pointer to the ANavigationData class but it isn't expose to Blueprint,
	 * so we will expose it casted to our AGraphAStarNavMesh in the constructor.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GraphAStarExample|PathFollowingComponent")
	AGraphAStarNavMesh *GraphAStarNavMesh;

protected:

	/** follow current path segment */
	virtual void FollowPathSegment(float DeltaTime) override;
};
