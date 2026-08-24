#include "DirtbagGym.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {
namespace {

int Slot(GymPrice p) { return static_cast<int>(p); }
int Slot(GymSetMix m) { return static_cast<int>(m); }
int Slot(GymEquip e) { return static_cast<int>(e); }
int Slot(GymCampaign c) { return static_cast<int>(c); }

bool CampaignRunning(const Gym& gym, int day) {
  return gym.campaign != GymCampaign::None && gym.campaignUntil >= day;
}

}  // namespace

const char* GymPriceName(GymPrice p) {
  switch (p) {
    case GymPrice::Budget: return "Budget";
    case GymPrice::Standard: return "Standard";
    case GymPrice::Premium: return "Premium";
    default: return "";
  }
}

const char* GymSetMixName(GymSetMix m) {
  switch (m) {
    case GymSetMix::Beginner: return "Beginner-Friendly";
    case GymSetMix::AllComers: return "All-Comers";
    case GymSetMix::Hardcore: return "Hardcore";
    default: return "";
  }
}

const char* GymEquipName(GymEquip e) {
  switch (e) {
    case GymEquip::AsBought: return "As Bought";
    case GymEquip::HoldsAndMats: return "New Holds & Mats";
    case GymEquip::FullRenovation: return "Full Renovation";
    default: return "";
  }
}

const char* GymCampaignName(GymCampaign c) {
  switch (c) {
    case GymCampaign::Flyers: return "Flyers & Local Ads";
    case GymCampaign::Social: return "Social Media Push";
    default: return "";
  }
}

const char* GymPriceBlurb(GymPrice p) {
  switch (p) {
    case GymPrice::Budget:
      return "Cheap punch cards. Packed floor, thin margins.";
    case GymPrice::Standard:
      return "Fair market rate. Steady, unremarkable.";
    case GymPrice::Premium:
      return "Boutique pricing. Fewer members, real money each -- if they stay.";
    default: return "";
  }
}

const char* GymSetMixBlurb(GymSetMix m) {
  switch (m) {
    case GymSetMix::Beginner:
      return "Easy, forgiving sets. Packs the floor with newcomers, thin margins.";
    case GymSetMix::AllComers:
      return "A bit of everything. No lean either way.";
    case GymSetMix::Hardcore:
      return "Serious training walls. Fewer members, but they are dialled in "
             "and paying for it.";
    default: return "";
  }
}

const char* GymEquipBlurb(GymEquip e) {
  switch (e) {
    case GymEquip::AsBought:
      return "Whatever came with the building. It works.";
    case GymEquip::HoldsAndMats:
      return "Fresh rubber, real crash pads. A better first impression.";
    case GymEquip::FullRenovation:
      return "A real board, real showers, real AC. This is a destination now.";
    default: return "";
  }
}

const char* GymCampaignBlurb(GymCampaign c) {
  switch (c) {
    case GymCampaign::Flyers:
      return "Posters at the coffee shop, a stack of cards by the register.";
    case GymCampaign::Social:
      return "Paid reach, a real campaign. Costs more, pulls a lot more.";
    default: return "";
  }
}

bool BuyTheGym(Gym& gym, double& cash, const std::string& name, int day,
               const GymDials& dials) {
  if (gym.owned) return false;
  if (cash < dials.price) return false;
  cash -= dials.price;
  gym = Gym{};
  gym.owned = true;
  gym.name = name;
  gym.ownedDay = day;
  gym.lastTickDay = day;
  gym.members = static_cast<double>(dials.seedMembers);
  return true;
}

void SetPrice(Gym& gym, GymPrice p) {
  // Free, and instant to decide -- what is not instant is the floor
  // noticing, which is `memberDrift`'s job.
  if (gym.owned) gym.price = p;
}

void SetMix(Gym& gym, GymSetMix m) {
  if (gym.owned) gym.mix = m;
}

bool UpgradeEquipment(Gym& gym, double& cash, const GymDials& dials) {
  if (!gym.owned) return false;
  const int next = Slot(gym.equip) + 1;
  if (next >= kGymEquipCount) return false;
  // **Cumulative, not incremental**, so the ladder is a direct lookup and
  // the second rung costs what is left of it rather than the whole price.
  const double owing = dials.equipCost[next] - dials.equipCost[Slot(gym.equip)];
  if (cash < owing) return false;
  cash -= owing;
  gym.equip = static_cast<GymEquip>(next);
  return true;
}

bool Hire(Gym& gym, double& cash, bool frontDesk, const GymDials& dials) {
  if (!gym.owned) return false;
  bool& seat = frontDesk ? gym.frontDesk : gym.setter;
  if (seat) return false;
  const double cost = frontDesk ? dials.frontDeskCost : dials.setterCost;
  if (cash < cost) return false;
  cash -= cost;
  seat = true;
  return true;
}

bool LaunchCampaign(Gym& gym, double& cash, GymCampaign which, int day,
                    const GymDials& dials) {
  if (!gym.owned || which == GymCampaign::None) return false;
  // One at a time. Launching does not stack, and it is gated on none
  // currently running rather than on the money.
  if (CampaignRunning(gym, day)) return false;
  const double cost = dials.campaignCost[Slot(which)];
  if (cash < cost) return false;
  cash -= cost;
  gym.campaign = which;
  gym.campaignUntil = day + dials.campaignDays[Slot(which)];
  return true;
}

double MembersItPullsToward(const Gym& gym, const GymDials& dials) {
  if (!gym.owned) return 0.0;
  double target = dials.target[Slot(gym.price)] +
                  dials.mixTarget[Slot(gym.mix)] +
                  dials.equipTarget[Slot(gym.equip)];
  // The campaign counts while it runs, read against the last day the books
  // were done -- so an expired one contributes nothing even before anybody
  // gets round to clearing it.
  if (CampaignRunning(gym, gym.lastTickDay)) {
    target += dials.campaignBoost[Slot(gym.campaign)];
  }
  if (gym.frontDesk) target += dials.frontDeskTarget;
  if (gym.setter) target += dials.setterTarget;
  // **Rival gyms and the season multiply this in the original** (`GYM-4`
  // and `GYM-6`) and are not ported yet, so both are 1.0 here. They belong
  // as multipliers on the finished sum rather than terms inside it, which
  // is why the shape is written this way now.
  return std::max(0.0, target);
}

double RatePerMember(const Gym& gym, const GymDials& dials) {
  if (!gym.owned) return 0.0;
  return dials.rate[Slot(gym.price)] + dials.mixRate[Slot(gym.mix)];
}

double DailyOverhead(const Gym& gym, const GymDials& dials) {
  if (!gym.owned) return 0.0;
  double out = dials.overhead + dials.equipOverhead[Slot(gym.equip)];
  if (gym.frontDesk) out += dials.frontDeskWage;
  if (gym.setter) out += dials.setterWage;
  return out;
}

GymNight GymDay(Gym& gym, const Rng& worldRng, int day, const GymDials& dials) {
  GymNight out;
  if (!gym.owned) return out;

  gym.lastTickDay = day;
  const double target = MembersItPullsToward(gym, dials);

  // A fraction of the gap, plus the day's wobble. Its own derived stream so
  // the floor's noise cannot shift the weather or anybody's turnout.
  Rng rng = worldRng.Derive("gym#" + std::to_string(day));
  const double drift = (target - gym.members) * dials.memberDrift;
  const double noise = rng.FloatRange(-dials.memberNoise, dials.memberNoise);
  gym.members = std::max(0.0, std::round(gym.members + drift + noise));

  out.net = gym.members * RatePerMember(gym, dials) - DailyOverhead(gym, dials);
  gym.balance += out.net;

  // **Consecutive days under water, and it resets the moment you are not.**
  // A fortnight is long enough to fix a bad month and short enough that
  // ignoring it costs you the building.
  gym.debtDays = gym.balance < 0.0 ? gym.debtDays + 1 : 0;

  if (gym.campaign != GymCampaign::None && gym.campaignUntil < day) {
    gym.campaign = GymCampaign::None;
    out.campaignEnded = true;
  }

  if (gym.debtDays >= dials.bankruptcyDays) {
    out.foreclosed = true;
    // The bank takes it back, and everything about it goes with it. The
    // standing hit is the caller's, because standing is not this file's.
    gym = Gym{};
  }
  return out;
}

std::string GymWarning(const Gym& gym, const GymDials& dials) {
  if (!gym.owned || gym.debtDays <= 0) return "";
  const int left = dials.bankruptcyDays - gym.debtDays;
  if (left <= 0) return "";
  if (left <= 3) {
    return gym.name + " is " + std::to_string(left) +
           " days from the bank taking it.";
  }
  return gym.name + " has been in the red " + std::to_string(gym.debtDays) +
         " days.";
}

std::string GymLine(const Gym& gym, const GymDials& dials) {
  if (!gym.owned) return "";
  const double net =
      gym.members * RatePerMember(gym, dials) - DailyOverhead(gym, dials);
  std::string out = gym.name + " -- " +
                    std::to_string(static_cast<int>(gym.members)) + " members, ";
  out += net >= 0.0 ? "up $" : "down $";
  out += std::to_string(static_cast<int>(std::fabs(std::round(net))));
  out += " a day";
  return out;
}

}  // namespace dirtbag
