#pragma once

// Somebody taught you, and you teach somebody -- `WRLD-10`, `TUT-6`,
// `ROSTER-1`, `COACH-5` and `JOB-10`, ported from the 2D source. The last of
// tier one, and two halves of one thing, which is why they share a file and
// a vocabulary.
//
// The port has a rival, partners, a crew, locals, a youth squad and a gym
// full of regulars. **Nobody in it has ever taught you anything, and nobody
// has ever paid you to teach them.** The youth team is the nearest thing and
// it is neither: those are kids on a squad, not a person who took you under
// their wing and not a client with a plan.
//
// ## The half where you are taught (`WRLD-10`)
//
// An old crusher who haunts the crag notices you once you are climbing real
// grades, and teaches you five things in order: **footwork, reading the
// line, falling, resting, and then everything she has left.** One lesson per
// session, each a real boon and a piece of her.
//
// The source's last lesson is the whole design and is worth quoting rather
// than paraphrasing:
//
//   *"Last one. Everything I've got about moving on rock -- it's yours now.
//   I'm getting old; these shoulders are shot. Won't be long 'fore the young
//   ones are watching you climb. So pass it on, hey? That's the whole
//   deal."*
//
// **`TUT-6` is why she is at the Lot before she is at the crag.** She was
// crag-only and gated on V3, which meant a brand-new player never met her --
// so the character who exists to teach you the game was unreachable until
// after you had worked it out. She is in the Lot from day one now, saying one
// true thing per visit about your actual situation, and her last line hands
// you to the arc.
//
// ## The half where you teach (`ROSTER-1`)
//
// Standing clients, up to three, each with a name, a strength and a
// weakness. **You set the plan**, and coaching somebody on their weakness is
// meaningfully better than a comfortable session on what they are already
// good at -- which is the whole of what a plan is. Progress carries across
// sessions and a client who reaches their ceiling **graduates**, so the slot
// frees up and the relationship has an end rather than a treadmill.
//
// `COACH-5` is the part that makes it real: **your people show up.** A
// client who knows you turns up to league night, and how they do is written
// by who they are -- their strength carries them, their weakness is where it
// goes sideways -- and it lands on your standing, because everyone in the
// room knows whose client that is.
//
// Engine-free like everything in Sim/.

#include <string>
#include <vector>

#include "DirtbagCharacter.h"
#include "DirtbagCore.h"
#include "DirtbagRng.h"

namespace dirtbag {

// The five areas both halves speak. The source reuses one vocabulary for the
// mentor, the mentee and the roster on purpose: what somebody needs and what
// you can teach are the same list.
enum class Focus { Power, Technique, Siege, Head, Durable };
constexpr int kFocusCount = 5;
const char* FocusName(Focus f);
// "precise feet and quiet technique"
const char* FocusTeaches(Focus f);
// "a silky technician"
const char* FocusBecomes(Focus f);
// Which kind of climbing a focus is about, for `COACH-5`'s league night.
//
// **`FocusLane` used to sit here and nothing read it.** It mapped a focus to
// one of your five skills, which is a sentence about *you* and this system
// is about somebody else -- a client's grade moves on progress, not on a
// lane. A dial nobody reads is worse than no dial.
RouteType FocusRoute(Focus f);

// --- the half where you are taught ---------------------------------------

struct MentorDials {
  // She only bothers with you once you are climbing real grades. Before that
  // there is nothing she can tell you that the rock is not already saying.
  double minGrade = 3.0;
  // Five, and the meet is the first.
  int sessions = 5;
  // She turns up reliably once the arc is live -- **not rarely**. A mentor
  // you have to farm is a spawn, not a person.
  double appearChance = 0.72;
  // A day between lessons at least, so five sessions is a season and not an
  // afternoon.
  int restDays = 1;
};

struct Mentor {
  // 0 unmet, 1..N lessons taken. Reaching N graduates you.
  int stage = 0;
  int lastDay = -1;
  // `TUT-6`: which of her Lot lines you have already heard. She says each
  // once, ever.
  int saidAtTheLot = 0;
};

// One lesson: her voice, what it teaches, and what you took from it.
struct Lesson {
  const char* title;
  const char* line;
  const char* learned;
  Skill lane;
  double amount;
  // A second lane, for the last one. `amount2` is zero on the rest.
  Skill lane2;
  double amount2;
};

// The lesson for a stage, or null past the end.
const Lesson* LessonAt(int stage, const MentorDials& dials = MentorDials{});

// Is she at the crag today, and is there a lesson in it? Deterministic on
// (seed, day) like every other roll in this game -- **the same day is the
// same day**, so a reload does not re-roll her.
bool SheIsAround(const Mentor& mentor, double yourGrade, const Rng& worldRng,
                 int day, const MentorDials& dials = MentorDials{});

// Take the lesson. Returns null when there is none to take.
const Lesson* ClimbWithHer(Mentor& mentor, Skills& skills, int day,
                           const MentorDials& dials = MentorDials{});

bool SheIsDoneWithYou(const Mentor& mentor,
                      const MentorDials& dials = MentorDials{});

// `TUT-6`: what she says at the Lot, before she is anybody's mentor. One
// thing per visit, each once ever, and **the last one hands you to the
// arc**. Empty when she has nothing left to say.
std::string WhatSheSaysAtTheLot(Mentor& mentor, double yourGrade, double cash,
                                bool hasVan,
                                const MentorDials& dials = MentorDials{});

// --- the half where you teach --------------------------------------------

struct RosterDials {
  int maxClients = 3;
  double startGrade = 2.0;
  // Grades gained from their start before they graduate.
  double gradesToGraduate = 4.0;
  double progressTarget = 100.0;
  // A standing client pays a premium over a drop-in walk-in.
  double sessionFee = 25.0;

  // **The plan is the mechanic.** Their weakness is the right answer, their
  // strength is a comfortable session that wastes the hour, anything else is
  // competent and generic.
  double fitOnWeakness = 1.0;
  double fitOnStrength = 0.55;
  double fitOtherwise = 0.35;
  double qualityFromFit = 0.75;
  // **Re-derived, and the test is what caught it.** The source's coaching
  // craft runs 0..12 and it weights it at 0.02 a point -- 0.24 at the top,
  // against fit's 0.75. This port's craft runs **0..100**, so the number as
  // written is worth 2.0 and pins quality at its cap for anybody past craft
  // 13: the plan becomes a label and coaching somebody on their weakness
  // stops meaning anything.
  //
  // Fifth re-derivation of this kind, and the first that is a *scale* rather
  // than a year length -- see `GYM-6`, `GYM-8`, `TAX-1` and `DEPTH-6`. The
  // shape is always the same: a number tuned against a range this port
  // measures differently.
  double qualityFromCraft = 0.0024;

  // Progress a session is worth, by how it went.
  double progressRough = 4.0, progressDecent = 9.0;
  double progressGood = 15.0, progressBreakthrough = 23.0;

  // A graduating client's parting gift, on top of the session fee, and what
  // it does for your name.
  double graduationCash = 60.0;
  double graduationRep = 8.0;

  // `COACH-5`: sessions with you before they would turn up to a league
  // night, the odds they do, what a public night is worth to them, and what
  // it is worth to you.
  int leagueSessions = 2;
  double leagueChance = 0.6;
  double leagueProgress = 6.0;
  double leagueStanding = 1.0;

  // `JOB-10`: at the top of the craft, **one client turns out to be the
  // real thing** -- and that is a fact about the person, rolled when you
  // take them on, not a dice roll every hour.
  //
  // The first cut rolled it per session and a thirty-year career produced
  // **1,647 prodigies in 5,475 sessions**. Three in ten hours cannot be a
  // revelation; it is a bonus with a nice name on it. It is a flag on the
  // client now, and it pays **once** -- on the session where you finally
  // read them right, which is the moment the source is describing.
  double prodigyFromCraft = 90.0;
  double prodigyChance = 0.3;
  double prodigyCash = 40.0;
  double prodigyRep = 6.0;
};

// Who they are. Fixed people rather than rolled numbers -- a client you
// coach for two years should have a name and one true thing about them.
struct ClientDef {
  const char* name;
  const char* blurb;
  Focus strength;
  Focus weakness;
};
// unwired-ok: the size of the cast, which the tests walk and the engine
// does not -- the engine walks your roster, and how many people exist to
// hire is not a thing any screen asks.
int HowManyClientsThereAre();
const ClientDef* TheClient(int who);

struct Client {
  int who = -1;             // index into the cast; -1 is an empty slot
  Focus plan = Focus::Technique;
  double grade = 2.0;
  double startGrade = 2.0;
  double progress = 0.0;
  int sessions = 0;
  // `JOB-10`: this one is the real thing, and you have not found out yet.
  bool prodigy = false;
  bool sawIt = false;
  std::string goal;         // the line they are working toward
  int startDay = 0;
  int lastSessionDay = -1;
};

struct Roster {
  std::vector<Client> clients;
  int graduated = 0;        // across a career
};

// Is there room, and is there anybody who has not been through already?
bool RoomForOneMore(const Roster& roster,
                    const RosterDials& dials = RosterDials{});

// Take somebody on. False when the roster is full. Their plan starts on
// whatever you have not been told about them yet -- **not their weakness**,
// because you do not know it on day one; that is what the first session is
// for.
bool TakeThemOn(Roster& roster, const Rng& worldRng, int day,
                const RosterDials& dials = RosterDials{});

// Set a client's plan. This is the decision the whole system is about.
bool SetThePlan(Roster& roster, int which, Focus plan);

struct CoachedSession {
  bool ran = false;
  bool graduated = false;
  bool prodigy = false;
  double cash = 0.0;
  double rep = 0.0;
  double progressGained = 0.0;
  bool gradedUp = false;
  std::string line;
};

// An hour with one of them. `craft` is your coaching craft, 0..100.
CoachedSession CoachThem(Roster& roster, int which, double craft,
                         const Rng& worldRng, int day,
                         const RosterDials& dials = RosterDials{});

// `COACH-5`: who turned up to league night, and how it went for them.
// `dominant` is what the room's setting favoured. Returns one line per
// client who came, and moves their progress and your standing through the
// out-parameters -- which is the point: **coaching that produces no public
// outcome is a number in a panel.**
std::vector<std::string> TheyTurnedUpToLeagueNight(
    Roster& roster, const Rng& worldRng, int day, RouteType dominant,
    double& standingGained, const RosterDials& dials = RosterDials{});

// What the roster reads like. Empty when nobody is on it.
std::string RosterLine(const Roster& roster,
                       const RosterDials& dials = RosterDials{});

}  // namespace dirtbag
