// A place where a day-verb happens: the cooler, the front desk, the van.
// Walk up, press E. The actor is only the doorbell — every consequence
// (cash, hunger, energy, the day advancing, the save) belongs to the sim.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DirtbagGameInstance.h"

#include "DirtbagDaySpot.generated.h"

class UBoxComponent;
class USceneComponent;
class UDirtbagGameInstance;

UENUM(BlueprintType)
enum class EDirtbagSpotKind : uint8
{
	/** Gas-station burrito economics: costs cash, buys back hunger. */
	Meal,
	/** The belay-desk shift: pays, and takes the hours and the energy. */
	Shift,
	/** Lights out — advances the day and writes the save. */
	Sleep,
	/** The drive: half an hour of the day, and you are somewhere else. */
	Travel,
	/** Sitting it out — the log by the pads, the tailgate. Passes time, and
	 *  outdoors will wait exactly until the rock comes good, which is the
	 *  verb the whole conditions system was missing. */
	Rest,
	/** The fire at the Lot. Rest, with people: the hours pass the same way
	 *  and you get the company for them — rapport, and what everyone is
	 *  working this week. */
	Fire,
	/** The bowl by the van. Feed the stray until it is not a stray. */
	Dog,
	/** Under the van with a spanner, or at the shop with a wallet. Fixes
	 *  the worst thing wrong with it the best way you can afford. */
	Van,
	/** The gear shop: resole while the uppers hold, replace when not. */
	GearShop
};

UCLASS()
class ADirtbagDaySpot : public AActor
{
	GENERATED_BODY()

public:
	ADirtbagDaySpot();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	EDirtbagSpotKind Kind = EDirtbagSpotKind::Meal;

	/** Travel only: the spot at the other end of the drive. Point the van's
	 *  spot at the gym's and vice versa. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Dirtbag|Travel")
	TObjectPtr<AActor> TravelTarget;

	/** Where this drive goes, for the prompt: "Drive to the gym?" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Travel")
	FString TravelName = TEXT("the gym");

	/** Hours the drive eats, used **only when the far end has no rock** —
	 *  the gym, the town. Outdoors the guidebook owns the number and this is
	 *  ignored, because `Crag::approachHours` says the same thing and used
	 *  to be read by nobody: two copies of one fact, and the one that
	 *  counted was whichever got typed into the level.
	 *
	 *  See DriveHours(). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Travel")
	double TravelHours = 0.5;

	/** Where this drive lands you. Set on arrival, so the HUD's conditions
	 *  line is right from the moment you step out of the van rather than
	 *  from whenever you first touch a wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Travel")
	EDirtbagVenue ArriveAt = EDirtbagVenue::Gym;

	/** Rest only. How long one press sits for, when there is no window to
	 *  wait for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Rest")
	double RestHours = 1.0;

	/** Rest only. With a window later today, one press waits exactly until
	 *  it opens rather than an hour at a time. Turn it off to sit in fixed
	 *  chunks and watch the forecast change under you. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Rest")
	bool bWaitForWindow = true;

	/** Fire only: which of the three stakes you sit down on — 0 is the
	 *  ante, 2 is the sim's ceiling. 1/2/3 change it at the table.
	 *
	 *  This replaced a hand-typed `CardStake = 20.0`, which was a number
	 *  beside a dial rather than a number from one: the sim's ceiling is
	 *  40, so "the stake" was permanently half of it and there was no way
	 *  to bet small on a bad hand — which removes the only decision poker
	 *  has that is not folding. See dirtbag::StakeNotch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Fire")
	int32 StartingStakeNotch = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Travel")
	float FadeSeconds = 0.4f;

	/** What this spot sounds like when you use it: a van door, a kettle, a
	 *  shop bell, a shovel of coals. One per spot, because that is what a
	 *  spot is — one verb, pressed once.
	 *
	 *  Optional, and the play site is guarded: with nothing assigned the
	 *  spot behaves exactly as it always has, which is the state it ships
	 *  in from the container. Anything that would need a *loop* — the fire
	 *  crackling while you sit at it, the Lot at night — is an
	 *  AmbientSound actor placed in the level beside the spot, not this. A
	 *  spot fires when you press a key; an atmosphere is somewhere you
	 *  are, and the level is where that belongs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Sound")
	TObjectPtr<class USoundBase> InteractSound;

	UPROPERTY(VisibleAnywhere, Category = "Dirtbag")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Dirtbag")
	TObjectPtr<UBoxComponent> Trigger;

private:
	UFUNCTION()
	void OnTriggerBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                    bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OnInteract();
	/** The fire's second verb, and its grammar: C commits, F backs down.
	 *  Tonight's game rotates with the day — you join whatever is being
	 *  played. C deals when nothing is pending; then C is stay/call/hit
	 *  and F is fold/pass/stand. Toasts are the whole table UI, which is
	 *  the blockout answer until the fire earns a widget. */
	void OnCommit();
	void OnBackDown();
	/** The gear shop's dream counter: 1/2/3 name the dream, once. At the
	 *  fire the same three keys set the stake, which is the same question
	 *  asked twice — how much of the float is this worth. */
	void OnChoose1();
	void OnChoose2();
	void OnChoose3();
	void ChooseDreamAt(EDirtbagDream Which);
	/** Returns true if it handled the key, i.e. this is the fire. */
	bool SetStakeNotch(int32 Notch);

	/** One exit for every settled hand: record the sentence, add it to the
	 *  evening's running total, clear the table. Three call sites used to
	 *  each build their own toast, which is how the night's total came to
	 *  be tracked by nobody. */
	void SettleFireHand(const FString& Line);

	/** Write the table into the game instance for the HUD to draw. Called
	 *  on every change rather than every frame: hands re-deal
	 *  deterministically from (day, number), so this can rebuild the whole
	 *  table from three integers and never holds a stale copy of one. */
	void RefreshFireTable();

	/** Push `PromptText()` to the HUD's prompt line. Called on every state
	 *  change rather than once on entry: the old toast said what the shop
	 *  was offering at the moment you walked in and then expired, so
	 *  buying the shoes left a four-second-old sentence about shoes on
	 *  screen and nothing about the pad it would now sell you. */
	void PushPrompt();

	/** Settle whatever is live, as if F had been pressed. Used when you
	 *  walk away from the table mid-hand — every game here takes the ante
	 *  at settlement rather than at the deal, so leaving used to be a free
	 *  abort: deal, look, walk out, no cost. */
	void SettleAndLeave();
	void BeginDrive();

	/** What this drive actually costs. The guidebook's approach when the
	 *  far end is rock, this spot's TravelHours when it is not.
	 *
	 *  One number, one place. Roadside is half an hour, the Cave forty
	 *  minutes, the Terrace nearly an hour, and none of that depends any
	 *  more on somebody remembering to type it into the details panel. */
	double DriveHours() const;
	void ArriveFromDrive();
	FString PromptText() const;

	UPROPERTY()
	TObjectPtr<UDirtbagGameInstance> Game;

	bool bPlayerNear = false;
	bool bBoundInput = false;
	/** A hand on the table at this spot. Hands re-deal deterministically
	 *  from (day, number), so this is the only state a game needs. */
	bool bHandPending = false;
	int32 HandNumber = 0;
	int32 HandDay = 0;
	/** Which stake notch is selected, and what the live hand is actually
	 *  playing for. Two fields rather than one because they are allowed to
	 *  differ: poker and liar's dice let you size the bet with the hand in
	 *  front of you — that *is* the decision — while blackjack locks at the
	 *  deal, because raising after you have seen your cards is not a game. */
	int32 StakeNotch = 1;
	double LiveStake = 0.0;
	/** The evening, which nothing tracked before the table existed. You
	 *  could lose two hundred dollars one $20 shrug at a time and never see
	 *  a number that said so. */
	int32 HandsSettled = 0;
	double NightDelta = 0.0;
	FTimerHandle DriveTimer;
};
