#include "DirtbagDreams.h"

#include <algorithm>

namespace dirtbag {

namespace {

int IndexOf(Dream d) {
  switch (d) {
    case Dream::Rig:      return 0;
    case Dream::WarChest: return 1;
    case Dream::HomeBase: return 2;
    case Dream::None:     break;
  }
  return -1;
}

}  // namespace

const char* DreamName(Dream d) {
  switch (d) {
    case Dream::Rig:      return "the Rig";
    case Dream::WarChest: return "the War Chest";
    case Dream::HomeBase: return "Home Base";
    case Dream::None:     break;
  }
  return "nothing in particular";
}

const char* DreamBlurb(Dream d) {
  switch (d) {
    case Dream::Rig:
      return "A van that starts. Everything new, and parts that last.";
    case Dream::WarChest:
      return "A year where nobody needs anything from you.";
    case Dream::HomeBase:
      return "An address. A bed, a shower, and rent for the rest of your "
             "life.";
    case Dream::None:
      break;
  }
  return "";
}

bool HasDream(const Dreams& dreams, Dream d) {
  const int i = IndexOf(d);
  return i >= 0 && dreams.has[i];
}

double CostOf(Dream d, const DreamDials& dials) {
  const int i = IndexOf(d);
  return i >= 0 ? dials.cost[i] : 0.0;
}

bool CanAfford(const Dreams& dreams, Dream d, double cash,
               const DreamDials& dials) {
  const int i = IndexOf(d);
  if (i < 0 || dreams.has[i]) return false;
  return cash >= dials.cost[i];
}

bool BuyDream(Dreams& dreams, Van& van, double& cash, Dream d,
              const DreamDials& dials) {
  if (!CanAfford(dreams, d, cash, dials)) return false;
  const int i = IndexOf(d);

  cash -= dials.cost[i];
  dreams.has[i] = true;
  // You got the thing you were saving for, so you are no longer saving for
  // it. Choosing the next one is the player's business, not ours.
  if (dreams.working == d) dreams.working = Dream::None;

  switch (d) {
    case Dream::Rig:
      // A Rig arrives with everything new. The multiplier lives on the van
      // rather than here because wear is applied in DriveVan and has to see
      // it on every drive.
      for (int p = 0; p < kVanPartCount; p++) {
        van.parts[p].wear = 0.0;
        van.parts[p].patches = 0;
      }
      van.rig = true;
      break;
    case Dream::WarChest:
      // Not a lump you spend down. A year you have already bought.
      dreams.seasonOffDaysLeft = dials.warChestDays;
      break;
    case Dream::HomeBase:
      // Nothing immediate. The bed pays out every night and the rent asks
      // every morning, both in DreamDay.
      break;
    case Dream::None:
      break;
  }
  return true;
}

void DreamDay(Dreams& dreams, double& cash, double& owed,
              const DreamDials& dials) {
  if (dreams.seasonOffDaysLeft > 0) dreams.seasonOffDaysLeft--;

  if (!HasDream(dreams, Dream::HomeBase)) return;
  // Rent behaves like every other bill in the game: cash floors at nothing
  // and the shortfall waits. An address you cannot afford is not repossessed
  // on the day, it is a debt with a door on it.
  const double due = dials.homeBaseRentPerDay;
  const double paid = std::min(cash, due);
  cash -= paid;
  owed += due - paid;
}

bool NoNeedToWork(const Dreams& dreams) {
  return dreams.seasonOffDaysLeft > 0;
}

double SkinBonus(const Dreams& dreams, const DreamDials& dials) {
  return HasDream(dreams, Dream::HomeBase) ? dials.homeBaseSkinBonus : 0.0;
}

std::string DreamText(const Dreams& dreams) {
  std::string out;
  const Dream all[kDreamCount] = {Dream::Rig, Dream::WarChest,
                                  Dream::HomeBase};
  int said = 0;
  for (Dream d : all) {
    if (!HasDream(dreams, d)) continue;
    if (said == 0) {
      out = DreamName(d);
    } else {
      out += ", ";
      out += DreamName(d);
    }
    said++;
  }
  if (said > 0) out += ".";

  // The one that is still running gets said whichever way round, because a
  // year of not working is the only dream you can watch go.
  if (dreams.seasonOffDaysLeft > 0) {
    if (!out.empty()) out += "  ";
    out += std::to_string(dreams.seasonOffDaysLeft) +
           " days of the War Chest left.";
  }
  return out;
}

}  // namespace dirtbag
