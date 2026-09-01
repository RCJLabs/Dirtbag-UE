#include "DirtbagStyle.h"

#include <algorithm>
#include <cmath>

namespace dirtbag {
namespace {

int Idx(RouteType t) { return static_cast<int>(t); }

// An even spread across six styles.
constexpr double kEvenShare = 1.0 / static_cast<double>(kRouteTypeCount);

}  // namespace

void Climbed(StyleLog& log, RouteType type, bool sent,
             const StyleDials& dials) {
  const int i = Idx(type);
  log.xp[i] += dials.perAttempt + (sent ? dials.perSend : 0.0);
  if (sent) log.sends[i] += 1.0;
}

double StyleVolume(const StyleLog& log) {
  double total = 0.0;
  for (int i = 0; i < kRouteTypeCount; i++) total += log.xp[i];
  return total;
}

bool HasAStyle(const StyleLog& log, const StyleDials& dials) {
  return StyleVolume(log) >= dials.minVolume;
}

double StyleDeviation(const StyleLog& log, RouteType type,
                      const StyleDials& dials) {
  const double total = StyleVolume(log);
  if (total < dials.minVolume) return 0.0;
  return log.xp[Idx(type)] / total - kEvenShare;
}

double AffinityOdds(const StyleLog& log, RouteType type,
                    const StyleDials& dials) {
  if (!HasAStyle(log, dials)) return 0.0;
  const double dev = StyleDeviation(log, type, dials);
  if (dev >= 0.0) {
    return std::min(dev * dials.specialtySlope, dials.specialtyCap);
  }
  return std::max(dev * dials.antiStyleSlope, -dials.antiStyleCap);
}

double StyleGainMultiplier(const StyleLog& log, RouteType type,
                           const StyleDials& dials) {
  if (!HasAStyle(log, dials)) return 1.0;
  const double dev = StyleDeviation(log, type, dials);
  if (dev >= 0.0) {
    // Fully specialised is climbing nothing else, which is a share of one.
    const double f = std::min(dev / (1.0 - kEvenShare), 1.0);
    return 1.0 - (1.0 - dials.gainOnSpecialty) * f;
  }
  // Fully neglected is a share of zero, which is a deviation of -kEvenShare.
  const double f = std::min(-dev / kEvenShare, 1.0);
  return 1.0 + (dials.gainOnAntiStyle - 1.0) * f;
}

const char* StyleTierName(const StyleLog& log, RouteType type,
                          const StyleDials& dials) {
  if (!HasAStyle(log, dials)) return "";
  const double a = AffinityOdds(log, type, dials);
  if (a >= dials.specialtyAt)  return "specialty";
  if (a >= dials.solidAt)      return "solid";
  if (a <= dials.antiStyleAt)  return "anti-style";
  if (a <= dials.weakAt)       return "weak";
  return "neutral";
}

RouteType YourStyle(const StyleLog& log, const StyleDials& dials) {
  (void)dials;
  int best = 0;
  for (int i = 1; i < kRouteTypeCount; i++) {
    if (log.xp[i] > log.xp[best]) best = i;
  }
  return static_cast<RouteType>(best);
}

RouteType YourAntiStyle(const StyleLog& log, const StyleDials& dials) {
  (void)dials;
  int worst = 0;
  for (int i = 1; i < kRouteTypeCount; i++) {
    if (log.xp[i] < log.xp[worst]) worst = i;
  }
  return static_cast<RouteType>(worst);
}

std::string StyleLine(const StyleLog& log, const StyleDials& dials) {
  if (!HasAStyle(log, dials)) return std::string();
  const RouteType best = YourStyle(log, dials);
  const RouteType worst = YourAntiStyle(log, dials);
  const std::string strong = StyleTierName(log, best, dials);
  const std::string weak = StyleTierName(log, worst, dials);
  // **A generalist is a real answer and gets its own sentence.** Naming a
  // best and a worst when neither is outside neutral would tell a
  // well-rounded climber they have a weakness they do not have.
  if (strong != "specialty" && strong != "solid" && weak != "anti-style" &&
      weak != "weak") {
    return "You climb a bit of everything, and none of it owes you anything.";
  }
  std::string line;
  if (strong == "specialty" || strong == "solid") {
    line = std::string("Known for ") + RouteTypeName(best) + ".";
  }
  if (weak == "anti-style" || weak == "weak") {
    if (!line.empty()) line += " ";
    line += std::string("Everybody has watched you on ") +
            RouteTypeName(worst) + ".";
  }
  return line;
}

bool AStyleWantsAName(const StyleLog& log, const Signature& already,
                      const Signature& second, RouteType& out,
                      const StyleDials& dials) {
  // The second one is the last one. Nothing past it.
  if (!second.name.empty()) return false;
  const bool first = already.name.empty();
  const double bar = first ? dials.signatureAt : dials.secondSignatureAt;

  int best = -1;
  for (int i = 0; i < kRouteTypeCount; i++) {
    // `CHAR-6b`: never the style you are already known for. Two signatures
    // in one style is not a second identity, it is the same one twice.
    if (!first && static_cast<RouteType>(i) == already.type) continue;
    if (log.sends[i] < bar) continue;
    if (best < 0 || log.sends[i] > log.sends[best]) best = i;
  }
  if (best < 0) return false;
  out = static_cast<RouteType>(best);
  return true;
}

double SignatureBonus(const Signature& first, const Signature& second,
                      RouteType type, const StyleDials& dials) {
  double bonus = 0.0;
  if (!first.name.empty() && first.type == type) bonus += dials.signatureBonus;
  if (!second.name.empty() && second.type == type) {
    bonus += dials.signatureBonus;
  }
  return bonus;
}

const char* SignatureKind(RouteType type) {
  switch (type) {
    case RouteType::Crimp:     return "crimp sequence";
    case RouteType::Power:     return "power move";
    case RouteType::Endurance: return "endurance burn";
    case RouteType::Technical: return "technical sequence";
    case RouteType::Dyno:      return "dyno";
    case RouteType::Crack:     return "crack pitch";
  }
  return "move";
}

std::string SignatureLine(const Signature& sig) {
  if (sig.name.empty()) return std::string();
  return "\"" + sig.name + "\" -- your " + SignatureKind(sig.type) +
         ". The scene knows you for it now.";
}

}  // namespace dirtbag
