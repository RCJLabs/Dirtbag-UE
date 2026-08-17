#include "DirtbagSimLibrary.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "DirtbagRng.h"

namespace
{
FString SaveFilePath(const FString& Filename)
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), Filename);
}

EDirtbagLoadResult ToUEResult(dirtbag::LoadResult Result)
{
	switch (Result)
	{
	case dirtbag::LoadResult::Ok:            return EDirtbagLoadResult::Ok;
	case dirtbag::LoadResult::BadFormat:     return EDirtbagLoadResult::BadFormat;
	case dirtbag::LoadResult::FutureVersion: return EDirtbagLoadResult::FutureVersion;
	}
	return EDirtbagLoadResult::BadFormat;
}

// The ledger to read for a route the player may not have touched yet —
// a default memory if it's new, without mutating the player.
dirtbag::ProjectMemory PeekMemory(const dirtbag::PlayerState& Player,
                                  const std::string& RouteName)
{
	for (const dirtbag::ProjectMemory& M : Player.projects)
	{
		if (M.routeName == RouteName)
		{
			return M;
		}
	}
	dirtbag::ProjectMemory Fresh;
	Fresh.routeName = RouteName;
	return Fresh;
}
}  // namespace

double UDirtbagLiveAttempt::PeekOdds(double Execution) const
{
	return dirtbag::PeekOdds(Live, Execution);
}

FDirtbagMoveResult UDirtbagLiveAttempt::StepMove(double Execution)
{
	return DirtbagConvert::FromSim(dirtbag::StepMove(Live, Execution));
}

double UDirtbagLiveAttempt::ShakeOut()
{
	return dirtbag::ShakeOut(Live);
}

bool UDirtbagLiveAttempt::IsOver() const
{
	return dirtbag::AttemptOver(Live);
}

double UDirtbagLiveAttempt::GetPump() const
{
	return Live.pump;
}

int32 UDirtbagLiveAttempt::GetNextMoveIndex() const
{
	return Live.nextMove;
}

FDirtbagAttemptResult UDirtbagLiveAttempt::Finish() const
{
	return DirtbagConvert::FromSim(dirtbag::FinishAttempt(Live));
}

void UDirtbagLiveAttempt::CommitToSession(FDirtbagSessionState& Session,
                                          FDirtbagProjectMemory& Memory) const
{
	dirtbag::SessionState SimSession = DirtbagConvert::ToSim(Session);
	dirtbag::ProjectMemory SimMemory = DirtbagConvert::ToSim(Memory);
	dirtbag::CommitAttempt(SimSession, SimMemory, Live.input.route,
	                       dirtbag::FinishAttempt(Live));
	Session = DirtbagConvert::FromSim(SimSession);
	Memory = DirtbagConvert::FromSim(SimMemory);
}

FDirtbagRoute UDirtbagSimLibrary::BuildRoute(const FString& WorldSeed,
                                             const FString& RouteName,
                                             int32 Grade, int32 TrueGrade,
                                             EDirtbagRouteType Type,
                                             EDirtbagDiscipline Discipline)
{
	const dirtbag::Rng World = dirtbag::Rng::FromStream(
	    TCHAR_TO_UTF8(*WorldSeed), dirtbag::Stream::Worldgen);
	return DirtbagConvert::FromSim(dirtbag::BuildRoute(
	    World, TCHAR_TO_UTF8(*RouteName), Grade, TrueGrade,
	    static_cast<dirtbag::RouteType>(Type),
	    static_cast<dirtbag::Discipline>(Discipline)));
}

FDirtbagSessionState UDirtbagSimLibrary::StartSession(
    const FDirtbagClimber& Climber)
{
	return DirtbagConvert::FromSim(
	    dirtbag::StartSession(DirtbagConvert::ToSim(Climber)));
}

FDirtbagAttemptResult UDirtbagSimLibrary::AttemptInSession(
    const FString& SessionSeed, FDirtbagSessionState& Session,
    FDirtbagProjectMemory& Memory, const FDirtbagClimber& Climber,
    const FDirtbagRoute& Route, double Friction, double BotExecution)
{
	const dirtbag::Rng SessionRng = dirtbag::Rng::FromStream(
	    TCHAR_TO_UTF8(*SessionSeed), dirtbag::Stream::Session);
	dirtbag::SessionState SimSession = DirtbagConvert::ToSim(Session);
	dirtbag::ProjectMemory SimMemory = DirtbagConvert::ToSim(Memory);
	dirtbag::Conditions Conditions;
	Conditions.friction = Friction;

	const dirtbag::AttemptResult Result = dirtbag::AttemptInSession(
	    SessionRng, SimSession, SimMemory, DirtbagConvert::ToSim(Climber),
	    DirtbagConvert::ToSim(Route), Conditions, {}, BotExecution);

	Session = DirtbagConvert::FromSim(SimSession);
	Memory = DirtbagConvert::FromSim(SimMemory);
	return DirtbagConvert::FromSim(Result);
}

UDirtbagLiveAttempt* UDirtbagSimLibrary::BeginLiveAttempt(
    const FString& SessionSeed, const FDirtbagSessionState& Session,
    const FDirtbagProjectMemory& Memory, const FDirtbagClimber& Climber,
    const FDirtbagRoute& Route, double Friction)
{
	const dirtbag::Rng SessionRng = dirtbag::Rng::FromStream(
	    TCHAR_TO_UTF8(*SessionSeed), dirtbag::Stream::Session);
	const dirtbag::SessionState SimSession = DirtbagConvert::ToSim(Session);
	const dirtbag::ProjectMemory SimMemory = DirtbagConvert::ToSim(Memory);
	const dirtbag::Route SimRoute = DirtbagConvert::ToSim(Route);
	dirtbag::Conditions Conditions;
	Conditions.friction = Friction;

	const dirtbag::Rng AttemptRng =
	    dirtbag::DeriveAttemptRng(SessionRng, SimMemory, SimRoute);
	const dirtbag::AttemptInput Input = dirtbag::BuildSessionAttemptInput(
	    SimSession, SimMemory, DirtbagConvert::ToSim(Climber), SimRoute,
	    Conditions);

	UDirtbagLiveAttempt* Attempt = NewObject<UDirtbagLiveAttempt>();
	Attempt->Live = dirtbag::BeginAttempt(AttemptRng, Input);
	return Attempt;
}

FString UDirtbagSimLibrary::GradeName(int32 Grade,
                                      EDirtbagDiscipline Discipline)
{
	return Discipline == EDirtbagDiscipline::Boulder
	           ? FString(dirtbag::BoulderGradeName(Grade))
	           : FString(dirtbag::SportGradeName(Grade));
}

TArray<FDirtbagRoute> UDirtbagSimLibrary::GymBoard(const FString& WorldSeed,
                                                   int32 Count)
{
	const dirtbag::Rng World = dirtbag::Rng::FromStream(
	    TCHAR_TO_UTF8(*WorldSeed), dirtbag::Stream::Worldgen);
	TArray<FDirtbagRoute> Out;
	for (const dirtbag::Route& R : dirtbag::GymBoard(World, Count))
	{
		Out.Add(DirtbagConvert::FromSim(R));
	}
	return Out;
}

FDirtbagDayState UDirtbagSimLibrary::WakeUp(const FDirtbagPlayerState& Player)
{
	return DirtbagConvert::FromSim(
	    dirtbag::WakeUp(DirtbagConvert::ToSim(Player)));
}

void UDirtbagSimLibrary::PassHours(FDirtbagDayState& Day, double Hours)
{
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	dirtbag::PassHours(SimDay, Hours);
	Day = DirtbagConvert::FromSim(SimDay);
}

bool UDirtbagSimLibrary::EatMeal(FDirtbagPlayerState& Player,
                                 FDirtbagDayState& Day)
{
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	const bool bAte = dirtbag::EatMeal(SimPlayer, SimDay);
	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
	return bAte;
}

void UDirtbagSimLibrary::WorkShift(FDirtbagPlayerState& Player,
                                   FDirtbagDayState& Day)
{
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	dirtbag::WorkShift(SimPlayer, SimDay);
	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
}

FDirtbagClimber UDirtbagSimLibrary::ClimberForSession(
    const FDirtbagPlayerState& Player, const FDirtbagDayState& Day)
{
	const dirtbag::Climber C = dirtbag::ClimberForSession(
	    DirtbagConvert::ToSim(Player), DirtbagConvert::ToSim(Day));
	FDirtbagClimber Out;
	Out.Power = C.skills.power;
	Out.Fingers = C.skills.fingers;
	Out.Technique = C.skills.technique;
	Out.Endurance = C.skills.endurance;
	Out.Head = C.skills.head;
	Out.Morphology = static_cast<EDirtbagMorphology>(C.morphology);
	Out.Skin = C.skin;
	Out.Psyche = C.psyche;
	return Out;
}

void UDirtbagSimLibrary::StartGymSession(FDirtbagPlayerState& Player,
                                         FDirtbagDayState& Day)
{
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	dirtbag::StartGymSession(SimPlayer, SimDay);
	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
}

FDirtbagAttemptResult UDirtbagSimLibrary::DayAttempt(
    const FString& SessionSeed, FDirtbagPlayerState& Player,
    FDirtbagDayState& Day, const FDirtbagRoute& Route, double Friction,
    double BotExecution)
{
	const dirtbag::Rng SessionRng = dirtbag::Rng::FromStream(
	    TCHAR_TO_UTF8(*SessionSeed), dirtbag::Stream::Session);
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	const dirtbag::Route SimRoute = DirtbagConvert::ToSim(Route);
	dirtbag::Conditions Conditions;
	Conditions.friction = Friction;

	const dirtbag::AttemptResult Result = dirtbag::AttemptInSession(
	    SessionRng, SimDay.session, dirtbag::MemoryFor(SimPlayer, SimRoute),
	    dirtbag::ClimberForSession(SimPlayer, SimDay), SimRoute, Conditions, {},
	    BotExecution);
	dirtbag::ApplyAttemptToDay(SimPlayer, SimDay, SimRoute, Result);

	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
	return DirtbagConvert::FromSim(Result);
}

UDirtbagLiveAttempt* UDirtbagSimLibrary::BeginDayLiveAttempt(
    const FString& SessionSeed, const FDirtbagPlayerState& Player,
    const FDirtbagDayState& Day, const FDirtbagRoute& Route, double Friction)
{
	const dirtbag::Rng SessionRng = dirtbag::Rng::FromStream(
	    TCHAR_TO_UTF8(*SessionSeed), dirtbag::Stream::Session);
	const dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	const dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	const dirtbag::Route SimRoute = DirtbagConvert::ToSim(Route);
	const dirtbag::ProjectMemory Memory = PeekMemory(SimPlayer, SimRoute.name);
	dirtbag::Conditions Conditions;
	Conditions.friction = Friction;

	UDirtbagLiveAttempt* Attempt = NewObject<UDirtbagLiveAttempt>();
	Attempt->Live = dirtbag::BeginAttempt(
	    dirtbag::DeriveAttemptRng(SessionRng, Memory, SimRoute),
	    dirtbag::BuildSessionAttemptInput(
	        SimDay.session, Memory,
	        dirtbag::ClimberForSession(SimPlayer, SimDay), SimRoute,
	        Conditions));
	return Attempt;
}

FDirtbagAttemptResult UDirtbagSimLibrary::CommitLiveAttempt(
    UDirtbagLiveAttempt* Attempt, FDirtbagPlayerState& Player,
    FDirtbagDayState& Day)
{
	if (!Attempt)
	{
		return FDirtbagAttemptResult();
	}
	const dirtbag::AttemptResult Result = dirtbag::FinishAttempt(Attempt->Live);
	const dirtbag::Route& SimRoute = Attempt->Live.input.route;
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);

	dirtbag::CommitAttempt(SimDay.session,
	                       dirtbag::MemoryFor(SimPlayer, SimRoute), SimRoute,
	                       Result);
	dirtbag::ApplyAttemptToDay(SimPlayer, SimDay, SimRoute, Result);

	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
	return DirtbagConvert::FromSim(Result);
}

void UDirtbagSimLibrary::SleepToNextDay(FDirtbagPlayerState& Player,
                                        FDirtbagDayState& Day)
{
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	dirtbag::SleepToNextDay(SimPlayer, SimDay);
	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
}

FString UDirtbagSimLibrary::SaveToText(const FString& Seed,
                                       const FDirtbagPlayerState& Player)
{
	dirtbag::SaveGame Save;
	Save.seed = TCHAR_TO_UTF8(*Seed);
	Save.player = DirtbagConvert::ToSim(Player);
	return UTF8_TO_TCHAR(dirtbag::SerializeSave(Save).c_str());
}

EDirtbagLoadResult UDirtbagSimLibrary::LoadFromText(
    const FString& Text, FString& OutSeed, FDirtbagPlayerState& OutPlayer)
{
	dirtbag::SaveGame Save;
	const dirtbag::LoadResult Result =
	    dirtbag::DeserializeSave(TCHAR_TO_UTF8(*Text), Save);
	if (Result == dirtbag::LoadResult::Ok)
	{
		OutSeed = UTF8_TO_TCHAR(Save.seed.c_str());
		OutPlayer = DirtbagConvert::FromSim(Save.player);
	}
	return ToUEResult(Result);
}

bool UDirtbagSimLibrary::SaveToFile(const FString& Seed,
                                    const FDirtbagPlayerState& Player,
                                    const FString& Filename)
{
	return FFileHelper::SaveStringToFile(SaveToText(Seed, Player),
	                                     *SaveFilePath(Filename));
}

EDirtbagLoadResult UDirtbagSimLibrary::LoadFromFile(
    const FString& Filename, FString& OutSeed, FDirtbagPlayerState& OutPlayer)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *SaveFilePath(Filename)))
	{
		return EDirtbagLoadResult::FileMissing;
	}
	return LoadFromText(Text, OutSeed, OutPlayer);
}
