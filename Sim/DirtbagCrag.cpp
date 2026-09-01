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
//
// Not all of them are hard. A crag's unclimbed lines are the ones nobody
// got round to, and the reason is as often a bad landing or an ugly piece of
// rock as it is difficulty — which matters here beyond flavour: a starting
// climber has to be able to reach a first ascent of their own, and three
// V7-and-up projects put the whole point of the phase behind a season of
// training. The moderates are the way in; the hard ones are what you come
// back for.
struct ProjectEntry {
  const char* description;
  int guess;         // the book's guess; a project's grade is an opinion
  RouteType type;
  // How well the guess is actually informed, which is not the same question
  // as how hard the line is. The cave's projects are bolted, so somebody has
  // already hung on them and the drift there is tighter for the same reason.
  // A line at the back of a crag that nobody has touched is a real guess; a
  // sit start to a classic that everybody has pulled on is close to known.
  bool wellKnown = false;
};

const ProjectEntry kProjects[] = {
    // Nobody's bothered. The landing is a boulder field and the line is
    // plain, so it has sat there unclimbed next to a car park for years.
    {"the slab right of the pull-off",        3, RouteType::Technical},
    // Loose-looking, and everyone assumes somebody has done it.
    {"the short wall behind the cattle grid", 5, RouteType::Crimp},
    // And then the ones that are unclimbed for the obvious reason.
    {"the arete left of Diesel",              7, RouteType::Power},
    {"the low traverse into Chalk Ghost",     8, RouteType::Endurance},
    {"the blank wall behind the parking",     9, RouteType::Crimp},
    // And the one in between, which is a third reason entirely. Shade Line
    // is V4 and one of the best things here, so its sit start has been
    // brushed by everybody and done by nobody: too hard for the locals who
    // love the crag, too soft to interest the visitor who came for Send
    // Train. That is what a project at the top of a normal career looks
    // like — not a frontier line, just the good one you have to become
    // strong enough to deserve.
    //
    // It is here because the guidebook had a hole in it. Roadside's other
    // projects come out around V3, V4, V8, V8 and V10, and a career peaks
    // at V6.6 on average (notes/phase4-the-lot.md): the player cleared the
    // two moderates in their first season and then had thirty years with
    // nothing to aim at. Technical rather than Power on purpose — across a
    // measured career power collapses to 16.8 while technique ends *higher*
    // than it started, so this is a line you can still come back for at
    // fifty, which is the more interesting promise.
    //
    // Appended rather than slotted into grade order: the drift below draws
    // from a stateful stream, so inserting it mid-list would re-roll every
    // project after it and change the difficulty of lines players are
    // already part-way through.
    {"the sit start to Shade Line",           6, RouteType::Technical, true},
};
constexpr int kProjectCount =
    static_cast<int>(sizeof(kProjects) / sizeof(kProjects[0]));


// --- The Sun Terrace ---------------------------------------------------------
//
// The winter crag, and the measurement is what made it south-facing rather
// than the folklore. Sweeping all four aspects across a year (see
// notes/phase4-crag3.md): in **high summer North, South and West are
// literally identical** — 25 days, 0.880 friction, a 1.34h window at 5:30am
// — because the summer window lands before the sun is on any face at all.
// Aspect only does anything in **winter**, and there it does a lot: South
// gets **42 days against North's 28**, at the price of a window less than
// half as long (1.30h against 3.06h) landing at about **1:45pm**.
//
// That last number is the crag's identity, stated carefully. A winter
// window at 1:45pm is a window a nine-to-five sits on top of — but not
// exclusively, and the tempting version of this claim is false: a salaried
// climber reaches 22 of the terrace's 63 windows over the first ninety days
// against 13 of the cave's 48, so this is not the crag you *cannot* climb
// around a job.
//
// It is the crag where a job costs you the most. 41 windows lost against
// the cave's 35 — the most good days in the valley a nine-to-five takes off
// you — because it has the most winter days to lose and they land in the
// middle of them.
//
// So: fewer lines than Roadside, harder, and sparse. It is not somewhere you
// go instead of Roadside; it is somewhere you go in January, or when
// Roadside has stopped being hard enough, and it asks what your job is
// worth to you.
const BookEntry kTerrace[] = {
    // The approach boulders, done cold with numb fingers before the sun
    // comes round.
    {"Frozen Fingers",        3, 3, RouteType::Crimp,      1},
    {"Numb",                  4, 4, RouteType::Technical,  1},

    // The terrace proper. South-facing granite, and in condition for about
    // ninety minutes in the middle of a January day.
    {"One O'Clock Sun",       5, 5, RouteType::Crimp,      3},   // the classic
    {"Thermals Off",          6, 6, RouteType::Power,      2},
    {"The Sit Start",         6, 7, RouteType::Power,      1},   // stiff for the grade
    {"Chalk On Ice",          7, 7, RouteType::Technical,  2},
    {"Sending Temps",         8, 8, RouteType::Crimp,      3},
    {"Day Off Work",          8, 8, RouteType::Endurance,  2},

    // The high boulders at the back, which nobody gets to in a lunch hour.
    {"Short Days",            9, 9, RouteType::Power,      2},
    {"The Last Hour",        10, 10, RouteType::Crimp,     3},
    {"Headtorch Walk-Out",   11, 11, RouteType::Dyno,      2},
};
constexpr int kTerraceCount =
    static_cast<int>(sizeof(kTerrace) / sizeof(kTerrace[0]));

// Two, and both hard. A crag this far up the hill does not have easy
// unclimbed lines lying around — anything soft was done years ago by
// somebody with a lunch break.
const ProjectEntry kTerraceProjects[] = {
    // The obvious one, and the reason people come up here with a camera.
    {"the prow above the terrace",            10, RouteType::Power},
    // Everyone has tried the first move. Nobody has done the second.
    {"the two-move problem under the block",  12, RouteType::Crimp},
};
constexpr int kTerraceProjectCount =
    static_cast<int>(sizeof(kTerraceProjects) / sizeof(kTerraceProjects[0]));

// The rope crag. Grades read on the YDS ladder through SportGradeName —
// index 5 is 5.11a, 7 is 5.12a, 9 is 5.13a — and the spread is shaped the
// way a real sport cave is rather than the way a boulder field is: far
// fewer easy lines (nobody bolts 5.7 in a cave), a deep middle where the
// crag's reputation lives, and two or three testpieces that most visitors
// only ever hang on.
const BookEntry kCave[] = {
    // The warmup wall at the left end, out of the steep.
    {"Cave Dweller",          2, 2, RouteType::Endurance,  1},   // 5.9
    {"Left-Hand Route",       3, 3, RouteType::Technical,  1},   // 5.10a
    {"Morning Sickness",      4, 4, RouteType::Endurance,  2},   // 5.10c
    {"The Warm-Up Lap",       4, 4, RouteType::Crimp,      1},

    // The main cave. Steep, pumpy, and the reason anybody walks up here.
    {"Belay Slave",           5, 5, RouteType::Endurance,  2},   // 5.11a
    {"Kneebar Rest",          5, 5, RouteType::Technical,  3},   // the classic
    {"Forty Minutes Up",      6, 6, RouteType::Endurance,  2},
    {"Second Clip",           6, 7, RouteType::Power,      1},   // stiff, and known for it
    {"The Pump Clock",        7, 7, RouteType::Endurance,  3},   // 5.12a
    {"Slack!",                7, 7, RouteType::Crimp,      2},
    {"Take, Take, TAKE",      6, 6, RouteType::Power,      1},
    {"Redpoint Crux",         8, 8, RouteType::Endurance,  3},   // 5.12c
    {"North Face Special",    8, 8, RouteType::Technical,  2},
    {"Shade All Day",         7, 7, RouteType::Technical,  2},

    // The back of the cave, where the holds stop being holds.
    {"The Tufa",              9, 9, RouteType::Endurance,  3},   // 5.13a
    {"Dogging It",            9, 10, RouteType::Crimp,     2},   // sandbagged, famously
    {"One Hang",             10, 10, RouteType::Endurance, 3},   // 5.13c
    {"Project For Life",     11, 11, RouteType::Power,     2},   // 5.14a
};
constexpr int kCaveCount =
    static_cast<int>(sizeof(kCave) / sizeof(kCave[0]));

// Three lines bolted and never climbed. A sport project is a different
// animal from a boulder one: somebody has already been up it on a rope and
// put the bolts in, so the grade guess is better informed and the line is
// not filthy — what is unknown is whether it goes at all.
const ProjectEntry kCaveProjects[] = {
    {"the bolted line through the roof",      10, RouteType::Power},
    {"the right-hand finish to The Tufa",      9, RouteType::Endurance},
    {"the blank panel past the third bolt",   12, RouteType::Crimp},
};
constexpr int kCaveProjectCount =
    static_cast<int>(sizeof(kCaveProjects) / sizeof(kCaveProjects[0]));

// The trad crag. Shaped like neither of the others, and for reasons that
// are about the discipline rather than about the rock:
//
//   **The spread starts low and stays low.** A cave is bolted from 5.9 up
//   because nobody bolts easy steep rock; a buttress has been climbed since
//   before anybody owned a drill, and its classics are moderate. Half of
//   this list is inside the reach of a first-season climber, which is the
//   point — trad is the one discipline where the entry-level lines are the
//   famous ones.
//
//   **Almost everything is a crack.** RouteType::Crack carries HoldType::
//   Crack through SignatureHold, and TakesGear reads the hold: a crack line
//   is a line you can protect, and a face line up here is a line you mostly
//   cannot. That is the whole of what makes one route on this crag a
//   different lead from another, and it is derived rather than authored.
//
//   **The hard ones are hard because of the gear.** "Ropeless in a Sense"
//   is a face route on this list on purpose. It goes at a grade a good
//   climber can pull, and TakesGear will offer them almost nothing on it.
const BookEntry kButtress[] = {
    // The apron at the bottom. Everybody's first lead is on this wall.
    {"Sunday Crack",          0, 0, RouteType::Crack,      2},   // 5.7
    {"The Staircase",         1, 1, RouteType::Crack,      1},   // 5.8
    {"Hand Jam Holiday",      2, 2, RouteType::Crack,      3},   // 5.9, the classic
    {"Wide Awake",            2, 3, RouteType::Crack,      1},   // offwidth, stiff
    {"Layback Flake",         3, 3, RouteType::Technical,  2},   // 5.10a

    // The main buttress. Where the guidebook's stars are.
    {"The Sentry Box",        4, 4, RouteType::Crack,      3},   // 5.10c
    {"Green Corner",          4, 4, RouteType::Technical,  2},
    {"Rusty Piton Direct",    5, 5, RouteType::Crack,      2},   // 5.11a
    {"Fingers Crossed",       5, 6, RouteType::Crimp,      2},   // thin, and thin gear
    {"The Long Reach",        6, 6, RouteType::Crack,      2},
    {"Second's Nightmare",    6, 6, RouteType::Technical,  1},
    {"Bombproof",             7, 7, RouteType::Crack,      3},   // 5.12a, sews up

    // The headwall, which is where the grades stop meaning what they say.
    {"Ropeless in a Sense",   7, 7, RouteType::Power,      3},   // face; no gear
    {"The Groundfall Pitch",  8, 8, RouteType::Crimp,      2},   // 5.12c
    {"Last Piece Below You",  9, 9, RouteType::Crack,      3},   // 5.13a
};
constexpr int kButtressCount =
    static_cast<int>(sizeof(kButtress) / sizeof(kButtress[0]));

// A trad project is the third animal again. Nobody has bolted it, so
// nobody has hung on it, so the grade is a guess made from the ground with
// binoculars — and the real question is not whether it goes but whether it
// can be protected, which is a question you can only answer while on it.
const ProjectEntry kButtressProjects[] = {
    {"the seam left of Bombproof",             9, RouteType::Crimp},
    {"the unclimbed arête on the headwall",   10, RouteType::Power},
    {"the roof crack nobody has racked for",   8, RouteType::Crack},
};
constexpr int kButtressProjectCount =
    static_cast<int>(sizeof(kButtressProjects) / sizeof(kButtressProjects[0]));

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
    // A well-known line's guess is narrow and can miss in either direction:
    // people have pulled on the moves, so the book is close, but nobody has
    // linked them and the last move is always the surprise. A line nobody
    // has touched gets the wide spread, which is what makes the hard end of
    // the book a genuine question.
    const double roll = projectRng.NextDouble();
    const int drift =
        e.wellKnown ? (roll < 0.20 ? -1 : (roll < 0.75 ? 0 : 1))
                    : (roll < 0.25 ? -1 : (roll < 0.6 ? 0 : (roll < 0.9 ? 1 : 2)));
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

Crag SunTerrace(const Rng& worldRng) {
  Crag crag;
  crag.name = "the Sun Terrace";
  crag.aspect = Aspect::South;
  // Further than the cave, and up rather than along. Nothing reads this
  // yet — see notes/engine-bridge-gaps.md — but the level's travel spot
  // should match it.
  crag.approachHours = 0.9;

  // Its own stream, for the same narrow reason the cave has one: name
  // collisions stay different rock, and the projects draw from a stream
  // that is genuinely stateful.
  const Rng terraceRng = worldRng.Derive("sun-terrace");

  crag.lines.reserve(kTerraceCount + kTerraceProjectCount);
  for (int i = 0; i < kTerraceCount; i++) {
    const BookEntry& e = kTerrace[i];
    CragLine line;
    line.route = BuildRoute(terraceRng, e.name, e.grade, e.trueGrade, e.type,
                            Discipline::Boulder);
    line.stars = e.stars;
    line.isProject = false;
    line.firstAscentBy = "unknown";
    crag.lines.push_back(line);
  }

  Rng projectRng = terraceRng.Derive("terrace-projects");
  for (int i = 0; i < kTerraceProjectCount; i++) {
    const ProjectEntry& e = kTerraceProjects[i];
    CragLine line;
    // Same guess-and-drift as everywhere else: the book's grade for a line
    // nobody has done is an opinion, and finding out is the payoff.
    const double roll = projectRng.NextDouble();
    const int drift = roll < 0.25 ? -1 : (roll < 0.6 ? 0 : (roll < 0.9 ? 1 : 2));
    const int trueGrade = std::max(0, e.guess + drift);
    line.route = BuildRoute(terraceRng, e.description, e.guess, trueGrade,
                            e.type, Discipline::Boulder);
    line.stars = 0;
    line.isProject = true;
    line.description = e.description;
    crag.lines.push_back(line);
  }
  return crag;
}

Crag ShadedCave(const Rng& worldRng) {
  Crag crag;
  crag.name = "the Shaded Cave";
  // North-facing, and that is the whole point of the place. SunOnRock
  // returns zero for north aspects, so in high summer — when the season
  // model puts Roadside's window at dawn and nowhere else — this is the
  // only rock in the valley worth walking to. It costs forty minutes each
  // way and it costs a belayer.
  crag.aspect = Aspect::North;
  crag.approachHours = 0.7;

  // Its own stream. Roadside's rock is not actually at risk from call
  // order — BuildRoute salts by route name, so a line's shape depends on
  // its name and nothing else, which is what makes routes stable without a
  // stored move list. What this buys is narrower and still worth having: if
  // a cave line and a Roadside line ever share a name they stay different
  // pieces of rock, and the projects below draw from a stream that really
  // is stateful.
  const Rng caveRng = worldRng.Derive("shaded-cave");

  crag.lines.reserve(kCaveCount + kCaveProjectCount);
  for (int i = 0; i < kCaveCount; i++) {
    const BookEntry& e = kCave[i];
    CragLine line;
    line.route = BuildRoute(caveRng, e.name, e.grade, e.trueGrade, e.type,
                            Discipline::Sport);
    line.stars = e.stars;
    line.isProject = false;
    line.firstAscentBy = "unknown";
    crag.lines.push_back(line);
  }

  Rng projectRng = caveRng.Derive("cave-projects");
  for (int i = 0; i < kCaveProjectCount; i++) {
    const ProjectEntry& e = kCaveProjects[i];
    CragLine line;
    // A bolted project's guess is better informed than a boulder's —
    // somebody has already hung on it putting the bolts in — so the drift
    // is tighter. What is unknown is whether it goes at all, not roughly
    // how hard it is.
    const double roll = projectRng.NextDouble();
    const int drift = roll < 0.35 ? 0 : (roll < 0.8 ? 1 : 2);
    const int trueGrade = std::max(0, e.guess + drift);

    line.route = BuildRoute(caveRng, e.description, e.guess, trueGrade, e.type,
                            Discipline::Sport);
    line.stars = 0;
    line.isProject = true;
    line.description = e.description;
    crag.lines.push_back(line);
  }
  return crag;
}

Crag TheOldButtress(const Rng& worldRng) {
  Crag crag;
  crag.name = "the Old Buttress";
  // East-facing and high up: sun until mid-morning, then shade and wind for
  // the rest of the day. It is the longest walk in the valley and the only
  // place you need a rack, and both of those are the same fact — the lines
  // up here were climbed on gear because nobody was carrying a drill that
  // far.
  crag.aspect = Aspect::East;
  crag.approachHours = 1.1;

  // Its own stream, for the same narrow reason the cave has one: the
  // projects below draw from a stateful stream, and a shared one would make
  // the buttress's unclimbed lines depend on how many the cave has.
  const Rng buttressRng = worldRng.Derive("old-buttress");

  crag.lines.reserve(kButtressCount + kButtressProjectCount);
  for (int i = 0; i < kButtressCount; i++) {
    const BookEntry& e = kButtress[i];
    CragLine line;
    line.route = BuildRoute(buttressRng, e.name, e.grade, e.trueGrade, e.type,
                            Discipline::Trad);
    line.stars = e.stars;
    line.isProject = false;
    line.firstAscentBy = "unknown";
    crag.lines.push_back(line);
  }

  Rng projectRng = buttressRng.Derive("buttress-projects");
  for (int i = 0; i < kButtressProjectCount; i++) {
    const ProjectEntry& e = kButtressProjects[i];
    CragLine line;
    // The widest drift of the three crags, and deliberately wide in the
    // stiff direction. Nobody has hung on this line to find out what it is;
    // a boulder project has been tried from the ground and a bolted one has
    // been inspected on a rope, and this one has been looked at.
    const double roll = projectRng.NextDouble();
    const int drift = roll < 0.20 ? 0 : (roll < 0.55 ? 1 : (roll < 0.85 ? 2 : 3));
    const int trueGrade = std::max(0, e.guess + drift);

    line.route = BuildRoute(buttressRng, e.description, e.guess, trueGrade,
                            e.type, Discipline::Trad);
    line.stars = 0;
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
