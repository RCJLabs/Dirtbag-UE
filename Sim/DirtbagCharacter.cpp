#include "DirtbagCharacter.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace dirtbag {
namespace {

// -100..100 -> -1..1, and the effect of an axis is linear in that.
double Axis(double v) {
  return std::clamp(v / kPersonalityMax, -1.0, 1.0);
}

}  // namespace

const char* SkillName(Skill which) {
  switch (which) {
    case Skill::Power:     return "power";
    case Skill::Fingers:   return "fingers";
    case Skill::Technique: return "technique";
    case Skill::Endurance: return "endurance";
    default:               return "head";
  }
}

// ---------------------------------------------------------------------
// Archetypes. Every row sums to zero -- see the header, and TestCharacter
// checks it rather than trusting the arithmetic below.
// ---------------------------------------------------------------------

const ArchetypeDef& Describe(Archetype a) {
  static const ArchetypeDef kDefs[kArchetypeCount] = {
      {"The Boulderer",
       "Short, powerful problems -- explosive pulls and strong fingers.",
       +6.0, +3.0, -3.0, -4.0, -2.0},
      {"The Rope Gun",
       "Long routes and a cool head -- built to endure and commit.",
       -4.0, -4.0, -4.0, +7.0, +5.0},
      {"The Technician",
       "Precise feet, efficient movement, quiet control.",
       -4.0, +2.0, +6.0, -3.0, -1.0},
      {"The All-Rounder",
       "Balanced everywhere -- no glaring weakness, no specialty.",
       0.0, 0.0, 0.0, 0.0, 0.0},
  };
  return kDefs[static_cast<int>(a)];
}

const char* ArchetypeName(Archetype a) { return Describe(a).name; }

// ---------------------------------------------------------------------
// Origins. Each holds its perk in a lane no other origin touches.
// ---------------------------------------------------------------------

const OriginDef& Describe(Origin o) {
  // Built by a lambda rather than written as brace-initialisers, because
  // OriginDef has twenty fields of which each origin sets four -- positional
  // init would be twenty commas per row with the meaning in the column
  // number, which is how a perk ends up in the wrong lane.
  static const std::array<OriginDef, kOriginCount> kDefs = [] {
    std::array<OriginDef, kOriginCount> d{};

    // Sold It All -- the classic. The lane is shift pay: the CV still works.
    d[0].name = "Sold It All";
    d[0].blurb =
        "You had the salary, the apartment, the five-year plan. You sold all "
        "of it, bought a van, and drove until the cities ran out.";
    d[0].perk = "The resume still works -- every shift pays 12% more";
    d[0].endurance = 2.0; d[0].head = 2.0;
    d[0].cash = 300.0;
    d[0].shiftPay = 1.12;
    d[0].discipline = 15.0; d[0].purism = 10.0;

    // Gym Rat -- the lane is indoor gains, because the gym is home.
    d[1].name = "Gym Rat";
    d[1].blurb =
        "You came up in a city gym -- plastic, padded floors, music. Strong "
        "fingers, but real stone is a different animal.";
    d[1].perk = "The gym is home -- 12% more skill from every indoor session";
    d[1].power = 4.0; d[1].fingers = 4.0; d[1].head = -4.0;
    d[1].endurance = -2.0;
    d[1].cash = 60.0;
    d[1].indoorGain = 1.12;
    d[1].social = 12.0; d[1].purism = -15.0;

    // Desert Local -- the lane is the cost of living. Broke, and used to it.
    d[2].name = "Desert Local";
    d[2].blurb =
        "You grew up on desert sandstone -- no gym, no scene, just you and "
        "the rock before the heat came up. Money was always thin.";
    d[2].perk = "You know how to live on nothing -- 8% lower daily costs";
    d[2].head = 5.0; d[2].technique = 4.0; d[2].power = -3.0;
    d[2].cash = 30.0;
    d[2].standing = 5.0;
    d[2].dailyCost = 0.92;
    d[2].purism = 18.0; d[2].boldness = 8.0; d[2].social = -8.0;

    // Ex-Gymnast -- the lane is deliberate training. Fifteen years of it.
    d[3].name = "Ex-Gymnast";
    d[3].blurb =
        "Fifteen years on the mat built a motor most climbers would kill "
        "for. Now you point it at the wall -- if you can learn to trust a "
        "hold you might rip off of.";
    d[3].perk = "Structured training is second nature -- 12% more from a "
                "workout";
    d[3].power = 6.0; d[3].fingers = 2.0; d[3].head = -5.0;
    d[3].technique = -2.0;
    d[3].cash = 45.0;
    d[3].trainingGain = 1.12;
    d[3].discipline = 15.0;

    // Late Bloomer -- the lane is what getting fixed costs.
    //
    // **The 2D game's perk here was a 35% discount on health insurance, and
    // there is no insurance in this build** -- it is Phase 10. A perk in a
    // lane that does not exist is a field nothing reads, which is the one
    // bug this project keeps finding, so it moved to the nearest lane that
    // is real and has a door: physio, which since 2026-08-23 you can
    // actually walk up to and buy. Same intent -- your affairs are in order
    // and getting hurt costs you less -- and it moves back to premiums when
    // insurance lands.
    d[4].name = "Late Bloomer";
    d[4].blurb =
        "You didn't touch rock until thirty-two. Late to the party, but you "
        "showed up with savings, patience, and no illusions about being a "
        "prodigy.";
    d[4].perk = "Your affairs are in order -- the physio costs 35% less";
    d[4].head = 5.0; d[4].technique = 3.0; d[4].power = -4.0;
    d[4].fingers = -2.0;
    d[4].cash = 220.0;
    d[4].agePlus = 10;
    d[4].physioPrice = 0.65;
    d[4].discipline = 18.0; d[4].boldness = -10.0;

    // Trust-Fund Kid -- the lane is the counter. And the valley knows.
    d[5].name = "Trust-Fund Kid";
    d[5].blurb =
        "Mum and Dad covered the gear, the coach, the gap year. You own the "
        "latest of everything and something to prove -- and the scene can "
        "smell it on you.";
    d[5].perk = "Family money still quietly covers it -- 30% off the shops";
    d[5].technique = 2.0;
    d[5].cash = 400.0;
    d[5].standing = -8.0;
    d[5].shopPrice = 0.7;
    d[5].discipline = -15.0; d[5].purism = -12.0;
    return d;
  }();
  return kDefs[static_cast<int>(o)];
}

const char* OriginName(Origin o) { return Describe(o).name; }

// ---------------------------------------------------------------------

const FlawDef& Describe(Flaw f) {
  static const FlawDef kDefs[kFlawCount] = {
      {"Gumby", "Technique comes slow -- footwork trains at half rate."},
      {"Hard Gainer", "Muscle is slow -- power and finger gains reduced."},
      {"Fair-Weather Trainer",
       "You only really train when you feel good -- tired sessions barely "
       "build you."},
      {"Happy Feet",
       "Shaky footwork -- technical lines are harder than they read."},
      {"Tweaky Fingers", "Injury-prone pulleys. You know the feeling."},
  };
  return kDefs[static_cast<int>(f)];
}

const char* FlawName(Flaw f) { return Describe(f).name; }

// ---------------------------------------------------------------------

const TemperamentDef& Describe(Temperament t) {
  static const TemperamentDef kDefs[kTemperamentCount] = {
      {"The Purist",
       "Hard outdoor sends, done right. Disciplined, a little solitary.",
       {40.0, 10.0, -25.0, 55.0}},
      {"Send-or-Bust", "Throw yourself at it. Bold and impulsive.",
       {-30.0, 55.0, 10.0, -10.0}},
      {"The Lifer", "In it forever. Steady, consistent, low-key.",
       {50.0, -10.0, 0.0, 20.0}},
      {"The Influencer", "It is about the scene. Social, pragmatic.",
       {-10.0, 25.0, 55.0, -40.0}},
  };
  return kDefs[static_cast<int>(t)];
}

const char* TemperamentName(Temperament t) { return Describe(t).name; }

// ---------------------------------------------------------------------

const TalentDef& Describe(Talent t) {
  static const TalentDef kDefs[kTalentCount] = {
      {"", "", Skill::Power, true, 1.0, 1.0},  // None
      {"Natural Crimper",
       "Your fingers were built for small holds -- finger strength comes "
       "fast.",
       Skill::Fingers, true, 1.35, 1.0},
      {"Explosive", "Fast-twitch everything -- power piles on quick.",
       Skill::Power, true, 1.35, 1.0},
      {"Slow-Twitch Engine", "A bottomless tank -- endurance builds fast.",
       Skill::Endurance, true, 1.35, 1.0},
      {"Quiet Feet", "A natural mover -- technique clicks fast.",
       Skill::Technique, true, 1.35, 1.0},
      {"Ice in the Veins", "Unflappable -- your head game grows fast.",
       Skill::Head, true, 1.35, 1.0},
      {"Bomber Tendons", "Cabled tendons -- your fingers shrug off load.",
       Skill::Fingers, true, 1.10, 0.55},
      {"Glass Tendons",
       "Paper pulleys -- fingers come slow and tweak easy.",
       Skill::Fingers, false, 0.70, 1.70},
      {"No Pop", "Strength is a grind -- power comes slow.",
       Skill::Power, false, 0.70, 1.0},
      {"No Engine", "You pump out quick -- endurance comes slow.",
       Skill::Endurance, false, 0.70, 1.0},
      {"Stiff", "Movement fights you -- technique comes slow.",
       Skill::Technique, false, 0.70, 1.0},
      {"Skittish", "Fear runs hot -- head game comes slow.",
       Skill::Head, false, 0.70, 1.0},
  };
  return kDefs[static_cast<int>(t)];
}

const char* TalentName(Talent t) { return Describe(t).name; }

// ---------------------------------------------------------------------

void RollTalents(Character& c, const Rng& rng) {
  Rng r = rng.Derive("what-you-were-born-with");

  static const Talent kGifts[] = {
      Talent::NaturalCrimper, Talent::Explosive, Talent::SlowTwitchEngine,
      Talent::QuietFeet,      Talent::IceInTheVeins, Talent::BomberTendons};
  static const Talent kAnti[] = {Talent::GlassTendons, Talent::NoPop,
                                 Talent::NoEngine, Talent::Stiff,
                                 Talent::Skittish};

  c.gift = kGifts[r.IntRange(0, 5)];
  const Skill giftLane = Describe(c.gift).skill;

  // Never both on the same skill. Rolled and re-rolled rather than picked
  // from a filtered list, because a filtered list changes the odds of the
  // remaining anti-talents depending on which gift landed -- a climber with
  // good fingers would get No Pop more often than one with a good head,
  // for no reason anybody designed.
  for (int tries = 0; tries < 16; tries++) {
    const Talent candidate = kAnti[r.IntRange(0, 4)];
    if (Describe(candidate).skill != giftLane) {
      c.antiTalent = candidate;
      return;
    }
  }
  // Sixteen collisions in a row is impossible with one clashing option out
  // of five, but a loop with no exit is worse than a dull fallback.
  c.antiTalent =
      giftLane == Skill::Power ? Talent::NoEngine : Talent::NoPop;
}

Character MakeCharacter(const Build& build, const Rng& rng,
                        const CharacterDials& dials) {
  Character c;
  c.build = build;
  const OriginDef& o = Describe(build.origin);
  const TemperamentDef& t = Describe(build.temperament);

  // The temperament is what you picked; the origin leans it. Somebody who
  // sold a career to do this is disciplined before they choose to be.
  c.personality.discipline =
      std::clamp(t.p.discipline + o.discipline, -kPersonalityMax,
                 kPersonalityMax);
  c.personality.boldness = std::clamp(t.p.boldness + o.boldness,
                                      -kPersonalityMax, kPersonalityMax);
  c.personality.social =
      std::clamp(t.p.social + o.social, -kPersonalityMax, kPersonalityMax);
  c.personality.purism =
      std::clamp(t.p.purism + o.purism, -kPersonalityMax, kPersonalityMax);

  c.startingCash = o.cash;
  c.agePlus = o.agePlus;
  c.built = true;  // the only place this is ever set
  RollTalents(c, rng);
  (void)dials;
  return c;
}

Climber MakeClimber(const Build& build, const Rng& rng,
                    const CharacterDials& dials) {
  // The roll first, so the +/-6 of luck is the same for a given seed
  // whatever you built -- two players on the same world who pick
  // differently are comparing builds and not dice.
  Climber c = NewClimber(rng);

  const ArchetypeDef& a = Describe(build.archetype);
  const OriginDef& o = Describe(build.origin);
  const double k = dials.archetypeScale;

  c.skills.power += a.power * k + o.power;
  c.skills.fingers += a.fingers * k + o.fingers;
  c.skills.technique += a.technique * k + o.technique;
  c.skills.endurance += a.endurance * k + o.endurance;
  c.skills.head += a.head * k + o.head;

  // Nobody starts unable to pull on, and nobody starts at the top.
  const auto clampSkill = [](double& v) { v = std::clamp(v, 1.0, 99.0); };
  clampSkill(c.skills.power);
  clampSkill(c.skills.fingers);
  clampSkill(c.skills.technique);
  clampSkill(c.skills.endurance);
  clampSkill(c.skills.head);
  return c;
}

// ---------------------------------------------------------------------
// Effects
// ---------------------------------------------------------------------

double SkillGainMultiplier(const Character& c, Skill lane, bool indoor,
                           bool deliberateTraining, double psyche,
                           const CharacterDials& dials) {
  if (!c.built) return 1.0;
  double m = 1.0;

  // Talents are live whether or not you know about them. That is the whole
  // design: you find out you have good tendons by having had them.
  if (c.gift != Talent::None && Describe(c.gift).skill == lane) {
    m *= Describe(c.gift).gain;
  }
  if (c.antiTalent != Talent::None &&
      Describe(c.antiTalent).skill == lane) {
    m *= Describe(c.antiTalent).gain;
  }

  switch (c.build.flaw) {
    case Flaw::Gumby:
      if (lane == Skill::Technique) m *= dials.gumbyTechnique;
      break;
    case Flaw::HardGainer:
      if (lane == Skill::Power || lane == Skill::Fingers) {
        m *= dials.hardGainerPull;
      }
      break;
    case Flaw::FairWeather:
      // The only flaw that reads the day rather than the lane, and the only
      // one you can do something about -- which is why it is the interesting
      // one. Climb rested and it costs you nothing.
      if (psyche < dials.fairWeatherBelow) m *= dials.fairWeatherTired;
      break;
    default:
      break;
  }

  const OriginDef& o = Describe(c.build.origin);
  if (indoor) m *= o.indoorGain;
  if (deliberateTraining) m *= o.trainingGain;

  // Discipline is how much of a session actually lands. An impulsive
  // climber has the same sessions and keeps less of them.
  m *= 1.0 + dials.disciplineTraining * Axis(c.personality.discipline);
  return std::max(0.0, m);
}

double OddsPenalty(const Character& c, RouteType type,
                   const CharacterDials& dials) {
  if (!c.built) return 0.0;
  if (c.build.flaw == Flaw::HappyFeet && type == RouteType::Technical) {
    return dials.happyFeetOdds;
  }
  return 0.0;
}

double InjuryRiskMultiplier(const Character& c,
                            const CharacterDials& dials) {
  if (!c.built) return 1.0;
  double m = 1.0;
  if (c.gift != Talent::None) m *= Describe(c.gift).injury;
  if (c.antiTalent != Talent::None) m *= Describe(c.antiTalent).injury;
  if (c.build.flaw == Flaw::TweakyFingers) m *= dials.tweakyInjury;
  return m;
}

double ShiftPayMultiplier(const Character& c, const CharacterDials& dials) {
  if (!c.built) return 1.0;
  // Two lanes meet here and they are different things: the origin's perk is
  // a fact about your CV, and purism is a choice about what you will do for
  // money. A purist takes the worse-paid work that leaves the days free.
  const double purist =
      1.0 - dials.purismShiftPay * std::max(0.0, Axis(c.personality.purism));
  return Describe(c.build.origin).shiftPay * purist;
}

double DailyCostMultiplier(const Character& c) {
  if (!c.built) return 1.0;
  return Describe(c.build.origin).dailyCost;
}

double ShopPriceMultiplier(const Character& c) {
  if (!c.built) return 1.0;
  return Describe(c.build.origin).shopPrice;
}

double PhysioPriceMultiplier(const Character& c) {
  if (!c.built) return 1.0;
  return Describe(c.build.origin).physioPrice;
}

Talent WorkedOn(Character& c, Skill lane, double amount,
                const CharacterDials& dials) {
  if (!c.built) return Talent::None;
  const int i = static_cast<int>(lane);
  const double before = c.reps[i];
  c.reps[i] = before + std::max(0.0, amount);
  if (before >= dials.repsToSurface || c.reps[i] < dials.repsToSurface) {
    return Talent::None;
  }
  // This is the session it becomes obvious. At most one can surface here,
  // because a gift and an anti-talent are never in the same lane.
  if (c.gift != Talent::None && Describe(c.gift).skill == lane &&
      !c.giftKnown) {
    c.giftKnown = true;
    return c.gift;
  }
  if (c.antiTalent != Talent::None && Describe(c.antiTalent).skill == lane &&
      !c.antiKnown) {
    c.antiKnown = true;
    return c.antiTalent;
  }
  return Talent::None;
}

std::string TalentSurfaced(Talent t) {
  // Said the way somebody would notice it about themselves rather than the
  // way a stat screen would report it -- no numbers, no "unlocked", and the
  // lane named because that is what makes it land: you have been doing this
  // one thing for a season and it has started to be obvious.
  if (t == Talent::None) return std::string();
  const TalentDef& d = Describe(t);
  std::string s = "A season of ";
  s += SkillName(d.skill);
  s += " work, and it is starting to be obvious. ";
  s += d.name;
  s += ": ";
  s += d.desc;
  return s;
}

std::string WhoYouAre(const Character& c) {
  // Deliberately one sentence and deliberately not a stat line. The gate
  // for this phase is that a player can say who their climber is without
  // reading numbers; this is the game making the same attempt, and if it
  // reads like a form then the build is not legible enough yet.
  std::string s = OriginName(c.build.origin);
  s += " turned ";
  s += ArchetypeName(c.build.archetype);
  // Strip the article the archetype names carry, so it reads as a sentence.
  const std::string kThe = "The ";
  const std::size_t at = s.rfind(kThe);
  if (at != std::string::npos && at > 0) s.erase(at, kThe.size());
  s += ", ";
  s += TemperamentName(c.build.temperament);
  const std::size_t at2 = s.rfind(kThe);
  if (at2 != std::string::npos && at2 > 0) s.erase(at2, kThe.size());
  s += ", with ";
  s += FlawName(c.build.flaw);
  s += ".";
  if (c.giftKnown) {
    s += " You know by now that you are ";
    s += TalentName(c.gift);
    s += ".";
  }
  return s;
}

}  // namespace dirtbag
