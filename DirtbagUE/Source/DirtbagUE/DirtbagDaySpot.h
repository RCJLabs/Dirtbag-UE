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
	GearShop,
	/** **An evening on something that is not climbing.** The phone box, the
	 *  tailgate with the guitar out, the milk crate of paperbacks, the
	 *  stove. One spot per thread, set by `Thread` below -- see
	 *  Sim/DirtbagLife.h. The hours it takes are the hours it takes; that
	 *  is the whole trade. */
	Evening
};

UCLASS()
class ADirtbagDaySpot : public AActor
{
	GENERATED_BODY()

public:
	ADirtbagDaySpot();

protected:
	virtual void BeginPlay() override;
	/** Only runs while the road is on screen; see BeginTravelScreen. */
	virtual void Tick(float DeltaSeconds) override;

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

	/** Evening only. Which thread this spot is. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Life")
	EDirtbagThread Thread = EDirtbagThread::Home;

	// How long one press gives it is deliberately **not** a property here.
	// `dirtbag::AsksFor` owns it -- see Sim/DirtbagLife.h. A spot that could
	// set its own would be a second copy of the rule, and one go at a thing
	// being two hours here and six there is exactly how the evening stopped
	// costing anything the first time this was measured.

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

	/** Where this spot goes, in world terms rather than rock terms.
	 *
	 *  The 2D game has **two** travel rules and the port had one: zones are
	 *  connected and you walk between them; crags and comps need the van.
	 *  This is which of the two applies — a Lot or Town destination is a
	 *  walk with no van, no fuel and no breakdown roll, and everything else
	 *  is a drive exactly as before.
	 *
	 *  Defaults to Roadside on purpose: **a crag, so every travel spot
	 *  already placed keeps behaving exactly as it does today** until it is
	 *  deliberately told it is a walk. A default that silently made drives
	 *  free would be the worse direction to be wrong in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Travel")
	EDirtbagZone DestinationZone = EDirtbagZone::Roadside;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Travel")
	float FadeSeconds = 0.4f;

	/** How long the road takes on screen.
	 *
	 *  Short on purpose. The first trip is a moment; the fiftieth is a
	 *  keypress, and a career has hundreds. **Any key skips to the far
	 *  kerb** — a travel screen you cannot skip stops being a moment and
	 *  becomes a tax. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Travel")
	float TravelSeconds = 2.6f;

	/** What the road looks like. Optional, like every asset slot in this
	 *  project: with nothing assigned the screen draws a horizon and a
	 *  shape, and still says everything it needs to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Travel")
	TObjectPtr<class UTexture2D> RoadBackdrop;

	/** The van itself, crossing it. Not drawn on a walk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Travel")
	TObjectPtr<class UTexture2D> VanSprite;

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
	UFUNCTION()
	void OnChoose4();
	UFUNCTION()
	void OnChoose5();
	UFUNCTION()
	void OnChoose6();

	/** C or F at the van, when they have asked. Returns true if the key was
	 *  spent here. */
	bool AnswerTheRival(bool bYes);

	void ChooseDreamAt(EDirtbagDream Which);
	/** Returns true if it handled the key, i.e. this is the fire. */
	bool SetStakeNotch(int32 Notch);

	/** Take a gig off today's board. Returns true if the key belonged to
	 *  the board, whether or not the gig could actually be taken — a
	 *  number pressed at a work spot must never fall through to a dream or
	 *  a stake. */
	bool TakeGig(int32 Which);

	/** Stop climbing for good, at the van, on its own key.
	 *
	 *  **Its own key and its own confirm**, because it is the single most
	 *  irreversible thing in this game — there is no undo, no reload that
	 *  is not a lost evening, and the save is written immediately. E at the
	 *  van repairs it; a career should never end because somebody meant to
	 *  fix a wheel bearing. */
	void OnRetire();

	/** Open or close the crag's page. Bound on spots and on walls, which
	 *  is everywhere you would want to read it — see
	 *  notes/phase6-the-guidebook.md for why it is not a global key. */
	void OnGuidebook();

	/** Sign the deal on the table, at the counter. Its own key because E
	 *  buys shoes, and signing away three days a month should not be the
	 *  same press as buying rubber. */
	void OnSign();

	/** See a physio, at the counter. Its own key for the same reason
	 *  signing has one: E is for buying rubber. */
	void OnPhysio();

	// The care counter. Each of these is a decision with a wrong answer --
	// see Sim/DirtbagMedical.h for which answer is wrong and why.
	void OnLookAtIt();
	void OnTakeSomething();

	// Held while a gig's number is pressed: do the shift properly. See
	// Sim/DirtbagCraft.h for what that costs when you cannot.
	void OnHardWayDown();
	void OnHardWayUp();
	bool bHoldingTheHardWay = false;
	void OnTooth();
	void OnShrink();
	void OnPrehab();
	void OnTakeTheShot();
	void OnOperate();
	void OnPushOn();
	void OnCover();

	/** The gym membership, at the counter. */
	void OnMembership();

	/** Sign for the nine-to-five, or leave it. One key both ways, because
	 *  it is one decision made twice. */
	void OnSalary();

	/** Buy a hangboard at the counter; use it at the van. One key, two
	 *  spots, because it is one object and where you are says which you
	 *  meant. */
	void OnHangboard();
	void OnRack();

	/** While the book is open, 1/2/3 turn the page rather than naming a
	 *  dream or sizing a stake. Returns true if the book took the key. */
	bool TurnGuidebookPage(int32 Which);

	/** Advance the handover: the epitaph, then the choice, then arrival. */
	void StepHandover();

	/** Take one of the three. Returns true if the handover consumed the
	 *  key, so 1/2/3 mean the successor rather than a dream or a stake. */
	bool ChooseArrival(int32 Which);

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

	/** How long the same trip takes on your legs. Comes from the zone
	 *  model rather than from this actor, so the Lot-to-town walk is one
	 *  number in one place however many spots point along it. */
	double WalkHours() const;
	void ArriveFromDrive();

	/** Put the road up and take the sim's side of the trip in one go. */
	void BeginTravelScreen(bool bOnFoot, double Hours, int32 Broke);

	/** Cut to the far kerb, and say whether that is what this keypress
	 *  meant. Called first by every input handler on this actor: while the
	 *  road is up, **every key means "get on with it"**, because a player
	 *  on their fiftieth trip should not have to remember which one. */
	bool SkipTravel();
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

	/** True once R has been pressed at the van and the game is waiting for
	 *  the second press. Cleared by walking away, so a confirm cannot sit
	 *  armed across half a season. */
	bool bRetireArmed = false;
	FTimerHandle DriveTimer;

	/** How far along the road we are, in seconds, and how long it runs.
	 *  Only meaningful while Game->TravelReadout.bActive. */
	float TravelElapsed = 0.f;
	double TravelStartHour = 0.0;
	double TravelHoursTaken = 0.0;
};
