// Fill out your copyright notice in the Description page of Project Settings.


#include "EnvQueryGenerator_HexagonalGrid.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

UEnvQueryGenerator_HexagonalGrid::UEnvQueryGenerator_HexagonalGrid(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
	GenerateAround = UEnvQueryContext_Querier::StaticClass();
	GridRadius.DefaultValue = 3;
	SpaceBetween.DefaultValue = 100.0f;
}

void UEnvQueryGenerator_HexagonalGrid::GenerateItems(FEnvQueryInstance &QueryInstance) const
{
	UObject *BindOwner = QueryInstance.Owner.Get();
	GridRadius.BindData(BindOwner, QueryInstance.QueryID);
	SpaceBetween.BindData(BindOwner, QueryInstance.QueryID);

	// Maybe clamp it?
	int32 RadiusValue = GridRadius.GetValue();
	float DensityValue = SpaceBetween.GetValue();

	TArray<FVector> ContextLocations;
	QueryInstance.PrepareContext(GenerateAround, ContextLocations);

	int32 ItemCount{ 1 };
	for (int32 i{ 1 }; i <= RadiusValue; ++i)
	{
		ItemCount += 6 * i;
	}

	TArray<FNavLocation> GridPoints;
	GridPoints.Reserve(ItemCount * ContextLocations.Num());

	FHOrientation TileOrientation{};
	switch (Orientation)
	{
		case EHOrientationFlag::FLAT:
			TileOrientation = HFlatTopLayout;
			break;
		case EHOrientationFlag::POINTY:
			TileOrientation = HPointyLayout;
			break;
	}

	// hex grid generation here
	for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
	{
		for (int32 Q{ -RadiusValue }; Q <= RadiusValue; ++Q)
		{
			// Calculate R1
			int32 R1{ FMath::Max(-RadiusValue, -Q - RadiusValue) };

			// Calculate R2
			int32 R2{ FMath::Min(RadiusValue, -Q + RadiusValue) };

			for (int32 R{ R1 }; R <= R2; ++R)
			{
				float x = ((TileOrientation.f0 * Q) + (TileOrientation.f1 * R)) * DensityValue;
				float y = ((TileOrientation.f2 * Q) + (TileOrientation.f3 * R)) * DensityValue;

				FVector TestPointLocation{ FVector(x + ContextLocations[ContextIndex].X, y + ContextLocations[ContextIndex].Y, 0) };

				const FNavLocation TestPoint{ FNavLocation(TestPointLocation) };
				GridPoints.Add(TestPoint);
			}
		}
	}

	//----------------------

	ProjectAndFilterNavPoints(GridPoints, QueryInstance);
	StoreNavPoints(GridPoints, QueryInstance);
}

FText UEnvQueryGenerator_HexagonalGrid::GetDescriptionTitle() const
{
	return FText::Format(LOCTEXT("HexagonalGridDescriptionGenerateAroundContext", "{0}: generate around {1}"),
						 Super::GetDescriptionTitle(), UEnvQueryTypes::DescribeContext(GenerateAround));
}

FText UEnvQueryGenerator_HexagonalGrid::GetDescriptionDetails() const
{
	FText Desc = FText::Format(LOCTEXT("HexagonalGridDescription", "radius: {0}, space between: {1}"),
							   FText::FromString(GridRadius.ToString()), FText::FromString(SpaceBetween.ToString()));

	FText ProjDesc = ProjectionData.ToText(FEnvTraceData::Brief);
	if (!ProjDesc.IsEmpty())
	{
		FFormatNamedArguments ProjArgs;
		ProjArgs.Add(TEXT("Description"), Desc);
		ProjArgs.Add(TEXT("ProjectionDescription"), ProjDesc);
		Desc = FText::Format(LOCTEXT("HexagonalGridDescriptionWithProjection", "{Description}, {ProjectionDescription}"), ProjArgs);
	}

	return Desc;
}

#undef LOCTEXT_NAMESPACE
