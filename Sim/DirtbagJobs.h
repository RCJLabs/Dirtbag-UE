// Work: the odd-jobs board, and the trap at the end of it.
//
// The expense side of Phase 3 is built — gear wears, the van breaks, bills
// you cannot pay become debt. This is the other half, and the reason the
// phase's gate is about *pressure* rather than about money: the way a
// climbing life goes wrong is not that you run out of cash, it is that the
// obvious fix for running out of cash is a job, and a job is made of the
// hours you were going to climb in.
//
// Two shapes of work, deliberately different:
//
//   odd jobs   a few hours, cash today, gone tomorrow. Some need the van,
//              which quietly means a breakdown costs you the fix AND the
//              work that would have paid for it.
//
//   the salary reliable, generous, and it owns your week. Nine to five,
//              five days, which is precisely the hours the rock is in
//              condition. It solves money completely and costs you the
//              thing money was for.
//
// The trap is not that the salary is a bad deal. It is that it is a good
// one, and taking it is entirely reasonable, and a season later you are
// solvent and climbing V4. Nothing in here punishes it — the numbers simply
// are what they are, and `notes/phase3-jobs.md` measures what they do.
//
// Engine-free like everything in Sim/.

#pragma once

#include <string>
#include <vector>

#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

struct JobDials {
  // How many gigs are on the board on a given day. Some days there is
  // nothing worth having.
  int boardSize = 3;

  // The salaried job. Nine to five, five days a week — the hours are the
  // point, not the money.
  double salaryPerWeek = 520.0;
  int salaryDaysPerWeek = 5;
  double salaryStartHour = 9.0;
  double salaryHours = 8.0;
  double salaryEnergy = 45.0;

  // You do not walk into it off the street, and you cannot walk out of it
  // on a whim without burning something.
  double salaryQuitPsycheCost = 0.1;
};

struct OddJob {
  std::string name;
  double hours = 4.0;
  double pay = 60.0;
  double energy = 25.0;
  bool needsVan = false;   // no van, no job
};

// What is on the board today. Deterministic per world and day, on its own
// named stream.
std::vector<OddJob> OddJobBoard(const Rng& worldRng, int day,
                                const JobDials& dials = JobDials{});

// Employment, such as it is.
struct Job {
  bool salaried = false;
  int daysWorked = 0;      // lifetime, for the career line
  int weeksSalaried = 0;
};

// Is today one of the days the salary owns? Monday to Friday, counting from
// day 1, because the rock does not care what day it is and the job does.
bool SalariedToday(const Job& job, int day, const JobDials& dials = JobDials{});

// Does the salary have you during this hour?
bool SalaryOwnsHour(const Job& job, int day, double hour,
                    const JobDials& dials = JobDials{});

// What a week of it pays.
double SalaryDayPay(const JobDials& dials = JobDials{});

const char* JobText(const Job& job);

}  // namespace dirtbag
