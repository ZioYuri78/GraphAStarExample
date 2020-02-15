// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"
#include "HGPathFollowingComponentExp.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorBumpDelegateExp, const FVector &, BumpLocation);

/**
 * 
 */
UCLASS()
class GRAPHASTAREXAMPLE_API UHGPathFollowingComponentExp : public UPathFollowingComponent
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
	FOnActorBumpDelegateExp OnActorBumped;

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
	class AGraphAStarNavMesh *GraphAStarNavMesh;

protected:

	/** follow current path segment */
	virtual void FollowPathSegment(float DeltaTime) override;
};
