// The engine-side owner of career and day state. One PlayerState, one
// DayState, one seed — living here so they survive level transitions (the
// van→gym drive) and so every wall, HUD, and interact reads the same truth.
// Loads on boot, saves on sleep. All rules stay in Sim/; this class only
// holds state and forwards to it.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "DirtbagSimTypes.h"

#include "DirtbagGameInstance.generated.h"

UCLASS()
class UDirtbagGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	/** The world's identity; every RNG stream derives from it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	FString Seed = TEXT("gym-1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	FString SaveFilename = TEXT("dirtbag-save.txt");

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FDirtbagPlayerState Player;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FDirtbagDayState Day;

	/** True when the current Player came from disk rather than a fresh start. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	bool bLoadedFromSave = false;

	// --- Day verbs (mutating members must live here: Blueprint struct
	// --- access returns copies, so the owner does the mutating) ----------

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	bool EatMeal();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	void WorkShift();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	void PassHours(double Hours);

	/** First pull-on of the day seeds the session; later calls no-op. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	void EnsureAtGym();

	/** Lights out: sim sleep, then save — the save file's one write point. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	void Sleep();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	bool SaveNow();

	/** The gym's board for this world, cached. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	const TArray<FDirtbagRoute>& GetBoard();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FDirtbagRoute GetBoardRoute(int32 Index);

	/** Bot-driven burn on a route, day-integrated (session, ledger, time,
	 *  energy, training all move together). */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FDirtbagAttemptResult ReplayAttempt(const FDirtbagRoute& Route);

	/** Lifetime attempts on a route, for "Attempt N" staging. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	int32 AttemptsOn(const FDirtbagRoute& Route) const;

	// --- C++-side live-attempt plumbing for ADirtbagClimbWall ------------
	// Not UFUNCTIONs: sim types don't cross into Blueprint.

	dirtbag::LiveAttempt BeginLiveFor(const FDirtbagRoute& Route);
	FDirtbagAttemptResult CommitLiveFor(const dirtbag::LiveAttempt& Live);

	/** Session seed for today: attempts are replayable per world+day. */
	FString TodaysSessionSeed() const;

private:
	UPROPERTY()
	TArray<FDirtbagRoute> Board;
};
