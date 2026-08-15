#include "DirtbagSimLibrary.h"

#include "DirtbagRng.h"

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
