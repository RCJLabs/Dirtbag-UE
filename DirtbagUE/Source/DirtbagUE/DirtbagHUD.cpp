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

	// What the Lot did while you were not looking. Gold, like the naming
	// prompt, because losing a line and getting one are the same size of
	// event from opposite ends.
	if (!Game->LotNews.IsEmpty())
	{
		Y += 22.f;
		DrawText(Game->LotNews, FLinearColor(0.85f, 0.55f, 0.35f, 1.f), X, Y,
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
}
