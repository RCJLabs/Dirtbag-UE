#include "DirtbagSpeed.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace dirtbag {
namespace {

// Two decimals, because a speed time is read to two decimals and nothing
// else in this file needs a formatter.
std::string Secs(double s) {
  std::ostringstream out;
  const double r = std::round(s * 100.0) / 100.0;
  const long whole = static_cast<long>(r);
  const long cents = static_cast<long>(std::round((r - whole) * 100.0));
  out << whole << "." << (cents < 10 ? "0" : "") << cents << "s";
  return out.str();
}

}  // namespace

double SpeedTimeForGrade(double grade, const SpeedDials& dials) {
  const double t = dials.baseSeconds - grade * dials.perGrade;
  return std::min(dials.baseSeconds, std::max(dials.floorSeconds, t));
}

double SpeedPaceMs(double grade, const SpeedDials& dials) {
  const double p = dials.paceBaseMs - grade * dials.pacePerGrade;
  return std::min(dials.paceCeilingMs, std::max(dials.paceFloorMs, p));
}

double SpeedScore(double seconds, const SpeedDials& dials) {
  return std::max(0.0, dials.baseSeconds - seconds) * dials.pointsPerSecond;
}

double RunTime(double grade, const SpeedRun& run, const SpeedDials& dials) {
  if (run.falseStart) return dials.dnfSeconds;

  // The start. Quicker than the neutral reaction shaves time, slower adds
  // it, both capped -- see reactionCapSeconds for why the cap is not
  // optional.
  const double reactionDelta =
      (run.reactionMs - dials.reactionOkMs) / dials.reactionScaleMs;
  const double fromStart =
      std::min(dials.reactionCapSeconds,
               std::max(-dials.reactionCapSeconds, reactionDelta));

  // The cadence. Only error **past the window** costs anything: sixteen
  // reaches inside the window is a clean run, and a run of tiny errors that
  // each sat inside it is also a clean run, which is the whole reason the
  // window exists.
  const double slack = dials.beatWindowMs * dials.rungs;
  const double past = std::max(0.0, run.offBeatMs - slack) / 1000.0;

  const double t = SpeedTimeForGrade(grade, dials) + fromStart +
                   past * dials.offBeatPenalty +
                   run.fumbles * dials.fumbleSeconds;
  // The floor is the wall record and it holds against every input: no
  // reaction, no cadence and no grade combination gets under it.
  return std::min(dials.dnfSeconds, std::max(dials.floorSeconds, t));
}

SpeedRun BotRun(const Rng& worldRng, const std::string& salt, double grade,
                double nerve, const SpeedDials& dials) {
  (void)grade;
  Rng rng = worldRng.Derive("speedrun|" + salt);
  SpeedRun run;
  const double steady = std::min(1.0, std::max(0.0, nerve));

  const double falseChance =
      std::max(0.0, dials.botFalseStartBase - steady * dials.nerveFalseStart);
  if (rng.Chance(falseChance)) {
    run.falseStart = true;
    return run;
  }

  // Nerve is worth a faster start and nothing else. It does not make you
  // climb the wall better -- everybody in the lane has the wall memorised;
  // what a crowd takes from you is the tenth of a second at the buzzer.
  run.reactionMs = dials.reactionOkMs - steady * dials.nerveReactionMs +
                   (rng.NextDouble() - 0.5) * dials.botReactionSpreadMs;
  run.reactionMs = std::max(80.0, run.reactionMs);

  run.offBeatMs = rng.NextDouble() * dials.botOffBeatPerRung * dials.rungs;
  for (int i = 0; i < dials.rungs; ++i) {
    if (rng.Chance(dials.botFumbleChance)) run.fumbles++;
  }
  return run;
}

double FieldTime(const Rng& worldRng, const std::string& salt, double grade,
                 bool heat, const SpeedDials& dials) {
  Rng rng = worldRng.Derive("speedfield|" + salt);
  if (heat && rng.Chance(dials.opponentFalseStart)) return dials.dnfSeconds;
  const double spread =
      heat ? dials.heatSpreadSeconds : dials.fieldSpreadSeconds;
  const double t = SpeedTimeForGrade(grade, dials) +
                   (rng.NextDouble() - 0.5) * spread;
  return std::min(dials.baseSeconds, std::max(dials.floorSeconds, t));
}

void LogRun(SpeedRound& round, double seconds, const SpeedDials& dials) {
  if (RoundIsDone(round, dials)) return;
  round.runs.push_back(seconds);
  // The fastest counts, and a round with nothing but DNFs in it still has a
  // best -- it is just a DNF, which scores zero rather than being absent.
  round.best = round.runs[0];
  for (double t : round.runs) round.best = std::min(round.best, t);
  round.score = SpeedScore(round.best, dials);
}

bool RoundIsDone(const SpeedRound& round, const SpeedDials& dials) {
  return static_cast<int>(round.runs.size()) >= dials.runsPerRound;
}

const char* HeatName(int round, bool bronze) {
  if (round <= 0) return "Quarterfinal";
  if (round == 1) return "Semifinal";
  return bronze ? "Bronze match" : "Final";
}

namespace {

// Who you draw. Deeper in the bracket is a harder opponent, which is what
// seeding means: the field arrives sorted, so an index into it is a
// difficulty. The bronze match is the one exception -- you are racing
// somebody who also just lost a semi, so they are drawn from the middle
// rather than the top.
SpeedEntrant DrawOpponent(const std::vector<SpeedEntrant>& field, int round,
                          bool bronze, double fallbackGrade) {
  if (field.empty()) return SpeedEntrant{"a finalist", fallbackGrade};
  const int last = static_cast<int>(field.size()) - 1;
  int idx = 4;
  if (bronze)         idx = 3;
  else if (round == 1) idx = 2;
  else if (round >= 2) idx = 0;
  return field[std::min(last, std::max(0, idx))];
}

std::string HeatSalt(int day, int round, bool bronze) {
  return "speedheat|" + std::to_string(day) + "|" + std::to_string(round) +
         "|" + (bronze ? "b" : "m");
}

}  // namespace

SpeedBracket SeedTheBracket(const Rng& rng, int day,
                            const std::vector<SpeedEntrant>& qualifiers,
                            double yourGrade, const SpeedDials& dials) {
  SpeedBracket b;
  b.field = qualifiers;
  if (static_cast<int>(b.field.size()) > dials.bracketSize) {
    b.field.resize(static_cast<size_t>(dials.bracketSize));
  }
  b.opponent = DrawOpponent(b.field, 0, false, yourGrade);
  const std::string salt = HeatSalt(day, 0, false);
  b.opponentSeconds = FieldTime(rng, salt, b.opponent.grade, true, dials);
  b.opponentFalseStarted = b.opponentSeconds >= dials.dnfSeconds;
  return b;
}

void ResolveHeat(SpeedBracket& bracket, double yourSeconds,
                 const SpeedDials& dials) {
  if (bracket.done || bracket.resolved) return;

  bracket.yourSeconds = yourSeconds;
  const bool youFalse = yourSeconds >= dials.dnfSeconds;
  // A false start loses the heat outright. If you both blow it the clock
  // still decides, which is why a DNF is a time and not a flag: two DNFs
  // are equal and the tie goes to the opponent, the same way a dead heat
  // does everywhere else in this game.
  const bool won = youFalse ? false : yourSeconds < bracket.opponentSeconds;

  bracket.youWonIt = won;
  bracket.resolved = true;
  bracket.log.push_back(SpeedHeatLog{
      HeatName(bracket.round, bracket.bronze), bracket.opponent.name,
      yourSeconds, bracket.opponentSeconds, won,
      bracket.opponentFalseStarted});

  if (won) {
    if (bracket.round >= 2) {
      bracket.placement = bracket.bronze ? 3 : 1;
      bracket.done = true;
    }
  } else if (bracket.round <= 0) {
    // Out at the quarter. Five is the honest number: four people went
    // further and the other three quarter-final losers are level with you.
    bracket.placement = 5;
    bracket.done = true;
  } else if (bracket.round >= 2) {
    bracket.placement = bracket.bronze ? 4 : 2;
    bracket.done = true;
  }
}

void NextHeat(SpeedBracket& bracket, const Rng& rng, int day, double yourGrade,
              const SpeedDials& dials) {
  if (bracket.done || !bracket.resolved) return;
  // A win advances a round; a semifinal loss does not end the day, it drops
  // you into the bronze match -- which is round two either way.
  bracket.bronze = !bracket.youWonIt;
  bracket.round = bracket.youWonIt ? bracket.round + 1 : 2;
  bracket.opponent =
      DrawOpponent(bracket.field, bracket.round, bracket.bronze, yourGrade);
  const std::string salt = HeatSalt(day, bracket.round, bracket.bronze);
  bracket.opponentSeconds =
      FieldTime(rng, salt, bracket.opponent.grade, true, dials);
  bracket.opponentFalseStarted = bracket.opponentSeconds >= dials.dnfSeconds;
  bracket.yourSeconds = 0.0;
  bracket.resolved = false;
  bracket.youWonIt = false;
}

std::string HeatLine(const SpeedBracket& bracket) {
  if (!bracket.resolved || bracket.log.empty()) return std::string();
  const SpeedHeatLog& h = bracket.log.back();
  if (h.yourSeconds >= SpeedDials{}.dnfSeconds) {
    return "You went on the amber. " + h.opponent + " takes the heat.";
  }
  if (h.youWon) {
    return "You beat " + h.opponent + ", " + Secs(h.yourSeconds) + ".";
  }
  if (h.theyFalseStarted) {
    return h.opponent + " false-started and you still could not use it.";
  }
  return h.opponent + " edges you, " + Secs(h.theirSeconds) + ".";
}

SpeedPracticeResult PracticeRun(Skills& skills, double& energy,
                                double& personalBest, double seconds,
                                double trainingCeiling,
                                const SpeedDials& dials) {
  SpeedPracticeResult out;
  if (energy < dials.practiceEnergy) return out;

  out.ran = true;
  energy = std::max(0.0, energy - dials.practiceEnergy);
  out.seconds = seconds;
  out.clean = seconds < dials.dnfSeconds;
  if (!out.clean) return out;

  // The same taper every trained skill in this game uses -- a speed lap is
  // training and gets no exemption from the ceiling.
  const double powerRoom =
      std::max(0.15, 1.0 - skills.power / trainingCeiling);
  const double techRoom =
      std::max(0.15, 1.0 - skills.technique / trainingCeiling);
  out.powerGained = dials.practicePowerGain * powerRoom;
  out.techniqueGained =
      dials.practicePowerGain * dials.practiceTechniqueShare * techRoom;
  skills.power = std::min(100.0, skills.power + out.powerGained);
  skills.technique = std::min(100.0, skills.technique + out.techniqueGained);

  out.personalBest = personalBest <= 0.0 || seconds < personalBest;
  if (out.personalBest) personalBest = seconds;
  return out;
}

std::string PersonalBestLine(double personalBest) {
  if (personalBest <= 0.0) return std::string();
  return "Personal best " + Secs(personalBest) + ".";
}

}  // namespace dirtbag
