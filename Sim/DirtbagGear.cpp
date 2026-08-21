#include "DirtbagGear.h"

#include <algorithm>

namespace dirtbag {

void WearShoes(Shoes& shoes, int moves, int grade, const GearDials& dials) {
  if (moves <= 0) return;
  // Pulling harder eats rubber faster: it is toe pressure that kills shoes,
  // not distance.
  const double hard =
      1.0 + dials.wearPerGradeOverFive * std::max(0, grade - 5);
  shoes.wear = std::min(
      1.0, shoes.wear + (static_cast<double>(moves) / dials.shoeLifeMoves) * hard);
}

double ShoePenalty(const Shoes& shoes, bool edgingHold,
                   const GearDials& dials) {
  // Squared like skin, for the same reason: a slightly worn shoe is fine
  // and a dead one is a different sport.
  const double gone = Clamp01(shoes.wear);
  return dials.deadShoeGradePenalty * gone * gone *
         (edgingHold ? 1.0 : dials.deadShoeBiteOnGoodHolds);
}

bool CanResole(const Shoes& shoes, const GearDials& dials) {
  return shoes.resoles < dials.resolesPerPair;
}

bool Resole(Shoes& shoes, double& cash, bool sponsored,
            const GearDials& dials) {
  const double price = sponsored ? 0.0 : dials.resoleCost;
  if (!CanResole(shoes, dials) || cash < price) return false;
  cash -= price;
  shoes.resoles++;
  // Most of the performance back, and never quite new: each resole leaves a
  // little more of the shoe behind.
  shoes.wear = std::max(0.0, shoes.wear * (1.0 - dials.resoleRestores));
  return true;
}

bool BuyNewShoes(Shoes& shoes, double& cash, bool sponsored,
                 const GearDials& dials) {
  const double price = sponsored ? 0.0 : dials.newShoeCost;
  if (cash < price) return false;
  cash -= price;
  shoes.wear = 0.0;
  shoes.resoles = 0;
  shoes.pairsOwned++;
  return true;
}

std::string ShoeText(const Shoes& shoes, const GearDials& dials) {
  if (shoes.wear >= 0.95) return "you can see your toes";
  if (shoes.wear >= 0.8) return "the rubber is gone";
  if (shoes.wear >= dials.noticeablyWorn) {
    return CanResole(shoes, dials) ? "the rubber is going; worth a resole"
                                   : "the rubber is going, and the uppers "
                                     "will not take another";
  }
  if (shoes.wear >= 0.25) return "worn in";
  return "sharp edges, still";
}

}  // namespace dirtbag
