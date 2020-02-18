// Fill out your copyright notice in the Description page of Project Settings.


#include "HGPathFollowingComponentExp.h"
#include "GraphAstarNavMesh.h"
#include "HGAIControllerExp.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	if (Path && DrawDebug)
	{
		Path->DebugDraw(MyNavData, FColor::White, nullptr, false);
		DrawDebugSphere(GetWorld(), GetCurrentNavLocation(), 25.f, 16, FColor::Red);
	}

#if 1
	if (Path)
	{
		FNavPathPoint NextPathPoint{};
		FNavigationPath::GetPathPoint(&Path->AsShared().Get(), GetNextPathIndex(), NextPathPoint);
		FNavPathPoint CurrentPathPoint{};
		FNavigationPath::GetPathPoint(&Path->AsShared().Get(), GetCurrentPathIndex(), CurrentPathPoint);

		FVector CPPtoNPP{ NextPathPoint.Location - CurrentPathPoint.Location };
		float CPPtoNPPdist2D{ FVector::Dist2D(CurrentPathPoint.Location, NextPathPoint.Location) };

		FVector ANLtoNPP{ NextPathPoint.Location - MovementComp->GetActorNavLocation() };
		float ANLtoNPPdist2D{ FVector::Dist2D(MovementComp->GetActorNavLocation(), NextPathPoint.Location) };

		float CPPtoNPPdZ{ NextPathPoint.Location.Z - CurrentPathPoint.Location.Z };

		float Dot = FVector::DotProduct(CPPtoNPP.GetSafeNormal(), MovementComp->GetOwner()->GetActorForwardVector());

		ACharacter *Character{ Cast<ACharacter>(MovementComp->GetOwner()) };
		UCharacterMovementComponent *CharMove{ Cast<UCharacterMovementComponent>(MovementComp) };

		GEngine->AddOnScreenDebugMessage(INDEX_NONE, DeltaTime, FColor::Red, *FString::SanitizeFloat(ANLtoNPPdist2D));
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, DeltaTime, FColor::Red, *FString::SanitizeFloat(Dot));
		if (CPPtoNPPdZ >= 20.f && CPPtoNPPdZ < 70.f)
		{
			if (/*FMath::IsWithinInclusive(ANLtoNPPdist2D, 70.f, 170.f)*/ANLtoNPPdist2D > 120.f && Dot > 0.9f)
			{
				if (Character && CharMove && !MovementComp->IsFalling())
				{
					if (CharMove->JumpZVelocity > 500.f)
					{
						CharMove->JumpZVelocity = 500.f;
					}
									
					Character->Jump();
				}
			}
		}
		else if (CPPtoNPPdZ >= 70.f)
		{
			if (/*FMath::IsWithinInclusive(ANLtoNPPdist2D, 70.f, 170.f)*/ANLtoNPPdist2D > 120.f && Dot > 0.9f)
			{
				if (CharMove)
				{
					CharMove->JumpZVelocity = 650.f;
										
					Character->Jump();
				}
			}
		}
	}

#endif
}