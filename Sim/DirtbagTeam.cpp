#include "DirtbagTeam.h"

#include <algorithm>
#include <cctype>

namespace dirtbag {
namespace {

// What somebody is in the room, read off what they are known for. A
// teammate is a person before they are a number.
const char* RoleForSignature(RouteType t) {
  switch (t) {
    case RouteType::Crimp:     return "the crimper";
    case RouteType::Power:     return "the powerhouse";
    case RouteType::Endurance: return "the engine";
    case RouteType::Technical: return "the technician";
    case RouteType::Dyno:      return "the jumper";
    default:                   return "the crack climber";
  }
}

}  // namespace

const char* StatusText(TeamStatus s) {
  switch (s) {
    case TeamStatus::Named: return "on the national team";
    case TeamStatus::Cut:   return "off the national team";
    default:                return "unselected";
  }
}

const std::vector<Coach>& TheCoaches() {
  static const std::vector<Coach> kCoaches = {
      {"Ilse Brandt",
       "ran the junior programme for eleven years and has never once raised "
       "her voice"},
      {"Petro Vasilenko",
       "made a World Cup final in an era with no money in it, and still "
       "coaches like there is none"},
      {"Dana Whitlock",
       "keeps a notebook on every competitor in the country and will read "
       "yours back to you"},
      {"Emile Barre",
       "was a setter first, so he talks about movement while everyone else "
       "talks about training"},
  };
  return kCoaches;
}

Coach CoachFor(int season) {
  const std::vector<Coach>& all = TheCoaches();
  // Deterministic from the season rather than rolled, so a reload cannot
  // hand you a different coach for the same call-up -- and so the one you
  // got is a fact about *when* you arrived.
  const std::size_t i =
      static_cast<std::size_t>(std::max(0, season)) % all.size();
  return all[i];
}

std::vector<Teammate> RosterFrom(const Circuit& circuit,
                                 const TeamDials& dials) {
  // **The top of the national field**: the exact people you have spent a
  // career chasing up the standings, rather than five names invented for
  // the occasion. That is what makes being named feel like arriving
  // somewhere.
  const std::vector<Competitor>& field = TheField();
  std::vector<Teammate> all;
  for (std::size_t i = 0; i < field.size(); i++) {
    Teammate m;
    m.name = field[i].name;
    m.points =
        i < circuit.fieldPoints.size() ? circuit.fieldPoints[i] : 0.0;

    // The junior and the veteran are read off where they sit rather than
    // stored, so the roster turns over under you as the field does.
    if (field[i].gradeOffset <= -3.0) {
      m.role = "the junior";
    } else if (field[i].gradeOffset >= 1.0) {
      m.role = "the veteran";
    } else {
      m.role = RoleForSignature(field[i].signature);
    }
    all.push_back(m);
  }
  std::stable_sort(all.begin(), all.end(),
                   [](const Teammate& a, const Teammate& b) {
                     return a.points > b.points;
                   });
  if (static_cast<int>(all.size()) > dials.size) {
    all.resize(static_cast<std::size_t>(dials.size));
  }
  return all;
}

TeamReview ReviewTheTeam(NationalTeam& team, double rankingPoints,
                         const Circuit& circuit, int day, int season,
                         const TeamDials& dials) {
  TeamReview out;
  out.was = team.status;
  out.now = team.status;

  // **The line you are held to depends on whether you are already on it.**
  // Named, you hold down to `holdAt`; unselected, you have to clear
  // `selectAt`. That gap is the grace a committee gives somebody who was on
  // the paper last year, and without it a season hovering around the number
  // is a coin flip taken five times.
  const bool onNow = team.status == TeamStatus::Named;
  const double line = onNow ? dials.holdAt : dials.selectAt;
  const bool clears = rankingPoints >= line;

  const std::vector<Teammate> before = team.roster;

  if (clears && !onNow) {
    const bool returning = team.everNamed;
    team.status = TeamStatus::Named;
    team.everNamed = true;
    team.namedOnDay = day;
    // The coach is fixed at your *first* call-up. You do not get a new one
    // for coming back.
    if (team.coach.empty()) {
      const Coach c = CoachFor(season);
      team.coach = c.name;
      team.coachKnownFor = c.knownFor;
    }
    team.roster = RosterFrom(circuit, dials);
    // **Who you went past.** Somebody was on this paper and is not, and the
    // game knows their name -- which is the whole difference between a
    // promotion and a number going up.
    for (const Teammate& m : before) {
      bool still = false;
      for (const Teammate& n : team.roster) {
        if (n.name == m.name) still = true;
      }
      if (!still) team.passed = m.name;
    }
    out.changed = true;
    out.now = TeamStatus::Named;
    out.rep = returning ? dials.renamedRep : dials.namedRep;
    out.news = returning
                   ? "You are back on the national team."
                   : "You have been named to the national team. " +
                         team.coach + " " + team.coachKnownFor + ".";
    if (!returning && !team.passed.empty()) {
      out.news += " " + team.passed + " is not on it any more.";
    }
  } else if (!clears && onNow) {
    team.status = TeamStatus::Cut;
    team.cuts++;
    out.changed = true;
    out.now = TeamStatus::Cut;
    out.rep = -dials.cutRep;
    // Said without editorial. A committee does not explain itself and
    // neither does this.
    out.news = "The committee did not name you this year.";
  }

  if (team.status == TeamStatus::Named) {
    team.seasons++;
    // The roster is refreshed every review, so people come and go under you
    // whether or not your own status changed.
    const std::vector<Teammate> now = RosterFrom(circuit, dials);
    for (const Teammate& m : team.roster) {
      bool still = false;
      for (const Teammate& n : now) {
        if (n.name == m.name) still = true;
      }
      if (!still) team.gone.push_back(m.name);
    }
    team.roster = now;
    out.stipend = dials.stipend;
  }

  team.lastReviewPoints = rankingPoints;
  team.lastReviewSeason = season;
  return out;
}

std::string TeamLine(const NationalTeam& team, const TeamDials& dials) {
  // A team you have never been near is not a status.
  if (!team.everNamed) return std::string();

  // The status words come from `StatusText` rather than being written
  // again here: two places saying "on the national team" is two places to
  // change it, and the engine's UENUM already mirrors that one.
  std::string s = StatusText(team.status);
  s[0] = static_cast<char>(std::toupper(s[0]));

  if (team.status == TeamStatus::Named) {
    if (team.seasons > 1) {
      s += ", " + std::to_string(team.seasons) + " seasons";
    }
    s += ".";
    if (!team.coach.empty()) s += " " + team.coach + " has the squad.";
    return s;
  }
  // Cut, and the door is left open -- which is what "was" rather than
  // "never" is for.
  if (team.seasons > 0) {
    s += " after " + std::to_string(team.seasons) +
         (team.seasons == 1 ? " season" : " seasons");
  }
  s += ".";
  (void)dials;
  return s;
}

}  // namespace dirtbag
