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

	/** What the keys do where you are standing. Drawn along the bottom and
	 *  returns the Y it starts at, which is the floor the session bar and
	 *  the fire table sit on — so a three-line prompt pushes them up
	 *  instead of being drawn underneath one of them. */
	float DrawPrompt(class UDirtbagGameInstance* Game, float W, float H);

	/** The road. Takes the whole screen and suppresses everything else —
	 *  you are between places, and nothing you could read is actionable
	 *  until you arrive. */
	void DrawTravel(class UDirtbagGameInstance* Game, float W, float H);

	void DrawNeeds(class UDirtbagGameInstance* Game, float H);
	void DrawSession(class UDirtbagGameInstance* Game, float W, float H);
	/** The fire's table. Sits where the session bar sits, because they are
	 *  never both up: the two are the same slot in the player's attention,
	 *  one for the wall and one for the evening. */
	void DrawFire(class UDirtbagGameInstance* Game, float W, float H);
};
