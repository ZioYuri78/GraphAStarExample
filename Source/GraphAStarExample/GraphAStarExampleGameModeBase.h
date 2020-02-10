// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GraphAStarExampleGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class GRAPHASTAREXAMPLE_API AGraphAStarExampleGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, Category = "GraphAStarExample|Game Mode")
	bool IsPIE() const;
};
