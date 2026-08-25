#include "DirtbagGymTown.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "DirtbagConditions.h"
#include "DirtbagRng.h"

namespace dirtbag {
namespace {

struct RivalMove {
  const char* text;
  double pull;
};

struct RivalDef {
  const char* name;
  double base;
  const RivalMove* moves;
  int moveCount;
};

// **INVARIANT: each table sums to zero.** A table that is net-positive
// makes the competition a permanent tax rather than a weather system --
// the source records its first cut averaging 0.948 pressure, a silent 5%
// nerf to every gym in the game forever. `TheTownIsWeatherNotATax` asserts
// it directly so it cannot come back.
const RivalMove kSendCityMoves[] = {
  {"dropped their student rate to six dollars and put it on every lamppost "
   "in town", 0.20},
  {"opened a second lead wall -- the room is genuinely impressive now", 0.22},
  {"started a Sunday kids club and the lobby is full of parents with coffee",
   0.13},
  {"hired a setter away from somewhere with actual mountains", 0.15},
  {"let their front desk go, and the queue at six o'clock shows it", -0.32},
  {"put their rates up and did not tell anybody first", -0.38},
};

const RivalMove kCaveMoves[] = {
  {"went twenty-four hour with a fob on the door", 0.20},
  {"resurfaced every board in the place over one long weekend", 0.17},
  {"put a coffee bar in the corner and people are staying two hours longer",
   0.11},
  {"lost their best setter to Colorado, and the walls have not changed since",
   -0.20},
  {"built a slab wall nobody asked for", -0.12},
  {"had a hold spin during a busy session and word got round", -0.16},
};

const RivalDef kRivals[kGymRivalCount] = {
  {"Send City", 1.00, kSendCityMoves,
   static_cast<int>(sizeof(kSendCityMoves) / sizeof(kSendCityMoves[0]))},
  {"The Cave", 0.85, kCaveMoves,
   static_cast<int>(sizeof(kCaveMoves) / sizeof(kCaveMoves[0]))},
};

double RivalBaseTotal() {
  double out = 0.0;
  for (int i = 0; i < kGymRivalCount; i++) out += kRivals[i].base;
  return out;
}

// A number in [0, 1) that is a pure function of the string. The same
// derived-not-rolled idiom `DirtbagPartner` uses for a partner's talk.
double TownDraw(const std::string& label) {
  return Rng::FromSeed(label).NextDouble();
}

int Slot(GymRival r) { return static_cast<int>(r); }
int Slot(GymSeason s) { return static_cast<int>(s); }

const GymIncidentDef kIncidents[] = {
  {GymIncident::Pipe, "A pipe goes, over the boulder",
   "It let go at four in the morning above the cave section and the mats "
   "have been standing in it since. A third of your bouldering is currently "
   "a lake.",
   {{"Get somebody out today", 900.0, 0.0, 0.0,
     "Emergency rate, cash, no argument. The mats are up by Thursday and "
     "most people never knew.", false},
    {"Tape it off and deal with it later", 0.0, -6.0, -1.0,
     "The barrier tape stays up for three weeks. People notice what you did "
     "not fix.", false}}},

  {GymIncident::Spinner, "A hold spun, and somebody fell",
   "Mid-move, on the overhang, in front of the Tuesday crowd. She got up and "
   "said she was fine, and she probably is, and everybody in the room watched "
   "it happen.",
   {{"Comp her year and audit every hold", 450.0, 0.0, 3.0,
     "You shut the wall, torque every hold in the building, and tell the "
     "floor exactly what you found. The room takes that seriously.", false},
    {"Apologise and move on -- it happens", 0.0, -7.0, -3.0,
     "It does happen. It also gets talked about, in a small town, for a long "
     "time.", false}}},

  {GymIncident::Ac, "The AC dies in a heatwave",
   "Ninety-three outside and the unit on the roof has given up during the "
   "busiest week of the summer. The lead wall is unusable by two in the "
   "afternoon.",
   {{"Replace the unit", 1400.0, 0.0, 1.0,
     "It costs what a small van costs. The building is bearable by the "
     "weekend and the summer is saved.", false},
    {"Open the doors and hand out water", 40.0, -8.0, 0.0,
     "Fans, propped doors, and a cooler of water. People are gracious about "
     "it and then quietly stop coming until October.", false}}},

  {GymIncident::Viral, "A clip from your gym went round",
   "Somebody filmed a kid flashing the hard problem on your comp wall and it "
   "has done numbers. Your gym's name is on the tape in the corner of every "
   "frame.",
   {{"Put money behind it while it is hot", 600.0, 12.0, 2.0,
     "You spend on reach for four days while the clip is still moving. The "
     "lobby is full of people who found you on a phone.", false},
    {"Let it run its course", 0.0, 3.0, 1.0,
     "A handful of new faces turn up asking about the problem from the "
     "video. It is nice. It is not a strategy.", false}}},

  {GymIncident::Inspector, "The fire inspector has notes",
   "Three of them, on a clipboard, about the back exit, the extinguisher "
   "dates and a length of cable somebody would have sorted in an afternoon.",
   {{"Bring it all up to code", 700.0, 0.0, 1.0,
     "Signed off in a fortnight. Dull, expensive, and the sort of thing that "
     "only matters the once.", false},
    {"Fix the cheap one and hope", 60.0, -4.0, -2.0,
     "He comes back. He is not pleased. The re-inspection notice goes on the "
     "door where everybody can read it.", false}}},

  {GymIncident::Poach, "Somebody is trying to hire your setter",
   "The other gym in town made an approach, and a decent one. Your setter "
   "told you about it themselves, which tells you something, and now it is "
   "your move.",
   {{"Match it", 0.0, 0.0, 0.0,
     "You put their wage up on the spot and they stay. It costs you every "
     "day from here, and it was still the right call.", true},
    {"Wish them well", 0.0, -5.0, 0.0,
     "They give proper notice and leave on good terms. The walls stop "
     "changing, and the floor works out why within a fortnight.", false}}},
};

constexpr int kIncidentDefCount =
    static_cast<int>(sizeof(kIncidents) / sizeof(kIncidents[0]));

}  // namespace

const char* GymRivalName(GymRival r) {
  const int i = Slot(r);
  return i >= 0 && i < kGymRivalCount ? kRivals[i].name : "";
}

int TownWindow(int day, const GymTownDials& dials) {
  return std::max(0, day) / std::max(1, dials.moveDays);
}

TownMove WhatTheTownDid(int window, const std::string& salt) {
  TownMove out;
  // **Scrambled before the modulo.** With two rivals a raw seed taken
  // modulo two is a parity sum and they would strictly alternate forever --
  // which is not a town, it is a metronome.
  const int who = static_cast<int>(
      TownDraw("gymmove|who|" + salt + "|" + std::to_string(window)) *
      kGymRivalCount) % kGymRivalCount;
  const RivalDef& r = kRivals[who];
  const int what = static_cast<int>(
      TownDraw("gymmove|what|" + salt + "|" + std::to_string(window) + "|" +
              std::to_string(who)) * r.moveCount) % r.moveCount;
  out.who = static_cast<GymRival>(who);
  out.what = r.moves[what].text;
  out.pull = r.moves[what].pull;
  return out;
}

double RivalStrength(GymRival r, int day, const std::string& salt,
                     const GymTownDials& dials) {
  const int i = Slot(r);
  if (i < 0 || i >= kGymRivalCount) return 0.0;
  const RivalDef& def = kRivals[i];
  const int now = TownWindow(day, dials);
  double out = def.base;
  for (int w = std::max(0, now - std::max(1, dials.moveMemory) + 1); w <= now;
       w++) {
    const TownMove ev = WhatTheTownDid(w, salt);
    if (Slot(ev.who) != i) continue;
    out += ev.pull * std::pow(dials.moveDecay, now - w);
  }
  return std::max(def.base * dials.rivalFloor, out);
}

double RivalPull(int day, const std::string& salt,
                 const GymTownDials& dials) {
  double total = 0.0;
  for (int i = 0; i < kGymRivalCount; i++) {
    total += RivalStrength(static_cast<GymRival>(i), day, salt, dials);
  }
  return total;
}

double TownPressure(int day, const std::string& salt, double campaignShield,
                    const GymTownDials& dials) {
  const double total = RivalPull(day, salt, dials);
  const double shield = campaignShield > 0.0 ? campaignShield : 1.0;
  if (total <= 0.0) return dials.pressureMax;
  const double raw = RivalBaseTotal() / (total / shield);
  return std::min(dials.pressureMax, std::max(dials.pressureMin, raw));
}

GymSeason GymSeasonOf(int day) {
  // Off the climate model, not the calendar -- see the header. The strings
  // are `DirtbagConditions`'s and this is the one place they are read as a
  // value rather than printed.
  const std::string s = SeasonName(day);
  if (s == "summer") return GymSeason::Summer;
  if (s == "winter") return GymSeason::Winter;
  if (s == "autumn") return GymSeason::Autumn;
  return GymSeason::Spring;
}

const char* GymSeasonNote(GymSeason s) {
  switch (s) {
    case GymSeason::Spring:
      return "Spring -- the rock is coming into condition and the evening "
             "crowd is thinning.";
    case GymSeason::Summer:
      return "Summer -- too hot to send outside. The whole town is in here "
             "and the AC is losing.";
    case GymSeason::Autumn:
      return "Autumn -- send season. Everybody who can get outside is "
             "outside, and the floor shows it.";
    case GymSeason::Winter:
      return "Winter -- cold, wet, dark at four. This is the season the gym "
             "pays for the other three.";
    default: return "";
  }
}

double SeasonPull(int day, const GymTownDials& dials) {
  return dials.seasonPull[Slot(GymSeasonOf(day))];
}

const GymIncidentDef* IncidentDef(GymIncident id) {
  if (id == GymIncident::None) return nullptr;
  for (int i = 0; i < kIncidentDefCount; i++) {
    if (kIncidents[i].id == id) return &kIncidents[i];
  }
  return nullptr;
}

GymIncident IncidentToday(const std::string& gymName, int ownedDay, int day,
                          bool hasSetter, const GymTownDials& dials) {
  if (day - ownedDay < dials.incidentGrace) return GymIncident::None;
  const std::string key = "gymincident|" + gymName + "|" + std::to_string(day);
  if (TownDraw(key) >= dials.incidentChance) return GymIncident::None;

  // The poach only exists if there is somebody to poach, so the pool is
  // built before the pick rather than rolled and then rejected -- rejecting
  // would make a gym with no setter quietly have fewer incidents overall.
  GymIncident pool[kIncidentDefCount];
  int n = 0;
  for (int i = 0; i < kIncidentDefCount; i++) {
    if (kIncidents[i].id == GymIncident::Poach && !hasSetter) continue;
    pool[n++] = kIncidents[i].id;
  }
  if (n == 0) return GymIncident::None;
  const int which =
      static_cast<int>(TownDraw("gymincident|which|" + gymName + "|" +
                               std::to_string(day)) * n) % n;
  return pool[which];
}

}  // namespace dirtbag
