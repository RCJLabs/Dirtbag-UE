#include "DirtbagMentor.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {
namespace {

// Her curriculum, in order: movement, tactics, fear, efficiency, mastery.
// The amounts are the dials -- the whole arc tunes here.
const Lesson kLessons[] = {
    {"Footwork",
     "An old crusher has been watching you from the boulders -- leathery, "
     "chalk to the elbows. She ambles over and spits. \"Forty years I have "
     "climbed these rocks. Watched you flail that slab like the holds owe "
     "you money. Hands are for show, kid. Feet send. Watch mine. Not my "
     "hands.\"",
     "She walked you through your feet for an hour. Quiet feet, weight over "
     "the toe, trust the rubber.",
     Skill::Technique, 5.0, Skill::Technique, 0.0},
    {"Reading the Line",
     "She is posted up at the base, arms crossed. \"Back already. Good. "
     "Today you do not touch the rock until you have read it. Stand back. "
     "See the whole thing -- where it lets you rest, where it spits you "
     "off. I put up half these lines. Read every one before I left the "
     "ground.\"",
     "You read three lines cold before pulling on. The wall makes sense "
     "before your hands are chalked.",
     Skill::Head, 4.0, Skill::Head, 0.0},
    {"Falling",
     "She catches you bailing off a move you had. \"You climb scared "
     "because you fall bad. Took a forty-footer myself back in the day. "
     "Ground-up, no bolts, no spotter. Taught me to fall soft and quit "
     "death-gripping. Let go when I say. I have got you.\"",
     "She made you fall, and fall, and fall, until it stopped being a "
     "thing. You climb loose now. Committed.",
     Skill::Head, 5.0, Skill::Head, 0.0},
    {"Resting",
     "\"You are strong enough, kid. You just never learned to stop.\" She "
     "shows you the no-hands rest, the shakeout, the breath. \"My old "
     "partner beat this into me on a route neither of us had any business "
     "sending. Find the rest. Stay on the wall.\"",
     "No-hands rests, drop-knees, breathing through the pump. You can stay "
     "on the wall a lot longer now.",
     Skill::Endurance, 4.0, Skill::Endurance, 0.0},
    {"The Send",
     "She waves you over, quieter than usual, rolling a bad shoulder. "
     "\"Last one. Everything I have got about moving on rock -- it is "
     "yours now. I am getting old; these shoulders are shot. Will not be "
     "long before the young ones are watching you climb. So pass it on, "
     "hey? That is the whole deal.\"",
     "She shook your hand like an equal and wandered back to the boulders. "
     "You move like somebody taught you now. Pass it on someday.",
     Skill::Technique, 10.0, Skill::Head, 6.0},
};
constexpr int kLessonCount =
    static_cast<int>(sizeof(kLessons) / sizeof(kLessons[0]));

// `TUT-6`: what she says in the Lot, before she is anybody's mentor. **Each
// line is gated on something actually true about you right now**, so she is
// reading your situation rather than reciting a tutorial -- and the last one
// hands you to the arc.
struct LotLine {
  const char* text;
  // -1 for no gate. `wantsBroke` fires under `cash`, otherwise over it.
  double cash;
  bool wantsBroke;
  double minGrade;
  bool wantsNoVan;
};

const LotLine kLotLines[] = {
    {"\"New one. You will want to park up the end there, out of the wind. "
     "Nobody will move you on for a week or two.\"",
     -1.0, false, -1.0, false},
    {"She looks at your hands. \"Tape is cheaper than a doctor and a doctor "
     "is cheaper than a season. Ask me how I know.\"",
     -1.0, false, -1.0, false},
    {"\"Broke, hey. Everybody here is broke. The ones who last are the ones "
     "who are broke on purpose -- they know exactly what they are spending "
     "it on.\"",
     60.0, true, -1.0, false},
    {"\"No van yet. Sleep light and do not get comfortable. A van is not "
     "freedom, it is just the cheapest room in town that moves.\"",
     -1.0, false, -1.0, true},
    {"\"You are starting to climb something. I have been watching. Come "
     "find me at the crag -- I have got a couple of things you are doing "
     "wrong and about forty years of why.\"",
     -1.0, false, 2.0, false},
};
constexpr int kLotLineCount =
    static_cast<int>(sizeof(kLotLines) / sizeof(kLotLines[0]));

// The cast. Fixed people, because a client you coach for two years should
// have a name and one true thing about them.
const ClientDef kClients[] = {
    {"Priya", "reads sequences like poetry, and tenses up the second "
              "falling becomes real.", Focus::Technique, Focus::Head},
    {"Theo", "throws himself at everything, footwork be damned.",
     Focus::Power, Focus::Technique},
    {"Nadia", "will grind a project for an hour, and will not leave the "
              "ground on anything steep.", Focus::Siege, Focus::Head},
    {"Cole", "picks apart beta faster than anyone, then gasses out three "
             "moves from the finish.", Focus::Technique, Focus::Siege},
    {"Wes", "strong as an ox, never gets hurt, and has zero patience for a "
            "long project.", Focus::Durable, Focus::Siege},
    {"Esme", "meticulous with warmups, injury-free two years running, "
             "freezes above her comfort grade.", Focus::Durable, Focus::Head},
    {"Jonah", "psyched and fearless, always going for it, always skipping "
              "the warmup.", Focus::Head, Focus::Durable},
    {"Lena", "grinds a project for weeks without complaint. Footwork is an "
             "afterthought and it shows.", Focus::Siege, Focus::Technique},
};
constexpr int kClientCount =
    static_cast<int>(sizeof(kClients) / sizeof(kClients[0]));

const char* kGoals[] = {"The Egg",          "Kneebar Special",
                        "The Undercling",   "Thin Crimps",
                        "The Sloper Problem", "Heel Hook Traverse",
                        "The Mantle",       "Powerful Start",
                        "The Reachy One",   "Slab of Doubt"};
constexpr int kGoalCount =
    static_cast<int>(sizeof(kGoals) / sizeof(kGoals[0]));

std::string GradeWord(double grade) {
  return std::string("V") + std::to_string(static_cast<int>(grade + 0.5));
}

}  // namespace

const char* FocusName(Focus f) {
  switch (f) {
    case Focus::Power:     return "power";
    case Focus::Technique: return "technique";
    case Focus::Siege:     return "projecting";
    case Focus::Head:      return "the mental game";
    case Focus::Durable:   return "longevity";
  }
  return "technique";
}

const char* FocusTeaches(Focus f) {
  switch (f) {
    case Focus::Power:     return "hard bouldering and explosive pulling";
    case Focus::Technique: return "precise feet and quiet technique";
    case Focus::Siege:     return "reading sequences and sieging projects";
    case Focus::Head:      return "commitment and going for it first try";
    case Focus::Durable:   return "warming up and managing load";
  }
  return "precise feet and quiet technique";
}

const char* FocusBecomes(Focus f) {
  switch (f) {
    case Focus::Power:     return "a powerful boulderer";
    case Focus::Technique: return "a silky technician";
    case Focus::Siege:     return "a patient projector";
    case Focus::Head:      return "a fearless sender";
    case Focus::Durable:   return "a durable lifer";
  }
  return "a silky technician";
}

RouteType FocusRoute(Focus f) {
  switch (f) {
    case Focus::Power:     return RouteType::Power;
    case Focus::Technique: return RouteType::Technical;
    case Focus::Siege:     return RouteType::Endurance;
    case Focus::Head:      return RouteType::Dyno;
    case Focus::Durable:   return RouteType::Crimp;
  }
  return RouteType::Technical;
}

// --- the half where you are taught ---------------------------------------

const Lesson* LessonAt(int stage, const MentorDials& dials) {
  (void)dials;
  if (stage < 0 || stage >= kLessonCount) return nullptr;
  return &kLessons[stage];
}

bool SheIsAround(const Mentor& mentor, double yourGrade, const Rng& worldRng,
                 int day, const MentorDials& dials) {
  if (SheIsDoneWithYou(mentor, dials)) return false;
  if (yourGrade < dials.minGrade) return false;
  if (mentor.lastDay >= 0 && day - mentor.lastDay < dials.restDays) {
    return false;
  }
  // Keyed on the day, so the same day is the same day and a reload does not
  // re-roll her -- the no-reroll rule every gamble in this game lives under.
  Rng rng = worldRng.Derive("mentor|" + std::to_string(day));
  return rng.Chance(dials.appearChance);
}

const Lesson* ClimbWithHer(Mentor& mentor, Skills& skills, int day,
                           const MentorDials& dials) {
  const Lesson* lesson = LessonAt(mentor.stage, dials);
  if (lesson == nullptr) return nullptr;

  const auto teach = [&skills](Skill lane, double amount) {
    if (amount <= 0.0) return;
    switch (lane) {
      case Skill::Power:     skills.power = std::min(100.0, skills.power + amount); break;
      case Skill::Fingers:   skills.fingers = std::min(100.0, skills.fingers + amount); break;
      case Skill::Technique: skills.technique = std::min(100.0, skills.technique + amount); break;
      case Skill::Endurance: skills.endurance = std::min(100.0, skills.endurance + amount); break;
      case Skill::Head:      skills.head = std::min(100.0, skills.head + amount); break;
    }
  };
  teach(lesson->lane, lesson->amount);
  teach(lesson->lane2, lesson->amount2);

  mentor.stage++;
  mentor.lastDay = day;
  return lesson;
}

bool SheIsDoneWithYou(const Mentor& mentor, const MentorDials& dials) {
  return mentor.stage >= std::min(dials.sessions, kLessonCount);
}

std::string WhatSheSaysAtTheLot(Mentor& mentor, double yourGrade, double cash,
                                bool hasVan, const MentorDials& dials) {
  (void)dials;
  for (int i = 0; i < kLotLineCount; i++) {
    // Said once, ever. A bitfield rather than a list because there are five
    // of them and there will never be fifty.
    if ((mentor.saidAtTheLot & (1 << i)) != 0) continue;
    const LotLine& l = kLotLines[i];
    if (l.cash >= 0.0 && (l.wantsBroke ? cash >= l.cash : cash < l.cash)) {
      continue;
    }
    if (l.minGrade >= 0.0 && yourGrade < l.minGrade) continue;
    if (l.wantsNoVan && hasVan) continue;
    mentor.saidAtTheLot |= (1 << i);
    return l.text;
  }
  return std::string();
}

// --- the half where you teach --------------------------------------------

int HowManyClientsThereAre() { return kClientCount; }

const ClientDef* TheClient(int who) {
  if (who < 0 || who >= kClientCount) return nullptr;
  return &kClients[who];
}

bool RoomForOneMore(const Roster& roster, const RosterDials& dials) {
  int taken = 0;
  for (const Client& c : roster.clients) {
    if (c.who >= 0) taken++;
  }
  return taken < dials.maxClients && taken < kClientCount;
}

bool TakeThemOn(Roster& roster, const Rng& worldRng, int day,
                const RosterDials& dials) {
  if (!RoomForOneMore(roster, dials)) return false;

  Rng rng = worldRng.Derive("roster|" + std::to_string(day) + "|" +
                            std::to_string(roster.clients.size()));
  // Somebody who is not already on the books. Walked rather than rejected in
  // a loop, so a roster holding most of the cast still finds the rest.
  std::vector<int> free;
  for (int i = 0; i < kClientCount; i++) {
    bool taken = false;
    for (const Client& c : roster.clients) {
      if (c.who == i) { taken = true; break; }
    }
    if (!taken) free.push_back(i);
  }
  if (free.empty()) return false;

  Client fresh;
  fresh.who = free[static_cast<std::size_t>(
      rng.IntRange(0, static_cast<int>(free.size()) - 1))];
  fresh.grade = dials.startGrade;
  fresh.startGrade = dials.startGrade;
  fresh.startDay = day;
  // **Not their weakness.** You do not know it on day one -- that is what
  // the first session is for, and starting them on the right answer would
  // make the plan a formality.
  fresh.plan = kClients[fresh.who].strength;
  // `JOB-10`: whether this one is the real thing is decided here, before
  // you have coached them once. Only a coach at the top of the craft would
  // ever recognise it, so only they roll for it -- but it was true about
  // the person either way.
  fresh.prodigy = rng.Chance(dials.prodigyChance);
  fresh.goal = std::string(kGoals[rng.IntRange(0, kGoalCount - 1)]) + " (" +
               GradeWord(fresh.grade + 1.0) + ")";
  roster.clients.push_back(fresh);
  return true;
}

bool SetThePlan(Roster& roster, int which, Focus plan) {
  if (which < 0 || which >= static_cast<int>(roster.clients.size())) {
    return false;
  }
  if (roster.clients[which].who < 0) return false;
  roster.clients[which].plan = plan;
  return true;
}

namespace {

double FitFor(const Client& c, const RosterDials& dials) {
  const ClientDef* def = TheClient(c.who);
  if (def == nullptr) return dials.fitOtherwise;
  if (c.plan == def->weakness) return dials.fitOnWeakness;
  if (c.plan == def->strength) return dials.fitOnStrength;
  return dials.fitOtherwise;
}

}  // namespace

CoachedSession CoachThem(Roster& roster, int which, double craft,
                         const Rng& worldRng, int day,
                         const RosterDials& dials) {
  CoachedSession out;
  if (which < 0 || which >= static_cast<int>(roster.clients.size())) {
    return out;
  }
  Client& c = roster.clients[which];
  const ClientDef* def = TheClient(c.who);
  if (def == nullptr) return out;
  // One session a day with one person. An hour is an hour.
  if (c.lastSessionDay == day) return out;

  out.ran = true;
  c.sessions++;
  c.lastSessionDay = day;
  out.cash = dials.sessionFee;

  const double quality =
      std::min(1.0, std::max(0.0, dials.qualityFromFit * FitFor(c, dials) +
                                      craft * dials.qualityFromCraft));

  Rng rng = worldRng.Derive("coach|" + std::to_string(c.who) + "|" +
                            std::to_string(day));
  const double roll = rng.NextDouble();
  double progress = dials.progressRough;
  const char* how = "a rough hour";
  if (roll < quality * 0.35) {
    progress = dials.progressBreakthrough;
    how = "something clicked";
  } else if (roll < quality * 0.75) {
    progress = dials.progressGood;
    how = "a good session";
  } else if (roll < quality + 0.25) {
    progress = dials.progressDecent;
    how = "a decent hour";
  }

  // `JOB-10`: **you only find out because you read them right**, and only
  // once. The talent was theirs before you met them; the seeing is yours.
  if (c.prodigy && !c.sawIt && craft >= dials.prodigyFromCraft &&
      c.plan == def->weakness) {
    c.sawIt = true;
    out.prodigy = true;
    out.cash += dials.prodigyCash;
    out.rep += dials.prodigyRep;
    progress = dials.progressBreakthrough;
    how = "you saw it";
  }

  c.progress += progress;
  out.progressGained = progress;

  if (c.progress >= dials.progressTarget) {
    c.progress -= dials.progressTarget;
    c.grade += 1.0;
    out.gradedUp = true;
  }

  out.line = std::string(def->name) + ": " + how + ". " +
             (out.gradedUp ? "They are climbing " + GradeWord(c.grade) + " now."
                           : "Working " + c.goal + ".");

  if (c.grade - c.startGrade >= dials.gradesToGraduate) {
    out.graduated = true;
    out.cash += dials.graduationCash;
    out.rep += dials.graduationRep;
    out.line = std::string(def->name) + " does not need you any more. Four "
               "grades in " + std::to_string((day - c.startDay) / 30) +
               " months, and they paid for the beer.";
    roster.graduated++;
    roster.clients.erase(roster.clients.begin() + which);
  }
  return out;
}

std::vector<std::string> TheyTurnedUpToLeagueNight(
    Roster& roster, const Rng& worldRng, int day, RouteType dominant,
    double& standingGained, const RosterDials& dials) {
  std::vector<std::string> said;
  standingGained = 0.0;
  for (Client& c : roster.clients) {
    const ClientDef* def = TheClient(c.who);
    if (def == nullptr) continue;
    if (c.sessions < dials.leagueSessions) continue;

    Rng rng = worldRng.Derive("coachleague|" + std::to_string(c.who) + "|" +
                              std::to_string(day));
    if (!rng.Chance(dials.leagueChance)) continue;

    // **Written by who they are.** The room either played to their strength
    // or found their weakness; anything else is a coin flip. A public
    // outcome nobody could have predicted from the client would not be a
    // coaching outcome at all.
    bool good;
    if (dominant == FocusRoute(def->strength)) {
      good = true;
    } else if (dominant == FocusRoute(def->weakness)) {
      good = false;
    } else {
      good = rng.Chance(0.5);
    }

    if (good) {
      c.progress += dials.leagueProgress;
      standingGained += dials.leagueStanding;
      said.push_back(std::string(def->name) + " had a night. Sent their "
                     "number in front of the whole room and looked like they "
                     "belonged.");
    } else {
      said.push_back(std::string(def->name) + " found the wall that finds "
                     "their " + FocusName(def->weakness) +
                     ". Rough night, and they stayed till the pizza.");
    }
  }
  return said;
}

std::string RosterLine(const Roster& roster, const RosterDials& dials) {
  (void)dials;
  if (roster.clients.empty()) {
    return roster.graduated > 0
               ? "Nobody on the books. " + std::to_string(roster.graduated) +
                     " have come and gone."
               : std::string();
  }
  std::string line;
  for (const Client& c : roster.clients) {
    const ClientDef* def = TheClient(c.who);
    if (def == nullptr) continue;
    if (!line.empty()) line += " ";
    line += std::string(def->name) + " on " + FocusName(c.plan) + ", " +
            GradeWord(c.grade) + ".";
  }
  return line;
}

}  // namespace dirtbag
