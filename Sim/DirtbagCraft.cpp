#include "DirtbagCraft.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {

const char* CraftName(Craft c) {
  switch (c) {
    case Craft::Setting: return "setting";
    case Craft::Coaching: return "coaching";
    case Craft::Counter: return "the counter";
    case Craft::Labour: return "labour";
    case Craft::Trail: return "trail work";
    case Craft::Camera: return "the camera";
    case Craft::Courier: return "the courier run";
    case Craft::Rescue: return "rescue";
    case Craft::Bar: return "the bar";
    case Craft::Office: return "the office";
    case Craft::None:
    default: return "nothing in particular";
  }
}

Craft CraftForGig(const std::string& g) {
  // Matched on the gig's own name, because the board is data and the name
  // is the only stable thing about an entry -- an index would repoint the
  // day somebody adds a gig in the middle.
  const auto has = [&g](const char* s) {
    return g.find(s) != std::string::npos;
  };
  if (has("setting")) return Craft::Setting;
  if (has("belaying")) return Craft::Coaching;
  if (has("gear shop") || has("dishes")) return Craft::Counter;
  if (has("furniture") || has("firewood") || has("shelves")) {
    return Craft::Labour;
  }
  if (has("trail work")) return Craft::Trail;
  if (has("photos")) return Craft::Camera;
  if (has("courier") || has("delivery")) return Craft::Courier;
  if (has("callout") || has("rescue")) return Craft::Rescue;
  if (has("bar")) return Craft::Bar;
  // Flyering, and anything added later that nobody gets good at.
  return Craft::None;
}

Skill CraftTeaches(Craft craft) {
  switch (craft) {
    // A day spent reading movement, and nothing else in this game teaches
    // that except climbing.
    case Craft::Setting: return Skill::Technique;
    // Talking somebody through being frightened, over and over.
    case Craft::Coaching: return Skill::Head;
    // Six hours of carrying rock uphill.
    case Craft::Trail: return Skill::Endurance;
    // Furniture, firewood, pallets.
    case Craft::Labour: return Skill::Power;
    // Hanging off a rope one-handed with a camera in the other.
    case Craft::Camera: return Skill::Head;
    // Hauling gear over ground nobody built a path on.
    case Craft::Rescue: return Skill::Endurance;
    // On your feet for eight hours carrying things up stairs.
    case Craft::Courier: return Skill::Endurance;
    // Standing up all night. It is something.
    case Craft::Bar: return Skill::Endurance;
    case Craft::Counter: return Skill::Technique;
    case Craft::Office:
    case Craft::None:
    default: return Skill::Technique;
  }
}

WorkedShift WorkTheTrade(Craftsman& hand, Craft craft, double hours,
                         const CraftDials& d) {
  WorkedShift out;
  out.teaches = CraftTeaches(craft);
  if (craft == Craft::None || hours <= 0.0) return out;
  const int i = static_cast<int>(craft);

  // The same diminishing returns climbing has: the first fifty come fast
  // and the last ten never quite arrive.
  const double headroom =
      std::max(0.15, 1.0 - hand.skill[i] / std::max(1.0, d.craftCeiling));
  hand.skill[i] =
      std::min(d.craftCeiling, hand.skill[i] + d.gainPerHour * hours * headroom);
  hand.shifts[i]++;

  // Turning up is worth a little, every time. **This is the only way back
  // from a bad patch**, and it is slow on purpose.
  hand.standing[i] = std::max(-1.0, std::min(1.0,
                                             hand.standing[i] + d.easyStanding));
  // And it is how a sacking ends, if it ever does.
  if (hand.sacked[i] && hand.standing[i] >= d.rehireAt) {
    hand.sacked[i] = false;
  }

  // **Tiny on purpose.** Ten years of setting is worth a couple of grades
  // of technique; ten years of climbing is worth far more. Work that
  // trained you as well as climbing did would make the trap not a trap.
  out.skillGain = d.trainsPerHour * hours;
  return out;
}

double CraftPay(const Craftsman& hand, Craft craft, const CraftDials& d) {
  if (craft == Craft::None) return 1.0;
  const double at = std::max(0.0, std::min(1.0,
      hand.skill[static_cast<int>(craft)] / std::max(1.0, d.craftCeiling)));
  return d.payAtNothing + (d.payAtMastery - d.payAtNothing) * at;
}

bool WillTheyHireYou(const Craftsman& hand, Craft craft,
                     const CraftDials& d) {
  if (craft == Craft::None) return true;   // nobody is precious about flyering
  (void)d;
  return !hand.sacked[static_cast<int>(craft)];
}

// --- the moment ---------------------------------------------------------

namespace {

struct Situation {
  Craft craft;
  const char* what;
  const char* hard;
  const char* easy;
  double needs;
};

// **Two ways to handle it, and the right one is harder.** Every easy
// option here is a real option -- it is what a tired person does, it is
// never punished, and it never gets you anywhere. If the easy way were
// wrong this would be a skill check rather than a decision.
const Situation kSituations[] = {
    {Craft::Setting, "A hold spins under somebody mid-session.",
     "Strip the problem and reset it properly.",
     "Nip it up and carry on.", 45.0},
    {Craft::Setting, "The comp board is a grade soft and everybody knows.",
     "Rebuild the top three before the doors open.",
     "Leave it. Nobody has complained.", 62.0},
    {Craft::Coaching, "A kid freezes at the top and will not come down.",
     "Talk them down yourself, slowly.",
     "Lower them and hand them to their mum.", 40.0},
    {Craft::Coaching, "A parent wants their eight-year-old on the lead wall.",
     "Say no, and explain it so they hear it.",
     "Say the manager decides that.", 55.0},
    {Craft::Counter, "Somebody is about to buy shoes two sizes wrong.",
     "Spend twenty minutes and sell them the right pair.",
     "Ring it through. They asked for them.", 35.0},
    {Craft::Counter, "The card machine dies with a queue out the door.",
     "Work the queue on paper and reconcile after.",
     "Shut the till and tell them to come back.", 50.0},
    {Craft::Labour, "The piano does not fit round the landing.",
     "Take the door off and pivot it.",
     "Force it and hope.", 40.0},
    {Craft::Labour, "Half the pallet is water-damaged.",
     "Sort it now and flag what is ruined.",
     "Stack it and say nothing.", 45.0},
    {Craft::Trail, "The new steps will wash out in the first storm.",
     "Rebuild the drainage while you are here.",
     "Finish the steps. That was the job.", 50.0},
    {Craft::Trail, "There is a nesting bird twenty feet off the line.",
     "Reroute the last stretch around it.",
     "Carry on. It is twenty feet.", 42.0},
    {Craft::Camera, "The light on the crux is gone by the time you are set.",
     "Wait for tomorrow's light and eat the day.",
     "Shoot it flat. It will print.", 55.0},
    {Craft::Camera, "The climber you are shooting is clearly cheating the beta.",
     "Say so, and shoot it honestly.",
     "Get the shot. It is not your route.", 60.0},
    {Craft::Courier, "The address does not exist and the phone is off.",
     "Work it out from the round and deliver it.",
     "Mark it undeliverable and move on.", 38.0},
    {Craft::Courier, "You are forty minutes down and the last drop is a hospital.",
     "Reorder the round and make the hospital.",
     "Run it in order. Rules are rules.", 52.0},
    {Craft::Rescue, "The party above you are moving slower than the weather.",
     "Go up and get them moving before it lands.",
     "Wait at the base and call it in.", 58.0},
    {Craft::Rescue, "The stretcher lower needs a second anchor and there is one bolt.",
     "Build it off gear and back it up.",
     "Send it off the bolt. It will hold.", 65.0},
    {Craft::Bar, "Somebody has had far too much and is going to drive.",
     "Take the keys off them.",
     "Not your problem. Cut them off and move on.", 40.0},
    {Craft::Bar, "The Friday rush lands three deep and you are on your own.",
     "Work it properly and keep the room happy.",
     "Serve who shouts loudest.", 48.0},
    {Craft::Office, "The report is wrong and the meeting is at nine.",
     "Stay and fix it.",
     "Send it. Nobody reads them.", 45.0},
};
constexpr int kSituationCount =
    static_cast<int>(sizeof(kSituations) / sizeof(kSituations[0]));

}  // namespace

ShiftMoment MomentOnShift(Craft craft, const Rng& worldRng, int day,
                          const CraftDials& d) {
  ShiftMoment out;
  out.craft = craft;
  if (craft == Craft::None) return out;

  // Deterministic on the day and the trade, so a reload does not reroll
  // it -- the same no-reroll rule every gamble in this game lives under.
  Rng rng = worldRng.Derive("moment#" + std::to_string(day) + "#" +
                            std::to_string(static_cast<int>(craft)));
  if (rng.NextDouble() >= d.momentChance) return out;

  std::vector<int> forThisTrade;
  for (int i = 0; i < kSituationCount; i++) {
    if (kSituations[i].craft == craft) forThisTrade.push_back(i);
  }
  if (forThisTrade.empty()) return out;

  const Situation& s =
      kSituations[forThisTrade[rng.IntRange(
          0, static_cast<int>(forThisTrade.size()) - 1)]];
  out.happened = true;
  out.what = s.what;
  out.theHardWay = s.hard;
  out.theEasyWay = s.easy;
  out.needs = s.needs;
  return out;
}

MomentOutcome DecideTheMoment(Craftsman& hand, const ShiftMoment& moment,
                              bool theHardWay, const Rng& worldRng, int day,
                              const CraftDials& d) {
  MomentOutcome out;
  if (!moment.happened || moment.craft == Craft::None) return out;
  const int i = static_cast<int>(moment.craft);

  if (!theHardWay) {
    // **Never wrong and never gets you anywhere.** It is what a tired
    // person does; the game does not editorialise about it.
    hand.momentsDucked++;
    out.news = moment.theEasyWay;
    return out;
  }

  hand.momentsTaken++;
  // At the line it is a coin flip you usually win; well under it you are
  // mostly making things worse. **Reaching past your craft is the whole
  // decision** -- the easy way is always there.
  const double have = hand.skill[i];
  const double odds =
      moment.needs <= 0.0
          ? 1.0
          : std::max(0.0, std::min(1.0,
                                   d.botchBelow * (have / moment.needs)));
  Rng rng = worldRng.Derive("decide#" + std::to_string(day) + "#" +
                            std::to_string(i));
  if (rng.NextDouble() < odds) {
    out.payMultiplier = 1.0 + d.momentBonus;
    out.standingShift = d.momentStanding;
    out.news = std::string(moment.theHardWay) + "  It went fine.";
  } else {
    out.botched = true;
    hand.momentsBotched++;
    out.standingShift = -d.botchStanding;
    // Said without editorial. You tried something you were not ready for
    // and it did not come off, which is a thing that happens at work.
    out.news = std::string(moment.theHardWay) + "  It did not go fine.";
  }

  hand.standing[i] =
      std::max(-1.0, std::min(1.0, hand.standing[i] + out.standingShift));
  if (!hand.sacked[i] && hand.standing[i] <= d.sackAt) {
    hand.sacked[i] = true;
    hand.sackings++;
    out.sacked = true;
    // **And it outlives the job**: the gig comes off your board and stays
    // off, and no amount of climbing gets it back.
    out.news += "  They will not be calling you again.";
  }
  return out;
}

// --- who that makes you -------------------------------------------------

Craft YourTrade(const Craftsman& hand, const CraftDials& d) {
  Craft best = Craft::None;
  double most = d.tradeAt;
  for (int i = 1; i < kCraftCount; i++) {
    if (hand.skill[i] > most) {
      most = hand.skill[i];
      best = static_cast<Craft>(i);
    }
  }
  return best;
}

std::string TradeText(const Craftsman& hand, double climbingGrade,
                      const CraftDials& d) {
  const Craft trade = YourTrade(hand, d);
  if (trade == Craft::None) return std::string();
  const std::string what = CraftName(trade);
  // **Which way round it goes depends on how much of each you have**, and
  // both are read as a fraction of their own top so the comparison means
  // something. The craft's top is its ceiling. Climbing's is 12 --
  // `SkillToGrade(100)`, the top of the skill ladder -- rather than the 18
  // the guidebook goes to, because 13 and up is where `DirtbagSession.h`
  // says the exceptional live and no player character gets there.
  //
  // A climber who is better at setting than at climbing is a setter who
  // climbs, whatever they would say at a party.
  const double atTrade =
      hand.skill[static_cast<int>(trade)] / std::max(1.0, d.craftCeiling);
  const double atClimbing =
      std::min(1.0, std::max(0.0, climbingGrade) / 12.0);
  return atTrade > atClimbing ? "Somebody who does " + what + ", and climbs."
                              : "A climber who does " + what + ".";
}

std::string CraftText(const Craftsman& hand, const CraftDials& d) {
  std::string out;
  for (int i = 1; i < kCraftCount; i++) {
    if (hand.shifts[i] <= 0) continue;
    if (!out.empty()) out += "  ";
    out += std::string(CraftName(static_cast<Craft>(i))) + ": " +
           std::to_string(static_cast<int>(std::lround(hand.skill[i])));
    if (hand.sacked[i]) {
      out += " (not welcome)";
    } else if (hand.standing[i] > 0.4) {
      out += " (they ask for you)";
    }
  }
  (void)d;
  return out;
}

}  // namespace dirtbag
