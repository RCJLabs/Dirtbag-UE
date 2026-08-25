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
// ## Pass one was GYM-1, the P&L engine; this is pass two
//
// Pass one shipped five levers and a daily tick, and every lever pulled
// against a static number. Pass two makes the number push back:
//
//   - `GYM-3` -- **the staff are people.** A hire is a choice between three
//     candidates, each with a name, one true thing about them, a quality
//     that scales the boost the books already used, and a wage they will
//     eventually want more of.
//   - `GYM-4` and `GYM-6` -- the town and the year, in
//     `DirtbagGymTown.{h,cpp}` next door.
//   - `GYM-12` -- **wings.** The equipment ladder is an order everybody
//     climbs in the same order; wings are independent, expensive enough
//     that you will not build them all, and what they move is *who* comes.
//   - `GYM-1` phase 3 -- **passive.** Once both seats are filled the place
//     can run without you, cheaper, with the day-to-day levers locked.
//
// Still not here, and each its own family: the members as named people with
// arcs (`GYM-2`, `GYM-5`), the league you run (`GYM-10`), hosting a circuit
// round (`GYM-9`), the youth team (`GYM-8`).
//
// **Two wing fields are deliberately cut**: the source's `youth` and `bid`
// feed `GYM-8` and `GYM-9`, and neither is ported. A dial nothing reads is
// a dial that lies, so they come back with the systems that read them.
//
// **The core idiom is one this port already has three times**: members do
// not jump to a target, they drift toward it. The original's own comment
// calls it *"a meter, not a switch — the same feel as training load"*, and
// it is the same shape as `Thread::warmth` and `Local::known`.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagGymTown.h"
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

// **The deliberate opposite of the equipment ladder.** `GymEquip` is an
// order -- 0 to 1 to 2, everybody's gym ends up the same shape, and all it
// moves is how many people come. Wings are independent, buildable in any
// order, and what they move is *who* comes. Two gyms with the same
// membership and the same books can be completely different buildings, and
// the one you end up with is a description of what you thought mattered.
enum class GymWing { Showers, Woody, Kids, Annex, Cafe, kGymWingCount };
constexpr int kGymWingCount = static_cast<int>(GymWing::kGymWingCount);

// **One of the two people who actually run your building.** Pass one had
// them as a pair of booleans -- hired or not, one wage, one boost each --
// which made them the only people in this game without a name and one true
// thing about them.
//
// The wage spread is deliberately wide enough to beat the quality spread at
// budget pricing and lose to it everywhere else, so the right hire depends
// on a lever the player already pulled: when a member is worth $8 you want
// a cheap body on the desk, and when they are worth $22 you want the one
// who knows their name.
struct GymStaffer {
  std::string name;
  std::string trait;
  double wage = 0.0;
  // Scales the target boost the books already used, so the lever is
  // unchanged in shape -- there are just numbers behind the numbers now.
  double quality = 1.0;
  int hiredDay = 0;
  int nextAskDay = 0;
  int refusals = 0;
};

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

  // Staffing: a hire price, a base daily wage, and what they pull. The wage
  // and the pull are both **base** figures now -- a candidate's trait moves
  // the wage and their quality scales the pull. See `GymCandidates`.
  double frontDeskCost = 2000.0, frontDeskWage = 25.0, frontDeskTarget = 5.0;
  double setterCost = 3500.0, setterWage = 35.0, setterTarget = 8.0;

  // --- GYM-3: and the people in those two jobs --------------------------
  // Candidates on the desk when you go looking. Deterministic from the
  // gym's name, the role and the week, so the shortlist does not reshuffle
  // under a player who is still deciding.
  int hirePool = 3;

  // Tenure before somebody asks, what they ask for as a fraction of their
  // wage, and the floor under any wage however bad the trait.
  int raiseDays = 40;
  double raiseFraction = 0.20;
  double lowestWage = 8.0;

  // **Refusing has two prices, in this order.** They keep turning up and
  // stop going out of their way; and if you do it again, they go.
  double spiteQuality = 0.75;
  int refusalsBeforeTheyGo = 2;

  // --- GYM-12: the wings ------------------------------------------------
  // Independent, in any order, and priced so you will not build them all.
  // Each is a capital cost, a daily upkeep, a pull on the target, and --
  // for exactly one of them -- money it earns by itself, per member per day.
  double wingCost[kGymWingCount] = {3800.0, 2800.0, 4000.0, 6500.0, 5500.0};
  double wingUpkeep[kGymWingCount] = {10.0, 4.0, 8.0, 12.0, 20.0};
  double wingTarget[kGymWingCount] = {9.0, 3.0, 8.0, 6.0, 5.0};
  double wingEarns[kGymWingCount] = {0.0, 0.0, 0.0, 0.0, 0.9};

  // --- GYM-4: the one lever of yours that touches the town --------------
  // A live campaign blunts a rival's good month. That is what marketing is
  // for, it already costs real money, and it gives their hot streak a
  // counter-move rather than a shrug. Indexed by GymCampaign.
  double campaignShield[kGymCampaignCount] = {1.0, 1.08, 1.18};

  // --- GYM-1 phase 3: hands off -----------------------------------------
  // What it saves a day to let efficient staff run the place rather than
  // fumbling the daily levers yourself. The cost is that pricing, mix and
  // marketing lock where you left them.
  double passiveOverheadSaving = 15.0;

  // The town and the year this gym sits in.
  GymTownDials town;

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

  // Whether the seat is filled, and who is in it. Kept as two fields
  // rather than folded into the staffer because that is what the save
  // already carried, and because "is there a setter" is the question three
  // other systems ask -- the poach incident among them.
  bool frontDesk = false;
  bool setter = false;
  GymStaffer desk;
  GymStaffer routesetter;

  // GYM-12. Independent, so a flag each rather than a tier.
  bool wings[kGymWingCount] = {false, false, false, false, false};

  // GYM-1 phase 3. Reversible any time.
  bool passive = false;

  // GYM-6: the open incident and the day it landed. Only the player's
  // *answer* is state -- which incident fires is derived.
  GymIncident incident = GymIncident::None;
  int incidentDay = 0;

  double members = 0.0;
  double balance = 0.0;
  int debtDays = 0;
  int lastTickDay = 0;
};

const char* GymPriceName(GymPrice p);
const char* GymSetMixName(GymSetMix m);
const char* GymEquipName(GymEquip e);
const char* GymCampaignName(GymCampaign c);

const char* GymWingName(GymWing w);

// unwired-ok: the blurbs are the shop copy and belong to whatever screen
// offers the choice; nothing in the sim reads them.
const char* GymPriceBlurb(GymPrice p);
const char* GymSetMixBlurb(GymSetMix m);
const char* GymEquipBlurb(GymEquip e);
const char* GymCampaignBlurb(GymCampaign c);
const char* GymWingBlurb(GymWing w);

// Buy it. Takes the money and opens the doors; false if you cannot afford
// it or already own one.
bool BuyTheGym(Gym& gym, double& cash, const std::string& name, int day,
               const GymDials& dials = GymDials{});

// The levers. Pricing and the set mix are free to change -- they are an
// identity rather than a purchase. Equipment is a ladder and only goes up.
void SetPrice(Gym& gym, GymPrice p);
void SetMix(Gym& gym, GymSetMix m);
bool UpgradeEquipment(Gym& gym, double& cash, const GymDials& dials = GymDials{});
// **GYM-3: who is on the desk this week.** Three of them, derived from the
// gym's name, the role and the week -- so the shortlist is stable while the
// player thinks about it, and the same save always sees the same people.
// Traits and names are drawn without repeats within a shortlist.
std::vector<GymStaffer> GymCandidates(const std::string& gymName,
                                      bool frontDesk, int day,
                                      const GymDials& dials = GymDials{});

// Take one of them. The hire cost is the same whoever you pick -- what
// differs is the wage you carry from tomorrow and how hard they work for
// it. `which` indexes today's shortlist.
bool Hire(Gym& gym, double& cash, bool frontDesk, int day, int which,
          const GymDials& dials = GymDials{});

// Who is actually in the seat. Null when it is empty.
const GymStaffer* WhoIsOn(const Gym& gym, bool frontDesk);

// **Tenure comes due.** What they are asking for, and whether they are
// asking at all.
bool IsAskingForARaise(const Gym& gym, bool frontDesk, int day,
                       const GymDials& dials = GymDials{});
double TheRaiseTheyWant(const Gym& gym, bool frontDesk,
                        const GymDials& dials = GymDials{});

// Grant it and they stay as they were, only dearer. Refuse and they keep
// turning up and stop trying quite so hard; refuse twice and they do not.
enum class RaiseAnswer { NotAsking, Granted, TheySulk, TheyQuit };
RaiseAnswer AnswerTheAsk(Gym& gym, bool frontDesk, bool grant, int day,
                         const GymDials& dials = GymDials{});

// GYM-12. Independent of every other wing and of the equipment ladder --
// there is no order and no prerequisite, only the money.
bool BuildWing(Gym& gym, double& cash, GymWing wing,
               const GymDials& dials = GymDials{});
bool HasWing(const Gym& gym, GymWing wing);

// GYM-1 phase 3. Needs both seats filled; locks pricing, mix and marketing
// where they are; reversible any time. False if it cannot go either way.
bool SetHandsOff(Gym& gym, bool handsOff);
bool LaunchCampaign(Gym& gym, double& cash, GymCampaign which, int day,
                    const GymDials& dials = GymDials{});

// What the floor is pulling toward today, and what a member is worth. Both
// public because the shop screen has to be able to say what a lever would
// do before you pull it. **The pull is the town's and the year's as much as
// yours**: your levers move the base, `TownPressure` and `SeasonPull`
// multiply it.
double MembersItPullsToward(const Gym& gym, const GymDials& dials = GymDials{});
double RatePerMember(const Gym& gym, const GymDials& dials = GymDials{});
double DailyOverhead(const Gym& gym, const GymDials& dials = GymDials{});

// What the place takes in a day beyond the memberships -- which is the
// cafe, and only the cafe.
double WhatTheWingsEarn(const Gym& gym, const GymDials& dials = GymDials{});

// One night of the books. Drifts the membership, banks the day's net, and
// counts the days in the red.
struct GymNight {
  double net = 0.0;
  bool campaignEnded = false;
  bool foreclosed = false;

  // GYM-6. One landed tonight, or the one that was open ran out of days and
  // **answered itself the way you would expect** -- the cheap option, and
  // its cost. `standing` is the lapsed choice's, handed back unapplied.
  GymIncident landed = GymIncident::None;
  GymIncident lapsed = GymIncident::None;
  double standing = 0.0;
  std::string news;
};
GymNight GymDay(Gym& gym, const Rng& worldRng, int day,
                const GymDials& dials = GymDials{});

// **GYM-6: answer the open incident**, 0 or 1 into its choice table.
// Everything is resolved before anything is written and the incident is
// cleared in the same call, so it cannot be answered twice. `standing` is
// handed back rather than applied, because standing is the scene's and the
// gym does not know about the scene -- the same split the foreclosure uses.
struct IncidentAnswer {
  bool answered = false;
  double standing = 0.0;
  std::string line;
};
IncidentAnswer AnswerTheIncident(Gym& gym, double& cash, int which,
                                 const GymDials& dials = GymDials{});

// What it says on the noticeboard, or empty when there is nothing to say.
// **Quiet while the books are fine**, like every other readout in this
// game: a line that reports a healthy balance every night teaches you to
// stop reading the one that will eventually say otherwise.
std::string GymWarning(const Gym& gym, const GymDials& dials = GymDials{});

// One line for a screen with room: what it is and how it is doing.
std::string GymLine(const Gym& gym, const GymDials& dials = GymDials{});

}  // namespace dirtbag
