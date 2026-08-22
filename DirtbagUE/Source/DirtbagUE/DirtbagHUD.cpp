#include "DirtbagHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"

#include "DirtbagGameInstance.h"

namespace
{
const FLinearColor kPanel(0.f, 0.f, 0.f, 0.55f);
const FLinearColor kTrack(1.f, 1.f, 1.f, 0.15f);
const FLinearColor kInk(0.92f, 0.92f, 0.90f, 1.f);
const FLinearColor kDim(0.72f, 0.72f, 0.70f, 1.f);

// Pump reads cool while you have something left and red once you don't —
// the one colour in this HUD that is allowed to shout.
FLinearColor PumpColour(double Pump)
{
	if (Pump < 50.0) return FLinearColor(0.35f, 0.75f, 0.45f, 1.f);
	if (Pump < 80.0) return FLinearColor(0.90f, 0.70f, 0.25f, 1.f);
	return FLinearColor(0.85f, 0.25f, 0.20f, 1.f);
}
}  // namespace

void ADirtbagHUD::DrawBar(const FString& Label, double Frac, float X, float Y,
                          float W, float H, FLinearColor Fill)
{
	DrawText(Label, kDim, X, Y - 15.f, GEngine->GetSmallFont(), 1.f);
	DrawRect(kTrack, X, Y, W, H);
	const float Filled = W * FMath::Clamp(static_cast<float>(Frac), 0.f, 1.f);
	if (Filled > 0.f)
	{
		DrawRect(Fill, X, Y, Filled, H);
	}
}

void ADirtbagHUD::DrawNeeds(UDirtbagGameInstance* Game, float H)
{
	const float X = 28.f;
	float Y = 28.f;

	DrawRect(kPanel, X - 12.f, Y - 10.f, 250.f, 128.f);

	const int32 Hour =
	    FMath::Clamp(FMath::FloorToInt(static_cast<float>(Game->Day.Hour)), 0, 23);
	const int32 Minute = FMath::Clamp(
	    FMath::FloorToInt(static_cast<float>((Game->Day.Hour - Hour) * 60.0)), 0,
	    59);
	DrawText(FString::Printf(TEXT("DAY %d      %02d:%02d      $%.0f"),
	                         Game->Player.Day, Hour, Minute, Game->Player.Cash),
	         kInk, X, Y, GEngine->GetMediumFont(), 1.f);

	Y += 40.f;
	DrawBar(TEXT("ENERGY"), Game->Day.Energy / 100.0, X, Y, 210.f, 9.f,
	        FLinearColor(0.45f, 0.65f, 0.85f, 1.f));
	Y += 30.f;
	// Skin is the one that actually ends sessions, so it gets the warm colour.
	DrawBar(TEXT("SKIN"), Game->Player.Climber.Skin / 9.0, X, Y, 210.f, 9.f,
	        FLinearColor(0.85f, 0.62f, 0.45f, 1.f));
	Y += 30.f;
	DrawBar(TEXT("HUNGER"), Game->Day.Hunger / 100.0, X, Y, 210.f, 9.f,
	        FLinearColor(0.70f, 0.60f, 0.35f, 1.f));

	// The body, directly under the two bars it belongs with. Both of these
	// stay quiet until they are saying something: an injury line only exists
	// while you are hurt, and load says nothing at all until it starts to
	// matter. A HUD that prints "fresh" every frame for a season teaches you
	// to stop reading the line that will one day say otherwise.
	if (Game->IsHurt())
	{
		Y += 20.f;
		DrawText(Game->InjuryLine(), FLinearColor(0.85f, 0.35f, 0.30f, 1.f), X,
		         Y, GEngine->GetSmallFont(), 1.f);
	}
	const int32 Warning = Game->LoadWarning();
	if (Warning > 0)
	{
		Y += 20.f;
		// Amber at "carrying a load", red at the warning — the point at
		// which the injury roll starts happening at all. Measured over
		// twelve seasons, a climber who backs off here sends 38 against 24
		// and spends 55 days hurt against 1,467, so this is the most
		// valuable sentence on the screen and it is coloured like it.
		DrawText(Game->LoadLine(),
		         Warning >= 2 ? FLinearColor(0.85f, 0.35f, 0.30f, 1.f)
		                      : FLinearColor(0.90f, 0.75f, 0.30f, 1.f),
		         X, Y, GEngine->GetSmallFont(), 1.f);
	}

	// Conditions, under the bars. Outdoors this is the line the day is
	// planned around, so it is coloured by how good it actually is rather
	// than left as flat text you stop reading.
	Y += 34.f;
	const double Friction = Game->CurrentFriction();
	const FLinearColor ConditionsInk =
	    Game->bIndoors
	        ? kDim
	        : (Friction >= 0.62 ? FLinearColor(0.45f, 0.80f, 0.50f, 1.f)
	                            : (Friction >= 0.40
	                                   ? FLinearColor(0.90f, 0.75f, 0.30f, 1.f)
	                                   : FLinearColor(0.85f, 0.35f, 0.30f, 1.f)));
	DrawText(Game->ConditionsLine(), ConditionsInk, X, Y,
	         GEngine->GetSmallFont(), 1.f);

	// Debt, and the van. Both are the kind of thing that should be in your
	// eyeline on the morning you are deciding what the day is for.
	if (Game->Player.Owed > 0.0 || !Game->VanRuns() ||
	    !Game->VanLine().IsEmpty())
	{
		Y += 20.f;
		FString Line;
		if (Game->Player.Owed > 0.0)
		{
			Line = FString::Printf(TEXT("owe $%.0f"), Game->Player.Owed);
		}
		const FString Van = Game->VanNews.IsEmpty() ? Game->VanLine()
		                                            : Game->VanNews;
		if (!Van.IsEmpty())
		{
			Line += Line.IsEmpty() ? Van : FString::Printf(TEXT("   %s"), *Van);
		}
		DrawText(Line,
		         (!Game->VanRuns() || Game->Player.Owed > 0.0)
		             ? FLinearColor(0.85f, 0.45f, 0.35f, 1.f)
		             : kDim,
		         X, Y, GEngine->GetSmallFont(), 1.f);
	}

	// And its opposite. Drawn in the same eyeline as the debt and the van,
	// because it is the same question answered the other way: what does
	// today need from you. Only the War Chest ever answers "nothing".
	const FString Free = Game->FreeDayLine();
	if (!Free.IsEmpty())
	{
		Y += 20.f;
		DrawText(Free, FLinearColor(0.70f, 0.85f, 0.70f, 1.f), X, Y,
		         GEngine->GetSmallFont(), 1.f);
	}

	// The dog sits with the conditions line, because on a warm day they are
	// the same sentence.
	if (Game->Player.Dog.bAdopted || Game->Player.Dog.Bond > 0.0)
	{
		Y += 20.f;
		const bool bWorried = !Game->DogWorry.IsEmpty() ||
		                      Game->Player.Dog.Fed < 0.35;
		DrawText(Game->DogWorry.IsEmpty() ? Game->DogLine() : Game->DogWorry,
		         bWorried ? FLinearColor(0.85f, 0.45f, 0.35f, 1.f) : kDim, X,
		         Y, GEngine->GetSmallFont(), 1.f);
	}

	// Where you stand, when there is anything to stand on. StandingText is
	// already silent below +/-0.3, so this draws only when the valley has an
	// opinion — and a shut crag is red, because it changes what today can
	// be rather than merely how it feels.
	const FString Standing = Game->StandingLine();
	if (!Standing.IsEmpty())
	{
		Y += 20.f;
		DrawText(Standing,
		         Game->CragIsOpen() ? kDim
		                            : FLinearColor(0.85f, 0.35f, 0.30f, 1.f),
		         X, Y, GEngine->GetSmallFont(), 1.f);
	}

	// What the Lot did while you were not looking. Gold, like the naming
	// prompt, because losing a line and getting one are the same size of
	// event from opposite ends.
	if (!Game->LotNews.IsEmpty())
	{
		Y += 22.f;
		DrawText(Game->LotNews, FLinearColor(0.85f, 0.55f, 0.35f, 1.f), X, Y,
		         GEngine->GetMediumFont(), 1.f);
	}

	// The sponsor's news: the month's money, or the yearly verdict. Same
	// treatment as the Lot's, because it arrives the same way — overnight,
	// already decided.
	if (!Game->SponsorNews.IsEmpty())
	{
		Y += 22.f;
		DrawText(Game->SponsorNews, FLinearColor(0.85f, 0.55f, 0.35f, 1.f), X,
		         Y, GEngine->GetMediumFont(), 1.f);
	}

	// And what came out about you. Same size and colour as the Lot's news
	// because it arrives the same way — overnight, already true, and about
	// something you did rather than something you are choosing.
	if (!Game->EthicsNews.IsEmpty())
	{
		Y += 22.f;
		DrawText(Game->EthicsNews, FLinearColor(0.85f, 0.55f, 0.35f, 1.f), X, Y,
		         GEngine->GetMediumFont(), 1.f);
	}

	// A first ascent waiting to be named. Drawn here as well as in the
	// widget so that the moment is never invisible — if the naming widget is
	// missing or not wired yet, the game still says plainly that something
	// happened and is waiting on you.
	if (Game->bNamingPending)
	{
		Y += 22.f;
		DrawText(FString::Printf(
		             TEXT("FIRST ASCENT — %s is yours to name"),
		             *Game->NamingLineText),
		         FLinearColor(0.95f, 0.85f, 0.40f, 1.f), X, Y,
		         GEngine->GetMediumFont(), 1.f);
	}

	// And once it is named, the book's own line — the answer to the prompt
	// above, and the only place the name you chose is ever said back to you.
	if (!Game->LastAscentLine.IsEmpty())
	{
		Y += 22.f;
		DrawText(Game->LastAscentLine, FLinearColor(0.95f, 0.85f, 0.40f, 1.f),
		         X, Y, GEngine->GetMediumFont(), 1.f);
	}

	// The slow numbers, bottom left, small and dim — age, the kit in the van,
	// and a sponsor if there is one. None of these change in a day and none
	// of them are a decision you make on this screen, so they sit out of the
	// eyeline rather than competing with the body and the weather. Sponsor
	// says "nobody is calling" when there is no deal, which is true and not
	// worth a line, so it only appears once somebody is.
	FString Slow = Game->AgeLine();
	const FString Kit = Game->KitLine();
	if (!Kit.IsEmpty())
	{
		Slow += TEXT("      ") + Kit;
	}
	if (Game->Player.Sponsor.Tier != EDirtbagSponsorTier::None)
	{
		Slow += TEXT("      ") + Game->SponsorLine();
	}
	// Years nobody owned. The slowest number on the screen — it moves once a
	// year — and silent until the first whole one, because a streak of
	// eleven days is a fortnight rather than a record.
	const FString Years = Game->DirtbagYearLine();
	if (!Years.IsEmpty())
	{
		Slow += TEXT("      ") + Years;
	}
	// And what the town calls you, which is the slowest fact of all — it is
	// said once in a career and never revised.
	const FString Crew = Game->CrewLine();
	if (!Crew.IsEmpty())
	{
		Slow += TEXT("      ") + Crew;
	}
	// What the money was for. Sits with the slow numbers because owning one
	// is a fact about your life rather than a thing happening today — but
	// the War Chest counts down here, which is the one that moves.
	const FString Dream = Game->DreamLine();
	if (!Dream.IsEmpty())
	{
		Slow += TEXT("      ") + Dream;
	}
	DrawText(Slow, kDim, X, H - 56.f, GEngine->GetSmallFont(), 1.f);

	// And the morning it lands, said once, up where news goes rather than
	// down here with the slow numbers. A year is not a slow number on the
	// day it completes.
	if (!Game->DirtbagYearNews.IsEmpty())
	{
		DrawText(Game->DirtbagYearNews, FLinearColor(0.95f, 0.90f, 0.70f, 1.f),
		         X, H - 76.f, GEngine->GetMediumFont(), 1.f);
	}
	// The morning the town names you. Sits above the year line rather than
	// sharing it, because a career can do both in one night and one
	// overwriting the other would lose the rarer of the two.
	if (!Game->CrewNews.IsEmpty())
	{
		DrawText(Game->CrewNews, FLinearColor(0.95f, 0.90f, 0.70f, 1.f), X,
		         H - 96.f, GEngine->GetMediumFont(), 1.f);
	}

	// The career, small, bottom left — it is a slow number and reads like one.
	DrawText(Game->GetCareerLine(), kDim, X, H - 38.f, GEngine->GetSmallFont(),
	         1.f);
}

void ADirtbagHUD::DrawSession(UDirtbagGameInstance* Game, float W, float H)
{
	const FDirtbagSessionReadout& S = Game->SessionReadout;
	const float BarW = 460.f;
	const float X = (W - BarW) * 0.5f;
	float Y = H - 150.f;

	DrawRect(kPanel, X - 18.f, Y - 46.f, BarW + 36.f, 132.f);
	DrawText(S.RouteLine, kInk, X, Y - 40.f, GEngine->GetMediumFont(), 1.f);

	DrawBar(TEXT("PUMP"), S.Pump / 100.0, X, Y, BarW, 16.f, PumpColour(S.Pump));
	Y += 44.f;

	// The grip meter draws its latch window on the track, so the verb is
	// visible instead of memorised: release while the marker is in the band.
	DrawText(TEXT("GRIP"), kDim, X, Y - 15.f, GEngine->GetSmallFont(), 1.f);
	DrawRect(kTrack, X, Y, BarW, 13.f);
	const float BandX = X + BarW * static_cast<float>(S.WindowStart);
	const float BandW =
	    BarW * static_cast<float>(FMath::Max(0.0, S.WindowEnd - S.WindowStart));
	DrawRect(FLinearColor(0.35f, 0.75f, 0.45f, 0.35f), BandX, Y, BandW, 13.f);
	if (S.Grip >= 0.0)
	{
		const bool bInWindow = S.Grip >= S.WindowStart && S.Grip <= S.WindowEnd;
		DrawRect(bInWindow ? FLinearColor(0.40f, 0.85f, 0.50f, 1.f)
		                   : FLinearColor(0.85f, 0.85f, 0.85f, 0.85f),
		         X, Y, BarW * FMath::Clamp(static_cast<float>(S.Grip), 0.f, 1.f),
		         13.f);
	}

	if (S.Odds >= 0.0)
	{
		DrawText(FString::Printf(TEXT("next move  %.0f%%"), S.Odds * 100.0),
		         S.Odds > 0.7 ? FLinearColor(0.45f, 0.80f, 0.50f, 1.f)
		                      : (S.Odds > 0.4 ? FLinearColor(0.90f, 0.75f, 0.30f, 1.f)
		                                      : FLinearColor(0.85f, 0.35f, 0.30f, 1.f)),
		         X + BarW - 130.f, Y + 24.f, GEngine->GetSmallFont(), 1.f);
	}
}

void ADirtbagHUD::DrawFire(UDirtbagGameInstance* Game, float W, float H)
{
	const FDirtbagFireReadout& T = Game->FireReadout;
	const float PanelW = 480.f;
	const float X = (W - PanelW) * 0.5f;

	// The panel grows with what is on it rather than reserving room for the
	// biggest possible night: three reads at poker, one tell at dice, none
	// at blackjack, and no result line until there has been a result.
	const float Rows = 1.f + (T.Yours >= 0.0 ? 1.f : 0.f) +
	                   (T.YoursLine.IsEmpty() ? 0.f : 1.f) +
	                   (T.TableLine.IsEmpty() ? 0.f : 1.f) +
	                   static_cast<float>(T.Reads.Num()) +
	                   (T.LastLine.IsEmpty() ? 0.f : 1.f);
	const float PanelH = 74.f + Rows * 22.f;
	float Y = H - 110.f - PanelH;

	DrawRect(kPanel, X - 18.f, Y - 14.f, PanelW + 36.f, PanelH);

	// The heading says what game and what the evening has come to. The
	// running total is up here with the name rather than buried at the
	// bottom, because "down $140" is the fact that should decide whether
	// there is a next hand.
	FString Head = T.GameLine;
	if (T.HandsTonight > 0)
	{
		Head += FString::Printf(TEXT("      %d hand%s tonight,  %+.0f"),
		                        T.HandsTonight,
		                        T.HandsTonight == 1 ? TEXT("") : TEXT("s"),
		                        T.NightDelta);
	}
	DrawText(Head,
	         T.HandsTonight > 0 && T.NightDelta < 0.0
	             ? FLinearColor(0.85f, 0.55f, 0.40f, 1.f)
	             : kInk,
	         X, Y, GEngine->GetMediumFont(), 1.f);
	Y += 30.f;

	if (T.Yours >= 0.0)
	{
		// Poker's hand strength and blackjack's total against 21 are the
		// same shape of fact -- how much have you got -- so they get the
		// same bar, and it is the one thing on this table you see exactly.
		DrawBar(TEXT("YOUR HAND"), T.Yours, X, Y, 260.f, 11.f,
		        T.Yours > 1.0 ? FLinearColor(0.85f, 0.25f, 0.20f, 1.f)
		                      : FLinearColor(0.60f, 0.70f, 0.85f, 1.f));
		Y += 26.f;
	}
	if (!T.YoursLine.IsEmpty())
	{
		DrawText(T.YoursLine, kInk, X, Y, GEngine->GetMediumFont(), 1.f);
		Y += 22.f;
	}
	if (!T.TableLine.IsEmpty())
	{
		DrawText(T.TableLine, kDim, X, Y, GEngine->GetSmallFont(), 1.f);
		Y += 22.f;
	}

	// The reads, kept beside the decision they inform. This is the whole
	// reason the fire needed a table: a read that has scrolled away is a
	// read you did not get, and rapport is the skill these games are made
	// of.
	for (const FString& Read : T.Reads)
	{
		DrawText(Read, FLinearColor(0.80f, 0.78f, 0.70f, 1.f), X + 12.f, Y,
		         GEngine->GetSmallFont(), 1.f);
		Y += 22.f;
	}

	if (!T.LastLine.IsEmpty())
	{
		DrawText(FString::Printf(TEXT("%s  %+.0f"), *T.LastLine, T.LastDelta),
		         T.LastDelta >= 0.0 ? FLinearColor(0.45f, 0.80f, 0.50f, 1.f)
		                            : FLinearColor(0.85f, 0.45f, 0.35f, 1.f),
		         X, Y, GEngine->GetSmallFont(), 1.f);
		Y += 22.f;
	}

	// The verbs, named for tonight's game, and the stake. Both are always
	// on screen: the grammar is two keys and it is not worth memorising.
	const FString Verbs =
	    T.bHandLive
	        ? FString::Printf(TEXT("C  %s        F  %s"), *T.CommitVerb,
	                          *T.BackVerb)
	        : FString(TEXT("C  deal"));
	DrawText(Verbs, kInk, X, Y + 6.f, GEngine->GetSmallFont(), 1.f);
	DrawText(FString::Printf(TEXT("stake $%.0f   (1/2/3)"), T.Stake),
	         T.StakeNotch >= 2 ? FLinearColor(0.90f, 0.75f, 0.30f, 1.f) : kDim,
	         X + PanelW - 150.f, Y + 6.f, GEngine->GetSmallFont(), 1.f);
}

void ADirtbagHUD::DrawHUD()
{
	Super::DrawHUD();

	UDirtbagGameInstance* Game = Cast<UDirtbagGameInstance>(GetGameInstance());
	if (!Game || !Canvas || !GEngine)
	{
		return;
	}

	const float W = static_cast<float>(Canvas->SizeX);
	const float H = static_cast<float>(Canvas->SizeY);

	DrawNeeds(Game, H);
	if (Game->SessionReadout.bActive)
	{
		DrawSession(Game, W, H);
	}
	else if (Game->FireReadout.bActive)
	{
		// Never both. You are not at the fire while you are on the wall,
		// and if a bug ever says you are, the wall is the one that matters.
		DrawFire(Game, W, H);
	}
}
