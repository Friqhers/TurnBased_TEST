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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Square Map Generation")
	int SquareMapSize;
	
private:
	FVector TileHalfExtents;
	FVector SquareMapMiddle;

	TArray<TArray<FTileInfo>> Tiles2D;
	TArray<FTileInfo> AllTiles;

	void SpawnBorderWalls();
public:
	bool UpdateTileAt(FTileInfo NewTileInfo, int TargetX, int TargetY);
	bool GetTileAt(FTileInfo& TargetTile, int TargetX, int TargetY);

	bool ClearTile(int TargetX, int TargetY);
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
	
	bool GetRandomEmptyTile(FTileInfo& FoundTile) const;

	/**
	 * @brief Gets the tile in the specified direction from this tile.
	 * @param TileResult
	 * @param SourceTile
	 * @param Direction The direction to check.
	 * @return If valid, returns the tile in the given direction from this tile, otherwise returns nullptr.
	 */
	bool GetTileAtDirection(FTileInfo& TileResult, const FTileInfo& SourceTile, const EDirectionType Direction) const;

	// Calls GetTileAtDirection for each direction and returns only empty adjacent tiles.
	TArray<FTileInfo> GetAllAdjacentEmptyTiles(const FTileInfo& SourceTile) const;

	// Calls GetTileAtDirection for each direction and returns all adjacent tiles.
	TArray<FTileInfo> GetAllAdjacentTiles(const FTileInfo& SourceTile) const;

	virtual FActorSpawnParameters GetActorSpawnParameters();
	

	
	FVector GetTileHalfExtents() const;
};
