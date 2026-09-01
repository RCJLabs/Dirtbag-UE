#include "DirtbagFactions.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {

const char* FactionName(Faction f) {
  switch (f) {
    case Faction::OldGuard:    return "the old guard";
    case Faction::Scene:       return "the scene";
    case Faction::Development: return "the development crew";
    case Faction::Stewardship: return "the stewards";
  }
  return "nobody";
}

Faction OppositeOf(Faction f) {
  switch (f) {
    case Faction::OldGuard:    return Faction::Scene;
    case Faction::Scene:       return Faction::OldGuard;
    case Faction::Development: return Faction::Stewardship;
    case Faction::Stewardship: return Faction::Development;
  }
  return f;
}

double StandingWith(const Standing& s, Faction f) {
  return s.with[static_cast<int>(f)];
}

namespace {
// Standing lives in -1..1, and there is no clearer way to say that than to
// say it.
double ClampStanding(double v) { return std::max(-1.0, std::min(1.0, v)); }
}  // namespace

void Shift(Standing& s, Faction f, double amount, const FactionDials& dials) {
  if (amount == 0.0) return;
  const int i = static_cast<int>(f);
  const int o = static_cast<int>(OppositeOf(f));
  s.with[i] = ClampStanding(s.with[i] + amount);
  // Opposed means opposed: what earns you one earns against the other.
  s.with[o] = ClampStanding(s.with[o] - amount * dials.oppositionBleed);
}

void DidFirstAscent(Standing& s, bool goodStyle, const FactionDials& dials) {
  // A new line is a new line, whoever you are.
  Shift(s, Faction::Development, 0.18, dials);
  // Doing it in good style is what the old guard actually care about, and
  // is the one act that pleases both of the axis's ends.
  if (goodStyle) Shift(s, Faction::OldGuard, 0.15, dials);
}

void ScrubbedALine(Standing& s, const FactionDials& dials) {
  // Cleaning opens terrain and takes a wire brush to a hillside. Both true.
  Shift(s, Faction::Development, 0.06, dials);
  Shift(s, Faction::Stewardship, -0.05, dials);
}

void TookTheGuidebookPhotos(Standing& s, const FactionDials& dials) {
  // The best-paying gig on the board, and the reason it pays that well.
  Shift(s, Faction::Scene, 0.16, dials);
  Shift(s, Faction::Stewardship, -0.12, dials);
}

void DidTrailWork(Standing& s, const FactionDials& dials) {
  Shift(s, Faction::Stewardship, 0.14, dials);
}

void SetAtTheGym(Standing& s, const FactionDials& dials) {
  Shift(s, Faction::Scene, 0.07, dials);
}

void FactionDay(Standing& s, const Rng& worldRng, int day,
                const FactionDials& dials) {
  // Memory fades, slowly and toward nothing rather than toward liking you.
  for (int i = 0; i < kFactionCount; i++) {
    if (s.with[i] > 0.0) {
      s.with[i] = std::max(0.0, s.with[i] - dials.driftPerDay);
    } else {
      s.with[i] = std::min(0.0, s.with[i] + dials.driftPerDay);
    }
  }

  if (s.closedDays > 0) {
    s.closedDays--;
    return;
  }

  // Access is not pulled out of spite. It is pulled when the people who
  // hold it have had enough, and you were part of that.
  if (StandingWith(s, Faction::Stewardship) < dials.closureBelow) {
    Rng rng = worldRng.Derive("access#" + std::to_string(day));
    if (rng.Chance(dials.closureChancePerDay)) {
      s.closedDays = dials.closureDays;
    }
  }
}

bool CragIsOpen(const Standing& s) { return s.closedDays <= 0; }

Faction FactionOf(const std::string& partnerName) {
  // The Lot's people speak for somebody, which is what makes standing a
  // thing you feel in a conversation rather than read on a screen.
  if (partnerName == "Margo") return Faction::OldGuard;   // twenty seasons
  if (partnerName == "Dev") return Faction::Scene;        // young and strong
  if (partnerName == "Trish") return Faction::Development;
  if (partnerName == "Ray") return Faction::Stewardship;  // put up half of it
  return Faction::Development;                            // Bo, and anyone new
}

double BetaMultiplierFor(const Standing& s, const std::string& partnerName,
                         const FactionDials& dials) {
  const double standing = StandingWith(s, FactionOf(partnerName));
  return 1.0 + dials.betaBonusAtFullStanding * standing;
}

std::string StandingText(const Standing& s) {
  if (!CragIsOpen(s)) {
    return "the crag is closed. The signs went up on the gate.";
  }
  // Say the loudest thing, in either direction, and say nothing when there
  // is nothing to say.
  int loudest = -1;
  double best = 0.3;
  for (int i = 0; i < kFactionCount; i++) {
    if (std::fabs(s.with[i]) > best) {
      best = std::fabs(s.with[i]);
      loudest = i;
    }
  }
  if (loudest < 0) return std::string();

  const std::string who = FactionName(static_cast<Faction>(loudest));
  if (s.with[loudest] > 0.0) {
    return s.with[loudest] > 0.7 ? "you are one of " + who
                                 : who + " have time for you";
  }
  return s.with[loudest] < -0.7 ? who + " would rather you left"
                                : who + " have gone quiet on you";
}

}  // namespace dirtbag
