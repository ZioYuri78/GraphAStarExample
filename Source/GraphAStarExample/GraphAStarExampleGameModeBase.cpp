// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.


#include "GraphAStarExampleGameModeBase.h"

bool AGraphAStarExampleGameModeBase::IsPIE() const
{
	return GetWorld() ? GetWorld()->WorldType == EWorldType::PIE : false;
}
