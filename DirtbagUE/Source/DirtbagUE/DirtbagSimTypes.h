// Blueprint-visible mirrors of the engine-free sim types (Sim/DirtbagCore.h,
// Sim/DirtbagSession.h, Sim/DirtbagSessionLoop.h). Mirrors only — no game
// logic lives here; the sim decides, these structs carry its answers to
// Blueprint. Enum orders match the sim exactly (static_asserts in the .cpp).

#pragma once

#include "CoreMinimal.h"

#include "DirtbagCampfire.h"
#include "DirtbagZones.h"
#include "DirtbagCharacter.h"
#include "DirtbagComp.h"
#include "DirtbagTeam.h"
#include "DirtbagWorldStage.h"
#include "DirtbagRival.h"
#include "DirtbagConditions.h"
#include "DirtbagCore.h"
#include "DirtbagCrag.h"
#include "DirtbagCrew.h"
#include "DirtbagDay.h"
#include "DirtbagDreams.h"
#include "DirtbagFactions.h"
#include "DirtbagJobs.h"
#include "DirtbagBody.h"
#include "DirtbagKit.h"
#include "DirtbagEthics.h"
#include "DirtbagSponsor.h"
#include "DirtbagTown.h"
#include "DirtbagDog.h"
#include "DirtbagGear.h"
#include "DirtbagVan.h"
#include "DirtbagPartner.h"
#include "DirtbagFirstAscent.h"
#include "DirtbagSave.h"
#include "DirtbagSession.h"
#include "DirtbagSessionLoop.h"
// The one that was missing. Every other sim header the engine touches is
// aggregated here; Sport was not, so the belay functions were invisible to
// DirtbagGameInstance.cpp and the build died on a container-green commit.
#include "DirtbagSport.h"

#include "DirtbagSimTypes.generated.h"

UENUM(BlueprintType)
enum class EDirtbagHold : uint8
{
	Crimp, Sloper, Pinch, Pocket, Jug, Dyno, Crack
};

UENUM(BlueprintType)
enum class EDirtbagRouteType : uint8
{
	Crimp, Power, Endurance, Technical, Dyno, Crack
};

UENUM(BlueprintType)
enum class EDirtbagDiscipline : uint8
{
	Boulder, Sport
};

UENUM(BlueprintType)
enum class EDirtbagStyle : uint8
{
	Onsight, Flash, Redpoint, Sent, Fell
};

UENUM(BlueprintType)
enum class EDirtbagMorphology : uint8
{
	Compact, Average, Lanky, Powerful
};

/** The ground-up read of a line, judged against its guidebook grade. */
UENUM(BlueprintType)
enum class EDirtbagRouteRead : uint8
{
	Warmup,
	Comfortable,
	AtYourLimit,
	Project,
	NotThisYear
};

USTRUCT(BlueprintType)
struct FDirtbagMove
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Difficulty = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	EDirtbagHold Hold = EDirtbagHold::Jug;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double RestQuality = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double ReachBias = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	bool bCrux = false;
};

USTRUCT(BlueprintType)
struct FDirtbagRoute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	int32 Grade = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	int32 TrueGrade = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	EDirtbagRouteType Type = EDirtbagRouteType::Technical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	EDirtbagDiscipline Discipline = EDirtbagDiscipline::Boulder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	TArray<FDirtbagMove> Moves;
};

/** What goes wrong, and where it bites. */
UENUM(BlueprintType)
enum class EDirtbagInjuryKind : uint8
{
	Pulley,      // a finger; crimps are over, slopers are fine
	Lumbrical,   // pockets, only pockets, and it takes forever
	Elbow,       // everything, a little; nobody rests it properly
	Shoulder,    // slopers and anything dynamic; crimping feels fine
};

/** What is currently wrong with you. */
USTRUCT(BlueprintType)
struct FDirtbagInjury
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Body")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Body")
	EDirtbagInjuryKind Kind = EDirtbagInjuryKind::Pulley;

	/** 0..1 — what it costs, and how long it holds you. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Body")
	double Severity = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Body")
	int32 DaysLeft = 0;
};

USTRUCT(BlueprintType)
struct FDirtbagClimber
{
	GENERATED_BODY()

	// Skills 0..100, as in the 2D game.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Power = 50.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Fingers = 50.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Technique = 50.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Endurance = 50.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Head = 50.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	EDirtbagMorphology Morphology = EDirtbagMorphology::Average;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Skin = 9.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Psyche = 0.7;

	/** Training load, 0..100 — the second body budget, and a much slower
	 *  one. Skin is back in six nights; this takes a month. It rises with
	 *  how hard you pull rather than how often, because that is what hurts
	 *  tendons, and past ~62 it is how you get injured. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Body")
	double Load = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Body")
	FDirtbagInjury Injury;
};

USTRUCT(BlueprintType)
struct FDirtbagMoveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	int32 Index = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double Odds = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double PumpAfter = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	bool bSuccess = false;
};

USTRUCT(BlueprintType)
struct FDirtbagAttemptResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	bool bSent = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	int32 Highpoint = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	EDirtbagStyle Style = EDirtbagStyle::Fell;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double SkinCost = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double PeakPump = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	TArray<FDirtbagMoveResult> Timeline;
};

USTRUCT(BlueprintType)
struct FDirtbagSessionState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double SkinLeft = 9.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Warmth = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Psyche = 0.7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	int32 AttemptsMade = 0;

	/** 0 bare ground .. 1 as padded as it gets. Costs you nothing low down
	 *  and climbs toward the top, which is where a boulderer backs off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Padding = 1.0;

	/** What is left of the rubber, copied off the career when the session
	 *  starts. 0 new .. 1 dead.
	 *
	 *  This used to be mirror-skipped on the grounds that Blueprint can
	 *  read Player.Shoes.Wear instead — true, and beside the point. The
	 *  engine round-trips the whole DayState through ToSim/FromSim a dozen
	 *  times a day, so a skipped field is not merely absent from Blueprint,
	 *  it is *erased from the sim* on the next call. Shoe wear arrived at
	 *  every attempt as 0.0, and dead rubber cost nothing in the game. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double ShoeWear = 0.0;
};

USTRUCT(BlueprintType)
struct FDirtbagProjectMemory
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	FString RouteName;

	/** Guidebook grade, recorded on first touch; -1 = unknown (pre-v2 save). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	int32 Grade = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	int32 Attempts = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	int32 BestHighpoint = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Beta = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	bool bSent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	EDirtbagStyle FirstSendStyle = EDirtbagStyle::Fell;

	/** 1 clean rock .. 0 never been touched. Only projects start dirty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|FirstAscent")
	double Cleanliness = 1.0;

	/** The name you gave it. Never a key — RouteName is the key, forever. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|FirstAscent")
	FString GivenName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|FirstAscent")
	bool bFirstAscent = false;

	/** What it turned out to be. -1 until somebody had done it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|FirstAscent")
	int32 ConfirmedGrade = -1;

	/** Boulder or pitch. A guidebook entry for a rope route reads "5.12a"
	 *  and one for a boulder reads "V7"; a card that cannot tell them apart
	 *  confidently prints the wrong ladder. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	EDirtbagDiscipline Discipline = EDirtbagDiscipline::Boulder;
};

/** Career state — everything that outlives a day; what the save carries. */
UENUM(BlueprintType)
enum class EDirtbagVanPart : uint8
{
	Tyres,
	Brakes,
	Belt,
	Battery,
	Radiator,
	Clutch
};

/** What kind of climber you are on day one. Mirrors dirtbag::Archetype —
 *  the five offsets of every archetype sum to zero, so this is a shape and
 *  never a score. */
UENUM(BlueprintType)
enum class EDirtbagArchetype : uint8
{
	Boulderer  UMETA(DisplayName = "The Boulderer"),
	RopeGun    UMETA(DisplayName = "The Rope Gun"),
	Technician UMETA(DisplayName = "The Technician"),
	AllRounder UMETA(DisplayName = "The All-Rounder"),
};

/** Where you came from. Mirrors dirtbag::Origin — each holds one permanent
 *  perk in a lane no other origin touches, and none of them goes near send
 *  odds. */
UENUM(BlueprintType)
enum class EDirtbagOrigin : uint8
{
	SoldItAll    UMETA(DisplayName = "Sold It All"),
	GymRat       UMETA(DisplayName = "Gym Rat"),
	DesertLocal  UMETA(DisplayName = "Desert Local"),
	ExGymnast    UMETA(DisplayName = "Ex-Gymnast"),
	LateBloomer  UMETA(DisplayName = "Late Bloomer"),
	TrustFund    UMETA(DisplayName = "Trust-Fund Kid"),
};

/** The one you chose, knowing. Mirrors dirtbag::Flaw. */
UENUM(BlueprintType)
enum class EDirtbagFlaw : uint8
{
	Gumby         UMETA(DisplayName = "Gumby"),
	HardGainer    UMETA(DisplayName = "Hard Gainer"),
	FairWeather   UMETA(DisplayName = "Fair-Weather Trainer"),
	HappyFeet     UMETA(DisplayName = "Happy Feet"),
	TweakyFingers UMETA(DisplayName = "Tweaky Fingers"),
};

/** What you are like. Mirrors dirtbag::Temperament — it seeds four
 *  personality axes, each of which bends a different mechanic. */
UENUM(BlueprintType)
enum class EDirtbagTemperament : uint8
{
	Purist     UMETA(DisplayName = "The Purist"),
	SendOrBust UMETA(DisplayName = "Send-or-Bust"),
	Lifer      UMETA(DisplayName = "The Lifer"),
	Influencer UMETA(DisplayName = "The Influencer"),
};

/** What you were born with. Mirrors dirtbag::Talent — one gift and one
 *  anti-talent, rolled at birth, on different skills, and **live from the
 *  first session whether or not you know about them.** */
UENUM(BlueprintType)
enum class EDirtbagTalent : uint8
{
	None             UMETA(DisplayName = "—"),
	NaturalCrimper   UMETA(DisplayName = "Natural Crimper"),
	Explosive        UMETA(DisplayName = "Explosive"),
	SlowTwitchEngine UMETA(DisplayName = "Slow-Twitch Engine"),
	QuietFeet        UMETA(DisplayName = "Quiet Feet"),
	IceInTheVeins    UMETA(DisplayName = "Ice in the Veins"),
	BomberTendons    UMETA(DisplayName = "Bomber Tendons"),
	GlassTendons     UMETA(DisplayName = "Glass Tendons"),
	NoPop            UMETA(DisplayName = "No Pop"),
	NoEngine         UMETA(DisplayName = "No Engine"),
	Stiff            UMETA(DisplayName = "Stiff"),
	Skittish         UMETA(DisplayName = "Skittish"),
};

/** Who your climber is, as opposed to what they can do. Mirrors
 *  dirtbag::Character.
 *
 *  **`bBuilt` is the load-bearing field.** Every effect in the sim returns
 *  exactly neutral while it is false, which is what keeps a
 *  default-constructed player from silently carrying an origin's perk and a
 *  flaw's cost — see Sim/DirtbagCharacter.h for what that cost was when it
 *  did. */
USTRUCT(BlueprintType)
struct FDirtbagCharacter
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	bool bBuilt = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	EDirtbagArchetype Archetype = EDirtbagArchetype::AllRounder;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	EDirtbagOrigin Origin = EDirtbagOrigin::SoldItAll;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	EDirtbagFlaw Flaw = EDirtbagFlaw::Gumby;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	EDirtbagTemperament Temperament = EDirtbagTemperament::Lifer;

	/** -100..100 each. Disciplined keeps more of a session; bold commits
	 *  further above the gear; social decides who turns up; a purist works
	 *  for less and their outdoor sends say more. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	double Discipline = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	double Boldness = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	double Social = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	double Purism = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	EDirtbagTalent Gift = EDirtbagTalent::None;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	EDirtbagTalent AntiTalent = EDirtbagTalent::None;

	/** Whether it has surfaced yet. The effect does not wait for this. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	bool bGiftKnown = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	bool bAntiKnown = false;

	/** Sessions worked in each lane, power/fingers/technique/endurance/head.
	 *  A gift in a lane you never train stays a secret forever. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	TArray<double> Reps;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	double StartingCash = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	int32 AgePlus = 0;
};

/** What kind of rival they are. Mirrors dirtbag::RivalVibe. */
UENUM(BlueprintType)
enum class EDirtbagRivalVibe : uint8
{
	Foil      UMETA(DisplayName = "A friendly foil"),
	Nemesis   UMETA(DisplayName = "A bitter nemesis"),
	Benchmark UMETA(DisplayName = "A quiet benchmark"),
};

/** What becomes of them. Mirrors dirtbag::RivalRole — they do not leave the
 *  world, which is the entire point of ageing them. */
UENUM(BlueprintType)
enum class EDirtbagRivalRole : uint8
{
	Coach  UMETA(DisplayName = "Coaching at the gym"),
	Author UMETA(DisplayName = "Writing the guidebook"),
	Gone   UMETA(DisplayName = "Gone"),
};

/** Where you stand nationally. Mirrors dirtbag::RankTier. **Not the comp
 *  tier** -- that is which room you are allowed into, and this is what the
 *  room says about you. */
UENUM(BlueprintType)
enum class EDirtbagRankTier : uint8
{
	Unranked         UMETA(DisplayName = "Unranked"),
	RegionalClimber  UMETA(DisplayName = "Regional Climber"),
	NationalProspect UMETA(DisplayName = "National Prospect"),
	NationalTeam     UMETA(DisplayName = "National Team"),
	OlympicHopeful   UMETA(DisplayName = "Olympic Hopeful"),
	WorldClass       UMETA(DisplayName = "World-Class"),
};

/** Whether your name is on the paper. Mirrors dirtbag::TeamStatus. */
UENUM(BlueprintType)
enum class EDirtbagTeamStatus : uint8
{
	Never UMETA(DisplayName = "Unselected"),
	Named UMETA(DisplayName = "On the national team"),
	Cut   UMETA(DisplayName = "Off the national team"),
};

/** Somebody else on the paper. A teammate is a person before a number. */
USTRUCT(BlueprintType)
struct FDirtbagTeammate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FString Name;

	/** What they are in the room -- the crimper, the engine, the junior. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FString Role;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	double Points = 0.0;
};

/** The national team. Mirrors dirtbag::NationalTeam -- a roster with your
 *  name typed on it, five other people, a head coach who has opinions, a
 *  stipend that does not cover rent, and a review that can take it back. */
USTRUCT(BlueprintType)
struct FDirtbagNationalTeam
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	EDirtbagTeamStatus Status = EDirtbagTeamStatus::Never;

	/** Getting the call once never un-happens. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	bool bEverNamed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	int32 Seasons = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	int32 Cuts = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	int32 NamedOnDay = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FString Coach;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FString CoachKnownFor;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	TArray<FDirtbagTeammate> Roster;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	TArray<FString> Gone;

	/** The climber you went past to get on it. They know. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FString Passed;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	double LastReviewPoints = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	int32 LastReviewSeason = 0;

	/** When the committee last sat, in days. Once a year. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	int32 LastReviewDay = 0;
};

/** One result on the ranking record. Mirrors dirtbag::RankingResult.
 *
 *  **The ranking is made of these and is not accumulated.** A lifetime
 *  total means a tier cleared once is cleared forever, and the named rungs
 *  stop saying anything about the climber you are now. */
USTRUCT(BlueprintType)
struct FDirtbagRankingResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	int32 Day = 0;

	/** Negative for a no-show. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	double Points = 0.0;
};

/** Which round of a comp is on the wall. Mirrors dirtbag::CompRound. */
UENUM(BlueprintType)
enum class EDirtbagCompRound : uint8
{
	Qualification UMETA(DisplayName = "Qualification"),
	Semi          UMETA(DisplayName = "Semi-final"),
	Final         UMETA(DisplayName = "Final"),
};

/** Which ladder the live board belongs to. Mirrors dirtbag::Stage.
 *
 *  The engine holds one comp at a time and the settle has to know where the
 *  result goes: a World Cup round banked into the domestic circuit is a
 *  title nobody won. */
UENUM(BlueprintType)
enum class EDirtbagStage : uint8
{
	Domestic UMETA(DisplayName = "The circuit"),
	WorldCup UMETA(DisplayName = "World Cup"),
	Games    UMETA(DisplayName = "The Games"),
};

/** A city you fly to. Mirrors dirtbag::WorldCupVenue.
 *
 *  Named for the sim's struct rather than shortened to `FDirtbagVenue`,
 *  because `DirtbagTown.h` already has a `Venue` and it means something
 *  else entirely: a town venue is a place at home that opens at nine. */
USTRUCT(BlueprintType)
struct FDirtbagWorldCupVenue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	FString City;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	FString Country;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	EDirtbagDiscipline Discipline = EDirtbagDiscipline::Boulder;

	/** What getting there costs. The dial that makes a season a budget
	 *  problem: Salt Lake is a domestic ticket and Seoul is most of a
	 *  month's money. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	double Travel = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	FString Blurb;
};

/** One round. Mirrors dirtbag::WorldCupRound. */
USTRUCT(BlueprintType)
struct FDirtbagWorldCupRound
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Day = 0;

	/** Index into the venue table. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Venue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	bool bResolved = false;

	/** Whether you actually went. A resolved round you did not fly to is
	 *  the whole mechanic: the field banked while you were at home. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	bool bFlown = false;
};

/** A World Cup season. Mirrors dirtbag::WorldCupSeason. */
USTRUCT(BlueprintType)
struct FDirtbagWorldCupSeason
{
	GENERATED_BODY()

	/** 1-based. Zero means none has started. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Season = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	TArray<FDirtbagWorldCupRound> Schedule;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	double YourPoints = 0.0;

	/** Parallel to the international field. They fly whether you do or
	 *  not, which is why this is banked per round rather than derived. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	TArray<double> FieldPoints;

	/** Career totals -- these carry across seasons. Anything about the
	 *  year you are in has to be counted off the schedule instead. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Starts = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Missed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Finals = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Podiums = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Wins = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Titles = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 BestRank = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 LastRank = 0;

	/** Whether the table has been read out. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	bool bClosed = false;
};

/** The Games. Mirrors dirtbag::Olympics. */
USTRUCT(BlueprintType)
struct FDirtbagOlympics
{
	GENERATED_BODY()

	/** The day they are held. Zero means not seeded yet. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 NextDay = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Appearances = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Gold = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Silver = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Bronze = 0;

	/** Which cycle the last one you competed in was, so a Games cannot be
	 *  entered twice. **Minus one, not zero** -- zero would mean you had
	 *  climbed the Games held on day zero. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 LastCompeted = -1;
};

/** Whether you can get on the plane. Mirrors dirtbag::FlightCheck --
 *  everything the door has to check, in one answer, decided sim-side so
 *  the rule can be tested from the harness. */
USTRUCT(BlueprintType)
struct FDirtbagFlightCheck
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	bool bCan = false;

	/** Which round, or -1 when there is none today. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	int32 Round = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	double Cost = 0.0;

	/** Why not, in the game's voice. Empty when you can, and empty when
	 *  there is simply no round on -- silence is not a refusal. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	FString Why;
};

/** A season of the circuit. Mirrors dirtbag::Circuit -- five firm dates,
 *  the last worth half as much again. */
USTRUCT(BlueprintType)
struct FDirtbagCircuit
{
	GENERATED_BODY()

	/** 1-based. Zero means none has started. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	int32 Season = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	int32 CompsDone = 0;

	/** Firm dates, in order. The last is the finals. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	TArray<int32> Schedule;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	double YourPoints = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	double RivalPoints = 0.0;

	/** Parallel to the named field. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	TArray<double> FieldPoints;

	/** Seasons won, across a career. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	int32 Titles = 0;
};

/** A line with a deadline on it. Mirrors dirtbag::Race. */
USTRUCT(BlueprintType)
struct FDirtbagRace
{
	GENERATED_BODY()

	/** Empty means no race is on. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	FString RouteName;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	int32 ByDay = 0;

	/** An open line, which is the version that cannot be undone. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	bool bForFirstAscent = false;
};

/** Somebody to beat. Mirrors dirtbag::Rival.
 *
 *  **Not the nemesis.** `FDirtbagPlayerState::Nemesis` is the unsent *line*
 *  you have fed the most burns; this is a *person*. §4 called them one
 *  thing and they are two. */
USTRUCT(BlueprintType)
struct FDirtbagRival
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	FString Name;

	/** What they are best at — leaned toward whatever you are weakest at. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	EDirtbagRouteType Style = EDirtbagRouteType::Power;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	EDirtbagRivalVibe Vibe = EDirtbagRivalVibe::Benchmark;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	int32 Generation = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	int32 BornOnDay = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	double StartAge = 26.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	double Grade = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	int32 LastStepDay = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	double PeakGrade = 0.0;

	/** Net head-to-head. Positive is you. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	double Rivalry = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	bool bAllied = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	bool bOffered = false;

	/** Before this they are a name and a number. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	bool bMet = false;

	/** Open lines they got to before you. Their name is on them forever. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	TArray<FString> FirstAscents;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	bool bRetired = false;

	/** What they are on right now, if anything. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	FDirtbagRace Race;
};

/** One who came before, and what became of them. */
USTRUCT(BlueprintType)
struct FDirtbagPastRival
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	EDirtbagRivalRole Role = EDirtbagRivalRole::Gone;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	EDirtbagRouteType Style = EDirtbagRouteType::Power;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	int32 RetiredOnDay = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	double Age = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	double PeakGrade = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	int32 Generation = 0;
};

/** What is on your feet, and how much of it is left. */
USTRUCT(BlueprintType)
struct FDirtbagShoes
{
	GENERATED_BODY()

	/** 0 new rubber .. 1 dead. Wears by the move, faster the harder you pull. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Gear")
	double Wear = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Gear")
	int32 Resoles = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Gear")
	int32 PairsOwned = 1;
};

USTRUCT(BlueprintType)
struct FDirtbagVanPart
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Van")
	double Wear = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Van")
	int32 Patches = 0;

	/** Stops the van until something is done about it. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Van")
	bool bFailed = false;
};

/** Shelter, transport, and the reason seasons end early. */
USTRUCT(BlueprintType)
struct FDirtbagVan
{
	GENERATED_BODY()

	/** Six parts, in EDirtbagVanPart order. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Van")
	TArray<FDirtbagVanPart> Parts;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Van")
	double HoursDriven = 0.0;

	/** Whether this is the Rig — the van you saved for. Parts last longer;
	 *  it does not stop breaking, it stops breaking so often. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Van")
	bool bRig = false;
};

/** The stray, and then the dog. */
USTRUCT(BlueprintType)
struct FDirtbagDog
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dog")
	FString Name = TEXT("the dog");

	/** A stray until you have fed it enough. There is no ceremony. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dog")
	bool bAdopted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dog")
	double Bond = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dog")
	double Fed = 0.4;
};

/** What a career remembers about somebody at the Lot. Their strength is
 *  derived from seed and date and is deliberately not here. */
USTRUCT(BlueprintType)
struct FDirtbagPartnerBond
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Lot")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Lot")
	double Rapport = 0.0;

	/** Route keys they got to first. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Lot")
	TArray<FString> FirstAscents;
};

/** Somebody at the Lot, as they are today. */
USTRUCT(BlueprintType)
struct FDirtbagPartner
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Lot")
	FString Name;

	/** One line of who they are, in their own register. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Lot")
	FString Tag;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Lot")
	FDirtbagClimber Climber;

	/** False for the neighbours. Ray put up half the crag and has not pulled
	 *  on in years. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Lot")
	bool bClimbs = true;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Lot")
	double Rapport = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Lot")
	TArray<FString> FirstAscents;
};

/** What a place is for. One primary service each — a venue that does
 *  everything is a menu, not a place. */
UENUM(BlueprintType)
enum class EDirtbagService : uint8
{
	Meal,        // the diner, the gas station
	Gear,        // resoles and rubber
	VanRepair,   // the garage, when you would rather not bodge it
	Gym,         // plastic, and the only climbing that ignores the weather
	Work,        // somewhere that hires by the shift
};

/** One place in town. The opening hours are the mechanic: the diner shuts
 *  at nine so a long day means eating from a warmer, and the gear shop
 *  keeps banker's hours so a resole competes with the window. */
USTRUCT(BlueprintType)
struct FDirtbagVenue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Town")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Town")
	EDirtbagService Service = EDirtbagService::Meal;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Town")
	double OpensAt = 8.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Town")
	double ClosesAt = 18.0;

	/** Against the baseline the day loop already uses. The gas station is
	 *  cheap and grim; the diner is dear and actually feeds you. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Town")
	double PriceFactor = 1.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Town")
	double QualityFactor = 1.0;

	/** Hours from the Lot. Town is a drive, and a drive is van wear. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Town")
	double TravelHours = 0.4;

	/** A line of who they are, in the game's register. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Town")
	FString Flavour;
};

USTRUCT(BlueprintType)
struct FDirtbagTown
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Town")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Town")
	TArray<FDirtbagVenue> Venues;
};

/** The five shortcuts. Each buys something real and each is a thing you
 *  would not say out loud at the fire. */
UENUM(BlueprintType)
enum class EDirtbagEthicalAct : uint8
{
	ChippedAHold,   // it goes now. It did not before, and it never will again.
	RetroBolted,    // somebody's ground-up line, made safe without asking
	ClaimedASend,   // the oldest one in the sport
	StagedAPhoto,   // a shot of a send that did not happen
	PulledOnGear,   // one hang nobody saw, and you called it clean
};

/** Something you did that nobody saw — and the day somebody found out. */
USTRUCT(BlueprintType)
struct FDirtbagSecret
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Ethics")
	EDirtbagEthicalAct Act = EDirtbagEthicalAct::ChippedAHold;

	/** The line it was done to. Empty for a staged photo. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Ethics")
	FString RouteKey;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Ethics")
	int32 DayDone = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Ethics")
	bool bKnown = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Ethics")
	int32 DayFound = 0;
};

/** The ladder. Each rung pays more and owns more of your calendar. */
UENUM(BlueprintType)
enum class EDirtbagSponsorTier : uint8
{
	None,
	Shoes,    // free rubber, and that is the whole deal. No strings.
	Gear,     // a small stipend, a bag of kit, and they want photos
	Title,    // real money, and they own days you would rather have
};

/** Who pays you to climb, and what they want for it. */
USTRUCT(BlueprintType)
struct FDirtbagSponsorship
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Sponsor")
	EDirtbagSponsorTier Tier = EDirtbagSponsorTier::None;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Sponsor")
	int32 SeasonsHeld = 0;

	/** What you had sent when they last looked. Review asks what you have
	 *  done lately, not what your peak was. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Sponsor")
	int32 GradeAtLastReview = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Sponsor")
	int32 SeasonsWithoutProgress = 0;

	/** Days spent hurt since the last review, which the review reads to
	 *  decide whether a flat season was failure or a torn pulley.
	 *
	 *  Unlike the obligation counters this cannot be mirror-skipped: the
	 *  sim increments it every night inside SleepToNextDay, and the engine
	 *  reaches that through a ToSim/FromSim round trip, so a mirror that
	 *  dropped it would throw the increment away every single night and the
	 *  injury pause would never once fire. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Sponsor")
	int32 DaysHurtThisSeason = 0;
};

/** What you own that buys you climbing. */
USTRUCT(BlueprintType)
struct FDirtbagKit
{
	GENERATED_BODY()

	/** You arrive with one. The second is the purchase, and it is the one
	 *  that covers the fall you did not expect — measured, both pads against
	 *  one is +37% sends across twelve seasons at identical skin spend. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Kit")
	int32 Pads = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Kit")
	bool bHangboard = false;

	/** Days of gym membership left. Zero is not a member. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Kit")
	int32 MembershipDaysLeft = 0;
};

/** Who has an opinion about you. Two opposed axes, four camps. */
UENUM(BlueprintType)
enum class EDirtbagFaction : uint8
{
	OldGuard,
	Scene,
	Development,
	Stewardship
};

/** Where you stand with each of them, -1..1, and whether the gate is shut. */
USTRUCT(BlueprintType)
struct FDirtbagStanding
{
	GENERATED_BODY()

	/** Indexed by EDirtbagFaction. -1 they will not have you, +1 you are one
	 *  of theirs. Gaining with one costs a little with its opposite. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Standing")
	TArray<double> With;

	/** Days left on an access closure. Above zero, the crag is shut. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Standing")
	int32 ClosedDays = 0;
};

/** Employment, such as it is. */
USTRUCT(BlueprintType)
struct FDirtbagJob
{
	GENERATED_BODY()

	/** Nine to five, five days a week. The hours are the point, not the pay. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	bool bSalaried = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	int32 DaysWorked = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	int32 WeeksSalaried = 0;

	/** Days into the streak running now. Broken by signing for the
	 *  nine-to-five, never by an odd job. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	int32 DaysSinceSalary = 0;

	/** Whole years lived without a boss. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	int32 DirtbagYears = 0;

	/** The longest unbroken run, in days, including one in progress. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	int32 LongestStreak = 0;
};

/** One player at the fire, as you see them: a name and a sentence about
 *  what they look like they have. Never a number — a number would make the
 *  campfire game arithmetic, and it is meant to be a person. */
USTRUCT(BlueprintType)
struct FDirtbagCampfireRead
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	FString Who;

	/** "Margo is not even looking at her cards." */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	FString Tell;
};

/** The hand in front of you. See Sim/DirtbagCampfire.h — how well you know
 *  somebody is how well you see what they have, so the skill is rapport
 *  rather than card counting. */
USTRUCT(BlueprintType)
struct FDirtbagCampfireHand
{
	GENERATED_BODY()

	/** 0..1, and you see this one exactly. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	double Yours = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	TArray<FDirtbagCampfireRead> Reads;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	double Pot = 0.0;
};

/** A bid at liar's dice, as it reaches you: whose it is, what they claimed,
 *  your own five, and what they look like saying it. */
USTRUCT(BlueprintType)
struct FDirtbagLiarsDice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	TArray<int32> Yours;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	FString Bidder;

	/** "Dev says there are seven fives." */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	FString Bid;

	/** "Dev took a moment too long." Never a number. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	FString Tell;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	int32 DiceOnTable = 0;
};

/** Blackjack: the one with nobody in it. */
USTRUCT(BlueprintType)
struct FDirtbagBlackjack
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	int32 Yours = 0;

	/** The one card of the deck's you can see. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	int32 DealerShows = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	bool bBust = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	bool bFinished = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Campfire")
	int32 Draws = 0;
};

/** Where you are in the world. See Sim/DirtbagZones.h — a zone is *where*
 *  and a venue is *what rock*, which are different questions: downtown
 *  holds the Gym venue plus three destinations that are not climbing at
 *  all.
 *
 *  **The first five ordinals are frozen and new zones go on the end.**
 *  This is a `uint8` and `ADirtbagDaySpot::DestinationZone` is an
 *  `EditAnywhere` property, so every travel spot already placed in a level
 *  stores its destination by ordinal. Inserting a zone anywhere but the
 *  end silently repoints all of them, with no compiler error and no failing
 *  test — the symptom is walking to the gym and arriving at a crag. The
 *  sim's `TestZones` pins the first five ordinals for the same reason;
 *  keep the two lists in the same order or the `static_cast` in
 *  `UDirtbagSimLibrary` quietly means something else. */
UENUM(BlueprintType)
enum class EDirtbagZone : uint8
{
	Lot        UMETA(DisplayName = "The Lot"),
	Town       UMETA(DisplayName = "Downtown"),
	Roadside   UMETA(DisplayName = "Roadside"),
	Cave       UMETA(DisplayName = "The Shaded Cave"),
	Terrace    UMETA(DisplayName = "The Sun Terrace"),
	OldTown    UMETA(DisplayName = "Old Town"),
	Midtown    UMETA(DisplayName = "Midtown"),
	Trailhead  UMETA(DisplayName = "The Trailhead"),
	Outskirts  UMETA(DisplayName = "The Outskirts"),
	Uptown     UMETA(DisplayName = "Uptown"),
	MarketRow  UMETA(DisplayName = "Market Row"),
	GrandPlaza UMETA(DisplayName = "Grand Plaza"),
	Park       UMETA(DisplayName = "Greenwood Park"),
	Lake       UMETA(DisplayName = "Trout Lake"),
	Village    UMETA(DisplayName = "The Olympic Village"),
	Farm       UMETA(DisplayName = "The Farm"),
};

/** Which game is out at the fire tonight. See Sim/DirtbagCampfire.h — the
 *  day decides, not the player, so this is read rather than set. */
UENUM(BlueprintType)
enum class EDirtbagFiresideGame : uint8
{
	Cards     UMETA(DisplayName = "Cards"),
	Dice      UMETA(DisplayName = "Liar's dice"),
	Blackjack UMETA(DisplayName = "Blackjack"),
};

/** What the money is for. See Sim/DirtbagDreams.h — the price is the
 *  buffer, not the number. */
UENUM(BlueprintType)
enum class EDirtbagDream : uint8
{
	None     UMETA(DisplayName = "Nothing in particular"),
	Rig      UMETA(DisplayName = "The Rig"),
	WarChest UMETA(DisplayName = "The War Chest"),
	HomeBase UMETA(DisplayName = "Home Base"),
};

USTRUCT(BlueprintType)
struct FDirtbagDreams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dreams")
	bool bRig = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dreams")
	bool bWarChest = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dreams")
	bool bHomeBase = false;

	/** The dream. Chosen once per career, and choosing closes the other
	 *  two. Survives the purchase — what your dream was is part of the
	 *  career. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dreams")
	EDirtbagDream Chosen = EDirtbagDream::None;

	/** The War Chest, being lived. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dreams")
	int32 SeasonOffDaysLeft = 0;
};

/** What the town calls the people you keep turning up with. Not yours to
 *  pick and not yours to change — see Sim/DirtbagCrew.h. */
USTRUCT(BlueprintType)
struct FDirtbagCrew
{
	GENERATED_BODY()

	/** Empty until they say it. Once said, it never changes. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crew")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crew")
	int32 NamedOnDay = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crew")
	int32 DaysReadingAsACrew = 0;

	/** How many it was on the day it stuck — them, and you. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crew")
	int32 MembersWhenNamed = 0;
};

/** A gig on the board: hours, money, and whether the van has to go. */
USTRUCT(BlueprintType)
struct FDirtbagOddJob
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	double Hours = 4.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	double Pay = 60.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	double Energy = 25.0;

	/** No van, no job. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	bool bNeedsVan = false;
};

USTRUCT(BlueprintType)
struct FDirtbagPlayerState
{
	GENERATED_BODY()

	/** What you are called. The crew hash includes it, so the town names
	 *  this generation's crew rather than re-issuing the last one's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	FDirtbagClimber Climber;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Cash = 420.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	int32 Day = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	TArray<FDirtbagProjectMemory> Projects;

	/** Who you know at the Lot. Rapport and claims only — nobody's strength
	 *  is stored, because it is derived from the seed and the date. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Lot")
	TArray<FDirtbagPartnerBond> Bonds;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dog")
	FDirtbagDog Dog;

	/** Who you are, as opposed to what you can do. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Character")
	FDirtbagCharacter Character;

	/** Somebody to beat. Not the nemesis, which is a route. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	FDirtbagRival Rival;

	/** And the ones who came before. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Rival")
	TArray<FDirtbagPastRival> PastRivals;

	/** National ranking points. What the comp tiers gate on. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	double RankingPoints = 0.0;

	/** The season you are in the middle of. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FDirtbagCircuit Circuit;

	/** Whether your name is on the paper. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	FDirtbagNationalTeam Team;

	/** What `RankingPoints` is made of: every result inside the last year.
	 *  Pruned as it is written, so it stays about twenty-five entries long
	 *  however long the career runs. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Comp")
	TArray<FDirtbagRankingResult> RankingRecord;

	/** The top of the ladder. A World Cup season is running from the first
	 *  night of a career whether or not you have ever heard of it. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	FDirtbagWorldCupSeason WorldCup;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|World")
	FDirtbagOlympics Olympics;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Gear")
	FDirtbagShoes Shoes;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Van")
	FDirtbagVan Van;

	/** What you could not pay. Cash floors at zero; the shortfall waits
	 *  here and is the first thing any wage goes to. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double Owed = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Work")
	FDirtbagJob Job;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crew")
	FDirtbagCrew Crew;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Dreams")
	FDirtbagDreams Dreams;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Standing")
	FDirtbagStanding Standing;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Kit")
	FDirtbagKit Kit;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Sponsor")
	FDirtbagSponsorship Sponsor;

	/** What you did that nobody saw. Carried, not priced — an act costs
	 *  nothing until the day somebody finds out. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Ethics")
	TArray<FDirtbagSecret> Secrets;

	/** The last day you saw a physio — rate limiting, so a rich season
	 *  cannot buy its way out of a bad one overnight. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Body")
	int32 LastPhysioDay = 0;
};

/** One day's body-clock. Never saved — saves happen at day boundaries. */
USTRUCT(BlueprintType)
struct FDirtbagDayState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Hour = 7.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Energy = 100.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	double Hunger = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	bool bAtGym = false;

	/** One board session a day. Without the cap, skin bought eleven. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	bool bHangboardDone = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag")
	FDirtbagSessionState Session;
};

/** What the project ledgers add up to. Derived, never stored. */
USTRUCT(BlueprintType)
struct FDirtbagCareerSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	double AbilityGrade = 0.0;

	/** Guidebook grade of your hardest send; -1 when nothing has gone down. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	int32 HardestSendGrade = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FString HardestSendName;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	EDirtbagStyle HardestSendStyle = EDirtbagStyle::Fell;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	int32 TotalSends = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	int32 TotalAttempts = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	int32 OpenProjects = 0;

	/** The unsent line you have fed the most burns. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	FString Nemesis;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag")
	int32 NemesisAttempts = 0;
};

// Which way the rock faces. This, not the forecast, decides what time of day
// a crag is climbable — an east face bakes at breakfast and comes into the
// shade mid-afternoon.
UENUM(BlueprintType)
enum class EDirtbagAspect : uint8
{
	North,
	East,
	South,
	West
};

/** One day's weather. Derived from world seed + day, so it never needs saving. */
USTRUCT(BlueprintType)
struct FDirtbagWeather
{
	GENERATED_BODY()

	/**
	 * Which day this is. Carried on the weather because everything that reads
	 * the weather also needs to know how much light the day has — daylight
	 * swings from 16.5 hours midsummer to 7.9 midwinter, and a window search
	 * that does not know the date searches a midwinter day all year.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	int32 Day = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	double HighTempF = 60.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	double LowTempF = 40.0;

	/** 0..1. The dirtbag's real enemy. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	double Humidity = 0.5;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	double Cloud = 0.3;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	double Wind = 0.2;
};

/** The day's best span. bExists is false on a day that never comes good. */
USTRUCT(BlueprintType)
struct FDirtbagPrimeWindow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	bool bExists = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	double StartHour = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	double EndHour = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	double PeakHour = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Conditions")
	double PeakFriction = 0.0;
};

/** One line in the guidebook: a route, plus what the book says about it. */
USTRUCT(BlueprintType)
struct FDirtbagCragLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crag")
	FDirtbagRoute Route;

	/** Guidebook quality 0..3. Quality and difficulty are separate axes. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crag")
	int32 Stars = 0;

	/** An unclimbed line: the grade is a guess and nobody has vouched for it. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crag")
	bool bIsProject = false;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crag")
	FString FirstAscentBy;

	/** Where to find it, when it has no name: "the arete left of Diesel". */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crag")
	FString Description;

	/** Its given name, once earned. Never a key — Route.Name is the key. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crag")
	FString DisplayName;
};

USTRUCT(BlueprintType)
struct FDirtbagCrag
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crag")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crag")
	EDirtbagAspect Aspect = EDirtbagAspect::North;

	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crag")
	TArray<FDirtbagCragLine> Lines;

	/** Hours from the van. The crag's real cost. */
	UPROPERTY(BlueprintReadOnly, Category = "Dirtbag|Crag")
	double ApproachHours = 0.5;
};

/** What the session would tell you if it could. */
UENUM(BlueprintType)
enum class EDirtbagSessionAdvice : uint8
{
	Ready,
	Cold,
	SkinThin,
	Wrecked
};

UENUM(BlueprintType)
enum class EDirtbagLoadResult : uint8
{
	Ok,
	BadFormat,
	FutureVersion,
	FileMissing
};

// UE-struct <-> sim-struct converters. Plain functions, not UFUNCTIONs.
namespace DirtbagConvert
{
	dirtbag::Climber ToSim(const FDirtbagClimber& In);
	dirtbag::Route ToSim(const FDirtbagRoute& In);
	dirtbag::SessionState ToSim(const FDirtbagSessionState& In);
	dirtbag::ProjectMemory ToSim(const FDirtbagProjectMemory& In);
	dirtbag::PlayerState ToSim(const FDirtbagPlayerState& In);
	dirtbag::DayState ToSim(const FDirtbagDayState& In);

	FDirtbagRoute FromSim(const dirtbag::Route& In);
	FDirtbagAttemptResult FromSim(const dirtbag::AttemptResult& In);
	FDirtbagMoveResult FromSim(const dirtbag::MoveResult& In);
	FDirtbagSessionState FromSim(const dirtbag::SessionState& In);
	FDirtbagProjectMemory FromSim(const dirtbag::ProjectMemory& In);
	FDirtbagPlayerState FromSim(const dirtbag::PlayerState& In);
	FDirtbagDayState FromSim(const dirtbag::DayState& In);
	FDirtbagCareerSummary FromSim(const dirtbag::CareerSummary& In);
	FDirtbagWeather FromSim(const dirtbag::Weather& In);
	FDirtbagPrimeWindow FromSim(const dirtbag::PrimeWindow& In);

	FDirtbagClimber FromSim(const dirtbag::Climber& In);
	FDirtbagDog FromSim(const dirtbag::Dog& In);
	dirtbag::Dog ToSim(const FDirtbagDog& In);
	FDirtbagShoes FromSim(const dirtbag::Shoes& In);
	dirtbag::Shoes ToSim(const FDirtbagShoes& In);
	FDirtbagCharacter FromSim(const dirtbag::Character& In);
	dirtbag::Character ToSim(const FDirtbagCharacter& In);
	FDirtbagNationalTeam FromSim(const dirtbag::NationalTeam& In);
	dirtbag::NationalTeam ToSim(const FDirtbagNationalTeam& In);
	FDirtbagCircuit FromSim(const dirtbag::Circuit& In);
	dirtbag::Circuit ToSim(const FDirtbagCircuit& In);
	FDirtbagRankingResult FromSim(const dirtbag::RankingResult& In);
	dirtbag::RankingResult ToSim(const FDirtbagRankingResult& In);
	FDirtbagWorldCupSeason FromSim(const dirtbag::WorldCupSeason& In);
	dirtbag::WorldCupSeason ToSim(const FDirtbagWorldCupSeason& In);
	FDirtbagOlympics FromSim(const dirtbag::Olympics& In);
	dirtbag::Olympics ToSim(const FDirtbagOlympics& In);
	FDirtbagWorldCupVenue FromSim(const dirtbag::WorldCupVenue& In);
	FDirtbagFlightCheck FromSim(const dirtbag::FlightCheck& In);
	FDirtbagRival FromSim(const dirtbag::Rival& In);
	dirtbag::Rival ToSim(const FDirtbagRival& In);
	FDirtbagPastRival FromSim(const dirtbag::PastRival& In);
	dirtbag::PastRival ToSim(const FDirtbagPastRival& In);
	FDirtbagVan FromSim(const dirtbag::Van& In);
	dirtbag::Van ToSim(const FDirtbagVan& In);
	FDirtbagPartnerBond FromSim(const dirtbag::PartnerBond& In);
	dirtbag::PartnerBond ToSim(const FDirtbagPartnerBond& In);
	FDirtbagPartner FromSim(const dirtbag::Partner& In);

	FDirtbagCragLine FromSim(const dirtbag::CragLine& In);
	FDirtbagCrag FromSim(const dirtbag::Crag& In);

	FDirtbagVenue FromSim(const dirtbag::Venue& In);
	dirtbag::Venue ToSim(const FDirtbagVenue& In);
	FDirtbagTown FromSim(const dirtbag::Town& In);
	FDirtbagInjury FromSim(const dirtbag::Injury& In);
	dirtbag::Injury ToSim(const FDirtbagInjury& In);
	FDirtbagSecret FromSim(const dirtbag::Secret& In);
	dirtbag::Secret ToSim(const FDirtbagSecret& In);
	FDirtbagSponsorship FromSim(const dirtbag::Sponsorship& In);
	dirtbag::Sponsorship ToSim(const FDirtbagSponsorship& In);
	FDirtbagKit FromSim(const dirtbag::Kit& In);
	dirtbag::Kit ToSim(const FDirtbagKit& In);
	FDirtbagStanding FromSim(const dirtbag::Standing& In);
	dirtbag::Standing ToSim(const FDirtbagStanding& In);
	FDirtbagJob FromSim(const dirtbag::Job& In);
	dirtbag::Job ToSim(const FDirtbagJob& In);
	FDirtbagCrew FromSim(const dirtbag::Crew& In);
	dirtbag::Crew ToSim(const FDirtbagCrew& In);
	FDirtbagDreams FromSim(const dirtbag::Dreams& In);
	dirtbag::Dreams ToSim(const FDirtbagDreams& In);
	FDirtbagBlackjack FromSim(const dirtbag::BlackjackHand& In);
	dirtbag::BlackjackHand ToSim(const FDirtbagBlackjack& In);
	FDirtbagOddJob FromSim(const dirtbag::OddJob& In);

	dirtbag::Weather ToSim(const FDirtbagWeather& In);
	dirtbag::Aspect ToSim(EDirtbagAspect In);
}
