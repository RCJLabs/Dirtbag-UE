#include "DirtbagEthics.h"

#include <algorithm>

namespace dirtbag {

const char* EthicalActName(EthicalAct act) {
  switch (act) {
    case EthicalAct::ChippedAHold: return "chipped a hold";
    case EthicalAct::RetroBolted:  return "retro-bolted somebody's line";
    case EthicalAct::ClaimedASend: return "claimed a send";
    case EthicalAct::StagedAPhoto: return "staged a photo";
    case EthicalAct::PulledOnGear: return "pulled on gear and called it clean";
  }
  return "something";
}

bool StripsTheAscent(EthicalAct act) {
  // Chipping and retro-bolting change the rock. The ascent stands — hollow,
  // and on a line that is not what it was, but it happened. The other three
  // are lies about what happened, and there is nothing left to stand.
  switch (act) {
    case EthicalAct::ChippedAHold:
    case EthicalAct::RetroBolted:
      return false;
    case EthicalAct::ClaimedASend:
    case EthicalAct::StagedAPhoto:
    case EthicalAct::PulledOnGear:
      return true;
  }
  return false;
}

Secret Commit(EthicalAct act, const std::string& routeKey, int day) {
  Secret s;
  s.act = act;
  s.routeKey = routeKey;
  s.dayDone = day;
  return s;
}

double VisibilityFrom(const Standing& standing, SponsorTier tier) {
  // Standing runs -1..1 and only the positive half is visibility: being
  // disliked by the Scene is not the same as being unknown to it, but it is
  // not what gets your old lines looked at either.
  const double scene = std::max(0.0, StandingWith(standing, Faction::Scene));
  const double sponsored = tier == SponsorTier::Title  ? 0.55
                           : tier == SponsorTier::Gear ? 0.3
                           : tier == SponsorTier::Shoes ? 0.1
                                                        : 0.0;
  return Clamp01(scene * 0.6 + sponsored);
}

int SomebodyFindsOut(std::vector<Secret>& secrets, const Rng& worldRng,
                     int day, double visibility, const EthicsDials& dials) {
  for (size_t i = 0; i < secrets.size(); i++) {
    Secret& s = secrets[i];
    if (s.known) continue;
    // A fresh one is safest: the people who were there have not compared
    // notes yet, and nobody is looking at a line that just went.
    if (day - s.dayDone < dials.quietDays) continue;

    const double chance =
        dials.baseDiscoveryPerDay *
        (1.0 + dials.visibilityMultiplier * Clamp01(visibility));

    // Its own stream, per secret: who talks must never move the weather or
    // how an attempt resolved.
    Rng rng = worldRng.Derive("ethics#" + std::to_string(day) + "#" +
                              std::to_string(i));
    if (rng.Chance(std::min(0.5, chance))) {
      s.known = true;
      s.dayFound = day;
      // One at a time. A career unravelling in a single afternoon is a
      // punishment; this is meant to be a story.
      return static_cast<int>(i);
    }
  }
  return -1;
}

void ItComesOut(const Secret& secret, Standing& standing, double& psyche,
                const EthicsDials& dials) {
  double cost = 0.0;
  switch (secret.act) {
    case EthicalAct::ChippedAHold: cost = dials.costChipped;     break;
    case EthicalAct::RetroBolted:  cost = dials.costRetroBolted; break;
    case EthicalAct::ClaimedASend: cost = dials.costClaimed;     break;
    case EthicalAct::StagedAPhoto: cost = dials.costStaged;      break;
    case EthicalAct::PulledOnGear: cost = dials.costPulledOn;    break;
  }

  // The old guard is the injured party — these are their ethics, and the
  // rock is theirs too. Shift handles the opposed-axis bleed, so this also
  // costs the Scene through the back door, on top of what it takes below.
  Shift(standing, Faction::OldGuard, -cost);

  // The Scene is not innocent, but it is not injured either: it punishes
  // being caught rather than the act, and less hard.
  Shift(standing, Faction::Scene, -cost * dials.sceneShareOfTheCost);

  // Chipping is a thing done to rock, so the stewards care about that one
  // specifically and about the others not at all.
  if (secret.act == EthicalAct::ChippedAHold ||
      secret.act == EthicalAct::RetroBolted) {
    Shift(standing, Faction::Stewardship, -cost * 0.5);
  }

  psyche = std::max(0.05, psyche - dials.psycheCost);
}

std::string EthicsText(const Secret& secret, int today) {
  if (!secret.known) return "";

  std::string out = "Everyone knows you ";
  out += EthicalActName(secret.act);
  if (!secret.routeKey.empty()) {
    out += " on " + secret.routeKey;
  }
  out += " now.";

  const int years = (today - secret.dayDone) / 365;
  if (years >= 1) {
    out += " It was " + std::to_string(years);
    out += years == 1 ? " year ago." : " years ago.";
    out += " It does not matter that it was.";
  }
  return out;
}

std::vector<const Secret*> Unknown(const std::vector<Secret>& secrets) {
  std::vector<const Secret*> out;
  for (const Secret& s : secrets) {
    if (!s.known) out.push_back(&s);
  }
  return out;
}

}  // namespace dirtbag
