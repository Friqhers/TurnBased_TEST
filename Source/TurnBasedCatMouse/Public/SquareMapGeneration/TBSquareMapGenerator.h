// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TBSquareMapGenerator.generated.h"

// forward declarations
class UHierarchicalInstancedStaticMeshComponent;
class ATBMammalBase;
class ATBTile;

USTRUCT()
struct FTileInfo
{
	GENERATED_BODY()
public:
	bool bIsEmptyTile;
	TSubclassOf<ATBMammalBase> MammalClass;
};


UCLASS()
class TURNBASEDCATMOUSE_API ATBSquareMapGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATBSquareMapGenerator();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squara Map Generation")
	UHierarchicalInstancedStaticMeshComponent* InstancedStaticMeshComponent;
	

	// Tile class to spawn
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Square Map Generation")
	TSubclassOf<ATBTile> TileClass;

	// Wall class to spawn (it should be the same size as TileClass)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Square Map Generation")
	TSubclassOf<ATBTile> WallClass;

	// Generated map will be SquareMapSizexSquareMapSize
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Square Map Generation")
	int SquareMapSize;
	
private:
	TArray<TArray<ATBTile*>> SpawnedTiles2D;
	TArray<ATBTile*> AllSpawnedTiles;
	FVector TileHalfExtents;
	FVector SquareMapMiddle;

	TArray<TArray<FTileInfo>> Tiles2D;
	TArray<FTileInfo> AllTiles;

	void SpawnBorderWalls();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Square Map Generation")
	FVector GetSquareMapMiddle();

	UFUNCTION(BlueprintCallable, Category = "Square Map Generation")
	virtual bool GenerateSquareMap();

	UFUNCTION(BlueprintCallable, Category = "Square Map Generation")
	virtual bool GenerateSquareMap_V2();

	ATBTile* GetRandomEmptyTile() const;

	virtual FActorSpawnParameters GetActorSpawnParameters();

	TArray<TArray<ATBTile*>> GetSpawnedTiles2D();
};
