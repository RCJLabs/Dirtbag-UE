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
#include "DirtbagCraft.h"
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

  // A Dirtbag Year: three hundred and sixty-five days in a row without
  // taking the job that owns your hours. Odd jobs do not break it and are
  // not supposed to -- the board is how a dirtbag eats, and a year spent
  // hauling trail and setting at the gym is the most dirtbag year there is.
  // What breaks it is signing for the nine-to-five.
  //
  // 365 rather than a season because the point is that you got through a
  // *winter* on it. A season-length version would be handed to anyone who
  // arrives in spring and leaves before the money runs out, which is a
  // holiday.
  int dirtbagYearDays = 365;
};

struct OddJob {
  std::string name;
  double hours = 4.0;
  double pay = 60.0;
  double energy = 25.0;
  bool needsVan = false;   // no van, no job
};

// **A sacking has to show up as work you cannot take**, or it is a number
// in a menu. The rule itself is `WillTheyHireYou` in Sim/DirtbagCraft.h and
// it lives in exactly one place; filtering a board with it is a two-line
// loop that each caller writes, because a convenience wrapper around a
// one-line rule was written here first, used by nothing but its own test,
// and deleted -- a second place for a rule to live is how the rule starts
// disagreeing with itself.
//
// `WorkOddJob` enforces it too, so a caller that forgets the filter shows
// the gig and cannot take it, rather than walking past a sacking.

// What is on the board today. Deterministic per world and day, on its own
// named stream.
std::vector<OddJob> OddJobBoard(const Rng& worldRng, int day,
                                const JobDials& dials = JobDials{});

// Employment, such as it is.
struct Job {
  bool salaried = false;
  int daysWorked = 0;      // lifetime, for the career line
  int weeksSalaried = 0;

  // The counterweight to the salaried trap. The trap costs you the hours
  // the day was for and pays in money -- and until now the only thing
  // refusing it bought was the absence of a cost, which is not something a
  // player can feel. This is the thing you have instead.
  int daysSinceSalary = 0;      // the streak running now
  int dirtbagYears = 0;         // how many whole ones you have banked
  int longestStreak = 0;        // in days, including the one in progress
};

// A day passes without the nine-to-five. Returns true on the day a year
// completes -- the caller's cue to say so, once.
bool DirtbagDay(Job& job, const JobDials& dials = JobDials{});

// You signed. Whatever the streak was, it is over -- not paused. Returns
// what it cost you, in days, so the game can be honest about it at the
// moment of signing rather than in a summary nobody reads.
int BreakTheStreak(Job& job);

// "Two Dirtbag Years. 118 days into a third." Empty until there is
// something to say, which is the first whole year -- a streak of eleven
// days is not an achievement, it is a fortnight.
std::string DirtbagYearText(const Job& job, const JobDials& dials = JobDials{});

// Is today one of the days the salary owns? Monday to Friday, counting from
// day 1, because the rock does not care what day it is and the job does.
bool SalariedToday(const Job& job, int day, const JobDials& dials = JobDials{});

// Does the salary have you during this hour?
//
// unwired-ok: unreachable by construction rather than by omission. The
// engine works the salaried day at dawn, which fast-forwards the clock to
// the far side of the shift, so by the time anybody could ask this the
// answer is always no. Kept because a game that lets you *start* a day
// before work -- rather than waking on the far side of it -- would need it
// back, and that is a real design option rather than a settled one.
bool SalaryOwnsHour(const Job& job, int day, double hour,
                    const JobDials& dials = JobDials{});

// What a week of it pays.
double SalaryDayPay(const JobDials& dials = JobDials{});

// unwired-ok: a formatter, and the board draws its own rows
const char* JobText(const Job& job);

}  // namespace dirtbag
