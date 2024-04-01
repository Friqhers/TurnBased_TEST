// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TBSquareMapGenerator.generated.h"

enum class EDirectionType : uint8
{
	North,
	South,
	East,
	West
};

// forward declarations
class UHierarchicalInstancedStaticMeshComponent;
class ATBMammalBase;

USTRUCT()
struct FTileInfo
{
	GENERATED_BODY()
public:
	FVector2D Pos2D;
	FVector WordLocation;
	bool bIsEmptyTile;
	
	UPROPERTY()
	ATBMammalBase* MammalRef;
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
	
	// Wall class to spawn (it should be the same size as TileClass)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Square Map Generation")
	TSubclassOf<AActor> WallClass;

	// Generated map will be SquareMapSizexSquareMapSize
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Square Map Generation")
	int SquareMapSize;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Square Map Generation")
	bool ShowDebug;
	
private:
	FVector TileHalfExtents;
	FVector SquareMapMiddle;

	// 2d array of all the tiles
	TArray<TArray<FTileInfo*>> Tiles;

	// holding all the tiles as 1d array
	TArray<FTileInfo*> AllTiles;

	// Spawns border walls. WallClass must be valid
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
	
	virtual FActorSpawnParameters GetActorSpawnParameters();
	
	FVector GetTileHalfExtents() const;

	/**
	 * @brief Gets the tile in the specified direction from the given SourceTile tile.
	 * @param SourceTile The tile to check the direction from.
	 * @param Direction The direction to check.
	 * @return If valid, returns the tile in the given direction from the SourceTile, otherwise returns nullptr.
	 */
	FTileInfo* GetTileAtDirection(const FTileInfo* SourceTile, const EDirectionType Direction) const;

	// Calls GetTileAtDirection for each direction and returns only empty adjacent tiles.
	TArray<FTileInfo*> GetAllAdjacentEmptyTiles(const FTileInfo* SourceTile) const;

	// Calls GetTileAtDirection for each direction and returns all adjacent tiles.
	TArray<FTileInfo*> GetAllAdjacentTiles(const FTileInfo* SourceTile) const;
	
	FTileInfo* GetRandomEmptyTile() const;
};
