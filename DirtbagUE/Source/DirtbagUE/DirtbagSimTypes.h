// Blueprint-visible mirrors of the engine-free sim types (Sim/DirtbagCore.h,
// Sim/DirtbagSession.h, Sim/DirtbagSessionLoop.h). Mirrors only — no game
// logic lives here; the sim decides, these structs carry its answers to
// Blueprint. Enum orders match the sim exactly (static_asserts in the .cpp).

#pragma once

#include "CoreMinimal.h"

#include "DirtbagCore.h"
#include "DirtbagDay.h"
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
};

/** Career state — everything that outlives a day; what the save carries. */
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
}
