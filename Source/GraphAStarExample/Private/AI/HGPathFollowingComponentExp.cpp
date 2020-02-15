// Fill out your copyright notice in the Description page of Project Settings.


#include "HGPathFollowingComponentExp.h"
#include "GraphAstarNavMesh.h"
#include "HGAIControllerExp.h"
#include "GameFramework/Character.h"

void UHGPathFollowingComponentExp::BeginPlay()
{
	Super::BeginPlay();

	// We cast the MyNavData parent class member so we can expose it to Blueprint.
	GraphAStarNavMesh = Cast<AGraphAStarNavMesh>(MyNavData);
}


void UHGPathFollowingComponentExp::OnActorBump(AActor *SelfActor, AActor *OtherActor, FVector NormalImpulse, const FHitResult &Hit)
{
	Super::OnActorBump(SelfActor, OtherActor, NormalImpulse, Hit);

	// Let's see if we are moving or waiting and if the other actor is of the same class.
	if ((SelfActor->GetClass() == OtherActor->GetClass()) && (GetStatus() != EPathFollowingStatus::Idle))
	{
		// Just broadcast the event.
		OnActorBumped.Broadcast(OtherActor->GetActorLocation());
	}

}


void UHGPathFollowingComponentExp::FollowPathSegment(float DeltaTime)
{
	Super::FollowPathSegment(DeltaTime);

#if 0
	if (Path)
	{
		FNavPathPoint CurrentNavPoint{};
		FNavigationPath::GetPathPoint(&Path->AsShared().Get(), GetCurrentPathIndex(), CurrentNavPoint);

		FNavPathPoint NextNavPoint{};
		FNavigationPath::GetPathPoint(&Path->AsShared().Get(), GetNextPathIndex(), NextNavPoint);

		float CurrentZ{ CurrentNavPoint.Location.Z };
		float NextZ{ NextNavPoint.Location.Z };
		float HeightDifference{ NextZ - CurrentZ };
		if (NextZ > CurrentZ && HeightDifference > 50.f)
		{
			AHGAIControllerExp *HGAIController{ Cast<AHGAIControllerExp>(GetOwner()) };
			if (HGAIController)
			{
				ACharacter *Player{ Cast<ACharacter>(HGAIController->GetPawn()) };
				Player->Jump();
			}
		}

	}
#endif
}