#include "DirtbagPartner.h"

#include <algorithm>

#include "DirtbagSession.h"

namespace dirtbag {

namespace {

// The Lot's cast. Authored, because five people are content and not a
// distribution — and because "two named neighbours" means named.
struct Regular {
  const char* name;
  const char* tag;
  double baseSkill;   // where they were on day one
  double ambition;    // how hard they are still trying
  bool climbs;
};

const Regular kRegulars[] = {
    // The three partners.
    {"Margo",
     "twenty seasons here, knows every line and undersells all of them",
     58.0, 0.35, true},
    {"Dev",
     "young, strong, and here for the hard stuff — will have your project "
     "if you leave it lying around",
     72.0, 0.95, true},
    {"Trish",
     "psyched beyond all reason, gives terrible beta with total confidence",
     42.0, 0.6, true},

    // The two neighbours: the Lot is not only climbers, and the people who
    // stopped climbing are the ones who remember why the lines are called
    // what they are.
    {"Ray",
     "hasn't pulled on in years, put up half the crag, tells you which half",
     0.0, 0.0, false},
    {"Bo",
     "passing through for the last four months, fixes vans for beer",
     0.0, 0.0, false},
};
constexpr int kRegularCount =
    static_cast<int>(sizeof(kRegulars) / sizeof(kRegulars[0]));

}  // namespace

Climber PartnerOn(const Rng& worldRng, const std::string& name,
                  double baseSkill, double ambition, int day,
                  const PartnerDials& dials) {
  // Derived, never stored: strength is a function of who they are and what
  // day it is, so it cannot drift and does not need saving.
  Rng rng = worldRng.Derive("partner:" + name);

  Climber c;
  const double creep = dials.skillPerDay * ambition * std::max(0, day - 1);
  // A spread so nobody is flat across every discipline — the same numbers
  // every day for the same person.
  const auto at = [&]() {
    return std::min(dials.ceiling,
                    baseSkill + creep + (rng.NextDouble() * 8.0 - 4.0));
  };
  c.skills.power = at();
  c.skills.fingers = at();
  c.skills.technique = at();
  c.skills.endurance = at();
  c.skills.head = at();
  c.skin = 9.0;
  c.psyche = 0.7;
  return c;
}

std::vector<Partner> LotRegulars(const Rng& worldRng, int day,
                                 const PartnerDials& dials) {
  std::vector<Partner> out;
  out.reserve(kRegularCount);
  for (int i = 0; i < kRegularCount; i++) {
    const Regular& r = kRegulars[i];
    Partner p;
    p.name = r.name;
    p.tag = r.tag;
    p.ambition = r.ambition;
    p.climbs = r.climbs;
    if (r.climbs) {
      p.climber = PartnerOn(worldRng, r.name, r.baseSkill, r.ambition, day,
                            dials);
    }
    out.push_back(p);
  }
  return out;
}

bool KnowsLine(const Partner& partner, const CragLine& line) {
  if (!partner.climbs) {
    return false;   // Ray knows the history, not the moves
  }
  if (line.isProject) {
    // Nobody knows an unclimbed line, unless they are the one who did it.
    return std::find(partner.firstAscents.begin(), partner.firstAscents.end(),
                     line.route.name) != partner.firstAscents.end();
  }
  // Everything in the book is common knowledge to somebody who can climb it.
  return AbilityOnRoute(partner.climber, line.route) >=
         static_cast<double>(line.route.trueGrade);
}

double ShareBeta(const Partner& partner, const CragLine& line,
                 ProjectMemory& memory, const PartnerDials& dials) {
  return ShareBeta(partner, line, memory, 1.0, dials);
}

double ShareBeta(const Partner& partner, const CragLine& line,
                 ProjectMemory& memory, double generosity,
                 const PartnerDials& dials) {
  if (!KnowsLine(partner, line)) {
    return 0.0;
  }
  const double share = Clamp01(
      (dials.betaShareAtNoRapport +
       (dials.betaShareAtFullRapport - dials.betaShareAtNoRapport) *
           partner.rapport) *
      std::max(0.0, generosity));
  // They can only give you what they have, and only the part you are
  // missing. Beta never goes backwards.
  const double gained = (1.0 - memory.beta) * share;
  if (gained <= 0.0) {
    return 0.0;
  }
  memory.beta = std::min(1.0, memory.beta + gained);
  return gained;
}

double PsycheFrom(const Partner& partner, const PartnerDials& dials) {
  // Even the non-climbers are worth something at the fire.
  const double base = partner.climbs ? 1.0 : 0.6;
  return dials.psychePerRapport * partner.rapport * base;
}

void SpendDayWith(Partner& partner, bool together, const PartnerDials& dials) {
  if (together) {
    partner.rapport = std::min(1.0, partner.rapport + dials.rapportPerDay);
  } else {
    partner.rapport = std::max(0.0, partner.rapport - dials.rapportDecayPerDay);
  }
}

std::vector<std::string> SpokenFor(const std::vector<ProjectMemory>& projects,
                                   const PartnerDials& dials) {
  std::vector<std::string> out;
  for (const ProjectMemory& m : projects) {
    // Anything you have pulled on, anything you have already put up, and
    // anything you have brushed -- taking a wire brush to a line is the
    // most public way there is of saying you are on it.
    if (m.attempts > 0 || m.firstAscent ||
        m.cleanliness > dials.brushedEnoughToBeYours) {
      out.push_back(m.routeName);
    }
  }
  return out;
}

int PartnerTakesFirstAscent(const Rng& worldRng, const Partner& partner,
                            const Crag& crag,
                            const std::vector<std::string>& spokenFor,
                            int day, const PartnerDials& dials) {
  if (!partner.climbs) {
    return -1;
  }
  // Its own stream: the Lot getting on with its life must never shift the
  // rng the player's own attempts are resolved on.
  Rng rng = worldRng.Derive("lot:" + partner.name + "#" + std::to_string(day));

  for (size_t i = 0; i < crag.lines.size(); i++) {
    const CragLine& line = crag.lines[i];
    if (!line.isProject) {
      continue;
    }
    if (std::find(spokenFor.begin(), spokenFor.end(), line.route.name) !=
        spokenFor.end()) {
      continue;   // claimed, or somebody is visibly on it
    }
    // They commit only to lines comfortably inside their level. Nobody at
    // the Lot is projecting at their limit for months; that is the player's
    // particular sickness.
    const double ability = AbilityOnRoute(partner.climber, line.route);
    if (ability < line.route.trueGrade + dials.partnerMarginGrades) {
      continue;
    }
    if (rng.Chance(dials.firstAscentChancePerDay * partner.ambition)) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

namespace {

// Names in the register the crag already speaks: wry, local, and mostly
// about the day rather than the rock. Deliberately none of them heroic —
// people who put up lines at a roadside boulder field name them after what
// went wrong on the walk in.
const char* const kTheirNames[] = {
    "Local Knowledge",     "Rest Day",           "The Long Way Round",
    "Beta Spray",          "Closing Time",       "Third Time Lucky",
    "Van Life",            "Somebody's Project", "The Wrong Shoes",
    "Grit Under the Nails", "Down at Heel",      "Two Pads and a Prayer",
    "Not My Grade",        "The Short Straw",    "Borrowed Chalk",
    "Last Orders",         "Cold Enough",        "The Other Arete",
};
constexpr int kTheirNameCount =
    static_cast<int>(sizeof(kTheirNames) / sizeof(kTheirNames[0]));

}  // namespace

std::string NameTheirLine(const std::string& who, const CragLine& line) {
  // Hashed off both, through the project's own seed hash so this behaves
  // like everything else that has to be stable: same person, same rock,
  // same name, forever, and no engine randomness anywhere near it.
  const std::size_t h = HashSeedString(who + "|" + line.route.name);
  return kTheirNames[h % static_cast<std::size_t>(kTheirNameCount)];
}

bool TheyPutUpTheLine(CragLine& line, const std::string& who) {
  if (!line.isProject || !line.firstAscentBy.empty()) return false;
  if (who.empty()) return false;

  line.displayName = NameTheirLine(who, line);
  line.firstAscentBy = who;
  line.isProject = false;
  // They know what it went at, the same way the player would.
  line.route.grade = line.route.trueGrade;
  return true;
}

void ApplyBonds(std::vector<Partner>& lot,
                const std::vector<PartnerBond>& bonds) {
  for (Partner& p : lot) {
    for (const PartnerBond& b : bonds) {
      if (b.name == p.name) {
        p.rapport = b.rapport;
        p.firstAscents = b.firstAscents;
        break;
      }
    }
  }
}

std::vector<PartnerBond> BondsFrom(const std::vector<Partner>& lot) {
  std::vector<PartnerBond> out;
  out.reserve(lot.size());
  for (const Partner& p : lot) {
    // A stranger you have never climbed with is not worth a line in the
    // save file; they will be exactly as much of a stranger next time.
    if (p.rapport <= 0.0 && p.firstAscents.empty()) {
      continue;
    }
    PartnerBond b;
    b.name = p.name;
    b.rapport = p.rapport;
    b.firstAscents = p.firstAscents;
    out.push_back(b);
  }
  return out;
}

std::string LotTalk(const Partner& partner, const Crag& crag, int day) {
  if (!partner.climbs) {
    return partner.name + ": " + partner.tag;
  }
  // What they are on this week — visible from the fire, so a project you are
  // about to lose is something you watched them working.
  Rng rng = Rng::FromSeed("lottalk:" + partner.name + "#" +
                          std::to_string(day / 7));
  std::vector<const CragLine*> plausible;
  for (const CragLine& line : crag.lines) {
    const double ability = AbilityOnRoute(partner.climber, line.route);
    if (ability >= line.route.trueGrade - 1.0 &&
        ability <= line.route.trueGrade + 3.0) {
      plausible.push_back(&line);
    }
  }
  if (plausible.empty()) {
    return partner.name + ": " + partner.tag;
  }
  const CragLine* on =
      plausible[static_cast<size_t>(rng.NextDouble() * plausible.size()) %
                plausible.size()];
  const std::string what =
      on->isProject ? on->description : DisplayName(*on);
  return partner.name + " has been on " + what + " all week.";
}

}  // namespace dirtbag
