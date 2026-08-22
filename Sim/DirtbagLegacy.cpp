#include "DirtbagLegacy.h"

#include <algorithm>

namespace dirtbag {

bool TimeToThinkAboutIt(const PlayerState& player, int consecutiveInjuries,
                        double peakGradeEver, const LegacyDials& dials) {
  const double age = AgeOn(player.day);
  const CareerSummary career = SummarizeCareer(player);

  // A body that keeps breaking says it before the numbers do.
  if (consecutiveInjuries >= dials.injuriesInARowToHint) return true;

  // Two grades off your best, sustained. Not age on its own — plenty of
  // people climb their hardest at forty, and telling one of them to pack it
  // in because of a birthday would be both wrong and insulting.
  const bool wellPast =
      peakGradeEver > 0.0 &&
      career.abilityGrade < peakGradeEver - dials.gradesOffPeakToHint;
  if (wellPast && age >= dials.retirementAgeHint) return true;

  return false;
}

Legacy TallyCareer(const PlayerState& player, const std::string& name,
                   int seasons, const LegacyDials& dials) {
  (void)dials;
  Legacy out;
  out.name = name;
  out.seasons = seasons;
  out.retiredAt = AgeOn(player.day);
  out.standing = player.standing;
  out.cragOpenAtTheEnd = CragIsOpen(player.standing);

  const CareerSummary career = SummarizeCareer(player);
  out.hardestSendGrade = career.hardestSendGrade;
  out.hardestSendName = career.hardestSendName;
  for (const ProjectMemory& m : player.projects) {
    if (m.sent && m.routeName == career.hardestSendName) {
      out.hardestSendDiscipline = m.discipline;
    }
  }
  out.totalSends = career.totalSends;
  out.totalAttempts = career.totalAttempts;
  out.nemesis = career.nemesis;
  out.nemesisAttempts = career.nemesisAttempts;
  out.dirtbagYears = player.job.dirtbagYears;
  out.longestDirtbagStreak = player.job.longestStreak;
  out.crewName = player.crew.name;
  out.dreams = player.dreams;

  for (const ProjectMemory& m : player.projects) {
    if (!m.firstAscent) continue;
    NamedLine line;
    // The ledger key, never the given name. Naming has never been allowed
    // to move the key and it must not start here — a guidebook that keys on
    // what somebody called a line loses the line the day it is renamed.
    line.routeKey = m.routeName;
    line.givenName = m.givenName;
    line.confirmedGrade = m.confirmedGrade;
    line.style = m.firstSendStyle;
    line.discipline = m.discipline;
    line.by = name;
    out.firstAscents.push_back(line);
  }
  return out;
}

PlayerState Inherit(const Legacy& previous, const LegacyDials& dials) {
  PlayerState next;

  // Nothing physical carries. The next climber is twenty-four with a fresh
  // body and nothing in the fingers, because inheriting somebody else's
  // tendons is nonsense and would make the second life a save-scum of the
  // first.
  //
  // "Nothing in the fingers" means *a beginner*, not *a zero*. This used to
  // read "skills stay at whatever a new PlayerState starts with", and what
  // a new PlayerState starts with is a zeroed struct — so every inherited
  // climber had 0 in all five, could not send a V0 (measured: 45 attempts
  // on Roadside Attraction, the warm-up), and could never be offered
  // retirement, because that test needs a peak above zero and they never
  // had one. They climbed to 85 and past it. See notes/phase4-career.md.
  next.climber = NewClimber();
  next.day = 1;
  next.cash = dials.inheritedCash;

  // The Lot knows whose van that is. Being somebody's kid brother cuts both
  // ways, which is why this is signed rather than a bonus — inherit a
  // career that annoyed the stewards and you arrive already annoying them.
  for (int i = 0; i < kFactionCount; i++) {
    next.standing.with[i] =
        previous.standing.with[i] * dials.inheritedStandingShare;
  }
  // A closure is not inherited: the signs come down between generations,
  // because a gate that stays shut forever is a dead crag rather than a
  // consequence. Already true of a fresh PlayerState — this is written down
  // so that the day somebody copies the whole Standing across, the decision
  // is here to be found rather than silently reversed.
  next.standing.closedDays = 0;

  // The van, and whatever was in the coffee tin. Deliberately not the gear:
  // a career that ended rich must not hand the next one a shortcut past the
  // part of this game that is about being broke.
  return next;
}

std::string GuidebookEntry(const NamedLine& line) {
  std::string out = line.givenName.empty() ? line.routeKey : line.givenName;
  if (line.confirmedGrade >= 0) {
    out += ", ";
    out += line.discipline == Discipline::Sport
               ? SportGradeName(line.confirmedGrade)
               : BoulderGradeName(line.confirmedGrade);
  }
  if (!line.by.empty()) {
    out += ". FA ";
    out += line.by;
  }
  // Style is the score, and the book has always said so.
  if (line.style == Style::Onsight) out += ", onsight";
  else if (line.style == Style::Flash) out += ", flash";
  return out;
}

std::string LegacyText(const Legacy& legacy) {
  std::string out;
  out += std::to_string(legacy.seasons);
  out += legacy.seasons == 1 ? " season. " : " seasons. ";

  if (legacy.hardestSendGrade >= 0) {
    out += "Hardest: " + legacy.hardestSendName + ", ";
    out += legacy.hardestSendDiscipline == Discipline::Sport
               ? SportGradeName(legacy.hardestSendGrade)
               : BoulderGradeName(legacy.hardestSendGrade);
    out += ". ";
  } else {
    out += "Never sent anything that stuck. ";
  }

  const size_t fas = legacy.firstAscents.size();
  if (fas == 1) {
    out += "One line that is yours now. ";
  } else if (fas > 1) {
    out += std::to_string(fas) + " lines that are yours now. ";
  }

  // Who they were, before what they did. A career that got called something
  // was a career somebody was watching.
  if (!legacy.crewName.empty()) {
    out += "They called them " + legacy.crewName + ". ";
  }

  // What the money was for. After the years, because the years are what
  // paid for it -- and silent for a career that never bought one, the same
  // as everything else here that would otherwise say zero.
  const std::string had = DreamText(legacy.dreams);
  if (!had.empty()) out += had + " ";

  // Years nobody owned. Said before the nemesis and the retirement, because
  // this is the part of the record that was a choice rather than a result --
  // every one of these was a season somebody offered you money to stop.
  if (legacy.dirtbagYears == 1) {
    out += "A Dirtbag Year. ";
  } else if (legacy.dirtbagYears > 1) {
    out += std::to_string(legacy.dirtbagYears) + " Dirtbag Years. ";
  }

  // The one that never went is part of the record. Every career has one and
  // leaving it out would be a kinder story and a less true one.
  if (!legacy.nemesis.empty() && legacy.nemesisAttempts > 20) {
    out += legacy.nemesis + " never went, after " +
           std::to_string(legacy.nemesisAttempts) + " tries. ";
  }

  out += "Retired at " + std::to_string(static_cast<int>(legacy.retiredAt));
  out += legacy.cragOpenAtTheEnd ? ", with the crag open."
                                 : ", with the gate still shut.";
  return out;
}

}  // namespace dirtbag
