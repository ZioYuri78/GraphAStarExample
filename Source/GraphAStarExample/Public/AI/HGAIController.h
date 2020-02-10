// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HGAIController.generated.h"

/**
 * This is the parent class of BP_AIController_Example_B
 *
 * We need this inherited class just only to set and use our custom HGPathFollowingComponent
 * (look at the constructor in .cpp)
 *
 */
UCLASS()
class GRAPHASTAREXAMPLE_API AHGAIController : public AAIController
{
	GENERATED_BODY()
	
public:

	AHGAIController(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());
};
