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
// Break a paragraph at word boundaries near a column.
//
// The canvas draws a string as one line however long it is, and the career
// epitaph is a paragraph -- so without this, twenty years of somebody's
// life runs off the right-hand edge of the screen. Measured in characters
// rather than pixels because these are fixed-width debug fonts and a real
// text measure would be a widget's job, which is the thing this HUD exists
// to avoid needing.
TArray<FString> WrapToWidth(const FString& Text, int32 Columns)
{
	TArray<FString> Lines;
	TArray<FString> Words;
	Text.ParseIntoArray(Words, TEXT(" "), true);

	FString Line;
	for (const FString& Word : Words)
	{
		if (!Line.IsEmpty() && Line.Len() + 1 + Word.Len() > Columns)
		{
			Lines.Add(Line);
			Line.Reset();
		}
		Line += Line.IsEmpty() ? Word : TEXT(" ") + Word;
	}
	if (!Line.IsEmpty())
	{
		Lines.Add(Line);
	}
	// A caller that hands us nothing should get nothing to draw, not one
	// empty line that silently eats a row of layout.
	return Lines;
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

	// The panel grows by a line when the control reminder is up, which is
	// only ever until the first latch of a session.
	const float PanelH = S.bShowTheVerb ? 154.f : 132.f;
	DrawRect(kPanel, X - 18.f, Y - 46.f, BarW + 36.f, PanelH);
	DrawText(S.RouteLine, kInk, X, Y - 40.f, GEngine->GetMediumFont(), 1.f);
	// "Attempt 14" is true for the whole go and is most of what a session
	// feels like, so it sits beside the route for all of it rather than
	// flashing once at the start.
	if (S.Attempt > 0)
	{
		DrawText(FString::Printf(TEXT("attempt %d"), S.Attempt), kDim,
		         X + BarW - 90.f, Y - 38.f, GEngine->GetSmallFont(), 1.f);
	}

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

	// The verb, under the bar it describes, until the first latch. It was a
	// six-second toast fired at the top of the route -- the one moment a
	// first-time player is watching the climber rather than reading text.
	if (S.bShowTheVerb)
	{
		DrawText(TEXT("HOLD Space to load the move.  Release inside the "
		              "green to latch it, early to shake out."),
		         FLinearColor(0.60f, 0.80f, 0.90f, 1.f), X, Y + 42.f,
		         GEngine->GetSmallFont(), 1.f);
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

float ADirtbagHUD::DrawPrompt(UDirtbagGameInstance* Game, float W, float H)
{
	const FDirtbagPrompt& P = Game->Prompt;
	if (!P.bActive || P.Lines.Num() == 0)
	{
		return H;
	}

	const float PanelW = 560.f;
	const float X = (W - PanelW) * 0.5f;
	const float PanelH = 18.f + static_cast<float>(P.Lines.Num()) * 22.f;
	const float Top = H - 22.f - PanelH;

	DrawRect(kPanel, X - 18.f, Top, PanelW + 36.f, PanelH);

	float Y = Top + 9.f;
	for (const FDirtbagPromptLine& Line : P.Lines)
	{
		// Three tones and no more. A prompt that colours every clause is a
		// prompt nobody reads: plain is what is here, good is worth
		// crossing a valley for, blocked is in your way.
		FLinearColor Ink = kInk;
		if (Line.Tone == EDirtbagPromptTone::Good)
		{
			Ink = FLinearColor(0.95f, 0.85f, 0.40f, 1.f);
		}
		else if (Line.Tone == EDirtbagPromptTone::Blocked)
		{
			Ink = FLinearColor(0.85f, 0.45f, 0.35f, 1.f);
		}
		DrawText(Line.Text, Ink, X, Y, GEngine->GetMediumFont(), 1.f);
		Y += 22.f;
	}
	return Top;
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

void ADirtbagHUD::DrawTravel(UDirtbagGameInstance* Game, float W, float H)
{
	const FDirtbagTravelReadout& T = Game->TravelReadout;
	const float Alpha = FMath::Clamp(static_cast<float>(T.Progress), 0.f, 1.f);

	// The road. A backdrop if the spot has one; otherwise a sky, a ground
	// and a horizon, which is enough to read as somewhere rather than as a
	// loading screen. Everything here works with no assets assigned -- the
	// same rule the sound slots ship under.
	const float Horizon = H * 0.62f;
	if (T.Backdrop)
	{
		// Stretched to fill rather than DrawTextureSimple, which scales and
		// would letterbox a backdrop that is not the player's aspect.
		DrawTexture(T.Backdrop, 0.f, 0.f, W, H, 0.f, 0.f, 1.f, 1.f);
	}
	else
	{
		DrawRect(FLinearColor(0.09f, 0.11f, 0.15f, 1.f), 0.f, 0.f, W, Horizon);
		DrawRect(FLinearColor(0.13f, 0.12f, 0.10f, 1.f), 0.f, Horizon, W,
		         H - Horizon);
		DrawRect(FLinearColor(0.35f, 0.32f, 0.28f, 1.f), 0.f, Horizon - 2.f, W,
		         2.f);
	}

	// The van, crossing it. Off the near edge at 0 and off the far one at
	// 1, so it enters and leaves rather than starting and stopping in
	// frame -- a van parked at the edge of the screen for a beat at each
	// end reads as a bug.
	const float VanW = 220.f;
	const float VanH = 110.f;
	const float VanX = FMath::Lerp(-VanW, W, Alpha);
	const float VanY = Horizon - VanH * 0.72f;
	if (!T.bOnFoot)
	{
		if (T.VanImage)
		{
			DrawTexture(T.VanImage, VanX, VanY, VanW, VanH, 0.f, 0.f, 1.f,
			            1.f);
		}
		else
		{
			// A shape, honestly a shape. It is a van the way the blockout
			// wall was a boulder.
			DrawRect(FLinearColor(0.72f, 0.68f, 0.58f, 1.f), VanX,
			         VanY + VanH * 0.35f, VanW, VanH * 0.5f);
			DrawRect(FLinearColor(0.55f, 0.52f, 0.45f, 1.f),
			         VanX + VanW * 0.08f, VanY + VanH * 0.1f, VanW * 0.5f,
			         VanH * 0.3f);
		}
	}

	// Where you are going, big, centred, and the only thing on screen that
	// is not moving.
	const FString Head =
	    T.bOnFoot ? FString::Printf(TEXT("Walking to %s"), *T.ToName)
	              : FString::Printf(TEXT("Driving to %s"), *T.ToName);
	DrawText(Head, kInk, W * 0.5f - 150.f, H * 0.16f, GEngine->GetLargeFont(),
	         1.4f);

	// The clock, running. This is the cost of the trip made visible: a
	// forty-minute approach eats a window, and a number that ticks says so
	// better than one that jumps.
	const int32 Hour =
	    FMath::Clamp(FMath::FloorToInt(static_cast<float>(T.ShownHour)), 0, 23);
	const int32 Minute = FMath::Clamp(
	    FMath::FloorToInt(static_cast<float>((T.ShownHour - Hour) * 60.0)), 0,
	    59);
	FString Cost = FString::Printf(TEXT("%02d:%02d      %.0f minutes"), Hour,
	                               Minute, T.Minutes);
	if (!T.bOnFoot && T.Fuel > 0.0)
	{
		Cost += FString::Printf(TEXT("      $%.0f of fuel"), T.Fuel);
	}
	DrawText(Cost, kDim, W * 0.5f - 150.f, H * 0.16f + 34.f,
	         GEngine->GetMediumFont(), 1.f);

	// And the one thing the road ever has to say beyond that.
	if (!T.Note.IsEmpty())
	{
		DrawText(T.Note, FLinearColor(0.85f, 0.45f, 0.35f, 1.f),
		         W * 0.5f - 150.f, H * 0.16f + 62.f, GEngine->GetMediumFont(),
		         1.f);
	}

	DrawText(TEXT("any key to skip"), FLinearColor(0.6f, 0.6f, 0.58f, 0.75f),
	         W - 190.f, H - 46.f, GEngine->GetSmallFont(), 1.f);
}

void ADirtbagHUD::DrawHandover(UDirtbagGameInstance* Game, float W, float H)
{
	const FDirtbagHandoverReadout& O = Game->Handover;
	const float X = W * 0.5f - 380.f;
	float Y = H * 0.16f;

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.88f), 0.f, 0.f, W, H);

	// An epitaph means a career ended here; without one this screen is a
	// fresh climber putting a name to themselves, which is the same
	// furniture doing a much smaller job.
	const bool bSomethingEnded = !O.Epitaph.IsEmpty();

	if (bSomethingEnded)
	{
		DrawText(O.Generation > 0
		             ? FString::Printf(TEXT("Generation %d"), O.Generation)
		             : FString(TEXT("A career")),
		         kDim, X, Y, GEngine->GetMediumFont(), 1.f);
		Y += 40.f;

		// The paragraph. Wrapped by hand at a sane column, because the
		// canvas will not do it and a career summary running off the edge
		// of the screen is a poor way to end twenty years.
		for (const FString& Line : WrapToWidth(O.Epitaph, 78))
		{
			DrawText(Line, kInk, X, Y, GEngine->GetMediumFont(), 1.f);
			Y += 26.f;
		}
		Y += 26.f;
	}

	switch (O.Step)
	{
	case EDirtbagHandoverStep::Epitaph:
		DrawText(TEXT("E to hand it on."), kInk, X, Y,
		         GEngine->GetMediumFont(), 1.f);
		break;

	case EDirtbagHandoverStep::Choosing:
	{
		DrawText(bSomethingEnded
		             ? TEXT("Somebody turns up at the Lot.")
		             : TEXT("Somebody asks who you are."),
		         kInk, X, Y, GEngine->GetMediumFont(), 1.f);
		Y += 34.f;
		for (int32 i = 0; i < O.Candidates.Num(); i++)
		{
			DrawText(FString::Printf(TEXT("%d.  %s"), i + 1, *O.Candidates[i]),
			         FLinearColor(0.60f, 0.80f, 0.90f, 1.f), X + 14.f, Y,
			         GEngine->GetLargeFont(), 1.f);
			Y += 34.f;
		}
		break;
	}

	default:
	{
		DrawText(bSomethingEnded
		             ? FString::Printf(
		                   TEXT("%s. Twenty-four, nothing in the fingers, and "
		                        "a valley with your predecessor's name on it."),
		                   *O.Arrival)
		             : FString::Printf(TEXT("%s, then."), *O.Arrival),
		         kInk, X, Y, GEngine->GetMediumFont(), 1.f);
		Y += 40.f;

		// The book, read here rather than over the epitaph -- these are
		// the lines *earlier* careers put up, and the one that just ended
		// is newly among them. This is the whole point of the system: what
		// survives you is a page somebody else opens.
		if (bSomethingEnded)
		{
			if (O.Guidebook.Num() > 0)
			{
				DrawText(TEXT("IN THE BOOK"), kDim, X, Y,
				         GEngine->GetSmallFont(), 1.f);
				Y += 26.f;
				for (const FString& Entry : O.Guidebook)
				{
					DrawText(Entry, FLinearColor(0.95f, 0.85f, 0.40f, 1.f),
					         X + 14.f, Y, GEngine->GetMediumFont(), 1.f);
					Y += 24.f;
				}
			}
			else
			{
				// Said plainly rather than left blank. Putting nothing up
				// is a real career and the game should not act as though
				// the page failed to load.
				DrawText(TEXT("Nothing in the book yet. Plenty of days, "
				              "though."),
				         kDim, X, Y, GEngine->GetMediumFont(), 1.f);
				Y += 24.f;
			}
			Y += 20.f;
		}

		DrawText(TEXT("E to get on with it."), kDim, X, Y,
		         GEngine->GetSmallFont(), 1.f);
		break;
	}
	}
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

	// A career ending outranks everything, including the road -- you cannot
	// be driving and retiring at once, but if a bug ever says you are, this
	// is the one that matters.
	if (Game->Handover.bActive)
	{
		DrawHandover(Game, W, H);
		return;
	}

	// Between places: the road takes the whole screen and nothing else
	// draws. There is no decision available until you arrive, so a pump bar
	// and a shop prompt over the top of it would be furniture.
	if (Game->TravelReadout.bActive)
	{
		DrawTravel(Game, W, H);
		return;
	}

	DrawNeeds(Game, H);

	// The prompt owns the bottom of the screen and everything else stacks
	// on top of it, so a three-line wall prompt cannot end up underneath
	// the pump bar.
	const float Floor = DrawPrompt(Game, W, H);
	if (Game->SessionReadout.bActive)
	{
		DrawSession(Game, W, Floor);
	}
	else if (Game->FireReadout.bActive)
	{
		// Never both. You are not at the fire while you are on the wall,
		// and if a bug ever says you are, the wall is the one that matters.
		DrawFire(Game, W, Floor);
	}
}
