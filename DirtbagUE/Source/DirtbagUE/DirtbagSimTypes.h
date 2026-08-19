// Blueprint-visible mirrors of the engine-free sim types (Sim/DirtbagCore.h,
// Sim/DirtbagSession.h, Sim/DirtbagSessionLoop.h). Mirrors only — no game
// logic lives here; the sim decides, these structs carry its answers to
// Blueprint. Enum orders match the sim exactly (static_asserts in the .cpp).

#pragma once

#include "CoreMinimal.h"

#include "DirtbagConditions.h"
#include "DirtbagCore.h"
#include "DirtbagCrag.h"
#include "DirtbagDay.h"
#include "DirtbagDog.h"
#include "DirtbagPartner.h"
#include "DirtbagFirstAscent.h"
#include "DirtbagSave.h"
#include "DirtbagSession.h"
#include "DirtbagSessionLoop.h"

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
};

/** Career state — everything that outlives a day; what the save carries. */
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

USTRUCT(BlueprintType)
struct FDirtbagPlayerState
{
	GENERATED_BODY()

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
	FDirtbagPartnerBond FromSim(const dirtbag::PartnerBond& In);
	dirtbag::PartnerBond ToSim(const FDirtbagPartnerBond& In);
	FDirtbagPartner FromSim(const dirtbag::Partner& In);

	FDirtbagCragLine FromSim(const dirtbag::CragLine& In);
	FDirtbagCrag FromSim(const dirtbag::Crag& In);

	dirtbag::Weather ToSim(const FDirtbagWeather& In);
	dirtbag::Aspect ToSim(EDirtbagAspect In);
}
