// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HGLPTypes.h"
#include "HexGridActor.generated.h"

UCLASS()
class HEXGRIDLIBPLUGIN_API AHexGridActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHexGridActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grids Plugin|Grid")
	EGridShape GridShape {
		EGridShape::NONE
	};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grids Plugin|Grid")
	FHTileLayout TileLayout {};

	// Used with Hexagonal and Triangle shape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grids Plugin|Grid")
	int32 Radius {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grids Plugin|Grid")
	int32 Width {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grids Plugin|Grid")
	int32 Height {};

	UPROPERTY(BlueprintReadWrite, Category = "Hex Grids Plugin|Grid")
	TArray<FHTile> GridTiles{};

};
