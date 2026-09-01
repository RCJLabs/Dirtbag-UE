// Which zone the player is standing in, as a fact about the level.
//
// **This exists because of a bug that does not exist yet and will exist the
// day the Lot blockout lands.**
//
// `UDirtbagGameInstance::CurrentZone` is written by exactly one line in the
// whole project: a travel spot's arrival. That is correct today, because
// today every zone is a pocket reached by pressing E on a trigger volume —
// there is no ground between them to walk across.
//
// Phase 6's whole design is that this stops being true. Eleven of the
// sixteen zones are **connected ground**: one continuous landscape you walk
// between, where the Lot and downtown are diagonal neighbours and you can
// see one from the other. The moment that exists, a player can walk from
// the Lot into the Outskirts without pressing anything — and `CurrentZone`
// still says the Lot.
//
// That is not a crash and it is not visible. It is worse: `WalkHours` reads
// `CurrentZone` as the *origin* of the next walk, so every walk after the
// first is priced from wherever you last *travelled* rather than from where
// you are. A twenty-minute walk costs somebody else's twenty minutes. The
// same class of silent-wrong-number that the widening itself introduced and
// that `CurrentZone` was added to fix — one layer further out.
//
// So: a box you drop over a region while you are sculpting it. Walk in, and
// the world knows where you are.
//
// ## Why an actor and not a query
//
// The alternative is asking "which zone is this world position in?" from a
// table of bounds. That means the level's shape lives in a header, two
// places have to agree about where the Outskirts start, and the answer is
// wrong the first time somebody drags the landscape. **The level is the
// authority on where places are**, which is what a volume says and a table
// does not.
//
// ## Placing them
//
// One per walkable zone, sized to cover that zone's ground, overlapping at
// the borders is fine — the last one entered wins, which is what crossing a
// border means. Van-only destinations do not need one: you arrive there by
// travelling, and the travel spot already says so. Placing one anyway is
// harmless.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DirtbagGameInstance.h"

#include "DirtbagZoneVolume.generated.h"

class UBoxComponent;
class USceneComponent;

UCLASS()
class ADirtbagZoneVolume : public AActor
{
	GENERATED_BODY()

public:
	ADirtbagZoneVolume();

protected:
	virtual void BeginPlay() override;

	/** Which zone this patch of ground is. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Zone")
	EDirtbagZone Zone = EDirtbagZone::Lot;

	/** Say the zone's name and its one line when you first walk in, once a
	 *  day. **Nine of the eleven walkable zones have no content**, so
	 *  without this the wider map is silent grey boxes — and a map you
	 *  cannot tell apart is not a map.
	 *
	 *  Turn it off for the Lot once the Lot looks like the Lot: you do not
	 *  need telling where you sleep. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Zone")
	bool bAnnounceOnEntry = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dirtbag")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dirtbag")
	TObjectPtr<UBoxComponent> Bounds;

	UFUNCTION()
	void OnEnter(UPrimitiveComponent* Overlapped, AActor* Other,
	             UPrimitiveComponent* OtherComp, int32 BodyIndex,
	             bool bFromSweep, const FHitResult& Sweep);

private:
	UPROPERTY()
	TObjectPtr<UDirtbagGameInstance> Game;

	/** The day this zone last announced itself, so walking back and forth
	 *  across a border does not narrate. Not saved: it is worth one line to
	 *  re-earn and a save version bump for it would not be. */
	int32 AnnouncedOnDay = -1;
};
