// The session, drawn. Reads the game instance and nothing else — every
// number here was decided by the sim; this only makes it legible.
// Deliberately canvas-drawn rather than UMG: blockout quality, no assets,
// and the same fields a real widget will bind to later.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "DirtbagHUD.generated.h"

UCLASS()
class ADirtbagHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	/** Label + track + fill, the one shape this HUD is made of. */
	void DrawBar(const FString& Label, double Frac, float X, float Y, float W,
	             float H, FLinearColor Fill);

	void DrawNeeds(class UDirtbagGameInstance* Game, float W, float H);
	void DrawSession(class UDirtbagGameInstance* Game, float W, float H);
};
