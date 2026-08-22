#include "DirtbagSimLibrary.h"

#include "DirtbagSport.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogDirtbagSave, Log, All);

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
	dirtbag::ApplyAttemptToDay(SimPlayer, SimDay, SimRoute, Result,
	                           dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*SessionSeed)));

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
	// The attempt's own stream: this burn either got away with climbing on
	// an injury or did not, and that is a fact about this burn.
	dirtbag::ApplyAttemptToDay(SimPlayer, SimDay, SimRoute, Result,
	                           Attempt->Live.rng);

	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
	return DirtbagConvert::FromSim(Result);
}

void UDirtbagSimLibrary::SleepToNextDay(const FString& Seed,
                                        FDirtbagPlayerState& Player,
                                        FDirtbagDayState& Day)
{
	dirtbag::PlayerState SimPlayer = DirtbagConvert::ToSim(Player);
	dirtbag::DayState SimDay = DirtbagConvert::ToSim(Day);
	dirtbag::SleepToNextDay(SimPlayer, SimDay,
	                        dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)));
	// A night's upkeep, not just a night's recovery: rock you cleaned gives
	// a little back to the weather. It lives here rather than inside
	// SleepToNextDay because the first-ascent layer sits above the day loop
	// and must not be inverted — but every night must run it, so the single
	// node that means "a night" is where it goes.
	dirtbag::WeatherProjects(SimPlayer);
	Player = DirtbagConvert::FromSim(SimPlayer);
	Day = DirtbagConvert::FromSim(SimDay);
}

EDirtbagRouteRead UDirtbagSimLibrary::ReadRoute(const FDirtbagClimber& Climber,
                                                const FDirtbagRoute& Route)
{
	return static_cast<EDirtbagRouteRead>(dirtbag::ReadRoute(
	    DirtbagConvert::ToSim(Climber), DirtbagConvert::ToSim(Route)));
}

FString UDirtbagSimLibrary::ReadRouteText(EDirtbagRouteRead Read)
{
	return FString(dirtbag::ReadRouteText(static_cast<dirtbag::RouteRead>(Read)));
}

double UDirtbagSimLibrary::HowClose(const FDirtbagAttemptResult& Result,
                                    int32 TotalMoves)
{
	// Every field carried across rather than only the three HowClose reads
	// today. Filling in what a function currently happens to use is how a
	// bridge quietly starts lying the day the function reads one more.
	dirtbag::AttemptResult R;
	R.sent = Result.bSent;
	R.highpoint = Result.Highpoint;
	R.style = static_cast<dirtbag::Style>(Result.Style);
	R.skinCost = Result.SkinCost;
	R.peakPump = Result.PeakPump;
	R.timeline.reserve(static_cast<size_t>(Result.Timeline.Num()));
	for (const FDirtbagMoveResult& M : Result.Timeline)
	{
		dirtbag::MoveResult Out;
		Out.index = M.Index;
		Out.odds = M.Odds;
		Out.pumpAfter = M.PumpAfter;
		Out.success = M.bSuccess;
		R.timeline.push_back(Out);
	}
	return dirtbag::HowClose(R, TotalMoves);
}

FString UDirtbagSimLibrary::HowCloseText(double Close)
{
	return FString(dirtbag::HowCloseText(Close));
}

double UDirtbagSimLibrary::PumpShows(double Pump)
{
	return dirtbag::PumpShows(Pump);
}

FDirtbagCareerSummary UDirtbagSimLibrary::SummarizeCareer(
    const FDirtbagPlayerState& Player)
{
	return DirtbagConvert::FromSim(
	    dirtbag::SummarizeCareer(DirtbagConvert::ToSim(Player)));
}

FString UDirtbagSimLibrary::CareerLine(const FDirtbagCareerSummary& Career)
{
	dirtbag::CareerSummary Sim;
	Sim.abilityGrade = Career.AbilityGrade;
	Sim.hardestSendGrade = Career.HardestSendGrade;
	Sim.hardestSendName = TCHAR_TO_UTF8(*Career.HardestSendName);
	Sim.hardestSendStyle = static_cast<dirtbag::Style>(Career.HardestSendStyle);
	Sim.totalSends = Career.TotalSends;
	Sim.totalAttempts = Career.TotalAttempts;
	Sim.openProjects = Career.OpenProjects;
	Sim.nemesis = TCHAR_TO_UTF8(*Career.Nemesis);
	Sim.nemesisAttempts = Career.NemesisAttempts;
	return UTF8_TO_TCHAR(dirtbag::CareerLine(Sim).c_str());
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

bool DirtbagSaveIO::SaveGameToFile(const FString& Seed,
                                   const FDirtbagPlayerState& Player,
                                   const std::vector<dirtbag::Legacy>& Legacies,
                                   const FString& Filename)
{
	dirtbag::SaveGame Save;
	Save.seed = TCHAR_TO_UTF8(*Seed);
	Save.player = DirtbagConvert::ToSim(Player);
	Save.legacies = Legacies;
	return FFileHelper::SaveStringToFile(
	    UTF8_TO_TCHAR(dirtbag::SerializeSave(Save).c_str()),
	    *SaveFilePath(Filename));
}

EDirtbagLoadResult DirtbagSaveIO::LoadGameFromFile(
    const FString& Filename, FString& OutSeed, FDirtbagPlayerState& OutPlayer,
    std::vector<dirtbag::Legacy>& OutLegacies)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *SaveFilePath(Filename)))
	{
		return EDirtbagLoadResult::FileMissing;
	}
	dirtbag::SaveGame Save;
	const dirtbag::LoadResult Result =
	    dirtbag::DeserializeSave(TCHAR_TO_UTF8(*Text), Save);
	if (Result == dirtbag::LoadResult::Ok)
	{
		OutSeed = UTF8_TO_TCHAR(Save.seed.c_str());
		OutPlayer = DirtbagConvert::FromSim(Save.player);
		OutLegacies = Save.legacies;

		// Say what the migration did, because nothing else ever does. The
		// file on disk keeps its old version until the next SaveNow (the
		// next sleep) -- deliberately, so a load can never corrupt the
		// only copy -- and without this line the only way to learn that
		// was to open the raw file, see "version=13", and reasonably
		// conclude the migration never ran. It ran; the disk just has not
		// heard yet. The pre-migration number is read off the file's own
		// first line, since DeserializeSave hands the struct back already
		// upgraded.
		int32 FileVersion = 0;
		FParse::Value(*Text, TEXT("version="), FileVersion);
		if (FileVersion != dirtbag::kSaveVersion)
		{
			UE_LOG(LogDirtbagSave, Display,
			       TEXT("Loaded '%s': file is v%d, migrated to v%d in "
			            "memory. The file itself updates at the next "
			            "sleep."),
			       *Filename, FileVersion, dirtbag::kSaveVersion);
		}
		else
		{
			UE_LOG(LogDirtbagSave, Display,
			       TEXT("Loaded '%s' (v%d, current)."), *Filename,
			       FileVersion);
		}
	}
	else
	{
		UE_LOG(LogDirtbagSave, Warning,
		       TEXT("Save '%s' did not load (%s). Starting fresh; the file "
		            "is left on disk untouched for post-mortems."),
		       *Filename,
		       Result == dirtbag::LoadResult::FutureVersion
		           ? TEXT("from a newer build")
		           : TEXT("bad format"));
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

// --- Conditions --------------------------------------------------------------

FDirtbagWeather UDirtbagSimLibrary::WeatherFor(const FString& WorldSeed,
                                               int32 Day)
{
	const dirtbag::Rng World =
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*WorldSeed));
	return DirtbagConvert::FromSim(dirtbag::GenerateWeather(World, Day));
}

double UDirtbagSimLibrary::FrictionAt(const FDirtbagWeather& Weather,
                                      EDirtbagAspect Aspect, double Hour)
{
	return dirtbag::ConditionsAt(DirtbagConvert::ToSim(Weather),
	                             DirtbagConvert::ToSim(Aspect), Hour)
	    .friction;
}

double UDirtbagSimLibrary::RockTempF(const FDirtbagWeather& Weather,
                                     EDirtbagAspect Aspect, double Hour)
{
	return dirtbag::RockTempAt(DirtbagConvert::ToSim(Weather),
	                           DirtbagConvert::ToSim(Aspect), Hour);
}

FDirtbagPrimeWindow UDirtbagSimLibrary::PrimeWindowFor(
    const FDirtbagWeather& Weather, EDirtbagAspect Aspect)
{
	return DirtbagConvert::FromSim(dirtbag::FindPrimeWindow(
	    DirtbagConvert::ToSim(Weather), DirtbagConvert::ToSim(Aspect)));
}

double UDirtbagSimLibrary::FirstLightHour(int32 Day)
{
	return dirtbag::FirstLightHour(Day);
}

double UDirtbagSimLibrary::LastLightHour(int32 Day)
{
	return dirtbag::LastLightHour(Day);
}

// --- Sport -------------------------------------------------------------------

TArray<int32> UDirtbagSimLibrary::BoltsFor(const FDirtbagRoute& Route)
{
	TArray<int32> Out;
	for (int Bolt : dirtbag::BoltsFor(DirtbagConvert::ToSim(Route)))
	{
		Out.Add(Bolt);
	}
	return Out;
}

double UDirtbagSimLibrary::RunoutAt(const FDirtbagRoute& Route,
                                    int32 MoveIndex)
{
	return dirtbag::RunoutAt(DirtbagConvert::ToSim(Route), MoveIndex);
}

bool UDirtbagSimLibrary::OnTheRope(const FDirtbagRoute& Route, int32 MoveIndex)
{
	return dirtbag::OnTheRope(DirtbagConvert::ToSim(Route), MoveIndex);
}

bool UDirtbagSimLibrary::IsClippingMove(const FDirtbagRoute& Route,
                                        int32 MoveIndex)
{
	return dirtbag::IsClippingMove(DirtbagConvert::ToSim(Route), MoveIndex);
}

FString UDirtbagSimLibrary::RunoutText(double Runout)
{
	return UTF8_TO_TCHAR(dirtbag::RunoutText(Runout).c_str());
}

bool UDirtbagSimLibrary::NeedsABelayer(const FDirtbagRoute& Route)
{
	return dirtbag::NeedsABelayer(DirtbagConvert::ToSim(Route));
}

FDirtbagCrag UDirtbagSimLibrary::SunTerrace(const FString& Seed)
{
	return DirtbagConvert::FromSim(
	    dirtbag::SunTerrace(dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed))));
}

FDirtbagCrag UDirtbagSimLibrary::ShadedCave(const FString& Seed)
{
	// FromSeed, exactly as RoadsideCrag does — the two crags have to be
	// generated from the same world or they are not in the same valley.
	return DirtbagConvert::FromSim(
	    dirtbag::ShadedCave(dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed))));
}

// --- The town ----------------------------------------------------------------

FDirtbagTown UDirtbagSimLibrary::Town()
{
	return DirtbagConvert::FromSim(dirtbag::DirtbagTown());
}

bool UDirtbagSimLibrary::VenueIsOpen(const FDirtbagVenue& Venue, double Hour)
{
	// Asked of the sim rather than reimplemented here. Two copies of "is it
	// open" would drift, and the drift would show up as a door the HUD says
	// is open and the day loop says is not.
	return dirtbag::IsOpen(DirtbagConvert::ToSim(Venue), Hour);
}

TArray<FDirtbagVenue> UDirtbagSimLibrary::VenuesFor(EDirtbagService Service)
{
	const dirtbag::Town SimTown = dirtbag::DirtbagTown();
	TArray<FDirtbagVenue> Out;
	for (const dirtbag::Venue* V : dirtbag::VenuesFor(
	         SimTown, static_cast<dirtbag::Service>(Service)))
	{
		Out.Add(DirtbagConvert::FromSim(*V));
	}
	return Out;
}

FDirtbagVenue UDirtbagSimLibrary::OpenVenueFor(EDirtbagService Service,
                                               double Hour, bool& bFound)
{
	const dirtbag::Town SimTown = dirtbag::DirtbagTown();
	const dirtbag::Venue* V = dirtbag::OpenVenueFor(
	    SimTown, static_cast<dirtbag::Service>(Service), Hour);
	bFound = V != nullptr;
	// Empty is a real answer — the town shuts, and "nowhere is open" is the
	// thing a late finish is supposed to cost you.
	return V ? DirtbagConvert::FromSim(*V) : FDirtbagVenue();
}

FString UDirtbagSimLibrary::VenueText(const FDirtbagVenue& Venue, double Hour)
{
	return UTF8_TO_TCHAR(
	    dirtbag::VenueText(DirtbagConvert::ToSim(Venue), Hour).c_str());
}

TArray<FDirtbagOddJob> UDirtbagSimLibrary::OddJobBoard(const FString& Seed,
                                                       int32 Day)
{
	TArray<FDirtbagOddJob> Out;
	for (const dirtbag::OddJob& J : dirtbag::OddJobBoard(
	         dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*Seed)), Day))
	{
		Out.Add(DirtbagConvert::FromSim(J));
	}
	return Out;
}

FString UDirtbagSimLibrary::ConditionsText(double Friction)
{
	return FString(UTF8_TO_TCHAR(dirtbag::ConditionsText(Friction)));
}

FString UDirtbagSimLibrary::WindowText(const FDirtbagPrimeWindow& Window)
{
	dirtbag::PrimeWindow W;
	W.exists = Window.bExists;
	W.startHour = Window.StartHour;
	W.endHour = Window.EndHour;
	W.peakHour = Window.PeakHour;
	W.peakFriction = Window.PeakFriction;
	return FString(UTF8_TO_TCHAR(dirtbag::WindowText(W).c_str()));
}

// --- The crag ----------------------------------------------------------------

FDirtbagCrag UDirtbagSimLibrary::RoadsideCrag(const FString& WorldSeed)
{
	const dirtbag::Rng World =
	    dirtbag::Rng::FromSeed(TCHAR_TO_UTF8(*WorldSeed));
	return DirtbagConvert::FromSim(dirtbag::RoadsideCrag(World));
}

FString UDirtbagSimLibrary::GuidebookLine(const FDirtbagCragLine& Line)
{
	// Rebuilt rather than round-tripped: the sim owns how a book entry
	// reads, and only the fields it reads need to make the trip.
	dirtbag::CragLine Sim;
	Sim.route.name = TCHAR_TO_UTF8(*Line.Route.Name);
	Sim.route.grade = Line.Route.Grade;
	Sim.stars = Line.Stars;
	Sim.isProject = Line.bIsProject;
	Sim.description = TCHAR_TO_UTF8(*Line.Description);
	if (Line.DisplayName != Line.Route.Name)
	{
		Sim.displayName = TCHAR_TO_UTF8(*Line.DisplayName);
	}
	return FString(UTF8_TO_TCHAR(dirtbag::GuidebookLine(Sim).c_str()));
}

void UDirtbagSimLibrary::WriteLegaciesIntoTheBook(
    FDirtbagCrag& Book, const std::vector<dirtbag::Legacy>& Legacies)
{
	// Oldest first, so that if two careers somehow claimed the same rock the
	// later one is the name that stands -- which is also what a real
	// guidebook does when a disputed line gets re-recorded.
	for (const dirtbag::Legacy& L : Legacies)
	{
		for (const dirtbag::NamedLine& N : L.firstAscents)
		{
			for (FDirtbagCragLine& Line : Book.Lines)
			{
				dirtbag::CragLine Sim;
				Sim.route.name = TCHAR_TO_UTF8(*Line.Route.Name);
				Sim.route.grade = Line.Route.Grade;
				Sim.isProject = Line.bIsProject;
				Sim.firstAscentBy = TCHAR_TO_UTF8(*Line.FirstAscentBy);
				if (!dirtbag::WriteIntoTheBook(Sim, N))
				{
					continue;
				}
				Line.DisplayName = UTF8_TO_TCHAR(dirtbag::DisplayName(Sim).c_str());
				Line.FirstAscentBy = UTF8_TO_TCHAR(Sim.firstAscentBy.c_str());
				Line.bIsProject = Sim.isProject;
				Line.Route.Grade = Sim.route.grade;
			}
		}
	}
}

void UDirtbagSimLibrary::WriteTheLotIntoTheBook(
    FDirtbagCrag& Book, const FDirtbagPlayerState& Player)
{
	for (const FDirtbagPartnerBond& Bond : Player.Bonds)
	{
		for (const FString& Key : Bond.FirstAscents)
		{
			for (FDirtbagCragLine& Line : Book.Lines)
			{
				if (Line.Route.Name != Key)
				{
					continue;
				}
				dirtbag::CragLine Sim;
				Sim.route.name = TCHAR_TO_UTF8(*Line.Route.Name);
				Sim.route.grade = Line.Route.Grade;
				Sim.route.trueGrade = Line.Route.TrueGrade;
				Sim.isProject = Line.bIsProject;
				Sim.firstAscentBy = TCHAR_TO_UTF8(*Line.FirstAscentBy);
				if (!dirtbag::TheyPutUpTheLine(Sim, TCHAR_TO_UTF8(*Bond.Name)))
				{
					continue;
				}
				Line.DisplayName = UTF8_TO_TCHAR(dirtbag::DisplayName(Sim).c_str());
				Line.FirstAscentBy = UTF8_TO_TCHAR(Sim.firstAscentBy.c_str());
				Line.bIsProject = Sim.isProject;
				Line.Route.Grade = Sim.route.grade;
			}
		}
	}
}

void UDirtbagSimLibrary::WriteIntoTheBook(FDirtbagCrag& Book,
                                          const FDirtbagPlayerState& Player,
                                          const FString& By)
{
	for (FDirtbagCragLine& Line : Book.Lines)
	{
		const FDirtbagProjectMemory* Ledger = nullptr;
		for (const FDirtbagProjectMemory& M : Player.Projects)
		{
			if (M.RouteName == Line.Route.Name)
			{
				Ledger = &M;
				break;
			}
		}
		if (Ledger == nullptr)
		{
			continue;
		}

		// Rebuilt rather than round-tripped, as GuidebookLine is — and
		// pointedly without DisplayName, which arrives already filled with
		// the fallback. Feeding that back in would tell the sim every line
		// in the book had been named.
		dirtbag::CragLine Sim;
		Sim.route.name = TCHAR_TO_UTF8(*Line.Route.Name);
		Sim.route.grade = Line.Route.Grade;
		Sim.isProject = Line.bIsProject;
		Sim.firstAscentBy = TCHAR_TO_UTF8(*Line.FirstAscentBy);

		if (!dirtbag::WriteIntoTheBook(Sim, DirtbagConvert::ToSim(*Ledger),
		                               TCHAR_TO_UTF8(*By)))
		{
			continue;
		}
		Line.DisplayName = UTF8_TO_TCHAR(dirtbag::DisplayName(Sim).c_str());
		Line.FirstAscentBy = UTF8_TO_TCHAR(Sim.firstAscentBy.c_str());
		Line.bIsProject = Sim.isProject;
		Line.Route.Grade = Sim.route.grade;
	}
}

