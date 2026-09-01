#include "DirtbagNarrator.h"

#include <algorithm>
#include <cmath>

#include "DirtbagSessionLoop.h"
#include "DirtbagSport.h"
#include "DirtbagTrad.h"

namespace dirtbag {

namespace {

// How the attempt ended, derived rather than passed in. The last timeline
// entry failing is a fall and a full highpoint is a top-out; anything else
// is a climber who is still on the wall, which is what a live attempt looks
// like every time the staging layer asks.
enum class Ending { StillOn, Fell, Topped };

Ending EndingOf(const AttemptInput& in, const AttemptResult& r) {
  (void)in;
  // **Read off `sent` rather than off the highpoint**, and the asymmetry is
  // deliberate: a fall is known the instant it happens, and a top-out is a
  // *style* judgement that `FinishAttempt` makes. `LiveAttempt::partial`
  // has the last move in it and does not yet know whether that was an
  // onsight, a flash or a redpoint -- so a wall reading the partial said
  // a bare "Top." while the replay of the same climb said "first go, no
  // idea what was coming, and it went". One climb, two paths, two
  // different things, which is precisely what this file promises not to do.
  if (r.sent) return Ending::Topped;
  if (!r.timeline.empty() && !r.timeline.back().success) return Ending::Fell;
  return Ending::StillOn;
}

// What you were carrying when you pulled on to move `i`. The resolver
// records pump *after* each move, so the pump a move faced is the previous
// entry's -- and move zero faces nothing, which is the only reason anybody
// ever gets off the ground.
double PumpBefore(const AttemptResult& r, int i) {
  if (i <= 0 || i > static_cast<int>(r.timeline.size())) return 0.0;
  return r.timeline[i - 1].pumpAfter;
}

// Later is louder. The same crux at move three and at move nineteen are not
// the same moment, because by move nineteen you have spent an afternoon
// getting there.
double Weigh(double base, int move, int moves, const NarratorDials& dials) {
  if (moves <= 0 || move < 0) return Clamp01(base);
  const double through = static_cast<double>(move + 1) / static_cast<double>(moves);
  return Clamp01(base * ((1.0 - dials.lateness) + dials.lateness * 2.0 * through));
}

void Say(std::vector<Beat>& out, int move, BeatKind kind, double weight,
         std::string line) {
  Beat b;
  b.move = move;
  b.kind = kind;
  b.weight = Clamp01(weight);
  b.line = std::move(line);
  out.push_back(b);
}

// The one thing that is wrong today, or nothing. Ordered by how much a
// climber standing at the bottom would actually be thinking about it.
std::string WhatIsWrongToday(const AttemptInput& in) {
  if (in.climber.injury.active) {
    return "And you are still hurt, and you are getting on it anyway.";
  }
  if (in.climber.skin <= 2.0) {
    return "There is nothing left on your tips.";
  }
  if (in.warmth <= 0.35) {
    return "Stone cold, and straight onto it.";
  }
  if (in.cleanliness <= 0.4) {
    return "Nobody has touched this. It is filthy.";
  }
  if (in.shoeWear >= 0.75) {
    return "The rubber went a while ago and you know it.";
  }
  return std::string();
}

// **Why it ended**, decided once and rendered twice. A watcher in the
// moment gets the flourish; the career's own history gets the fact. Two
// sets of strings would be two places for one rule to live, and this
// project has a standing note about how that ends.
enum class Why { Redline, BlewTheClip, AtTheCrux, OneMove, OffEarly, NeverInHand };

Why WhyItEnded(const AttemptInput& in, const AttemptResult& r, int at,
               const SportDials& sport) {
  const int moves = static_cast<int>(in.route.moves.size());
  if (PumpBefore(r, at) >= 100.0) return Why::Redline;
  if (in.route.discipline != Discipline::Boulder &&
      IsClippingMove(in.route, at, sport)) {
    return Why::BlewTheClip;
  }
  if (at >= 0 && at < moves && in.route.moves[at].crux) return Why::AtTheCrux;
  if (at >= moves - 1 && moves > 1) return Why::OneMove;
  if (at <= 1) return Why::OffEarly;
  return Why::NeverInHand;
}

const char* WhyText(Why why, bool forTheLog) {
  switch (why) {
    case Why::Redline:
      // A redlined fall is not luck and the resolver does not roll for it:
      // the hands open. Saying so is the difference between a player who
      // thinks the game cheated and one who knows they went too fast.
      return forTheLog ? "The hands opened."
                       : "The hands open. Nothing you could have done about "
                         "that one -- it was gone three moves ago.";
    case Why::BlewTheClip:
      return forTheLog ? "Blew the clip."
                       : "Off pulling up the rope. That is the classic and "
                         "it never stops being infuriating.";
    case Why::AtTheCrux:   return "Off at the crux.";
    case Why::OneMove:
      return forTheLog ? "One move short."
                       : "One move. That was the go.";
    case Why::OffEarly:
      return forTheLog ? "Off early."
                       : "Off early. Nothing learned but the first two moves.";
    case Why::NeverInHand:
      return forTheLog ? "Never in hand."
                       : "And off. It was never quite in hand.";
  }
  return "";
}

std::string ToppedLine(const AttemptResult& r) {
  switch (r.style) {
    case Style::Onsight:
      return "Top. First go, no idea what was coming, and it went. That is "
             "an onsight and they do not come back.";
    case Style::Flash:
      return "Top, first go. You knew what was coming and you still had to "
             "do it.";
    case Style::Redpoint:
      return "Top. It took what it took.";
    case Style::Sent:
    case Style::Fell:
      break;
  }
  return "Top.";
}

}  // namespace

const char* BeatKindName(BeatKind kind) {
  switch (kind) {
    case BeatKind::Ground:      return "ground";
    case BeatKind::OffTheDeck:  return "off the deck";
    case BeatKind::Placed:      return "placed";
    case BeatKind::Crux:        return "crux";
    case BeatKind::NearlyBlew:  return "nearly blew it";
    case BeatKind::Shake:       return "shake";
    case BeatKind::Pumped:      return "pumped";
    case BeatKind::Runout:      return "runout";
    case BeatKind::Fell:        return "fell";
    case BeatKind::Topped:      return "topped";
    case BeatKind::kBeatKindCount: break;
  }
  return "";
}

std::vector<Beat> CallTheAttempt(const AttemptInput& input,
                                 const AttemptResult& result,
                                 const NarratorDials& dials) {
  std::vector<Beat> out;
  const Route& route = input.route;
  const int moves = static_cast<int>(route.moves.size());
  const int climbed = static_cast<int>(result.timeline.size());

  // --- From the ground --------------------------------------------------
  {
    // The read from the ground is what you say on go one. After that the
    // interesting thing is not what the route looks like, it is how many
    // times you have been here -- which is the whole difference between an
    // onsight and a project, and the resolver already counts it.
    std::string line;
    if (input.attemptNumber <= 1) {
      line = ReadRouteText(ReadRoute(input.climber, route));
    } else if (input.beta >= dials.knowsItAt) {
      line = "You know every move on this one.";
    } else if (input.attemptNumber >= dials.lostCountAt) {
      line = "You have lost count of how many times you have pulled on to "
             "this.";
    } else {
      line = "Back on it.";
    }
    const std::string wrong = WhatIsWrongToday(input);
    if (!wrong.empty()) {
      line += " ";
      line += wrong;
    }
    Say(out, -1, BeatKind::Ground, dials.groundWeight, line);
  }

  const SportDials sport;
  const bool roped = route.discipline != Discipline::Boulder;
  bool saidOffTheDeck = false;
  bool saidForearms = false;
  bool saidNearlyGone = false;
  bool saidRunout = false;

  // **The narrator only speaks when something beats everything before it.**
  //
  // This is the mechanism that turns *every move* into *the moments*, and
  // it exists because the first version did not have it: on a route where
  // the climber was desperate the whole way, "that should not have stayed
  // on" fired on **every single move** -- sixteen beats on a sixteen-move
  // pitch, which is not commentary, it is a log with adjectives. A climber
  // fighting all the way up is not having sixteen moments, they are having
  // one long fight.
  //
  // Escalation says it once at the first shocker and again only at a worse
  // one, which on a uniformly hard route is exactly once and on a route
  // with a sting in the tail is exactly twice, in the right places.
  double worstOdds = 1.0;
  double bestRest = 0.0;
  double worstPiece = 1.0;

  for (int i = 0; i < climbed && i < moves; i++) {
    const MoveResult& mr = result.timeline[i];
    const Move& move = route.moves[i];

    // **Off the deck.** The first thing on the rope is the moment the whole
    // ground-fall model switches off -- on a trad lead it is a decision the
    // leader made, which is why it is worth a beat and a bolt clipped at
    // move two is worth one too.
    if (roped && !saidOffTheDeck && OnTheRope(route, i, sport, result.gear)) {
      saidOffTheDeck = true;
      const int at = LastPieceAtOrBelow(route, i, sport, result.gear);
      const double trust = at >= 0 ? PieceAt(route, at, sport, result.gear) : 0.0;
      std::string line;
      if (route.discipline == Discipline::Trad) {
        line = trust >= 0.75
                   ? "First piece, and it is bomber. The deck lets go of you."
                   : (trust >= 0.4
                          ? "First piece in. It will probably hold, and it is "
                            "better than the ground."
                          : "Something is in. You would not want to find out "
                            "how good it is.");
      } else {
        line = "Clipped. The ground stops being the question now.";
      }
      Say(out, i, BeatKind::OffTheDeck,
          Weigh(dials.offTheDeckWeight, i, moves, dials), line);
    }

    // **A piece you do not trust.** Only those: a bomber cam every four
    // moves is not news, and saying so five times a pitch teaches a player
    // to stop reading the narrator.
    if (route.discipline == Discipline::Trad && saidOffTheDeck &&
        i < static_cast<int>(result.gear.quality.size())) {
      const double q = result.gear.quality[i];
      if (q > 0.0 && q < dials.pieceWorthSayingBelow && q < worstPiece) {
        worstPiece = q;
        Say(out, i, BeatKind::Placed,
            Weigh(dials.placedWeight, i, moves, dials),
            std::string("Gets something in. ") + PieceText(q) +
                ", and you have spent the pump either way.");
      }
    }

    // **A long way above whatever is holding the rope.** Once, because it
    // is a state rather than an event and the second telling is nagging.
    if (roped && !saidRunout) {
      const double runout = RunoutAt(route, i, sport, result.gear);
      if (runout >= dials.runoutWorthSayingAt) {
        saidRunout = true;
        const int at = LastPieceAtOrBelow(route, i, sport, result.gear);
        const double trust =
            at >= 0 ? PieceAt(route, at, sport, result.gear) : 0.0;
        // In its own words rather than borrowed from `RunoutText`, which
        // says *bolt* -- correct for the HUD it was written for and wrong
        // out loud on a trad lead, where the whole point is that nobody
        // drilled anything. One vocabulary per voice.
        std::string line;
        if (trust < 0.5) {
          line = "A long way above a piece you do not believe in.";
        } else if (route.discipline == Discipline::Trad) {
          line = runout >= 0.85
                     ? "The last piece is a long way down now."
                     : "The gear is below your feet.";
        } else {
          line = runout >= 0.85 ? "A long way above the last clip."
                                : "The bolt is below your feet.";
        }
        Say(out, i, BeatKind::Runout,
            Weigh(dials.runoutWeight, i, moves, dials),
            line + " Whatever happens next, happens.");
      }
    }

    // **The forearms**, twice at most. The pump bar is on screen and saying
    // it every move is the bar's job rather than the narrator's.
    if (!saidForearms && mr.pumpAfter >= dials.forearmsGoingAt) {
      saidForearms = true;
      Say(out, i, BeatKind::Pumped,
          Weigh(dials.forearmsWeight, i, moves, dials),
          "The forearms are going.");
    }
    if (!saidNearlyGone && mr.pumpAfter >= dials.nearlyGoneAt) {
      saidNearlyGone = true;
      Say(out, i, BeatKind::Pumped,
          Weigh(dials.nearlyGoneWeight, i, moves, dials),
          "You are not going to be able to hold on much longer and you know "
          "it.");
    }

    // **A rest that worked.** The resolver writes a stance's pump back over
    // the arrival, so pump going *down* across a move is a shake-out that
    // paid -- there is no other way for it to fall.
    if (i > 0) {
      const double dropped = result.timeline[i - 1].pumpAfter - mr.pumpAfter;
      if (dropped >= dials.shakeWorthIt && dropped > bestRest) {
        bestRest = dropped;
        Say(out, i, BeatKind::Shake,
            Weigh(dials.shakeWeight, i, moves, dials),
            dropped >= dials.shakeWorthIt * 2.0
                ? "That is a proper rest. You have got the route back."
                : "Shakes it out. Some of it comes back.");
      }
    }

    // **The crux, arriving** -- and only when it was in doubt. A route
    // being harder in the middle is a fact about the route; a hard move you
    // might not do is a moment.
    if (move.crux && mr.odds <= dials.cruxIsSafeAbove) {
      const double pump = PumpBefore(result, i);
      Say(out, i, BeatKind::Crux, Weigh(dials.cruxWeight, i, moves, dials),
          pump >= dials.forearmsGoingAt
              ? "This is the one, and you are arriving at it tired."
              : "This is the one.");
    }

    // **A move that had no business staying on.**
    if (mr.success && mr.odds < dials.nearlyBlewBelow && mr.odds < worstOdds) {
      worstOdds = mr.odds;
      Say(out, i, BeatKind::NearlyBlew,
          Weigh(dials.nearlyBlewWeight, i, moves, dials),
          "That should not have stayed on.");
    }
  }

  // --- How it ended ------------------------------------------------------
  const Ending ending = EndingOf(input, result);
  if (ending == Ending::Topped) {
    Say(out, moves - 1, BeatKind::Topped, dials.endingWeight,
        ToppedLine(result));
  } else if (ending == Ending::Fell) {
    const int at = climbed - 1;
    Say(out, at, BeatKind::Fell, dials.endingWeight,
        WhyText(WhyItEnded(input, result, at, sport), false));
  }

  return out;
}

std::vector<Beat> Loudest(const std::vector<Beat>& beats, int n) {
  if (n <= 0) return {};
  std::vector<Beat> picked = beats;
  // Stable, so two beats of equal weight keep the order they happened in
  // rather than an order the sort felt like.
  std::stable_sort(picked.begin(), picked.end(),
                   [](const Beat& a, const Beat& b) {
                     return a.weight > b.weight;
                   });
  if (static_cast<int>(picked.size()) > n) {
    picked.resize(static_cast<std::size_t>(n));
  }
  // Back into the order they happened. A highlight reel out of order is not
  // a highlight reel.
  //
  // The ending sorts last among beats on the same move, because it is the
  // same move and it happened after: the forearms going and the top-out
  // both land on the last move of a pitch, and a reel that read *"you know
  // you cannot hold on much longer / top / the forearms are going"* was
  // three true lines in an order nobody climbed them in.
  const auto isEnding = [](const Beat& b) {
    return b.kind == BeatKind::Fell || b.kind == BeatKind::Topped;
  };
  std::stable_sort(picked.begin(), picked.end(),
                   [&isEnding](const Beat& a, const Beat& b) {
                     if (a.move != b.move) return a.move < b.move;
                     return !isEnding(a) && isEnding(b);
                   });
  return picked;
}

Beat LastWord(const AttemptInput& input, const AttemptResult& result,
              const NarratorDials& dials) {
  const std::vector<Beat> beats = CallTheAttempt(input, result, dials);
  const int frontier = static_cast<int>(result.timeline.size()) - 1;
  Beat best;
  bool found = false;
  for (const Beat& b : beats) {
    if (b.move != frontier) continue;
    if (!found || b.weight > best.weight) {
      best = b;
      found = true;
    }
  }
  // Silence, which is the right answer most moves and is the whole reason
  // this file is worth having.
  return found ? best : Beat{};
}

std::string HowItWent(const AttemptInput& input, const AttemptResult& result,
                      const NarratorDials& dials) {
  const std::vector<Beat> beats = CallTheAttempt(input, result, dials);
  const Ending ending = EndingOf(input, result);

  // The ending is the sentence. Everything else in the attempt is why.
  //
  // Rendered short rather than lifted off the beat: a watcher gets *"first
  // go, no idea what was coming, and it went -- that is an onsight and they
  // do not come back"*, and a career's history carrying three sentences a
  // burn for thirty years would be unreadable by the second season.
  if (ending == Ending::StillOn) return "Still on it.";
  std::string out;
  if (ending == Ending::Topped) {
    switch (result.style) {
      case Style::Onsight:  out = "Onsighted it."; break;
      case Style::Flash:    out = "Flashed it."; break;
      case Style::Redpoint: out = "Sent it."; break;
      default:              out = "Top."; break;
    }
  } else {
    const int at = static_cast<int>(result.timeline.size()) - 1;
    out = WhyText(WhyItEnded(input, result, at, SportDials{}), true);
  }
  if (out.empty()) return "Nothing happened.";

  // ...and one clause of why, taken from the loudest thing that happened on
  // the way up. A history that says "off at the crux, and the pump was
  // already gone" is worth reading; one that says "fell" is a row in a
  // table. A send gets the same treatment, because *how close it was* is
  // most of what makes a send worth remembering.
  // **Never the beat you fell on.** What happened at the moment it ended is
  // the ending, not the reason for it -- reaching for the loudest beat
  // without this produced *"off at the crux"* with nothing after it,
  // because the crux beat outranked the runout that actually explained the
  // afternoon and then declined to say anything about itself.
  int endedAt = -2;
  for (const Beat& b : beats) {
    if (b.kind == BeatKind::Fell || b.kind == BeatKind::Topped) endedAt = b.move;
  }
  const Beat* why = nullptr;
  for (const Beat& b : beats) {
    if (b.kind == BeatKind::Fell || b.kind == BeatKind::Topped) continue;
    if (b.kind == BeatKind::Ground || b.move == endedAt) continue;
    if (!why || b.weight > why->weight) why = &b;
  }
  if (!why) return out;
  const bool fell = ending == Ending::Fell;
  switch (why->kind) {
    case BeatKind::Pumped:
      return out + (fell ? " The pump was already gone."
                         : " You were empty by the end of it.");
    case BeatKind::Runout:
      return out + (fell ? " You had been a long way above the gear for a "
                           "while."
                         : " And a long way above the gear for most of it.");
    case BeatKind::NearlyBlew:
      return out + (fell ? " You had already got away with one."
                         : " You got away with one on the way, too.");
    case BeatKind::Shake:
      // A rest is never why you fell -- it is the reason you got as far as
      // you did. Appending it to a fall produced *"off pulling up the rope,
      // the rest bought you less than it looked like"*, which is a narrator
      // reaching for a cause because it had one rather than because it
      // explained anything.
      return fell ? out : out + " The rest in the middle is what did it.";
    case BeatKind::Placed:
      return out + (fell ? " You had spent pump on gear you did not trust."
                         : " On gear you would rather not have fallen on.");
    case BeatKind::Crux:
      return out + (fell ? "" : " The crux went, and it was not certain.");
    case BeatKind::OffTheDeck:
      // Getting off the deck explains neither outcome. It is a moment, not
      // a reason, and the log line is better without it.
    case BeatKind::Ground:
    case BeatKind::Fell:
    case BeatKind::Topped:
    case BeatKind::kBeatKindCount:
      break;
  }
  return out;
}

}  // namespace dirtbag
