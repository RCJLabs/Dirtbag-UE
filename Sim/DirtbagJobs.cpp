#include "DirtbagJobs.h"

#include <algorithm>

namespace dirtbag {

namespace {

// The board, authored. A gig is a shape — hours, money, whether it needs the
// van — and the flavour is what makes choosing between them a decision
// rather than arithmetic.
struct Gig {
  const char* name;
  double hours;
  double pay;
  double energy;
  bool needsVan;
};

const Gig kGigs[] = {
    {"washing dishes at the diner",        5.0,  60.0, 20.0, false},
    {"a shift at the gear shop",           4.0,  55.0, 12.0, false},
    {"setting at the gym",                 5.0,  95.0, 35.0, false},
    {"moving somebody's furniture",        3.0,  65.0, 40.0, true},
    {"hauling firewood",                   4.0,  70.0, 45.0, true},
    {"trail work for the park",            6.0,  80.0, 38.0, true},
    {"flyering for the climbing festival", 3.0,  35.0,  8.0, false},
    {"shooting photos for the guidebook",  2.0, 130.0, 10.0, true},
    {"stacking shelves, night shift",      6.0,  85.0, 30.0, false},
    {"belaying kids' birthdays",           4.0,  50.0, 15.0, false},
};
constexpr int kGigCount = static_cast<int>(sizeof(kGigs) / sizeof(kGigs[0]));

}  // namespace

std::vector<OddJob> OddJobBoard(const Rng& worldRng, int day,
                                const JobDials& dials) {
  // Its own stream: what work is going must never shift how an attempt
  // resolves.
  Rng rng = worldRng.Derive("jobs#" + std::to_string(day));

  std::vector<OddJob> out;
  std::vector<int> taken;
  for (int i = 0; i < dials.boardSize; i++) {
    // Draw without repeats, so a day's board is three different things.
    int pick = 0;
    for (int tries = 0; tries < 12; tries++) {
      pick = static_cast<int>(rng.NextDouble() * kGigCount) % kGigCount;
      if (std::find(taken.begin(), taken.end(), pick) == taken.end()) break;
    }
    if (std::find(taken.begin(), taken.end(), pick) != taken.end()) continue;
    taken.push_back(pick);

    const Gig& g = kGigs[pick];
    OddJob j;
    j.name = g.name;
    j.hours = g.hours;
    // Pay wobbles a little, so the same gig is worth having some weeks and
    // not others.
    j.pay = g.pay * (0.85 + rng.NextDouble() * 0.3);
    j.energy = g.energy;
    j.needsVan = g.needsVan;
    out.push_back(j);
  }
  return out;
}

bool SalariedToday(const Job& job, int day, const JobDials& dials) {
  if (!job.salaried) return false;
  // Day 1 is a Monday. Five on, two off.
  const int weekday = (day - 1) % 7;
  return weekday < dials.salaryDaysPerWeek;
}

bool SalaryOwnsHour(const Job& job, int day, double hour,
                    const JobDials& dials) {
  if (!SalariedToday(job, day, dials)) return false;
  return hour >= dials.salaryStartHour &&
         hour < dials.salaryStartHour + dials.salaryHours;
}

double SalaryDayPay(const JobDials& dials) {
  return dials.salaryPerWeek / std::max(1, dials.salaryDaysPerWeek);
}

const char* JobText(const Job& job) {
  return job.salaried ? "employed, nine to five"
                      : "no job, and no boss";
}

}  // namespace dirtbag
