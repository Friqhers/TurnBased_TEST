// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TBTile.generated.h"

UENUM()
enum class EDirectionType : uint8
{
	North,
	South,
	East,
	West
};


class ATBMammalBase;
class ATBSquareMapGenerator;

UCLASS()
class TURNBASEDCATMOUSE_API ATBTile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATBTile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile")
	UStaticMeshComponent* TileFloorMesh;

	UPROPERTY(BlueprintReadOnly)
	ATBMammalBase* MammalRef;

	UPROPERTY()
	ATBSquareMapGenerator* SquareMapGeneratorRef;
	
private:
	// x,y position of the tile in square map
	FVector2D Position2D;

	// Indicates whether this tile is empty or not
	bool bIsEmptyTile;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Destroys the Mammal on this tile and sets references accordingly.
	void ClearTile();

	/**
	 * @brief Spawns a mammal on this tile and sets references accordingly.
	 * @param MammalClass The class of the mammal to spawn.
	 * @return Returns a pointer to the spawned mammal.
	 * @remarks Sets @MammalRef to point to the spawned mammal and updates @bIsEmptyTile (a boolean indicating whether the tile is currently empty) accordingly.
	 */
	ATBMammalBase* SpawnMammal(TSubclassOf<ATBMammalBase> MammalClass);


	/**
	 * @brief Gets the tile in the specified direction from this tile.
	 * @param Direction The direction to check.
	 * @param SpawnedTiles Array of all spawned tiles.
	 * @return If valid, returns the tile in the given direction from this tile, otherwise returns nullptr.
	 */
	ATBTile* GetTileAtDirection(const EDirectionType Direction, const TArray<TArray<ATBTile*>>& SpawnedTiles) const;

	// Calls GetTileAtDirection for each direction and returns all found adjacent tiles. 
	TArray<ATBTile*> GetAllAdjacentTiles() const;

	// Calls GetTileAtDirection for each direction and returns only empty adjacent tiles.
	TArray<ATBTile*> GetAllAdjacentEmptyTiles() const;

public:
	//EVENTS
	
	// Called when a new mammal moves to this tile.
	UFUNCTION(BlueprintImplementableEvent, Category = "Tile|Events")
	void OnTileUpdated();

public:
	void SetPosition2D(const FVector2D& TargetPos);
	FVector2D GetPosition2D() const;

	UFUNCTION(BlueprintPure, Category = "Tile")
	bool IsTileEmpty() const;
	UFUNCTION(BlueprintCallable, Category = "Tile")
	void SetTileEmpty(bool bIsEmpty);

};
