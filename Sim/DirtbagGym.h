// The gym you bought.
//
// **Ported from the 2D source, not from memory.** `concepts/DIRTBAG.md` §4
// cut gym ownership past 1.0 and `DECISION-comps-are-back.md` re-affirmed
// that cut; `DECISION-gym-ownership.md` reverses it, on a measured argument:
// the most expensive thing in this game is Home Base at $30,000, and a
// career that climbs more days than any other policy holds $60,378.
//
// The original priced it at **$25,000**, which lands inside exactly that
// range. The gap this fills was already the gap it was built to fill.
//
// ## What this is, and what the audit got wrong about it
//
// `notes/the-2d-audit.md` files gym ownership under *Coaching & mentoring*,
// beside the mentee, the coaching roster and the youth team. Read from the
// source, that is misleading: it has **its own taxonomy family, `GYM-1`
// through `GYM-12`**, and it is a business — pricing, staffing, equipment,
// marketing, members, a balance, and a bank that takes it back. The youth
// team is one wing you can build onto it later, not what it is.
//
// Scoping it from the table row would have built the wrong system. That is
// the third time this project has been bitten by auditing a summary.
//
// ## This file is pass one: GYM-1, the P&L engine
//
// Five levers and a daily tick. Deliberately not here yet, and each is its
// own numbered family in the original: the staff as **named people** with
// traits and raise asks (`GYM-3`), **rival gyms** pulling your members
// (`GYM-4`), the **season** (`GYM-6`), **incidents** (`GYM-6`), **wings**
// (`GYM-12`), the league you run, hosting a comp, the youth team.
//
// Pass one models staff as two booleans, which is not a simplification I
// invented -- it is what `GYM-1` shipped, and the source says a save from
// before `GYM-3` "derives a plain 1.0-quality staffer on the base wage".
//
// **The core idiom is one this port already has three times**: members do
// not jump to a target, they drift toward it. The original's own comment
// calls it *"a meter, not a switch — the same feel as training load"*, and
// it is the same shape as `Thread::warmth` and `Local::known`.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>

#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

// What you charge. Fewer members at a higher rate, or a packed floor on
// thin margins -- the first real lever and the one the others lean on.
enum class GymPrice { Budget, Standard, Premium, kGymPriceCount };
constexpr int kGymPriceCount = static_cast<int>(GymPrice::kGymPriceCount);

// What gets set on the walls. **Not the same question as whether you employ
// a setter** -- that is a hire; this is the style they set in.
enum class GymSetMix { Beginner, AllComers, Hardcore, kGymSetMixCount };
constexpr int kGymSetMixCount = static_cast<int>(GymSetMix::kGymSetMixCount);

// A one-time capital ladder rather than a toggle, and it sticks.
enum class GymEquip { AsBought, HoldsAndMats, FullRenovation, kGymEquipCount };
constexpr int kGymEquipCount = static_cast<int>(GymEquip::kGymEquipCount);

// Discretionary, repeatable, time-limited -- a different shape from the
// four above on purpose, and only one runs at a time.
enum class GymCampaign { None, Flyers, Social, kGymCampaignCount };
constexpr int kGymCampaignCount = static_cast<int>(GymCampaign::kGymCampaignCount);

struct GymDials {
  // **The price of the building.** The number that makes this system the
  // answer to the ceiling measurement rather than another thing to buy:
  // above Home Base's $30,000? No -- just under it, which means a career
  // chooses between the two rather than doing both, and that is a better
  // shape than a new tier on top.
  double price = 25000.0;

  // Members you inherit with the doors.
  int seedMembers = 15;

  // --- the five levers --------------------------------------------------
  // Per price tier: what a member pays a day, and how many the tier pulls.
  double rate[kGymPriceCount] = {8.0, 14.0, 22.0};
  double target[kGymPriceCount] = {30.0, 15.0, 8.0};

  // Per set mix: what it does to the target, and to the rate.
  double mixTarget[kGymSetMixCount] = {10.0, 0.0, -2.0};
  double mixRate[kGymSetMixCount] = {-2.0, 0.0, 3.0};

  // Per equipment tier: what it costs to get there (cumulative, not
  // incremental), what it pulls, and what it adds to the daily overhead.
  double equipCost[kGymEquipCount] = {0.0, 4000.0, 8000.0};
  double equipTarget[kGymEquipCount] = {0.0, 6.0, 16.0};
  double equipOverhead[kGymEquipCount] = {0.0, 10.0, 25.0};

  // Per campaign: what it costs, what it pulls while it runs, how long.
  double campaignCost[kGymCampaignCount] = {0.0, 800.0, 2000.0};
  double campaignBoost[kGymCampaignCount] = {0.0, 8.0, 18.0};
  int campaignDays[kGymCampaignCount] = {0, 10, 10};

  // Staffing: a hire price, a daily wage, and what they pull.
  double frontDeskCost = 2000.0, frontDeskWage = 25.0, frontDeskTarget = 5.0;
  double setterCost = 3500.0, setterWage = 35.0, setterTarget = 8.0;

  // --- and the daily engine ---------------------------------------------
  // Rent, utilities, insurance, before anything you chose.
  double overhead = 180.0;

  // **The meter, not the switch.** A fraction of the gap to target closed
  // per day, so pricing takes about a week to show what it did. Same shape
  // as every other value in this port that ages toward something.
  double memberDrift = 0.15;

  // Day-to-day wobble, plus or minus. Through the named stream, never
  // `Math.random` -- the original rolls it outside its state updater for
  // React reasons this port does not have.
  double memberNoise = 2.0;

  // **Consecutive days in the red before the bank takes it**, and what
  // losing a business does to your standing. A real grace period rather
  // than a trap: a fortnight is long enough to fix a bad month.
  int bankruptcyDays = 14;
  double foreclosureStandingHit = 8.0;
};

// What you own. Null-shaped by `owned` rather than by a pointer, because a
// save carries it either way and a career mostly does not have one.
struct Gym {
  bool owned = false;
  std::string name;
  int ownedDay = 0;

  GymPrice price = GymPrice::Standard;
  GymSetMix mix = GymSetMix::AllComers;
  GymEquip equip = GymEquip::AsBought;

  GymCampaign campaign = GymCampaign::None;
  int campaignUntil = 0;

  bool frontDesk = false;
  bool setter = false;

  double members = 0.0;
  double balance = 0.0;
  int debtDays = 0;
  int lastTickDay = 0;
};

const char* GymPriceName(GymPrice p);
const char* GymSetMixName(GymSetMix m);
const char* GymEquipName(GymEquip e);
const char* GymCampaignName(GymCampaign c);

// unwired-ok: the blurbs are the shop copy and belong to whatever screen
// offers the choice; nothing in the sim reads them.
const char* GymPriceBlurb(GymPrice p);
const char* GymSetMixBlurb(GymSetMix m);
const char* GymEquipBlurb(GymEquip e);
const char* GymCampaignBlurb(GymCampaign c);

// Buy it. Takes the money and opens the doors; false if you cannot afford
// it or already own one.
bool BuyTheGym(Gym& gym, double& cash, const std::string& name, int day,
               const GymDials& dials = GymDials{});

// The levers. Pricing and the set mix are free to change -- they are an
// identity rather than a purchase. Equipment is a ladder and only goes up.
void SetPrice(Gym& gym, GymPrice p);
void SetMix(Gym& gym, GymSetMix m);
bool UpgradeEquipment(Gym& gym, double& cash, const GymDials& dials = GymDials{});
bool Hire(Gym& gym, double& cash, bool frontDesk,
          const GymDials& dials = GymDials{});
bool LaunchCampaign(Gym& gym, double& cash, GymCampaign which, int day,
                    const GymDials& dials = GymDials{});

// What the floor is pulling toward today, and what a member is worth. Both
// public because the shop screen has to be able to say what a lever would
// do before you pull it.
double MembersItPullsToward(const Gym& gym, const GymDials& dials = GymDials{});
double RatePerMember(const Gym& gym, const GymDials& dials = GymDials{});
double DailyOverhead(const Gym& gym, const GymDials& dials = GymDials{});

// One night of the books. Drifts the membership, banks the day's net, and
// counts the days in the red.
struct GymNight {
  double net = 0.0;
  bool campaignEnded = false;
  bool foreclosed = false;
};
GymNight GymDay(Gym& gym, const Rng& worldRng, int day,
                const GymDials& dials = GymDials{});

// What it says on the noticeboard, or empty when there is nothing to say.
// **Quiet while the books are fine**, like every other readout in this
// game: a line that reports a healthy balance every night teaches you to
// stop reading the one that will eventually say otherwise.
std::string GymWarning(const Gym& gym, const GymDials& dials = GymDials{});

// One line for a screen with room: what it is and how it is doing.
std::string GymLine(const Gym& gym, const GymDials& dials = GymDials{});

}  // namespace dirtbag
