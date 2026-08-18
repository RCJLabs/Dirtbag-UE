#include "DirtbagCrag.h"

#include <algorithm>
#include <string>

namespace dirtbag {

namespace {

// The guidebook for Roadside Crag, written down rather than rolled.
//
// The grade spread is deliberately not a ladder: a pile of moderates around
// V1-V4 because that is what most rock is, a thin band of hard classics, and
// a couple of things at the top that only exist because one strong visitor
// came through. Stars are spread across the range rather than tracking it,
// so that a V4 (Shade Line) is one of the five best lines here and a V5
// (Sloper Roulette) is filler — quality and difficulty are different axes,
// and a crag where they agreed would give the player nothing to choose.
//
// `trueGrade` above `grade` is a sandbag, and at a crag sandbags are famous
// and specific rather than random: Second Breakfast has been spitting people
// off for years and the book still says V2.
struct BookEntry {
  const char* name;
  int grade;
  int trueGrade;
  RouteType type;
  int stars;
};

const BookEntry kRoadside[] = {
    // The warmup boulder, right of the pull-off.
    {"Roadside Attraction",   0, 0, RouteType::Endurance,  1},
    {"Gravel Rash",           1, 1, RouteType::Technical,  0},
    {"Second Breakfast",      2, 3, RouteType::Crimp,      2},  // the famous one
    {"Cattle Guard",          1, 1, RouteType::Power,      1},
    {"Tick Marks",            2, 2, RouteType::Crimp,      1},

    // The main block. Where everyone actually spends the day.
    {"Diesel",                5, 5, RouteType::Power,      3},
    {"Shade Line",            4, 4, RouteType::Technical,  3},
    {"The Commute",           3, 3, RouteType::Endurance,  2},
    {"Dawn Patrol",           4, 5, RouteType::Crimp,      2},  // stiff for the grade
    {"Barn Door",             3, 3, RouteType::Technical,  1},
    {"Two Pads",              4, 4, RouteType::Dyno,       1},
    {"Spot Me",               5, 5, RouteType::Power,      2},
    {"The Undercling",        3, 3, RouteType::Crimp,      2},
    {"Chalk Ghost",           6, 6, RouteType::Crimp,      3},
    {"Sloper Roulette",       5, 5, RouteType::Technical,  1},

    // The back wall, in the shade, which is why it gets climbed in summer.
    {"Morning Shade",         2, 2, RouteType::Endurance,  1},
    {"Cold Fingers",          6, 6, RouteType::Crimp,      2},
    {"The Landing",           4, 4, RouteType::Dyno,       0},
    {"Pad People",            3, 4, RouteType::Power,      1},  // quietly stiff
    {"Rest Day Traverse",     2, 2, RouteType::Endurance,  2},

    // The hard end. Two visitors and a local account for all of it.
    {"The Guidebook Lied",    7, 7, RouteType::Crimp,      3},
    {"Nine to Five",          7, 7, RouteType::Power,      2},
    {"Highball Etiquette",    6, 6, RouteType::Technical,  2},
    {"Send Train",            8, 8, RouteType::Power,      3},
    {"Free Coffee",           5, 5, RouteType::Dyno,       1},
};
constexpr int kRoadsideCount =
    static_cast<int>(sizeof(kRoadside) / sizeof(kRoadside[0]));

// The open lines. No name, no confirmed grade — the book prints a guess and
// says where to find it. These are what the phase is for.
struct ProjectEntry {
  const char* description;
  int guess;         // the book's guess; a project's grade is an opinion
  RouteType type;
};

const ProjectEntry kProjects[] = {
    {"the arete left of Diesel",              7, RouteType::Power},
    {"the low traverse into Chalk Ghost",     8, RouteType::Endurance},
    {"the blank wall behind the parking",     9, RouteType::Crimp},
};
constexpr int kProjectCount =
    static_cast<int>(sizeof(kProjects) / sizeof(kProjects[0]));

}  // namespace

Crag RoadsideCrag(const Rng& worldRng) {
  Crag crag;
  crag.name = "Roadside Crag";
  // East-facing: sun on it all morning, into the shade mid-afternoon, and
  // climbable in the evening once the rock has given its heat back.
  crag.aspect = Aspect::East;
  crag.approachHours = 0.5;

  crag.lines.reserve(kRoadsideCount + kProjectCount);
  for (int i = 0; i < kRoadsideCount; i++) {
    const BookEntry& e = kRoadside[i];
    CragLine line;
    line.route = BuildRoute(worldRng, e.name, e.grade, e.trueGrade, e.type,
                            Discipline::Boulder);
    line.stars = e.stars;
    line.isProject = false;
    // Everything in the book has been done by somebody. Who, specifically,
    // is content the Lot will eventually supply.
    line.firstAscentBy = "unknown";
    crag.lines.push_back(line);
  }

  Rng projectRng = worldRng.Derive("crag-projects");
  for (int i = 0; i < kProjectCount; i++) {
    const ProjectEntry& e = kProjects[i];
    CragLine line;
    // The book's guess is a guess. Nobody has done the line, so nobody
    // actually knows what it is — and finding out is the payoff for doing
    // it. A guess is usually close and occasionally embarrassing in either
    // direction: the line you talked up as V9 goes at V8 and the locals are
    // kind about it, or it turns out to be the hardest thing here.
    const double roll = projectRng.NextDouble();
    const int drift = roll < 0.25 ? -1 : (roll < 0.6 ? 0 : (roll < 0.9 ? 1 : 2));
    const int trueGrade = std::max(0, e.guess + drift);

    // A project's moves are as real as anything else's — the rock does not
    // care that nobody has linked them. The name is the description until
    // somebody earns the right to change it.
    line.route = BuildRoute(worldRng, e.description, e.guess, trueGrade, e.type,
                            Discipline::Boulder);
    line.stars = 0;  // unclimbed lines have no stars; nobody can vouch yet
    line.isProject = true;
    line.description = e.description;
    crag.lines.push_back(line);
  }
  return crag;
}

std::vector<const CragLine*> LinesUpTo(const Crag& crag, int grade) {
  std::vector<const CragLine*> out;
  for (const CragLine& line : crag.lines) {
    if (!line.isProject && line.route.grade <= grade) out.push_back(&line);
  }
  return out;
}

std::vector<const CragLine*> OpenProjects(const Crag& crag) {
  std::vector<const CragLine*> out;
  for (const CragLine& line : crag.lines) {
    if (line.isProject) out.push_back(&line);
  }
  return out;
}

const std::string& DisplayName(const CragLine& line) {
  return line.displayName.empty() ? line.route.name : line.displayName;
}

std::string GuidebookLine(const CragLine& line) {
  if (line.isProject) {
    return "project, " + line.description + " — the book guesses " +
           BoulderGradeName(line.route.grade) + " and nobody has confirmed it";
  }
  std::string out = DisplayName(line);
  out += "  ";
  out += BoulderGradeName(line.route.grade);
  if (line.stars > 0) {
    out += "  ";
    for (int i = 0; i < line.stars; i++) out += "*";
  }
  return out;
}

}  // namespace dirtbag
