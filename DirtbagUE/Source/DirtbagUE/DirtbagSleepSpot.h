// The van, or wherever the day ends. Kept as its own class so levels that
// already placed one keep working; all the behaviour lives in ADirtbagDaySpot.

#pragma once

#include "CoreMinimal.h"
#include "DirtbagDaySpot.h"

#include "DirtbagSleepSpot.generated.h"

UCLASS()
class ADirtbagSleepSpot : public ADirtbagDaySpot
{
	GENERATED_BODY()

public:
	ADirtbagSleepSpot();
};
