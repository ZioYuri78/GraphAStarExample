// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "DataProviders/AIDataProvider.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"
#include "EnvQueryGenerator_HexagonalGrid.generated.h"

UENUM()
enum class EHOrientationFlag : uint8
{
	FLAT,
	POINTY,
	NONE
};

USTRUCT()
struct FHOrientation
{
	GENERATED_USTRUCT_BODY()

		FHOrientation()
	{
	}

	friend bool operator==(const FHOrientation &lhs, const FHOrientation &rhs)
	{
		return (lhs.f0 == rhs.f0) && (lhs.f1 == rhs.f1) && (lhs.f2 == rhs.f2) && (lhs.f3 == rhs.f3);
	}

	double f0, f1, f2, f3;	// f0, f1 for X, f2, f3 for Y				- used in HexToWorld
	double b0, b1, b2, b3;	// Inverse.	Q b0*x, b1*y - R b2*x, b3*y		- used in WorldToHex
};

const struct FHFlatTopOrientation : FHOrientation
{
	// Flat top hexagon layout (X = y, Y = -x where uppercase is the original coordinates from Red Blob Games article, lowercase is UE4 coordinates)
	FHFlatTopOrientation()
	{
		// UE4 | Original
		f0 = -FMath::Sqrt(3.0) / 2.0;	// -f2 | f0 = 3/2
		f1 = -FMath::Sqrt(3.0);			// -f3 | f1 = 0
		f2 = 3.0 / 2.0;					//  f0 | f2 = sqrt(3)/2
		f3 = 0.0;						//  f1 | f3 = sqrt(3)

		b0 = 0.0;						// b1 | b0 = 2/3
		b1 = 2.0 / 3.0;					// b0 | b1 = 0
		b2 = FMath::Sqrt(3.0) / 3.0;	// b3 | b2 = -1/3
		b3 = -1.0 / 3.0;				// b2 | b3 = sqrt(3)/3
	}

}HFlatTopLayout;

const struct FHPointyOrientation : FHOrientation
{
	FHPointyOrientation()
	{
		// UE4 | Original
		f0 = 0.0;						// -f2 | f0 = sqrt(3)
		f1 = -3.0 / 2.0;				// -f3 | f1 = sqrt(3)/2
		f2 = FMath::Sqrt(3.0);			//  f0 | f2 = 0
		f3 = FMath::Sqrt(3.0) / 2.0;	//  f1 | f3 = 3/2

		b0 = 1.0 / 3.0;					// -b1 | b0 = sqrt(3)/3
		b1 = FMath::Sqrt(3.0) / 3.0;	//  b0 | b1 = -1/3
		b2 = -2.0 / 3.0;				// -b3 | b2 = 0
		b3 = 0.0;						//  b2 | b3 = 2/3
	}

}HPointyLayout;


/**
 * 
 */
UCLASS(meta = (DisplayName = "Points: HexGrid"))
class GRAPHASTAREXAMPLE_API UEnvQueryGenerator_HexagonalGrid : public UEnvQueryGenerator_ProjectedPoints
{
	GENERATED_UCLASS_BODY()
	
	virtual void GenerateItems(FEnvQueryInstance &QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

	/** half of square's extent, like a radius */
	UPROPERTY(EditDefaultsOnly, Category = Generator, meta = (DisplayName = "Grid Radius"))
	FAIDataProviderIntValue GridRadius;

	/** generation density */
	UPROPERTY(EditDefaultsOnly, Category = Generator, meta = (ToolTip = "Tile size"))
	FAIDataProviderFloatValue SpaceBetween;

	/** context */
	UPROPERTY(EditDefaultsOnly, Category = Generator)
	TSubclassOf<UEnvQueryContext> GenerateAround;

	UPROPERTY(EditDefaultsOnly, Category = Generator, meta = (DisplayName = "Orientation"))
		EHOrientationFlag Orientation {
		EHOrientationFlag::POINTY
	};
};
