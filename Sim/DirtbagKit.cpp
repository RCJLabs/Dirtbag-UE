#include "DirtbagKit.h"

#include <algorithm>

namespace dirtbag {

namespace {

// Nothing here half-buys. Either the money is there and the thing is yours,
// or nothing happened at all.
bool Afford(double& cash, double price) {
  if (cash < price) return false;
  cash -= price;
  return true;
}

}  // namespace

bool BuyPad(Kit& kit, double& cash, const KitDials& dials) {
  if (!Afford(cash, dials.padCost)) return false;
  kit.pads++;
  return true;
}

bool BuyHangboard(Kit& kit, double& cash, const KitDials& dials) {
  if (kit.hangboard) return false;   // you only need the one
  if (!Afford(cash, dials.hangboardCost)) return false;
  kit.hangboard = true;
  return true;
}

bool RenewMembership(Kit& kit, double& cash, const KitDials& dials) {
  if (!Afford(cash, dials.membershipCost)) return false;
  // Renewing early stacks rather than resets, because losing the days you
  // already paid for would be a punishment for being organised.
  kit.membershipDaysLeft += dials.membershipDays;
  return true;
}

void KitDay(Kit& kit) {
  if (kit.membershipDaysLeft > 0) kit.membershipDaysLeft--;
}

bool IsGymMember(const Kit& kit) { return kit.membershipDaysLeft > 0; }

double PaddingFrom(const Kit& kit, const KitDials& dials) {
  const int matter = std::max(1, dials.padsThatMatter);
  return std::min(
      Clamp01(dials.mostFoamCanDo),
      Clamp01(static_cast<double>(kit.pads) / static_cast<double>(matter)));
}

std::string PadOfferText(const Kit& kit, const KitDials& dials) {
  if (kit.pads >= std::max(1, dials.padsThatMatter)) return std::string();
  std::string out = "A second pad, $";
  out += std::to_string(static_cast<int>(dials.padCost));
  out += ". Better landings, and one less reason to be brave.";
  return out;
}

std::string KitText(const Kit& kit) {
  std::string out;
  if (kit.pads == 0) {
    out = "no pad";
  } else if (kit.pads == 1) {
    out = "one pad";
  } else {
    out = "pads";
  }
  if (kit.hangboard) out += ", a board in the van";
  if (IsGymMember(kit)) {
    out += ", and the gym until the ";
    out += kit.membershipDaysLeft == 1 ? "end of the day"
                                       : "month runs out";
  }
  return out;
}

}  // namespace dirtbag
