#include "DirtbagSimTypes.h"

// The mirrors cast by value between enum sets; if the sim reorders, these
// fail the build instead of silently corrupting data.
static_assert(static_cast<int>(EDirtbagHold::Crack) == static_cast<int>(dirtbag::HoldType::Crack), "Hold enums out of sync");
static_assert(static_cast<int>(EDirtbagRouteType::Crack) == static_cast<int>(dirtbag::RouteType::Crack), "RouteType enums out of sync");
static_assert(static_cast<int>(EDirtbagDiscipline::Sport) == static_cast<int>(dirtbag::Discipline::Sport), "Discipline enums out of sync");
static_assert(static_cast<int>(EDirtbagStyle::Fell) == static_cast<int>(dirtbag::Style::Fell), "Style enums out of sync");
static_assert(static_cast<int>(EDirtbagMorphology::Powerful) == static_cast<int>(dirtbag::Morphology::Powerful), "Morphology enums out of sync");

namespace DirtbagConvert
{

dirtbag::Climber ToSim(const FDirtbagClimber& In)
{
	dirtbag::Climber Out;
	Out.skills.power = In.Power;
	Out.skills.fingers = In.Fingers;
	Out.skills.technique = In.Technique;
	Out.skills.endurance = In.Endurance;
	Out.skills.head = In.Head;
	Out.morphology = static_cast<dirtbag::Morphology>(In.Morphology);
	Out.skin = In.Skin;
	Out.psyche = In.Psyche;
	return Out;
}

dirtbag::Route ToSim(const FDirtbagRoute& In)
{
	dirtbag::Route Out;
	Out.name = TCHAR_TO_UTF8(*In.Name);
	Out.grade = In.Grade;
	Out.trueGrade = In.TrueGrade;
	Out.type = static_cast<dirtbag::RouteType>(In.Type);
	Out.discipline = static_cast<dirtbag::Discipline>(In.Discipline);
	Out.moves.reserve(In.Moves.Num());
	for (const FDirtbagMove& Move : In.Moves)
	{
		dirtbag::Move M;
		M.difficulty = Move.Difficulty;
		M.hold = static_cast<dirtbag::HoldType>(Move.Hold);
		M.restQuality = Move.RestQuality;
		M.reachBias = Move.ReachBias;
		M.crux = Move.bCrux;
		Out.moves.push_back(M);
	}
	return Out;
}

dirtbag::SessionState ToSim(const FDirtbagSessionState& In)
{
	dirtbag::SessionState Out;
	Out.skinLeft = In.SkinLeft;
	Out.warmth = In.Warmth;
	Out.psyche = In.Psyche;
	Out.attemptsMade = In.AttemptsMade;
	return Out;
}

dirtbag::ProjectMemory ToSim(const FDirtbagProjectMemory& In)
{
	dirtbag::ProjectMemory Out;
	Out.routeName = TCHAR_TO_UTF8(*In.RouteName);
	Out.attempts = In.Attempts;
	Out.bestHighpoint = In.BestHighpoint;
	Out.beta = In.Beta;
	Out.sent = In.bSent;
	Out.firstSendStyle = static_cast<dirtbag::Style>(In.FirstSendStyle);
	return Out;
}

FDirtbagRoute FromSim(const dirtbag::Route& In)
{
	FDirtbagRoute Out;
	Out.Name = UTF8_TO_TCHAR(In.name.c_str());
	Out.Grade = In.grade;
	Out.TrueGrade = In.trueGrade;
	Out.Type = static_cast<EDirtbagRouteType>(In.type);
	Out.Discipline = static_cast<EDirtbagDiscipline>(In.discipline);
	Out.Moves.Reserve(In.moves.size());
	for (const dirtbag::Move& M : In.moves)
	{
		FDirtbagMove Move;
		Move.Difficulty = M.difficulty;
		Move.Hold = static_cast<EDirtbagHold>(M.hold);
		Move.RestQuality = M.restQuality;
		Move.ReachBias = M.reachBias;
		Move.bCrux = M.crux;
		Out.Moves.Add(Move);
	}
	return Out;
}

FDirtbagMoveResult FromSim(const dirtbag::MoveResult& In)
{
	FDirtbagMoveResult Out;
	Out.Index = In.index;
	Out.Odds = In.odds;
	Out.PumpAfter = In.pumpAfter;
	Out.bSuccess = In.success;
	return Out;
}

FDirtbagAttemptResult FromSim(const dirtbag::AttemptResult& In)
{
	FDirtbagAttemptResult Out;
	Out.bSent = In.sent;
	Out.Highpoint = In.highpoint;
	Out.Style = static_cast<EDirtbagStyle>(In.style);
	Out.SkinCost = In.skinCost;
	Out.PeakPump = In.peakPump;
	Out.Timeline.Reserve(In.timeline.size());
	for (const dirtbag::MoveResult& MR : In.timeline)
	{
		Out.Timeline.Add(FromSim(MR));
	}
	return Out;
}

FDirtbagSessionState FromSim(const dirtbag::SessionState& In)
{
	FDirtbagSessionState Out;
	Out.SkinLeft = In.skinLeft;
	Out.Warmth = In.warmth;
	Out.Psyche = In.psyche;
	Out.AttemptsMade = In.attemptsMade;
	return Out;
}

FDirtbagProjectMemory FromSim(const dirtbag::ProjectMemory& In)
{
	FDirtbagProjectMemory Out;
	Out.RouteName = UTF8_TO_TCHAR(In.routeName.c_str());
	Out.Attempts = In.attempts;
	Out.BestHighpoint = In.bestHighpoint;
	Out.Beta = In.beta;
	Out.bSent = In.sent;
	Out.FirstSendStyle = static_cast<EDirtbagStyle>(In.firstSendStyle);
	return Out;
}

}  // namespace DirtbagConvert
