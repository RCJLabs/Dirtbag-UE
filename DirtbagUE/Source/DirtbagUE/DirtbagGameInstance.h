// The engine-side owner of career and day state. One PlayerState, one
// DayState, one seed — living here so they survive level transitions (the
// van→gym drive) and so every wall, HUD, and interact reads the same truth.
// Loads on boot, saves on sleep. All rules stay in Sim/; this class only
// holds state and forwards to it.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "DirtbagSimTypes.h"

// The sim's own container, deliberately: UHT parses every declaration in a
// UCLASS body and cannot resolve a plain namespaced C++ type inside a
// TArray. See the note in DirtbagSimLibrary.h.
#include <vector>

#include "DirtbagGameInstance.generated.h"

// A level placed without something it needs -- a wall with no spline, a
// travel spot with no target. Aimed at whoever is holding the editor
// rather than at a player, so it goes to the log where it outlives the
// playtest instead of flashing red for four seconds during it.
//
// Declared once here and defined once in DirtbagUE.cpp, which is the only
// arrangement that survives a unity build: two files each writing their
// own DEFINE_LOG_CATEGORY_STATIC for the same name compile fine alone and
// collide the moment UBT stitches them into one translation unit -- which
// it does by default, and which is precisely the mistake this pair of
// files was about to make.
DECLARE_LOG_CATEGORY_EXTERN(LogDirtbagSetup, Log, All);

/**
 * What the session wants drawn right now. Published by the wall, read by
 * the HUD — so the presentation layer shares one truth and a future UMG
 * widget binds to the same fields the debug HUD uses.
 */
USTRUCT(BlueprintType)
struct FDirtbagSessionReadout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	bool bActive = false;

	/** "Volume Country  V5" */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FString RouteLine;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double Pump = 0.0;

	/** Best-case odds for the move you're on; < 0 when no move is pending. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double Odds = -1.0;

	/** Grip charge 0..1; < 0 when not charging. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double Grip = -1.0;

	/** The latch window, so the bar can show you where to let go. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double WindowStart = 0.6;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double WindowEnd = 0.95;

	/** Which go this is on this line. Was a toast that flashed for four
	 *  seconds at the start of an attempt and then left the number
	 *  nowhere — while "attempt 14 on the same problem" is the whole
	 *  emotional content of a session and is true for all of it. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	int32 Attempt = 0;

	/** **What the last go was**, in one sentence — "Blew the clip.", "Off
	 *  at the crux. You had been a long way above the gear for a while."
	 *  Sits with the attempt number for the same reason: it is true from
	 *  the end of one burn until the start of the next, which is most of a
	 *  session, and a toast would have flashed it away while you were
	 *  still looking at the move that did it. See Sim/DirtbagNarrator.h.
	 *
	 *  The beats *during* a go are a different tier and are toasted by the
	 *  wall, because a moment is a moment. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FString LastBurn;

	/** Whether to show the control reminder under the grip bar. Mirrors
	 *  `bLearnedTheVerb` on the game instance — the readout is wiped at
	 *  the end of every session, and a reminder that came back for every
	 *  attempt would be the same noise the toast was. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	bool bShowTheVerb = true;
};

/** How a prompt line reads. Not a colour — the HUD owns colours; this is
 *  what the line *is*, so the same tone means the same thing whether it
 *  came from a wall, a shop counter or a van. */
UENUM(BlueprintType)
enum class EDirtbagPromptTone : uint8
{
	/** What is here and what the key does. */
	Plain,
	/** Something worth having: a line nobody has done, a belayer free. */
	Good,
	/** Something in the way: no money, no belayer, a wrecked body. */
	Blocked,
};

USTRUCT(BlueprintType)
struct FDirtbagPromptLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Prompt")
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Prompt")
	EDirtbagPromptTone Tone = EDirtbagPromptTone::Plain;
};

/**
 * What the keys do where you are standing.
 *
 * Toast triage, 2026-08-22 (`notes/phase5-toast-triage.md`). Forty-nine
 * on-screen messages, sorted by one question: **is there a decision
 * pending on this?** News — you ate, the van broke, somebody took your
 * line — is already true by the time you read it and there is nothing left
 * to do about it, which is exactly what a toast is for. But *"Shoes? (E)
 * — $180"* is not news. It is the interaction, and it expired after four
 * seconds while the player stood in the trigger with the decision unmade.
 *
 * Everything that says what a key does now lives here and is drawn for as
 * long as it is true. That also fixed a live bug the sorting turned up:
 * the wall's belayer line and its body-state advice were pushed through
 * the same keyed toast slot one line apart, so on a roped line at the gym
 * when you were tired, the second silently replaced the first and **you
 * were never told who was holding the rope**.
 *
 * Owned by the actor you are standing in, so two overlapping triggers
 * cannot clear each other's prompt.
 */
USTRUCT(BlueprintType)
struct FDirtbagPrompt
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Prompt")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Prompt")
	TArray<FDirtbagPromptLine> Lines;
};

/** Which page of the book. */
UENUM(BlueprintType)
enum class EDirtbagGuidebookView : uint8
{
	/** Everything here, in book order. */
	Everything,
	/** Lines nobody has done — the first-ascent pipeline's shopping list. */
	Projects,
	/** At or under what you have actually climbed. */
	InReach,
};

/** One line on the page. */
USTRUCT(BlueprintType)
struct FDirtbagGuidebookRow
{
	GENERATED_BODY()

	/** "Diesel  V5  ***" or "project — the arete left of Diesel". */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Guidebook")
	FString Entry;

	/** What you have done on it: a tick, a burn count and a highpoint, or
	 *  nothing at all. This is the projecting/nemesis system finally
	 *  visible — it has been tracked per line since Phase 1 and has only
	 *  ever surfaced as "attempt 14" on the wall you happened to be
	 *  standing at. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Guidebook")
	FString Yours;

	/** Whose it is, when somebody has done it first. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Guidebook")
	FString FirstAscent;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Guidebook")
	bool bSent = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Guidebook")
	bool bProject = false;
};

/**
 * The crag's page.
 *
 * `Sim/DirtbagCrag.h` carries three `unwired-ok` notes that all say the
 * same thing — *"the guidebook page's filter; there is no guidebook
 * screen"* — and they were right for months. `LinesUpTo` and
 * `OpenProjects` were written for a page that did not exist.
 *
 * Without it the player learns about a line by walking up to it. There is
 * no way to see what is at a crag before touring it, which lines are done,
 * where the open projects are, or how many burns are in the thing that
 * keeps spitting you off — and `concepts/DIRTBAG.md` §4 lists
 * *projecting/nemesis tracking* as port-wholesale. The tracking was
 * ported. The seeing was not.
 *
 * For a game whose entire loop is *pick a line, try it*, and whose fiction
 * is a guidebook, that was the hole in the middle of the table.
 */
USTRUCT(BlueprintType)
struct FDirtbagGuidebookReadout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Guidebook")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Guidebook")
	EDirtbagGuidebookView View = EDirtbagGuidebookView::Everything;

	/** Where this page is about. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Guidebook")
	FString CragName;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Guidebook")
	TArray<FDirtbagGuidebookRow> Rows;

	/** How many the view hid, so a filter never silently swallows the
	 *  crag. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Guidebook")
	int32 Hidden = 0;

	/** Sends and lines here, for the header — the one place the game says
	 *  how much of a crag you have actually got through. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Guidebook")
	int32 SentHere = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Guidebook")
	int32 LinesHere = 0;
};

/** Which part of the handover is on screen. */
UENUM(BlueprintType)
/** The four questions a career opens with, in order. See
 *  Sim/DirtbagCharacter.h — the first three are a build and the fourth is a
 *  disposition, and none of them is optional: a climber with no answers is
 *  the template everybody used to be. */
UENUM(BlueprintType)
enum class EDirtbagCreationStep : uint8
{
	/** What kind of climber. Four, and the offsets sum to zero. */
	Archetype,
	/** Where you came from. Six, each with a perk in its own lane. */
	Origin,
	/** What is wrong with you. Five, and you have to pick one. */
	Flaw,
	/** What you are like. Four. */
	Temperament,
	/** And one thing about you that is nobody's business but yours. Six,
	 *  each a small perk against a small cost — the only quirks you can
	 *  ever choose, because every other one has to be earned by climbing a
	 *  certain way for two seasons. See Sim/DirtbagHabits.h. */
	Quirk,
	/** Who that turned out to be. */
	Done,
};

/** One problem on the comp board, as the screen shows it. */
USTRUCT(BlueprintType)
struct FDirtbagCompProblem
{
	GENERATED_BODY()

	/** The tape colour. Comp problems are named by tape, not by poetry. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FString Colour;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FString Grade;

	/** What it is worth topped, and what a flash would have been. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	double Points = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	double FlashPoints = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	int32 Tries = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	bool bTopped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	bool bFlashed = false;

	/** 0 none, 1 low, 2 high. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	int32 Zone = 0;
};

/** A comp in progress, as a thing the HUD can draw.
 *
 *  **Five problems, seven attempts, and you choose where they go.** That is
 *  the whole mechanic and the reason a comp is worth porting -- see
 *  Sim/DirtbagComp.h. */
USTRUCT(BlueprintType)
struct FDirtbagCompReadout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FString Tier;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	TArray<FDirtbagCompProblem> Problems;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	int32 AttemptsLeft = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	double YourScore = 0.0;

	/** Set once it is over: the scoreboard, best first. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	bool bSettled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	TArray<FString> Board;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FString Placing;

	/** Which ladder this board belongs to. The settle reads it to decide
	 *  where the result goes -- a World Cup round banked into the domestic
	 *  circuit is a title nobody won. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	EDirtbagStage Stage = EDirtbagStage::Domestic;

	/** Where it is, when it is somewhere. Empty for a comp at the gym. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FString Where;

	/** Which round is on the wall. Qualification at a gym comp, and it
	 *  never moves off it -- a Tuesday is one board and done. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	EDirtbagCompRound Round = EDirtbagCompRound::Qualification;

	/** Does this comp run rounds at all? */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	bool bHasRounds = false;

	/** How many are left in it, once anybody has been cut. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	int32 StillIn = 0;

	/** "Through to the final." / "Out in the semi-final." Said once, when
	 *  a round closes, and cleared when the next one starts. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FString RoundNews;
};

/** The creation flow, as a thing the HUD can draw.
 *
 *  Deliberately the same shape as the handover, and for the same reason:
 *  **in this game a new career is an arrival.** The first climber and the
 *  one who turns up after you retire are answering the same question, so
 *  they go through one door rather than two that will drift apart. */
USTRUCT(BlueprintType)
struct FDirtbagCreationReadout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Creation")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Creation")
	EDirtbagCreationStep Step = EDirtbagCreationStep::Archetype;

	/** The question, in the game's voice rather than as a field label. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Creation")
	FString Question;

	/** What you can pick. Up to six; the key is the index plus one. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Creation")
	TArray<FString> Options;

	/** One line each, saying what it means rather than what it does. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Creation")
	TArray<FString> Blurbs;

	/** Who you turned out to be, once every question is answered. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Creation")
	FString WhoYouAre;
};

enum class EDirtbagHandoverStep : uint8
{
	/** What the career was, and what survives it. */
	Epitaph,
	/** Somebody turned up. 1/2/3. */
	Choosing,
	/** Who you are now. */
	Arrived,
};

/**
 * The end of a career, drawn.
 *
 * **This whole system was unreachable.** `RetireAndPassItOn`,
 * `TimeToThinkAboutIt`, `CareerEpitaph`, `GenerationsBefore` and
 * `InheritedGuidebook` are all BlueprintCallable and **nothing called any
 * of them** — no C++ caller, no Blueprint. Phase 4's headline system,
 * measured across ninety years and four generations in the harness, with
 * inheritance rules and save migrations and crew names carrying over,
 * worked perfectly and could not be performed.
 *
 * That is the same written-and-never-wired bug this project has now found
 * at five different layers, and the largest instance of it: the others
 * were a system without a verb, this is a whole phase without a door. The
 * reachability checker cannot see it — it proves *sim* declarations are
 * reachable from the engine, not that engine functions are reachable from
 * play.
 *
 * It also unblocks a gate. Phase 4 asks that *"a career reads like
 * somebody lived there"*, which needs a career **ending in a handover**,
 * and the only evidence for it has been headless because the handover
 * could not be performed at the desk.
 */
USTRUCT(BlueprintType)
struct FDirtbagHandoverReadout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Legacy")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Legacy")
	EDirtbagHandoverStep Step = EDirtbagHandoverStep::Epitaph;

	/** The career, in one paragraph. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Legacy")
	FString Epitaph;

	/** The lines you named, which are the whole of what survives you. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Legacy")
	TArray<FString> Guidebook;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Legacy")
	int32 Generation = 0;

	/** Three people who could turn up. Never a text box — see
	 *  dirtbag::ThreeWhoCouldTurnUp. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Legacy")
	TArray<FString> Candidates;

	/** Who did, once chosen. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Legacy")
	FString Arrival;
};

/**
 * The road, drawn.
 *
 * The 2D game shows the van crossing a background on its way somewhere.
 * The port had fade-to-black, teleport, fade-in — which says *time passed*
 * and nothing else.
 *
 * That matters more than it sounds. **This is the only time the van is on
 * screen as a vehicle** rather than as a thing you sleep in and repair, and
 * the van is what the money is for: it is the dream you save toward, the
 * thing that breaks, the reason a bad night at the fire is felt twice. A
 * fade cannot say any of that. A van crossing a background says *this is
 * the thing you keep alive*, once per trip, for free.
 *
 * It doubles as the one place the game can show a **walk** as different
 * from a drive — no van, no fuel, just twenty minutes of your legs — which
 * is the distinction `Sim/DirtbagZones.h` exists to make.
 *
 * Same shape as every other readout here: the spot fills it, the HUD draws
 * it. The backdrop and the van are asset slots and the screen works
 * without them.
 */
USTRUCT(BlueprintType)
struct FDirtbagTravelReadout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Travel")
	bool bActive = false;

	/** A walk shows no van and costs no fuel. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Travel")
	bool bOnFoot = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Travel")
	FString ToName;

	/** 0 at the near kerb, 1 at the far one. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Travel")
	double Progress = 0.0;

	/** The clock, interpolated for display only. The sim's hours are
	 *  applied once, at the start of the trip — a screen that advanced the
	 *  real clock as it drew would double-count the moment anything else
	 *  read it mid-trip. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Travel")
	double ShownHour = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Travel")
	double Minutes = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Travel")
	double Fuel = 0.0;

	/** What went wrong on the way, if anything. Empty is the usual case. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Travel")
	FString Note;

	/** The road and the van, carried from the spot that owns them — the
	 *  HUD cannot reach a trigger volume, and the assets belong to the
	 *  route rather than to the screen, so the road to the Cave can look
	 *  nothing like the road into town. Either may be null and the screen
	 *  draws without them. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Travel")
	TObjectPtr<class UTexture2D> Backdrop;

	/** Named VanImage rather than Van deliberately: this module already has
	 *  a `Van` on the player state and VanRuns/VanLine/VanNews beside it,
	 *  and a field whose name collides with four others is a field the
	 *  reachability checker reports as used no matter what. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Travel")
	TObjectPtr<class UTexture2D> VanImage;
};

/**
 * The fire's table, drawn.
 *
 * The three campfire games shipped riding on a toast that expires in twelve
 * seconds, which was the campfire note's own stated plan — *"the toast is
 * the whole table UI until an evening of play proves the fire earns a
 * widget"* — and an evening of play proved it (Evan, 2026-08-22:
 * *"definitely will need to improve the fire games"*).
 *
 * What a toast could not do: hold your hand on screen while you think about
 * it, keep the reads beside the decision they inform, show a blackjack hand
 * that takes four keypresses to finish without each press erasing the last,
 * or tell you what the *evening* has cost — which is the number that makes
 * the fire a decision and which nothing tracked at all.
 *
 * Same shape as FDirtbagSessionReadout above and for the same reason: the
 * spot fills it, the HUD draws it, and a real widget later binds to the
 * fields the canvas HUD already uses.
 */
USTRUCT(BlueprintType)
struct FDirtbagFireReadout
{
	GENERATED_BODY()

	/** You are at the fire. The table draws whenever this is true, hand or
	 *  no hand — the point is that you can see what is on offer before you
	 *  commit to it. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	bool bActive = false;

	/** A hand is live and the verbs mean something. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	bool bHandLive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	EDirtbagFiresideGame Tonight = EDirtbagFiresideGame::Cards;

	/** "CARDS", "LIAR'S DICE", "BLACKJACK" — the table's heading. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	FString GameLine;

	/** Your hand as a fraction, for the bar: poker's strength, blackjack's
	 *  total against 21. Negative when the game has no such thing (dice),
	 *  which is how the HUD knows not to draw a bar for it. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	double Yours = -1.0;

	/** "Under your cup:  3 5 5 1 6" / "You: 17, on three cards". The line
	 *  that says what you are holding, in the game's own words. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	FString YoursLine;

	/** What the table is showing: the bid, the dealer's up card, the pot. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	FString TableLine;

	/** The reads and tells, one per line, kept on screen for as long as the
	 *  decision they inform is. A read that has scrolled away is a read you
	 *  did not get. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	TArray<FString> Reads;

	/** What C does and what F does, named for tonight's game — "stay" and
	 *  "throw them in" are not "call" and "let it go". */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	FString CommitVerb;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	FString BackVerb;

	/** What you are putting in, and which of the three notches that is. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	double Stake = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	int32 StakeNotch = 1;

	/** The last hand's sentence, and what it cost. Stays until the next
	 *  deal rather than expiring, so the thing you are deciding about is
	 *  still on screen while you decide. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	FString LastLine;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	double LastDelta = 0.0;

	/** How the evening is going: hands played and the running total. This
	 *  is the number the toast could never show and the only one that turns
	 *  a series of $20 shrugs into a decision about whether to stop. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	int32 HandsTonight = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	double NightDelta = 0.0;
};

/**
 * Where you are climbing. A gym has a thermostat and somebody else's brush;
 * a crag has a shade line, dirt, and lines nobody has done.
 *
 * This is a property of the place, set by the walls and travel spots you
 * actually walk up to, rather than a switch on the game instance — a game
 * instance is not an actor, has no details panel, and cannot be selected in
 * the level, so anything that has to be flipped by hand there is a setting
 * nobody can find.
 */
UENUM(BlueprintType)
enum class EDirtbagVenue : uint8
{
	Gym,
	Crag,
	/** The Shaded Cave: rope routes, north-facing, forty minutes up the
	 *  hill. Outdoors like the Crag in every respect that matters -- the
	 *  window, the guidebook, cleaning, beta -- and different only in which
	 *  rock it loads and which way that rock faces. */
	Cave,
	/** The Sun Terrace: south-facing boulders, high and cold. The winter
	 *  crag -- 42 days in midwinter against the Cave's 28 -- in a window
	 *  half as long, landing at about 1:45pm. Which makes it the venue a
	 *  job costs you the most at: 41 winter windows lost to a nine-to-five
	 *  against the Cave's 35. */
	Terrace
};

/** Is this venue rock rather than plastic? Everything outdoors shares the
 *  weather, the guidebook and the brush; only the gym does not. Written
 *  once so that adding the third venue could not leave a `== Crag` test
 *  behind that quietly means "not the cave either". */
inline bool IsOutdoors(EDirtbagVenue Venue)
{
	return Venue != EDirtbagVenue::Gym;
}

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

	/** Which zone the player is standing in. You wake in the van, so the
	 *  Lot is where a career starts.
	 *
	 *  **Added 2026-08-23 with the wider map, and it fixes a bug the
	 *  widening created.** While there were exactly two walkable zones, a
	 *  travel spot could work out where you were walking *from* by looking
	 *  at where you were walking *to* — if the destination is town you must
	 *  be at the Lot, otherwise you must be in town — and
	 *  `ADirtbagDaySpot::WalkHours` did exactly that. With eleven walkable
	 *  zones that inference is simply false, and it fails silently: the walk
	 *  still costs *a* number, just the wrong one. So where you are is now
	 *  state rather than a guess.
	 *
	 *  Deliberately **not saved yet.** Nothing in the save has ever recorded
	 *  where you were standing, and adding it is a SAVE_VERSION bump for a
	 *  field that only starts mattering when Phase 6 makes the zones real
	 *  places you load back into. Recorded here rather than done quietly:
	 *  today a load puts you at the Lot, which is where the level spawns you
	 *  anyway, so the two agree. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	EDirtbagZone CurrentZone = EDirtbagZone::Lot;

	/** What a zone is called, and what it is. Read by the zone volumes
	 *  that tell the world where the player is standing -- see
	 *  DirtbagZoneVolume.h for the bug they exist to prevent. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Zone")
	FString ZoneNameOf(EDirtbagZone Which) const;

	UFUNCTION(BlueprintPure, Category = "Dirtbag|Zone")
	FString ZoneBlurbOf(EDirtbagZone Which) const;

	/** The creation flow. Active from the first frame of a fresh career
	 *  until all four questions are answered. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Creation")
	FDirtbagCreationReadout Creation;

	/** The comp, while one is being climbed. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FDirtbagCompReadout Comp;

	/** Days until the next one, or -1 outside the announcement window. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Comp")
	int32 DaysUntilComp() const;

	/** Is one on at the gym today? */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Comp")
	bool CompIsToday() const;

	/** Sign in. Costs the entry fee and the day. False if there is no comp,
	 *  you cannot pay, or you are already in one. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Comp")
	bool EnterComp();

	/** Spend one of your seven on problem `Which` (zero-based). */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Comp")
	bool CompAttempt(int32 Which);

	/** Turn in the scorecard. Ranks the field, places you, pays out. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Comp")
	bool SettleComp();

	/** What the gym has on a poster. Empty outside the window. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Comp")
	FString CompLine() const;

	/** Where you stand nationally, and how far to the next rung. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Comp")
	FString RankLine() const;

	/** Where you are in the season. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Comp")
	FString CircuitStandingLine() const;

	/** A season ended. Slow news, said once. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FString CompNews;

	/** The committee named you, or did not. Said once, at a season's close. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FString TeamNews;

	/** Whether your name is on the paper, and who has the squad. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Comp")
	FString TeamStandingLine() const;

	// --- the top of the ladder -----------------------------------------
	//
	// A World Cup season runs from the first night of a career whether or
	// not the player has heard of it, and the Games come round on a cycle.
	// Everything here is a door onto that; the rules themselves live in
	// Sim/DirtbagWorldStage.h, where they can be tested.

	/** Where the season is, or what is coming, or what it cost to stay
	 *  home. Empty before a career has a season. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|World")
	FString WorldCupLine() const;

	/** Days until the next round, or -1 outside the published window. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|World")
	int32 DaysUntilWorldCupRound() const;

	/** Whether you can get on the plane today, why not, and what the
	 *  ticket costs. `Round` is -1 when there is simply no round on --
	 *  silence rather than a refusal. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|World")
	FDirtbagFlightCheck CanFlyToday() const;

	/** Where today's round is, if there is one. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|World")
	FDirtbagWorldCupVenue RoundVenue() const;

	/** Get on the plane. Takes the ticket and the day, and puts you on a
	 *  board set at the world standard rather than at your grade. False
	 *  when `CanFlyToday` says so. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|World")
	bool FlyToTheRound();

	/** What the Games have to say, which is nothing at all until they are
	 *  close or you have been to one. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|World")
	FString GamesLine() const;

	/** Are they on today, and are you in? */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|World")
	bool GamesAreToday() const;

	/** Empty when you may start; otherwise why not. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|World")
	FString WhyNotTheGames() const;

	/** Start the final. Costs the day and nothing else -- the federation
	 *  got you here. False unless `WhyNotTheGames` is empty. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|World")
	bool EnterTheGames();

	// --- what is wrong with you ------------------------------------------
	//
	// Phase 3's injury was a wait with a price tag. This is the rest of it,
	// and the shape is one sentence: **an injury hides its grade until you
	// pay to look at it.** The rules live in Sim/DirtbagMedical.h, where
	// they can be tested; these are the doors onto them.

	/** What you can say about it, which depends on what you have paid to
	 *  know. Deliberately vague until you have -- that is the mechanic,
	 *  not a missing string. Empty when nothing is wrong. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Medical")
	FString MedicalLine() const;

	/** What the joints have been through. Empty when nothing has. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Medical")
	FString BodyHistoryLine() const;

	/** What a treatment costs you today, after cover. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Medical")
	double PriceOf(EDirtbagTreatment What) const;

	/** And what finding out costs: the hands, or the machine. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Medical")
	double PriceOfLook(bool bScan) const;

	/** Pay somebody to feel it. Cheap, close, and not always right. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Medical")
	bool SeeSomebody();

	/** Pay for the machine. Exact, expensive, and the only thing that
	 *  unlocks an operation. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Medical")
	bool GetItScanned();

	/** The shot. Works now; marks the joint forever. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Medical")
	bool TakeTheShot();

	/** The operation. Needs a scan and a bad enough injury. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Medical")
	bool BookTheSurgery();

	/** Move to the next stage of the comeback. **The decision with a wrong
	 *  answer**: early is a gamble, and the chance rises with how early. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Medical")
	bool PushOn();

	/** Is the current stage actually done? Only honest to show the player
	 *  when they have paid to know -- see `MedicalLine`. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Medical")
	bool ComebackStageIsDone() const;

	/** Take out a policy, or drop it. Refused while you are hurt. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Medical")
	bool BuyInsurance();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Medical")
	void CancelInsurance();

	/** What the policy has cost and what it has paid back. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Medical")
	FString InsuranceLine() const;

	/** A comeback stage passed, a setback, a scar. Said once. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Medical")
	FString MedicalNews;

	// --- and the rest of what is wrong with you --------------------------
	//
	// Every injury in this game is something you did. **These are
	// deliberately not** -- see Sim/DirtbagAilments.h.

	/** Being ill. Empty when you are not. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Medical")
	FString SickLine() const;

	/** Eleven dollars at the counter. The cheapest decision in the game,
	 *  and it is a decision anyway. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Medical")
	bool TakeSomethingForIt();

	/** The tooth, and what it costs to make it go away *now*. Empty when
	 *  there is nothing wrong with it. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Medical")
	FString TeethLine() const;

	/** What the dentist wants today. It only ever goes up. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Medical")
	double ToothPrice() const;

	/** Pay it. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Medical")
	bool FixTheTooth();

	/** Twenty minutes of a morning. Costs the time and nothing else, and
	 *  it is the only thing here that makes the odds better. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Medical")
	bool DoPrehab();

	/** The streak, and what it is worth. Empty until there is one. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Medical")
	FString UpkeepLine() const;

	/** An hour of talking about it. **The only thing in the game that
	 *  buys psyche back**, and it moves where psyche drifts to overnight,
	 *  which a rest day cannot. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Medical")
	bool SeeTheShrink();

	// --- the league ----------------------------------------------------
	//
	// The other end of the same system. Five dollars, ten goes, no ranking
	// points at all -- see Sim/DirtbagLeague.h for why that is the design
	// rather than an omission.

	/** What is on the gym whiteboard. Empty unless it is close. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|League")
	FString LeagueLine() const;

	/** Is it on tonight? */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|League")
	bool LeagueIsTonight() const;

	/** Sign in. Costs five dollars and the evening, and puts you on five
	 *  problems at your own grade with ten goes. False if there is no
	 *  league tonight, you cannot pay, or you are already on a board. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|League")
	bool EnterLeague();

	/** Where the block stands, and what your best is. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|League")
	FString LeagueStandingLine() const;

	/** A personal best, a night won, or a block finished. Said once. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|League")
	FString LeagueNews;

	/** A World Cup season ended. Slow news, said once. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	FString WorldCupNews;

	/** A medal, or a Games you were at. Said once. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	FString GamesNews;

	/** Live session readout for the HUD; the wall keeps this current. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FDirtbagSessionReadout SessionReadout;

	/** Set the first time the player latches a move, and never unset.
	 *  Lives here rather than on the session readout because the readout
	 *  is cleared at the end of every attempt, and a control reminder that
	 *  reappears on attempt fourteen is the toast's problem with extra
	 *  steps. Deliberately not saved: it is worth one latch to re-earn,
	 *  and a save version bump for a tutorial flag is not.
	 *
	 *  Set from C++ on the wall, so nothing in Blueprint has to remember
	 *  to. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	bool bLearnedTheVerb = false;

	/** The crag's page. Any spot or wall opens it with G. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FDirtbagGuidebookReadout Guidebook;

	/** The frame the book was last toggled on, so overlapping triggers
	 *  cannot toggle it twice with one keypress. Not a UPROPERTY: it is
	 *  frame bookkeeping and has no business in a save or a details
	 *  panel. */
	uint64 GuidebookToggledOnFrame = 0;

	/** Open or close the book, rebuilding the page for wherever you are.
	 *  Returns true if it is now open. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Guidebook")
	bool ToggleGuidebook();

	/** Everything / projects / in reach. Rebuilds the page. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Guidebook")
	void SetGuidebookView(EDirtbagGuidebookView View);

	/** The end of a career. The van raises it; the HUD draws it. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FDirtbagHandoverReadout Handover;

	/** The road; a travel spot keeps this current while you are on it. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FDirtbagTravelReadout TravelReadout;

	/** The fire's table; the fire spot keeps this current. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FDirtbagFireReadout FireReadout;

	/** What the keys do where you are standing. Spots and walls fill it
	 *  while you are inside them. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FDirtbagPrompt Prompt;

	/** Whoever last set the prompt. A spot clears the prompt only if it
	 *  still owns it, so walking from one overlapping trigger into another
	 *  cannot leave you looking at a blank where the second one's prompt
	 *  should be — or worse, at the first one's. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	TObjectPtr<AActor> PromptOwner;

	/** Take the prompt, replacing whatever was there. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Prompt")
	void SetPrompt(AActor* Owner, const TArray<FDirtbagPromptLine>& Lines);

	/** Give it back, but only if you still hold it. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Prompt")
	void ClearPrompt(AActor* Owner);

	/** True when the current Player came from disk rather than a fresh start. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	bool bLoadedFromSave = false;

	// --- Day verbs (mutating members must live here: Blueprint struct
	// --- access returns copies, so the owner does the mutating) ----------

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	bool EatMeal();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	void PassHours(double Hours);

	/** Sit it out. Hours pass, hunger with them, and a little energy comes
	 *  back — the shade, not a night's sleep. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	void Rest(double Hours);

	/** Hours until today's window opens: 0 if it is already open, has been
	 *  and gone, or the day never comes good at all. The thing waiting is
	 *  actually for. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	double HoursUntilWindow();

	/** "the rock comes good at 3:45pm" / "it is on, right now" / "today is
	 *  not going to happen". */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	FString WaitAdvice();

	/** First pull-on of the day seeds the session; later calls no-op. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	void EnsureAtGym();

	/** Lights out: sim sleep, then save — the save file's one write point. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	void Sleep();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	bool SaveNow();

	// --- Conditions ------------------------------------------------------
	// Weather is derived from Seed + Day, so it is never saved and cannot
	// drift from a reload. Aspect is a property of the place you are
	// climbing; the gym is indoors and ignores all of this.

	/** Which way the current crag faces. Set from the crag itself the moment
	 *  one is loaded, so the guidebook and the shade line cannot disagree;
	 *  change it afterwards to feel out how aspect moves the window. Indoors
	 *  it is ignored — a gym has no shade line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Conditions")
	EDirtbagAspect CragAspect = EDirtbagAspect::North;

	/** True while the player is climbing indoors, where conditions are a
	 *  thermostat rather than a decision. Set by SetVenue, which the walls
	 *  and travel spots call — do not expect to find this in a details
	 *  panel, because a game instance does not have one. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	bool bIndoors = true;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	EDirtbagVenue Venue = EDirtbagVenue::Gym;

	/** Arrive somewhere. Cheap and idempotent — the walls call it every time
	 *  you walk up to one, so where you are can never drift from what you
	 *  are standing in front of. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	void SetVenue(EDirtbagVenue NewVenue);

	/** The gym's flat, boring friction. Slightly under outdoor prime on
	 *  purpose: plastic in a warm room is never actually good. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Conditions")
	double IndoorFriction = 0.5;

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	FDirtbagWeather TodaysWeather() const;

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	FDirtbagPrimeWindow TodaysWindow() const;

	/** Friction right now — indoors, the thermostat; outdoors, the rock. This
	 *  is what every attempt this instance starts is resolved against. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	double CurrentFriction() const;

	/** "greasy - 61F on the rock. window 5:30pm to 7:15pm, best at 6:30pm" */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Conditions")
	FString ConditionsLine() const;

	// --- What you are climbing on ----------------------------------------
	// One index, two venues: indoors it selects a gym problem, outdoors a
	// line from the guidebook. Walls carry only the index, so moving a wall
	// between venues is a flag rather than a rebuild.

	/** The gym's board for this world, cached. Indoor venue only. */
	// blueprint-only: an accessor for Blueprint; C++ reads Crag.Lines directly
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	TArray<FDirtbagRoute> GetBoard();

	/** The route at this index in whichever venue is live. Prefer
	 *  GetRouteAt from anything that knows its own venue: this one reads
	 *  global state, which is only correct once the player has arrived. */
	// blueprint-only: an accessor for Blueprint; C++ uses GetRouteAt
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FDirtbagRoute GetBoardRoute(int32 Index);

	/** The route at this index in a named venue, independent of where the
	 *  player currently is. A wall resolves its route at BeginPlay, long
	 *  before anyone has walked up to it, so asking "where am I?" at that
	 *  moment gives the wrong answer — and gave a crag wall a gym problem. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FDirtbagRoute GetRouteAt(EDirtbagVenue AtVenue, int32 Index);

	/** This world's crag, cached. Outdoors only. */
	// blueprint-only: an accessor for Blueprint; C++ reads the Crag member
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Crag")
	FDirtbagCrag GetCrag();

	/** The guidebook entry at this index — stars, project status, the lot.
	 *  Meaningless indoors, where a gym problem has no book. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Crag")
	FDirtbagCragLine GetCragLine(int32 Index);

	/** A line at a named outdoor venue, so a wall can ask for cave rock
	 *  while the player is still standing at Roadside. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Crag")
	FDirtbagCragLine GetCragLineAt(EDirtbagVenue AtVenue, int32 Index);

	/** How long it takes to get to this venue's rock, in hours, from the
	 *  guidebook rather than from the level.
	 *
	 *  `Crag::approachHours` was set by all three crags (0.5, 0.7, 0.9) and
	 *  read by nothing, while the travel spot carried a hand-typed number
	 *  that meant the same thing. Two copies of one fact, and the level's
	 *  was the one that counted — so the cave's forty minutes up the hill
	 *  was true only if somebody remembered to type it.
	 *
	 *  Returns a negative number indoors, where there is no rock and the
	 *  spot's own Travel Hours is the right answer. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Crag")
	double ApproachHoursFor(EDirtbagVenue AtVenue);

	/** How many things there are to climb where you are standing. */
	// blueprint-only: an accessor for Blueprint; C++ reads Crag.Lines.Num()
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	int32 NumRoutesHere();

	/** Bot-driven burn on a route, day-integrated (session, ledger, time,
	 *  energy, training all move together). */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FDirtbagAttemptResult ReplayAttempt(const FDirtbagRoute& Route);

	// --- First ascents ---------------------------------------------------
	// clean -> work -> send -> name. No locks: a virgin line is simply
	// filthy, and filthy rock climbs about four grades harder than it will
	// once you have spent an afternoon on it with a brush.

	/** An hour on the brush at the line this wall points at. Costs the day's
	 *  time and energy, so cleaning competes with climbing. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	double CleanLine(int32 BoardIndex, double Hours);

	/** The one line the player has to be told, because it is the difference
	 *  between losing the valley's projects and taking them. Brushing a line
	 *  puts your name on it: SpokenFor counts a brushed project as claimed,
	 *  so the Lot leaves it alone even if you cannot climb it for another ten
	 *  years. Measured over ninety years and six seeds, a player who brushes
	 *  the lines above their grade ends 16 first ascents to the Lot's 7; one
	 *  who only brushes what they can already climb ends 6 to 15
	 *  (notes/phase4-staking-a-claim.md). Nothing in the game said so.
	 *
	 *  Empty until the brushing crosses the threshold, so it is news exactly
	 *  once per line rather than a label. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	FString ClaimText(int32 BoardIndex, double GainedThisPress);

	/** Whether it is worth pulling on yet. */
	// blueprint-only: an accessor for Blueprint; the wall reads cleanliness directly
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	bool IsWorkable(int32 BoardIndex);

	/** "filthy; you can find the holds but not use them" */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	FString CleanlinessText(int32 BoardIndex);

	/** True when you have done a line nobody had done, and the naming is
	 *  therefore yours. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	bool CanNameLine(int32 BoardIndex);

	/** Name it. Records the claim and confirms what it really went at —
	 *  which nobody, including the guidebook, knew until now. */
	// blueprint-only: WBP_NameFirstAscent calls it; the HUD only raises the flag
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	bool NameFirstAscent(int32 BoardIndex, const FString& Name);

	/** "Roadside Rites  V8  FA you (the book said V7)" — empty if unnamed. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	FString FirstAscentLine(int32 BoardIndex);

	// --- The naming prompt -----------------------------------------------
	// Typing a name needs a text box, and a text box means UMG. Rather than
	// bind C++ to one widget class, the wall raises a flag here and a widget
	// blueprint watches it — so the look of the prompt stays entirely in the
	// editor and this layer only says "there is a line waiting to be named".

	/** True from the moment a nameable line goes until the name is given or
	 *  the prompt is dismissed. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|FirstAscent")
	bool bNamingPending = false;

	/** Which line is waiting. Pass this straight back to NameFirstAscent. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|FirstAscent")
	int32 NamingBoardIndex = 0;

	/** "the arete left of Diesel" — what to show above the text box. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|FirstAscent")
	FString NamingLineText;

	/** "Bouncin  V4  FA you" — the line as the book will print it, set the
	 *  moment a name is given and carried for the rest of the day. Not
	 *  saved: it is an acknowledgement, and the ascent itself lives in the
	 *  ledger. Empty when nothing has been named today. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|FirstAscent")
	FString LastAscentLine;

	/** Raised by the wall when a line nobody had done goes. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	void OfferNaming(int32 BoardIndex);

	/** Walk away without naming it. The line stays yours to name later —
	 *  the ascent happened, and nothing about it expires. */
	// blueprint-only: WBP_NameFirstAscent calls it when you walk away from the prompt
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	void DismissNaming();

	/** Where this session stands: warm enough, skinned enough, or done.
	 *  Meaningful once you have pulled on today. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	EDirtbagSessionAdvice ReadSession() const;

	/** "still cold — pull on something easy first" */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FString SessionAdviceText() const;

	// --- Gear and the van ------------------------------------------------

	/** "the rubber is going; worth a resole" */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Gear")
	FString ShoeLine() const;

	/** What the rubber is costing you right now, in grades on edging holds.
	 *  0 on a new pair.
	 *
	 *  Worth showing because it is worth a lot and has never been visible:
	 *  measured over ten seasons, a climber who never replaces their shoes
	 *  sends 1.0 against 3.7 on the same burns, for about $9 of the year's
	 *  cash. The cheapest thing in the game that matters. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Gear")
	double ShoeCostInGrades() const;

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Gear")
	bool ResoleShoes();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Gear")
	bool BuyNewShoes();

	// --- Ethics ----------------------------------------------------------
	// An ethical shortcut is not a transaction, it is a secret with a fuse.
	// You take the benefit now and carry a thing that can come out — and
	// the more people are watching you, the more likely it is that somebody
	// noticed. Success is what exposes you.

	/** Do it. Records only that it happened; the caller applies whatever
	 *  the act buys, because the benefit differs per act and belongs where
	 *  it is felt. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Ethics")
	void DoSomethingYouWouldNotAdmitTo(EDirtbagEthicalAct Act,
	                                   const FString& OnRoute);

	/** How closely you are watched, 0..1 — Scene standing plus what a
	 *  sponsor has made of you. This is the number that decides how long
	 *  you get away with it. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Ethics")
	double HowWatchedYouAre() const;

	/** How many things you are carrying that nobody knows about. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Ethics")
	int32 ThingsNobodyKnows() const;

	/** Can this shortcut be taken on this line, right now?
	 *
	 *  Each act asks something different of the situation: you cannot pull
	 *  through on a line you never got on, and there is no point chiselling
	 *  one you have already done. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Ethics")
	bool CanTakeShortcut(EDirtbagEthicalAct Act, int32 BoardIndex);

	/** Take it, and apply what it buys.
	 *
	 *  `DoSomethingYouWouldNotAdmitTo` records only that it happened,
	 *  because the benefit differs per act — this is where the benefit
	 *  lives, and it is deliberately the exact mirror of what stripping
	 *  takes away when the truth comes out. Returns what gets said. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Ethics")
	FString TakeShortcut(EDirtbagEthicalAct Act, int32 BoardIndex);

	/** Roll for the day. Returns the line to show when something surfaces,
	 *  empty otherwise — "Everyone knows you chipped a hold on Chalk Ghost
	 *  now. It was nine years ago. It does not matter that it was."
	 *
	 *  Called from Sleep, so a secret comes out the way a player meets one:
	 *  by waking up to it. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Ethics")
	FString DoesAnybodyFindOutToday();

	// --- Sponsorship -----------------------------------------------------
	// The only money in the game that arrives because you climbed rather
	// than instead of it — and the only money whose cost is measured in
	// good days, because you cannot shoot climbing photos in the rain.

	/** What would be offered right now, given what people can see: what you
	 *  have sent, what you have put up, and what the Scene thinks. Ability
	 *  is not the currency — a crusher nobody has heard of gets nothing. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Sponsor")
	EDirtbagSponsorTier OfferOnTheTable() const;

	/** What the offer on the table is worth, in words — what it pays and
	 *  what it will want. Empty when nobody is asking. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Sponsor")
	FString WhatTheyAreOffering() const;

	/** Take it. Says something about you: the Scene likes a sponsored
	 *  climber and the old guard has opinions. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Sponsor")
	bool SignWithSponsor();

	/** "free shoes, and they want nothing" / "$640 a month, and they want
	 *  three days — and they will not be the rainy ones" */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Sponsor")
	FString SponsorLine() const;

	/** Do they own today? Only ever a day with a window. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Sponsor")
	bool SponsorOwnsToday() const;

	// --- Retiring --------------------------------------------------------

	/** Whose career this is. Set when the player names themselves; used on
	 *  the epitaph and on every line they put up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Legacy")
	FString ClimberName;

	/** Injuries back to back, which says it before the numbers do. Reset by
	 *  a season that does not end in one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Legacy")
	int32 ConsecutiveInjuries = 0;

	/** Bookkeeping for the streak above, ticked at Sleep. Not saved: a
	 *  reload resets a streak, which is a smaller wrong than a save
	 *  migration for two counters, and the streak re-earns itself in a
	 *  season. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Legacy")
	bool bWasHurtYesterday = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Legacy")
	int32 DaysSinceHurt = 0;

	/** The best this career ever was, so decline is measured against it
	 *  rather than against an age. Plenty of people climb their hardest at
	 *  forty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Legacy")
	double PeakGradeEver = 0.0;
	// Offered, never forced. Deciding when to stop is the last real choice a
	// climbing career contains, and taking it away would be the one
	// unforgivable thing to do to one.

	/** Has the game got something honest to say about stopping? Never a
	 *  command — a body that keeps breaking, or two grades off your best and
	 *  past the age. Age alone is never the reason. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Legacy")
	bool TimeToThinkAboutIt() const;

	/** "9 seasons. Hardest: The Guidebook Lied, V7. Two lines that are yours
	 *  now. Chalk Ghost never went, after 210 tries. Retired at 47, with the
	 *  crag open." */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Legacy")
	FString CareerEpitaph() const;

	/** End it. Tallies what the career was, files it with the ones before,
	 *  and hands the valley to somebody twenty-four with nothing in their
	 *  fingers — because the world remembers and the body does not. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Legacy")
	void RetireAndPassItOn(const FString& RetiringAs);

	/** Three people who could turn up, for the generation after this one.
	 *  Stable across a reload; different every handover. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Legacy")
	TArray<FString> WhoCouldTurnUp() const;

	/** Name the climber. Used by the handover, and by a fresh career that
	 *  has never been named — without it every first ascent in the game
	 *  was signed "you" and the crew hash was fed an empty string. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Legacy")
	void NameTheClimber(const FString& Name);

	/** Open the four questions. Called on a fresh career; a loaded one whose
	 *  climber was already built skips it, which is what makes old saves
	 *  work — see BeginCreation's body. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Creation")
	void BeginCreation();

	/** Answer the question on screen. `Which` is zero-based; the key the
	 *  player pressed is one more than that. Returns false when creation is
	 *  not up or the index names nothing, so the caller knows the key was
	 *  not spent here. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Creation")
	bool ChooseInCreation(int32 Which);

	/** Rebuild the on-screen options for the current question. */
	void RefreshCreation();

	/** Armed by `TakeTheGigTheHardWay`, consumed by the next gig. Not
	 *  saved: it is a decision about a shift you are standing in front of,
	 *  and a reload puts you back in front of it. */
	bool bTakeTheHardWay = false;

	/** Rebuild the comp board from the live sim state. */
	void RefreshComp();

	/** Everything the climber carries into any attempt, anywhere -- the
	 *  flaw, the temperament, the joints, the flu, the tooth, the rubber.
	 *  One place, because the comp resolver used to assume a clean body of
	 *  everybody and nobody noticed. See Sim/DirtbagBodyContext.h. */
	dirtbag::BodyContext TheBodyYouWalkedInWith() const;

	/** Put a result on the ranking record and refresh the derived total.
	 *  The one way `RankingPoints` is allowed to move from inside a day --
	 *  see the field's own note in Sim/DirtbagDay.h. */
	void RecordResult(double Points);

	/** Turn in a league scorecard. Its own function for the same reason
	 *  the world stage has one: it banks into a block table and a personal
	 *  best and touches neither the circuit nor the ranking. */
	bool SettleTheLeague();

	/** Turn in a World Cup or Games scorecard. Split out of `SettleComp`
	 *  rather than branching inside it: the domestic settle banks into the
	 *  circuit, moves the ranking and lets the committee sit, and none of
	 *  those three things is true up here. */
	bool SettleTheWorldStage();

	/** The average of your five skills, on the grade ladder. The number the
	 *  rival chases, and the one the HUD compares them to -- computed once
	 *  here rather than in three places that would drift. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag")
	double AllroundGrade() const;

	/** What the counter charges this climber, as a multiplier. One for
	 *  everybody except the Trust-Fund Kid, whose family money still quietly
	 *  covers it. Read here rather than inside the sim, because a price is a
	 *  fact about the shop and who is standing at it. */
	double ShopPrice() const;

	/** How many came before. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Legacy")
	int32 GenerationsBefore() const;

	/** The guidebook lines earlier generations put up: "Cattle Grid Arete,
	 *  V7. FA Evan". These are the whole of what survives a retirement. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Legacy")
	TArray<FString> InheritedGuidebook() const;

	// --- Age -------------------------------------------------------------
	// Derived from the day counter, never stored — which is why adding it
	// needed no save version, and why a save can never disagree with a
	// birthday.

	UFUNCTION(BlueprintPure, Category = "Dirtbag|Body")
	double Age() const;

	/** "31 — still going up" / "44 — the good years, if you are careful" */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Body")
	FString AgeLine() const;

	// --- Where you stand -------------------------------------------------

	/** "the crag is closed. The signs went up on the gate." — or empty when
	 *  nobody has an opinion about you yet. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Standing")
	FString StandingLine() const;

	/** -1 they will not have you .. +1 you are one of theirs. */
	// blueprint-only: an accessor for Blueprint; the HUD prints StandingLine
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Standing")
	double StandingWith(EDirtbagFaction Faction) const;

	/** False when the signs are up. Push the stewards far enough and access
	 *  gets pulled — for nine days, which is a season and not a sentence. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Standing")
	bool CragIsOpen() const;

	// --- Work ------------------------------------------------------------

	/** Today's board: three gigs, deterministic per world and day. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Work")
	TArray<FDirtbagOddJob> TodaysJobBoard() const;

	// --- work as a craft -------------------------------------------------
	//
	// A lever you pull for money is a lever. A trade you are getting better
	// at is a life. See Sim/DirtbagCraft.h.

	/** What came up on the shift you are about to take, if anything. Empty
	 *  when nothing did -- and it is read *before* the gig, because the
	 *  answer is part of taking it. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Work")
	FString ShiftMomentLine(const FDirtbagOddJob& Job) const;

	/** The harder answer, and what it needs. Empty when nothing came up. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Work")
	FString TheHardWayLine(const FDirtbagOddJob& Job) const;

	/** Arm the next `TakeOddJob` to answer the shift's decision the hard
	 *  way. **Reaching past your craft is how you botch it**, and the easy
	 *  answer is always there and never gets you anywhere. Cleared by the
	 *  gig, so it can never leak into the next one. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Work")
	void TakeTheGigTheHardWay();

	/** What the trades know about you. Empty before you have any. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Work")
	FString CraftLine() const;

	/** "Somebody who does setting, and climbs." The work identity, and
	 *  which way round it goes depends on how much of each you have.
	 *  Empty until one trade is enough of you to say so. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Work")
	FString TradeLine() const;

	/** What happened on the last shift. Said once. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	FString WorkNews;

	/** Take a gig. Costs the hours and the energy, pays into debt first,
	 *  and says something about you — the best-paying gig on the board is
	 *  shooting guidebook photos, and it costs you the old guard and the
	 *  stewards both. False if the van is dead and the gig needed it. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Work")
	bool TakeOddJob(const FDirtbagOddJob& Job);

	/** "2 Dirtbag Years.  118 days into another." Empty until the first
	 *  whole year — a streak of eleven days is a fortnight, not a record.
	 *
	 *  The counterweight to the salaried trap. The trap costs you the hours
	 *  the day was for and pays in money; until this existed, the only
	 *  thing refusing it bought was the absence of a cost, which is not
	 *  something a player can feel. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Work")
	FString DirtbagYearLine() const;

	/** Said once, on the morning a year completes. Empty every other day. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	FString DirtbagYearNews;

	// --- The fire ---------------------------------------------------------
	// SitAtTheFire gives rapport, psyche and a line of talk for spending
	// hours there, and asks nothing: no decision, no stake, nothing you can
	// be bad at. This is the verb. Measured, a player who never folds loses
	// about $5 a hand and one who reads the table wins $3 as a stranger and
	// $10 at full rapport — so the skill is knowing people, which is the
	// only skill the Lot was ever going to teach.

	/** Deal one. Hand numbers count up through an evening; the same day and
	 *  number always deal the same cards, so a reload cannot reroll a bad
	 *  night. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Campfire")
	FDirtbagCampfireHand DealCampfireHand(int32 HandNumber);

	/** Play it. Stake is what you put in on top of the ante; folding
	 *  forfeits the ante and nothing else, and is still an evening spent
	 *  with people. Returns what gets said. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Campfire")
	FString PlayCampfireHand(int32 HandNumber, double Stake, bool bFold);

	/** What the last hand did to your pocket. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	double LastHandCash = 0.0;

	// Liar's dice. Where poker asks what somebody has, this asks whether
	// they are lying and whether you dare say so. Measured: passing costs
	// the ante, calling everything loses $22 a round, reading a stranger
	// loses $7.7, and reading somebody you know wins $6.9.

	/** The bid as it reaches you. One decision: call it, or pass it on. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Campfire")
	FDirtbagLiarsDice DealLiarsDice(int32 RoundNumber);

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Campfire")
	FString PlayLiarsDice(int32 RoundNumber, double Stake, bool bCall);

	// Blackjack. No read, no rapport, no bluff — the game for a climber who
	// has just turned up and knows nobody, which is a real state here and
	// one that nothing else pays off. Measured: played well it costs you the
	// ante and nothing more.

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Campfire")
	FDirtbagBlackjack DealBlackjack(int32 HandNumber);

	/** Take another. Returns the card; the hand comes back through
	 *  `LastBlackjack`. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Campfire")
	int32 HitBlackjack(int32 HandNumber);

	/** Stop, and settle. The deck plays itself out to seventeen. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Campfire")
	FString StandBlackjack(int32 HandNumber, double Stake);

	/** The hand in progress — Hit and Stand both work on this. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	FDirtbagBlackjack LastBlackjack;

	/** What is out tonight. The day decides — you join what is being played
	 *  rather than ordering off a menu — so this is asked, never set. Lived
	 *  in the fire spot as four separate `Day % 3` expressions until the
	 *  table was built. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Campfire")
	EDirtbagFiresideGame WhatsOutTonight() const;

	/** "cards" / "liar's dice" / "blackjack", lower case for the middle of
	 *  a sentence. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Campfire")
	FString FiresideGameName(EDirtbagFiresideGame Which) const;

	/** What notch 0, 1 or 2 puts on the table, from the campfire dials. The
	 *  fire's 1/2/3 keys are these, so retuning `maxStake` moves them. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Campfire")
	double FireStake(int32 Notch) const;

	// --- Dreams -----------------------------------------------------------
	// The thing the money is for. Measured (notes/dreams-what-money-is-worth
	// .md): saving costs no climbing at all — a career that banks hard climbs
	// 970 days against 422 — so a dream priced only in money would be a timer.
	// The price is the buffer. You spend the float that was keeping the van
	// alive, and the reward arrives with a lean, fragile year attached.

	/** What each dream costs. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Dreams")
	double DreamCost(EDirtbagDream Which) const;

	/** Its name, and what it is actually for. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Dreams")
	FString DreamName(EDirtbagDream Which) const;

	UFUNCTION(BlueprintPure, Category = "Dirtbag|Dreams")
	FString DreamBlurb(EDirtbagDream Which) const;

	/** Can you put the money down today? Says nothing about whether you
	 *  should — and you almost never should, which is the point. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Dreams")
	bool CanAffordDream(EDirtbagDream Which) const;

	/** Name your dream. Once per career, and it holds: the other two stop
	 *  being for sale the moment you say it. False if this career already
	 *  chose. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Dreams")
	bool ChooseDream(EDirtbagDream Which);

	/** Buy it. Takes the cash and hands back the thing. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Dreams")
	bool BuyDream(EDirtbagDream Which);

	/** "the Rig.  212 days of the War Chest left." Empty until you own one. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Dreams")
	FString DreamLine() const;

	/** Said once, on the day you buy one. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dreams")
	FString DreamNews;

	/** "Nothing needs doing today." The counterpart to the debt line, and
	 *  the only thing in the game that says a day is entirely yours — the
	 *  War Chest is a year of them, bought in advance. Empty otherwise,
	 *  including when you merely happen to be flush: being able to afford
	 *  today is not the same as not having to think about it. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Dreams")
	FString FreeDayLine() const;

	/** "They call you the trail crew." Empty until the town says it — which
	 *  takes two people you actually climb with, for a month. You do not
	 *  pick this and you cannot change it. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Crew")
	FString CrewLine() const;

	/** Said once, on the morning the name lands. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crew")
	FString CrewNews;

	/** Nine to five, five days a week. The hours are the point, not the
	 *  money — and in midwinter the light is gone before you clock off.
	 *
	 *  Returns the days of streak this cost, so the game can be honest at
	 *  the moment of signing rather than in a summary nobody reads. Zero if
	 *  there was nothing to lose. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Work")
	int32 TakeSalariedJob();

	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Work")
	void QuitSalariedJob();

	/** Does the salary own today? */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Work")
	bool SalariedToday() const;

	/** The permanent position, as the board advertises it — or, once you
	 *  hold it, what leaving would cost. Both halves stated, always: the
	 *  design is that taking it is *reasonable*, so the game must not
	 *  editorialise, only be accurate. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Work")
	FString SalaryLine() const;

	/** Work it. Called from Sleep on any day the salary owns, not offered
	 *  as an action — a trap you can decline is not a trap. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Work")
	void WorkSalariedDay();

	// --- The body --------------------------------------------------------

	/** "a pulley in the ring finger — 3 weeks, if you are sensible", or
	 *  empty when nothing is wrong. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Body")
	FString InjuryLine() const;

	/** "carrying a load" / "everything aches; this is the warning". The
	 *  warning is worth listening to: measured over twelve seasons, a
	 *  climber who stops training at it sends 38 against 24 for one who
	 *  does not, and spends 55 days hurt against 1,467. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Body")
	FString LoadLine() const;

	/** How loudly to say it: 0 nothing worth saying, 1 worth noticing, 2 the
	 *  warning before an injury. The bands are the sim's, so the colour and
	 *  the sentence can never disagree. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Body")
	int32 LoadWarning() const;

	UFUNCTION(BlueprintPure, Category = "Dirtbag|Body")
	bool IsHurt() const;

	/** Money for time: takes days off an injury, once a week at most. The
	 *  one thing money buys that hands climbing back rather than moving it
	 *  around. False if you are not hurt, cannot afford it, or saw one too
	 *  recently. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Body")
	bool SeeAPhysio();

	/** What the physio would be, said at the counter: the price, or how
	 *  many days until they will see you again. Empty when you are not
	 *  hurt, because a healthy climber does not need telling. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Body")
	FString PhysioLine() const;

	// --- The kit ---------------------------------------------------------
	// Things worth money that buy you climbing. Each returns false if you
	// cannot afford it, and changes nothing when it does.

	/** A second pad. Measured across twelve seasons, both pads against one
	 *  is +37% sends at identical skin spend — it does not give you more
	 *  climbing, it makes the climbing you already had worth more. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Kit")
	bool BuyCrashPad();

	/** Plywood above the van door. Worth an hour on a day the weather has
	 *  already taken, and a mistake on any other — it spends the skin the
	 *  crag was waiting for. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Kit")
	bool BuyHangboard();

	/** Buys the next rung of the rack: nuts, then cams, then doubles.
	 *  False and unchanged when you cannot afford it or there is nothing
	 *  above what you have. The only purchase in the game that unlocks a
	 *  discipline rather than improving one. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Trad")
	bool BuyTradRack();

	/** A month of plastic. The only climbing that ignores the weather. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Kit")
	bool RenewGymMembership();

	/** The membership at the counter: what is left on it, or what it
	 *  costs. Always says something — unlike the physio line, because
	 *  whether you are a member is a standing fact rather than a symptom. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Kit")
	FString MembershipLine() const;

	/** The hangboard: the broke answer to a day you cannot climb. Empty
	 *  once you own one, because it is bought once and never again. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Kit")
	FString HangboardLine() const;

	/** What the shelf is offering a leader today, or empty once you have
	 *  doubles and there is nothing above it. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Trad")
	FString RackOfferLine() const;

	UFUNCTION(BlueprintPure, Category = "Dirtbag|Kit")
	bool IsGymMember() const;

	/** "one pad, a board in the van, and the gym until the month runs out" */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Kit")
	FString KitLine() const;

	/** "A second pad, $260. Better landings, and one less reason to be
	 *  brave." Empty once you own the pads that matter.
	 *
	 *  The pad is the purchase measured to move a season most, and its
	 *  price in head was invisible — a player found out months later that
	 *  they had stopped getting braver, with nothing having said so. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Kit")
	FString PadOfferLine() const;

	/** A day on plastic. False if you are not a member — the gym is the one
	 *  place in this game that checks. */
	/** An hour on the board. False without one, or on skin already gone. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Kit")
	bool HangboardSession();

	/** "the belt is on borrowed time" — empty when there is nothing to say. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	FString VanLine() const;

	/** Is the van going anywhere? */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	bool VanRuns() const;

	/** The part that needs attention, or -1 when nothing does. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	int32 WorstVanPart() const;

	/** Free, four hours, and it will not hold. Always available — being
	 *  broke must never end a save. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	bool BodgeVan();

	/** Cash, twice per part, and it holds a while. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	bool PatchVan();

	/** More cash, and actually fixed. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	bool ReplaceVanPart();

	/** Drive somewhere: wears the van, charges the fuel, and may break it.
	 *  Returns the part that went, or -1. Called by the travel spot.
	 *
	 *  Every drive in the game comes through here. That is deliberate: fuel
	 *  is the only cost that goes up the more you climb, and the way it came
	 *  to be free was that each travel spot could have charged it and none
	 *  had to. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Van")
	int32 DriveVan(double Hours);

	/** What the last drive cost at the pump. Read by the travel spot so the
	 *  toast can say it — a cost the player never sees is a cost that feels
	 *  like a bug. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Van")
	double LastDriveFuel = 0.0;

	/** Set when something let go on a drive. Cleared at lights out. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Van")
	FString VanNews;

	// --- The dog ---------------------------------------------------------

	/** Feed it. Costs cash; enough meals and the stray is yours, with no
	 *  ceremony because the life does not provide one. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Dog")
	bool FeedTheDog();

	/** "the dog is asleep under the van" / "there is a stray at the Lot" */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Dog")
	FString DogLine();

	/** What a parked van is running at right now. Vans are ovens; the
	 *  conditions system already knows the air temperature, so this is that
	 *  plus what a metal box in the sun adds to it. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Dog")
	double VanTempF();

	/** True when the van is somewhere a dog can be left today. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Dog")
	bool VanIsSafeForTheDog();

	/** How hot a parked van runs above the air around it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Dog")
	double VanGreenhouseF = 18.0;

	/** Set when you left it somewhere you should not have. Cleared at
	 *  lights out, like everything else you get to stop thinking about. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dog")
	FString DogWorry;

	/** What came out overnight, if anything did. Set by Sleep and cleared
	 *  by the next one, exactly like DogWorry and VanNews — a secret should
	 *  reach the player the way it reaches a real climber, which is that you
	 *  wake up and everyone already knows. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Ethics")
	FString EthicsNews;

	/** What you just found out about yourself. Set on the burn where a
	 *  rolled talent becomes obvious, and never again — a gift surfaces
	 *  once in a career.
	 *
	 *  It is a *notice*, not a stat: the effect has been live since your
	 *  first move, and all that changed is that you noticed. See
	 *  Sim/DirtbagCharacter.h. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	FString TalentNews;

	/** **The night you became something.** Two seasons of doing a thing the
	 *  same way, and one morning it is not a thing you are doing any more,
	 *  it is a thing you are.
	 *
	 *  Said in the same slow slot and the same register as a talent
	 *  surfacing, because it is the same kind of news: something you found
	 *  out about yourself rather than something you won. Unlike the talent,
	 *  the *noticing* is the sim's -- `HabitsDay` returns the quirk that
	 *  landed and the career carries it, so the probe sees it too. See
	 *  Sim/DirtbagHabits.h. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Habits")
	FString QuirkNews;

	/** How you have been climbing, in two or three words, or empty. Live:
	 *  stop doing it and this goes. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Habits")
	FString HabitDoingLine() const;

	/** What you have turned into, or empty. Permanent. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Habits")
	FString HabitAreLine() const;

	/** Both of them as one sentence, for a screen with room -- the career's
	 *  own answer to "what sort of climber was that". */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Habits")
	FString HowYouClimbLine() const;

	/** The rival came around, or one of them hung it up. Slow news, said
	 *  once, in the same register as a secret coming out. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	FString RivalNews;

	/** They have asked to rope up and you have not answered. Non-empty is
	 *  the prompt state: **C takes it, F does not**, and walking away leaves
	 *  it standing rather than declining for you. The offer fires once ever;
	 *  what you do with it is yours. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	FString RivalOffer;

	/** Take them up on it. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Rival")
	bool AcceptTheRival();

	/** Or do not. Declining is a real answer: they stop asking, they keep
	 *  racing you, and the head-to-head stays exactly where it was. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Rival")
	bool DeclineTheRival();

	/** What the sponsor did overnight: the month's money, or the once-a-year
	 *  verdict on whether they are keeping you. Empty on any night neither
	 *  happened. Not saved — it is news, and news is for the morning it
	 *  arrives. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Sponsor")
	FString SponsorNews;

	// --- The Lot ---------------------------------------------------------

	/** The Lot's people as they are today: strength derived from seed and
	 *  date, rapport and claims from the career. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Lot")
	TArray<FDirtbagPartner> GetLot();

	/** An hour at the fire. Time passes, rapport grows with everyone there,
	 *  and you hear what people are working — which is the answer to
	 *  waiting for a window being lonely. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Lot")
	FString SitAtTheFire(double Hours);

	/** What the fire has to say right now, without spending anything. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Lot")
	TArray<FString> LotTalk();

	/** Ask whoever knows this line best for beta. Returns how much you
	 *  learned, 0 if nobody there can help. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|Lot")
	double AskForBeta(int32 BoardIndex, FString& OutWho);

	/** News from overnight: "Dev got the arete left of Diesel." Stays up for
	 *  the day and is replaced at the next lights-out, because losing a line
	 *  is not a thing to glance at once and lose. Empty when nothing
	 *  happened, which is most nights. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Lot")
	FString LotNews;

	/** Every first ascent in this career, hardest first. */
	// blueprint-only: an accessor for Blueprint; the guidebook page builds its own rows
	UFUNCTION(BlueprintCallable, Category = "Dirtbag|FirstAscent")
	TArray<FDirtbagProjectMemory> GetFirstAscents() const;

	/** What the ledgers add up to, and the one-line version of it. */
	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FDirtbagCareerSummary GetCareer() const;

	UFUNCTION(BlueprintCallable, Category = "Dirtbag")
	FString GetCareerLine() const;

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
	void EnsureBoard();
	void EnsureCrag();

	/** The ledger for a line, created filthy the first time a virgin line is
	 *  touched. Returns null only when there is nothing at that index. */
	FDirtbagProjectMemory* LedgerFor(int32 BoardIndex);

	/** Who signs an ascent. A climber who has not named themselves is still
	 *  allowed to do first ascents; the book calls them "you" and means it. */
	FString AscentSignature() const;

	/** Today's Lot, with the career's bonds folded in. */
	std::vector<dirtbag::Partner> LotToday();

public:
	// --- The rope ---------------------------------------------------------
	// No partner, no pitch. A boulder needs nobody; a bolted line needs
	// somebody willing to stand at the bottom of it, and how long they will
	// stand there is rapport. The first thing in the game rapport buys that
	// nothing else can.

	/** "Margo will hold your rope all afternoon" / "nobody is going up there
	 *  with you today". Always answers, even on a boulder — the wall decides
	 *  whether the answer matters. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Rope")
	FString BelayLine() const;

	/** Is anybody at the Lot willing today? */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Rope")
	bool HasABelayer() const;

	/** How many burns they are good for. A stranger gives you a couple;
	 *  somebody who has known you a season gives you the day. 0 with nobody. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Rope")
	int32 BurnsHeldToday() const;

	/** Roped burns taken today, against that budget. Reset at lights out —
	 *  their patience comes back with the morning, like everything else. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rope")
	int32 RopedBurnsToday = 0;

	/** Can you tie in right now? False with nobody there, and false once
	 *  you have used up what they were good for. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Rope")
	bool CanTieIn() const;

	/** Why not, in the game's voice. Empty when you can. */
	UFUNCTION(BlueprintPure, Category = "Dirtbag|Rope")
	FString RopeRefusal() const;

private:

	/** Write rapport and claims back into the career. */
	void StoreBonds(const std::vector<dirtbag::Partner>& Lot);

	/** Overnight: the Lot climbs too, and rapport moves. */
	void AdvanceTheLot();

	/** True once the player has pulled on anything today — what rapport is
	 *  actually earned by. */
	bool bClimbedToday = false;

	/** True once a shift has been worked today: the dog was on its own for
	 *  four hours, which is what its bond is priced on. */
	bool bWorkedToday = false;

	/** The ones that came before, oldest first. Not a UPROPERTY: these are
	 *  sim types, saved and loaded through DirtbagSave with everything else,
	 *  and Blueprint reads them through InheritedGuidebook(). */
	std::vector<dirtbag::Legacy> Legacies;

	// The forecast changes once a day; the HUD asks for it every frame.
	// Cheap either way (~23us), but there is no reason to re-hash the seed
	// and re-integrate a day of solar loading sixty times a second.
	mutable int32 CachedWeatherDay = -1;
	mutable FDirtbagWeather CachedWeather;
	mutable int32 CachedWindowDay = -1;
	mutable EDirtbagAspect CachedWindowAspect = EDirtbagAspect::North;
	mutable FDirtbagPrimeWindow CachedWindow;

	UPROPERTY()
	TArray<FDirtbagRoute> Board;

	UPROPERTY()
	FDirtbagCrag Crag;

	bool bCragLoaded = false;

	/** Which crag `Crag` currently holds. Roadside and the cave are
	 *  different rock with different aspects, so arriving at one has to
	 *  reload rather than keep serving the other's lines. */
	EDirtbagVenue LoadedCrag = EDirtbagVenue::Crag;
};
