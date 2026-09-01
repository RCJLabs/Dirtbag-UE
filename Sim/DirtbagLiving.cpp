#include "DirtbagLiving.h"

#include <algorithm>

namespace dirtbag {

const char* GrimeWord(double grime, const LivingDials& dials) {
  if (grime >= dials.feralAt) return "feral";
  if (grime >= dials.ripeAt) return "ripe";
  // Twenty-five is not a dial in the original either: it is the line
  // between fresh and normal, and normal is what everybody here is.
  if (grime >= 25.0) return "lived-in";
  return "fresh";
}

double GrimeSocial(double grime, const LivingDials& dials) {
  if (grime >= dials.feralAt) return dials.feralSocial;
  if (grime >= dials.ripeAt) return dials.ripeSocial;
  return 1.0;
}

void LivingNight(Living& living, bool cold, bool rained,
                 const LivingDials& dials) {
  living.grime += dials.grimeNight;
  if (rained) living.grime -= dials.grimeRainRinse;
  living.grime = std::max(0.0, std::min(100.0, living.grime));

  living.water = std::max(0.0, living.water - dials.waterNight);
  if (cold) {
    living.propane = std::max(0.0, living.propane - dials.propaneHeat);
  }
}

void ClimbedToday(Living& living, const LivingDials& dials) {
  living.grime = std::min(100.0, living.grime + dials.grimeClimb);
}

bool WashInTheVan(Living& living, const LivingDials& dials) {
  if (living.water < dials.waterWash) return false;
  // **You cannot get properly clean out of a jug**, and the floor is the
  // whole reason the truck stop is worth eight dollars.
  if (living.grime <= dials.jugFloor) return false;
  living.water -= dials.waterWash;
  living.grime = std::max(dials.jugFloor, living.grime - dials.jugWash);
  return true;
}

bool ShowerAtTheTruckStop(Living& living, double& cash, double& hours,
                          const LivingDials& dials) {
  if (cash < dials.showerCost) return false;
  cash -= dials.showerCost;
  hours += dials.showerHours;
  // The only thing in the game that gets you all the way to nothing.
  living.grime = 0.0;
  return true;
}

void SwimInTheLake(Living& living, double& hours, const LivingDials& dials) {
  hours += dials.swimHours;
  living.grime = std::max(0.0, living.grime - dials.swimRinse);
}

bool FillTheJugs(Living& living, double& cash, const LivingDials& dials) {
  if (cash < dials.waterJugCost) return false;
  if (living.water >= dials.waterCap) return false;
  cash -= dials.waterJugCost;
  living.water = std::min(dials.waterCap, living.water + dials.waterJug);
  return true;
}

bool SwapTheBottle(Living& living, double& cash, const LivingDials& dials) {
  if (cash < dials.propaneBottleCost) return false;
  if (living.propane >= dials.propaneCap) return false;
  cash -= dials.propaneBottleCost;
  living.propane = dials.propaneCap;
  return true;
}

bool CanCook(const Living& living, const LivingDials& dials) {
  return living.propane >= dials.propaneCook && living.water >= dials.waterCook;
}

void Cooked(Living& living, const LivingDials& dials) {
  living.propane = std::max(0.0, living.propane - dials.propaneCook);
  living.water = std::max(0.0, living.water - dials.waterCook);
}

std::string LivingLine(const Living& living, const LivingDials& dials) {
  // Loudest first, and only one of them: a van with four warnings on it is
  // a dashboard, and this is a line at the edge of a screen.
  if (living.grime >= dials.feralAt) {
    return "People are standing further away than they used to.";
  }
  if (living.water < dials.waterNight) return "The jugs are empty.";
  if (living.propane < dials.propaneCook) return "The bottle is done.";
  if (living.grime >= dials.ripeAt) {
    return "You are getting ripe. The scene notices.";
  }
  return "";
}

}  // namespace dirtbag
