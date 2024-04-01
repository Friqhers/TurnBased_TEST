// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SquareMapGeneration/TBSquareMapGenerator.h"
#include "TBMammalBase.generated.h"

class ATBSquareMapGenerator;
enum class EDirectionType : uint8;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTurnFinishedSignature, ATBMammalBase*, PlayedMammal, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKilledSignature, ATBMammalBase*, KilledMammal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStarvedSignature, ATBMammalBase*, StarvedMammal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBredSignature, ATBMammalBase*, TargetMammal);



UCLASS()
class TURNBASEDCATMOUSE_API ATBMammalBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's pr
	// operties
	ATBMammalBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mammals")
	UStaticMeshComponent* MammalMesh;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mammals")
	TSubclassOf<ATBMammalBase> EatableMammalClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mammals", meta = (ClampMin=1))
	uint8 StarvationTurnCount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mammals", meta = (ClampMin=1))
	uint8 BreedTurnCount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mammals")
	bool bCanEat;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mammals")
	bool bCanStarve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mammals")
	bool bCanBreed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mammals")
	int MoveInterpSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mammals")
	FLinearColor CurrentTileColor;

public:
	UPROPERTY()
	ATBSquareMapGenerator* MapGeneratorRef;
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void SetCurrentTilePos(FVector2D TargetPos);
	
	FVector2D GetCurrentTilePos();

	void ExecuteTurn();

	UFUNCTION(BlueprintPure, Category = "Mammals") FORCEINLINE
	uint8 GetStarveCounter() const { return StarveCounter; }

	UFUNCTION(BlueprintPure, Category = "Mammals") FORCEINLINE
	uint8 GetBreedCounter() const { return BreedCounter; }
	
	UFUNCTION(BlueprintPure, Category = "Mammals") FORCEINLINE
	uint8 GetSavedBreedCounter() const { return SavedBreedCounter; } FORCEINLINE
	
	void SetSavedBreedCounter(const uint8 NewSavedBreed) {SavedBreedCounter = NewSavedBreed;}
public:
	// Events
	FOnStarvedSignature OnStarved;
	FOnKilledSignature OnKilled;
	FOnTurnFinishedSignature OnTurnFinished;
	FOnBredSignature OnBred;

private:

	// Current X,Y tile position of the mammal
	FVector2D CurrentTilePos;

	// Eat target for this round
	FTileInfo EatTarget;
	
	uint8 StarveCounter;
	uint8 BreedCounter;
	uint8 SavedBreedCounter;
	FVector CurrentMoveTargetPosition;
	bool bMoveStarted;
	bool bEatOnMoveFinished;

private:
	/* If possible, starts movement logic that moves mammal 1 unit in a random direction (North, South, East, or West).
	 * Movement will happen in Tick(). Which calls OnMoveFinished after movement ends.
	 */
	void StartRandomMove();
	
	/**
	 * @brief Starts eat logic by moving to target mammal on TargetTile.
	 * Movement will happen in Tick(). Which calls OnMoveFinished after movement ends.
	 * @param EatTargetTile Target tile to eat
	 */
	void StartEat(const FTileInfo& EatTargetTile);

	
	void OnMoveFinished(const bool bWasSuccessful);

	// if there are any "EatableMammalClass" within 1 unit return the tile, otherwise nullptr
	bool GetRandomEatTarget(FTileInfo& EatTargetTile) const;
	
	void TryBreed();
	void TryStarve();
	
};
