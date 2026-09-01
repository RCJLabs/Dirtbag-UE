#include "DirtbagGym.h"

#include <algorithm>
#include <cmath>

#include "DirtbagRng.h"

namespace dirtbag {
namespace {

int Slot(GymPrice p) { return static_cast<int>(p); }
int Slot(GymSetMix m) { return static_cast<int>(m); }
int Slot(GymEquip e) { return static_cast<int>(e); }
int Slot(GymCampaign c) { return static_cast<int>(c); }
int Slot(GymWing w) { return static_cast<int>(w); }

// --- GYM-3: the people ----------------------------------------------------
// A dozen names, so a long career hiring through the pool does not run out
// of strangers.
const char* kStaffNames[] = {
  "Rosalind", "Teo", "Marguerite", "Dev", "Halley", "Ozzie",
  "Nadia", "Bram", "Cleo", "Yusuf", "Winnie", "Arturo",
};
constexpr int kStaffNameCount =
    static_cast<int>(sizeof(kStaffNames) / sizeof(kStaffNames[0]));

struct Trait {
  const char* text;
  double quality;
  double wageMod;
};

// **Ordered best to worst, and the wage spread is wider than the quality
// spread on purpose.** The source records the first cut having the best
// candidate win at every price tier, which is not a choice.
const Trait kDeskTraits[] = {
  {"knows every member by name inside a week and it shows in the renewals",
   1.35, 18.0},
  {"runs the desk like a nurse runs a ward -- nothing dramatic ever happens",
   1.15, 8.0},
  {"reliable, unremarkable, never once late", 1.00, 0.0},
  {"perfectly pleasant, and on their phone", 0.75, -10.0},
};

const Trait kSetterTraits[] = {
  {"foreruns everything twice before the tape goes on", 1.35, 26.0},
  {"sets fast and sets a lot, and about a third of it is very good",
   1.15, 12.0},
  {"competent, unfussy, gets the wall turned over on schedule", 1.00, 0.0},
  {"has one good idea and sets it again every single week", 0.80, -12.0},
};

constexpr int kTraitCount =
    static_cast<int>(sizeof(kDeskTraits) / sizeof(kDeskTraits[0]));

const Trait* TraitsFor(bool frontDesk) {
  return frontDesk ? kDeskTraits : kSetterTraits;
}

double BaseWage(bool frontDesk, const GymDials& dials) {
  return frontDesk ? dials.frontDeskWage : dials.setterWage;
}

double HireDraw(const std::string& label) {
  return Rng::FromSeed(label).NextDouble();
}

// The week the shortlist belongs to. Seven days is long enough that a
// player cannot shop for a better list by sleeping on it.
int HiringWeek(int day) { return day / 7; }

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

const char* GymWingName(GymWing w) {
  switch (w) {
    case GymWing::Showers: return "Proper Changing Rooms";
    case GymWing::Woody: return "The Woody";
    case GymWing::Kids: return "Kids' Area";
    case GymWing::Annex: return "Training Annex";
    case GymWing::Cafe: return "The Cafe";
    default: return "";
  }
}

const char* GymWingBlurb(GymWing w) {
  switch (w) {
    case GymWing::Showers:
      return "Hot water, lockers that lock, somewhere to put a wet towel. "
             "The least glamorous thing you will ever buy and the one people "
             "actually stay for.";
    case GymWing::Woody:
      return "A back room, a steep board, a fan and no natural light. Six "
             "people will use it constantly and they are the six who talk "
             "about you.";
    case GymWing::Kids:
      return "Soft floor, low traverse, somewhere for a seven-year-old to be "
             "a seven-year-old. Parents climb for an hour instead of twenty "
             "minutes.";
    case GymWing::Annex:
      return "Rings, bars, a proper hangboard row and a floor you can drop "
             "things on. The people who were going to leave for a real gym "
             "now do not have to.";
    case GymWing::Cafe:
      return "Coffee, a toastie machine, four tables. People stay two hours "
             "longer and half of them are not even climbing.";
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
  // noticing, which is `memberDrift`'s job. **Locked once you have handed
  // over the keys**: that is the price of the place running without you.
  if (gym.owned && !gym.passive) gym.price = p;
}

void SetMix(Gym& gym, GymSetMix m) {
  if (gym.owned && !gym.passive) gym.mix = m;
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

std::vector<GymStaffer> GymCandidates(const std::string& gymName,
                                      bool frontDesk, int day,
                                      const GymDials& dials) {
  std::vector<GymStaffer> out;
  const Trait* traits = TraitsFor(frontDesk);
  const std::string role = frontDesk ? "frontDesk" : "setter";
  const std::string week = std::to_string(HiringWeek(day));
  const std::string stem = "gymhire|" + gymName + "|" + role + "|" + week + "|";
  const int wanted = std::max(1, std::min(dials.hirePool, kTraitCount));

  // **No repeats within a shortlist**, of trait or of name -- three
  // identical candidates is not a choice, and two Devs is a bug report.
  // Collision is resolved by walking forward rather than re-drawing, so the
  // pool size never changes the earlier picks.
  bool traitUsed[kTraitCount] = {false, false, false, false};
  bool nameUsed[kStaffNameCount] = {false};
  for (int i = 0; i < wanted; i++) {
    int t = static_cast<int>(HireDraw(stem + "t" + std::to_string(i)) *
                             kTraitCount) % kTraitCount;
    for (int guard = 0; traitUsed[t] && guard < kTraitCount; guard++) {
      t = (t + 1) % kTraitCount;
    }
    traitUsed[t] = true;

    int n = static_cast<int>(HireDraw(stem + "n" + std::to_string(i)) *
                             kStaffNameCount) % kStaffNameCount;
    for (int guard = 0; nameUsed[n] && guard < kStaffNameCount; guard++) {
      n = (n + 1) % kStaffNameCount;
    }
    nameUsed[n] = true;

    GymStaffer who;
    who.name = kStaffNames[n];
    who.trait = traits[t].text;
    who.wage = std::max(dials.lowestWage,
                        BaseWage(frontDesk, dials) + traits[t].wageMod);
    who.quality = traits[t].quality;
    who.hiredDay = day;
    who.nextAskDay = day + dials.raiseDays;
    out.push_back(who);
  }
  return out;
}

bool Hire(Gym& gym, double& cash, bool frontDesk, int day, int which,
          const GymDials& dials) {
  if (!gym.owned) return false;
  bool& seat = frontDesk ? gym.frontDesk : gym.setter;
  if (seat) return false;
  const std::vector<GymStaffer> pool =
      GymCandidates(gym.name, frontDesk, day, dials);
  if (which < 0 || which >= static_cast<int>(pool.size())) return false;
  // The hire price is the role's, not the person's. What the person costs
  // you is the wage, every day, from tomorrow.
  const double cost = frontDesk ? dials.frontDeskCost : dials.setterCost;
  if (cash < cost) return false;
  cash -= cost;
  seat = true;
  (frontDesk ? gym.desk : gym.routesetter) = pool[static_cast<size_t>(which)];
  return true;
}

const GymStaffer* WhoIsOn(const Gym& gym, bool frontDesk) {
  if (!gym.owned) return nullptr;
  if (!(frontDesk ? gym.frontDesk : gym.setter)) return nullptr;
  return frontDesk ? &gym.desk : &gym.routesetter;
}

bool IsAskingForARaise(const Gym& gym, bool frontDesk, int day,
                       const GymDials& dials) {
  (void)dials;
  const GymStaffer* who = WhoIsOn(gym, frontDesk);
  return who != nullptr && day >= who->nextAskDay;
}

double TheRaiseTheyWant(const Gym& gym, bool frontDesk,
                        const GymDials& dials) {
  const GymStaffer* who = WhoIsOn(gym, frontDesk);
  if (who == nullptr) return 0.0;
  // A floor under the ask, so the cheap hire's raise is still worth having
  // an opinion about.
  return std::max(3.0, std::round(who->wage * dials.raiseFraction));
}

RaiseAnswer AnswerTheAsk(Gym& gym, bool frontDesk, bool grant, int day,
                         const GymDials& dials) {
  if (!IsAskingForARaise(gym, frontDesk, day, dials)) {
    return RaiseAnswer::NotAsking;
  }
  GymStaffer& who = frontDesk ? gym.desk : gym.routesetter;
  if (grant) {
    who.wage += TheRaiseTheyWant(gym, frontDesk, dials);
    who.nextAskDay = day + dials.raiseDays;
    return RaiseAnswer::Granted;
  }
  who.refusals++;
  if (who.refusals >= dials.refusalsBeforeTheyGo) {
    (frontDesk ? gym.frontDesk : gym.setter) = false;
    who = GymStaffer{};
    // The place cannot run itself with an empty seat in it.
    gym.passive = false;
    return RaiseAnswer::TheyQuit;
  }
  // **They keep turning up. They stop going out of their way.** Quality is
  // the thing that moves, not attendance -- which is what makes this the
  // expensive kind of no.
  who.quality *= dials.spiteQuality;
  who.nextAskDay = day + dials.raiseDays;
  return RaiseAnswer::TheySulk;
}

bool BuildWing(Gym& gym, double& cash, GymWing wing, const GymDials& dials) {
  if (!gym.owned) return false;
  const int i = Slot(wing);
  if (i < 0 || i >= kGymWingCount) return false;
  if (gym.wings[i]) return false;
  if (cash < dials.wingCost[i]) return false;
  cash -= dials.wingCost[i];
  gym.wings[i] = true;
  return true;
}

bool HasWing(const Gym& gym, GymWing wing) {
  const int i = Slot(wing);
  return gym.owned && i >= 0 && i < kGymWingCount && gym.wings[i];
}

bool SetHandsOff(Gym& gym, bool handsOff) {
  if (!gym.owned) return false;
  if (handsOff == gym.passive) return false;
  // **Both seats, or it does not run without you.** That was the whole
  // trigger: the reward for staffing the place properly is not having to
  // be in it.
  if (handsOff && !(gym.frontDesk && gym.setter)) return false;
  gym.passive = handsOff;
  return true;
}

bool LaunchCampaign(Gym& gym, double& cash, GymCampaign which, int day,
                    const GymDials& dials) {
  if (!gym.owned || which == GymCampaign::None) return false;
  // One at a time. Launching does not stack, and it is gated on none
  // currently running rather than on the money.
  if (gym.passive) return false;
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
  // GYM-3: a staffer's quality scales the boost the books already used.
  if (const GymStaffer* d = WhoIsOn(gym, true)) {
    target += dials.frontDeskTarget * d->quality;
  }
  if (const GymStaffer* s = WhoIsOn(gym, false)) {
    target += dials.setterTarget * s->quality;
  }
  // GYM-12: and whatever you built onto it.
  for (int i = 0; i < kGymWingCount; i++) {
    if (gym.wings[i]) target += dials.wingTarget[i];
  }

  // **They move the multiplier, you move the base.** Every lever above is
  // already in the sum; scoring them into the town's share as well would
  // double-count the player's own choices, which is the bug the source
  // records finding in its first cut of `GYM-4`.
  const double shield = CampaignRunning(gym, gym.lastTickDay)
                            ? dials.campaignShield[Slot(gym.campaign)]
                            : 1.0;
  const double pressure =
      TownPressure(gym.lastTickDay, gym.name, shield, dials.town);
  const double season = 1.0 + SeasonPull(gym.lastTickDay, dials.town);
  return std::max(0.0, target * pressure * season);
}

double WhatTheWingsEarn(const Gym& gym, const GymDials& dials) {
  if (!gym.owned) return 0.0;
  double perMember = 0.0;
  for (int i = 0; i < kGymWingCount; i++) {
    if (gym.wings[i]) perMember += dials.wingEarns[i];
  }
  return std::round(perMember * gym.members);
}

double RatePerMember(const Gym& gym, const GymDials& dials) {
  if (!gym.owned) return 0.0;
  return dials.rate[Slot(gym.price)] + dials.mixRate[Slot(gym.mix)];
}

double BidStrength(const Gym& gym, double sceneStanding,
                   const GymDials& dials) {
  if (!gym.owned) return 0.0;
  double out = gym.members / std::max(1.0, dials.bidPerMembers) +
               Slot(gym.equip) * dials.bidPerEquipTier;
  // Your name counts, up to a point, and only in the direction that helps:
  // the federation is not going to give a round to a bigger room because
  // the scene likes you, and it is not going to take one away either.
  out += std::max(0.0, sceneStanding) * dials.bidStandingWorth;
  for (int i = 0; i < kGymWingCount; i++) {
    if (gym.wings[i]) out += dials.wingBid[i];
  }
  return out;
}

std::string WhyNotBid(const Gym& gym, double cash, int season,
                      const GymDials& dials) {
  if (!gym.owned) return "";
  if (gym.hostsSeason == season) return "You already have this season.";
  if (gym.askedSeason == season) {
    return "You have had their answer for this season. Next one.";
  }
  if (gym.members < dials.bidMinMembers) {
    return "They want a room that can hold a circuit round -- " +
           std::to_string(static_cast<int>(dials.bidMinMembers)) +
           " members, you have " +
           std::to_string(static_cast<int>(gym.members)) + ".";
  }
  if (Slot(gym.equip) < dials.bidMinEquip) {
    return "They will not put a national number on those walls. Upgrade the "
           "equipment first.";
  }
  if (cash < dials.bidCost) {
    return "The deposit and the sanctioning fee run $" +
           std::to_string(static_cast<int>(dials.bidCost)) + ".";
  }
  return "";
}

BidAnswer BidToHost(Gym& gym, double& cash, int season, double sceneStanding,
                    double rivalPull, const GymDials& dials) {
  if (!WhyNotBid(gym, cash, season, dials).empty()) {
    return BidAnswer::CannotAsk;
  }
  // **Gone either way.** The deposit is what makes this a decision rather
  // than a formality: you can lose $2,200 and a season together.
  cash -= dials.bidCost;
  gym.askedSeason = season;

  const double mine = BidStrength(gym, sceneStanding, dials);
  const double theirs = std::max(0.0, rivalPull);
  const double odds =
      mine + theirs > 0.0 ? mine / (mine + theirs) : dials.bidFloor;
  // Clamped at both ends: the biggest room in town is not guaranteed it,
  // and the smallest is not shut out of the sport forever.
  const double chance =
      std::min(dials.bidCeiling, std::max(dials.bidFloor, odds));
  // Derived from the gym's name and the season, so reopening the panel
  // cannot ask twice and get two answers.
  const double roll =
      Rng::FromSeed("gymbid|" + gym.name + "|" + std::to_string(season))
          .NextDouble();
  if (roll >= chance) return BidAnswer::TheyWentElsewhere;
  gym.hostsSeason = season;
  return BidAnswer::ItIsYours;
}

bool HoldsTheSeason(const Gym& gym, int season) {
  return gym.owned && season > 0 && gym.hostsSeason == season;
}

double WhatARoundPays(const GymDials& dials) {
  return std::round(dials.hostField) * dials.hostFeePerHead;
}

double WhatTheWingsTeach(const Gym& gym, const GymDials& dials) {
  if (!gym.owned) return 0.0;
  double out = 0.0;
  for (int i = 0; i < kGymWingCount; i++) {
    if (gym.wings[i]) out += dials.wingYouth[i];
  }
  return out;
}

double DailyOverhead(const Gym& gym, const GymDials& dials) {
  if (!gym.owned) return 0.0;
  double out = dials.overhead + dials.equipOverhead[Slot(gym.equip)];
  // GYM-3: the wage is the staffer's own now, not the role's.
  if (const GymStaffer* d = WhoIsOn(gym, true)) out += d->wage;
  if (const GymStaffer* s = WhoIsOn(gym, false)) out += s->wage;
  for (int i = 0; i < kGymWingCount; i++) {
    if (gym.wings[i]) out += dials.wingUpkeep[i];
  }
  // GYM-1 phase 3: efficient staff run leaner than you fumbling the levers.
  if (gym.passive) out -= dials.passiveOverheadSaving;
  return out;
}

GymNight GymDay(Gym& gym, const Rng& worldRng, int day, double otherWages,
                const GymDials& dials) {
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

  out.net = gym.members * RatePerMember(gym, dials) +
            WhatTheWingsEarn(gym, dials) - DailyOverhead(gym, dials) -
            std::max(0.0, otherWages);

  // **GYM-6, before the balance is struck**, because a lapsed incident is a
  // bill and belongs in tonight's books rather than tomorrow's.
  if (gym.incident != GymIncident::None &&
      day - gym.incidentDay >= dials.town.incidentDays) {
    const GymIncidentDef* def = IncidentDef(gym.incident);
    if (def != nullptr) {
      // It answers itself the way you would expect: the cheap option.
      const GymIncidentChoice& cheap = def->choices[1];
      out.lapsed = gym.incident;
      out.net -= cheap.cost;
      out.standing = cheap.standing;
      gym.members = std::max(0.0, gym.members + cheap.members);
      out.news = std::string(def->title) + ". You left it. " + cheap.line;
    }
    gym.incident = GymIncident::None;
  } else if (gym.incident == GymIncident::None) {
    const GymIncident fresh = IncidentToday(gym.name, gym.ownedDay, day,
                                            gym.setter, dials.town);
    if (fresh != GymIncident::None) {
      const GymIncidentDef* def = IncidentDef(fresh);
      gym.incident = fresh;
      gym.incidentDay = day;
      out.landed = fresh;
      if (def != nullptr) {
        out.news = std::string(def->title) + " -- " + gym.name +
                   " needs an answer in the next few days.";
      }
    }
  }

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

IncidentAnswer AnswerTheIncident(Gym& gym, double& cash, int which,
                                 const GymDials& dials) {
  IncidentAnswer out;
  if (!gym.owned || gym.incident == GymIncident::None) return out;
  if (which < 0 || which > 1) return out;
  const GymIncidentDef* def = IncidentDef(gym.incident);
  if (def == nullptr) return out;
  const GymIncidentChoice& choice = def->choices[which];
  if (cash < choice.cost) return out;

  cash -= choice.cost;
  gym.members = std::max(0.0, gym.members + choice.members);
  out.answered = true;
  out.standing = choice.standing;
  out.line = choice.line;

  // The one choice in the table with a tail: matching the offer costs you
  // every day from here.
  if (choice.matchesTheOffer && gym.setter) {
    gym.routesetter.wage += dials.town.poachRaise;
    out.line += " " + gym.routesetter.name + " is on $" +
                std::to_string(static_cast<int>(gym.routesetter.wage)) +
                " a day now.";
  }
  gym.incident = GymIncident::None;
  return out;
}

std::string GymWarning(const Gym& gym, const GymDials& dials) {
  if (!gym.owned) return "";
  // **An open incident outranks the balance**, because it is the thing with
  // a clock on it: the books can be red for a fortnight, and this cannot.
  if (gym.incident != GymIncident::None) {
    const GymIncidentDef* def = IncidentDef(gym.incident);
    if (def != nullptr) return std::string(def->title) + ". It needs an answer.";
  }
  if (gym.debtDays <= 0) return "";
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
  const double net = gym.members * RatePerMember(gym, dials) +
                     WhatTheWingsEarn(gym, dials) - DailyOverhead(gym, dials);
  std::string out = gym.name + " -- " +
                    std::to_string(static_cast<int>(gym.members)) + " members, ";
  out += net >= 0.0 ? "up $" : "down $";
  out += std::to_string(static_cast<int>(std::fabs(std::round(net))));
  out += " a day";
  return out;
}

}  // namespace dirtbag
