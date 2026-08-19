#include "DirtbagDog.h"

#include <algorithm>

namespace dirtbag {

bool FeedDog(Dog& dog, double& cash, const DogDials& dials) {
  if (cash < dials.foodCost) {
    return false;
  }
  cash -= dials.foodCost;
  dog.fed = std::min(1.0, dog.fed + dials.fedPerMeal);
  dog.bond = std::min(1.0, dog.bond + dials.bondPerMeal);

  // No ceremony, no decision, no prompt. You fed it enough times and now it
  // sleeps under your van.
  if (!dog.adopted && dog.bond >= dials.adoptAtBond) {
    dog.adopted = true;
  }
  return true;
}

void DogDay(Dog& dog, bool together, const DogDials& dials) {
  dog.fed = std::max(0.0, dog.fed - dials.hungerPerDay);
  if (together) {
    dog.bond = std::min(1.0, dog.bond + dials.bondPerDayTogether);
  } else {
    dog.bond = std::max(0.0, dog.bond - dials.bondDecayAlone);
  }
}

double DogPsyche(const Dog& dog, const DogDials& dials) {
  if (!dog.adopted) {
    return 0.0;   // a stray is not company, it is a stray
  }
  if (dog.fed < dials.hungryBelow) {
    // Sitting at the crag knowing the dog is hungry is its own thing.
    return -dials.psycheWhenHungry;
  }
  return dials.psycheFromBond * dog.bond;
}

bool VanIsSafe(double vanTempF, const DogDials& dials) {
  return vanTempF < dials.warmVanF;
}

double VanGuilt(const Dog& dog, double vanTempF, const DogDials& dials) {
  if (!dog.adopted || VanIsSafe(vanTempF, dials)) {
    return 0.0;
  }
  const double over = vanTempF - dials.warmVanF;
  return dials.guiltAtWarm + dials.guiltPerTenDegrees * (over / 10.0);
}

std::string DogText(const Dog& dog, const DogDials& dials) {
  if (!dog.adopted) {
    return dog.bond > 0.2 ? "the stray is hanging around again"
                          : "there is a stray at the Lot";
  }
  if (dog.fed < dials.hungryBelow) {
    return "the dog has not eaten since yesterday";
  }
  if (dog.bond > 0.8) {
    return "the dog is asleep under the van";
  }
  return "the dog is around somewhere";
}

}  // namespace dirtbag
