#include "DirtbagFirstAscent.h"

#include <algorithm>

namespace dirtbag {

ProjectMemory NewProjectLedger(const CragLine& line,
                               const FirstAscentDials& dials) {
  ProjectMemory m;
  m.routeName = line.route.name;   // the permanent key, never the given name
  m.grade = line.route.grade;      // the book's guess, for now
  m.cleanliness = line.isProject ? dials.virginCleanliness : 1.0;
  return m;
}

double CleanLine(PlayerState& player, DayState& day, ProjectMemory& memory,
                 double hours, const FirstAscentDials& dials,
                 const DayDials& dayDials) {
  if (hours <= 0.0) return 0.0;

  const double before = memory.cleanliness;
  memory.cleanliness =
      std::min(1.0, memory.cleanliness + hours / dials.hoursToClean);

  // The day pays for it exactly as it pays for anything else — this is an
  // afternoon you are not climbing in, which is the whole cost.
  PassHours(day, hours, dayDials);
  day.energy = std::max(0.0, day.energy - dials.energyPerHour * hours);
  (void)player;
  return memory.cleanliness - before;
}

bool IsWorkable(const ProjectMemory& memory, const FirstAscentDials& dials) {
  return memory.cleanliness >= dials.workableCleanliness;
}

std::string CleanlinessText(const ProjectMemory& memory,
                            const FirstAscentDials& dials) {
  if (memory.cleanliness >= 0.98) return "clean";
  if (memory.cleanliness >= 0.8) return "brushed, a few holds still gritty";
  if (memory.cleanliness >= dials.workableCleanliness)
    return "workable — dirty, but the holds are there";
  if (memory.cleanliness >= 0.25)
    return "filthy; you can find the holds but not use them";
  return "untouched — moss, dirt, and a rumour of a line";
}

bool CanName(const CragLine& line, const ProjectMemory& memory) {
  // You can name it if it was nobody's, you did it, and you have not already
  // named it. The line's own name never enters into it.
  return line.isProject && line.firstAscentBy.empty() && memory.sent &&
         memory.givenName.empty();
}

bool NameFirstAscent(ProjectMemory& memory, const CragLine& line,
                     const std::string& name) {
  if (!CanName(line, memory)) return false;
  if (name.empty()) return false;

  memory.givenName = name;
  memory.firstAscent = true;
  // The payoff: until this moment every grade on this line was an opinion,
  // including the book's. Now there is a fact, and it is yours.
  memory.confirmedGrade = line.route.trueGrade;
  return true;
}

std::string FirstAscentLine(const ProjectMemory& memory,
                            const std::string& by) {
  if (!memory.firstAscent || memory.givenName.empty()) {
    return std::string();
  }
  std::string out = memory.givenName + "  " +
                    BoulderGradeName(memory.confirmedGrade) + "  FA " + by;
  // The book's guess is worth printing when it was wrong — that is the story
  // the crag will tell about the line from now on.
  if (memory.grade >= 0 && memory.confirmedGrade != memory.grade) {
    out += " (the book said ";
    out += BoulderGradeName(memory.grade);
    out += ")";
  }
  return out;
}

void WeatherProjects(PlayerState& player, const FirstAscentDials& dials) {
  for (ProjectMemory& m : player.projects) {
    // Only lines that were dirty to begin with go back to being dirty; a
    // popular route stays clean because everyone else is climbing it too.
    if (m.cleanliness < 1.0) {
      m.cleanliness = std::max(0.0, m.cleanliness - dials.dirtPerNight);
    }
  }
}

std::vector<const ProjectMemory*> FirstAscents(const PlayerState& player) {
  std::vector<const ProjectMemory*> out;
  for (const ProjectMemory& m : player.projects) {
    if (m.firstAscent) out.push_back(&m);
  }
  std::sort(out.begin(), out.end(),
            [](const ProjectMemory* a, const ProjectMemory* b) {
              if (a->confirmedGrade != b->confirmedGrade)
                return a->confirmedGrade > b->confirmedGrade;
              return a->routeName < b->routeName;
            });
  return out;
}

}  // namespace dirtbag
