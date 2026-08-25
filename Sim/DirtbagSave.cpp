#include "DirtbagSave.h"

#include <algorithm>

#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace dirtbag {

namespace {

// %.17g round-trips an IEEE double exactly — the same discipline as the
// RNG golden vectors. Locale-independent by construction ('.' only).
std::string NumToStr(double v) {
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.17g", v);
  return buf;
}

std::string IntToStr(int v) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%d", v);
  return buf;
}

bool ParseDouble(const SaveFields& f, const std::string& key, double& out) {
  auto it = f.find(key);
  if (it == f.end()) return false;
  char* end = nullptr;
  out = std::strtod(it->second.c_str(), &end);
  return end && *end == '\0' && !it->second.empty();
}

bool ParseInt(const SaveFields& f, const std::string& key, int& out) {
  auto it = f.find(key);
  if (it == f.end()) return false;
  char* end = nullptr;
  const long v = std::strtol(it->second.c_str(), &end, 10);
  if (!end || *end != '\0' || it->second.empty()) return false;
  out = static_cast<int>(v);
  return true;
}

bool ParseString(const SaveFields& f, const std::string& key, std::string& out) {
  auto it = f.find(key);
  if (it == f.end()) return false;
  out = it->second;
  return true;
}

std::string ProjKey(int i, const char* field) {
  return "project." + IntToStr(i) + "." + field;
}

std::string BondKey(int i, const char* field) {
  return "bond." + IntToStr(i) + "." + field;
}

}  // namespace

namespace {

// v1 → v2: project ledgers gained the route's guidebook grade. Old ledgers
// name routes that may not exist any more (a gym resets its walls), so the
// honest answer is "unknown" — a guess here would put fictional grades in
// somebody's career history, which is exactly the data you can't fake.
void MigrateV1ToV2(SaveFields& fields) {
  int count = 0;
  if (!ParseInt(fields, "projects", count)) return;
  for (int i = 0; i < count; i++) {
    fields[ProjKey(i, "grade")] = "-1";
  }
}

// v2 → v3: ledgers gained the first-ascent record. Every line a v2 career
// touched was one that already existed in a book, so all of them are clean,
// none of them is a first ascent, and no grade needs confirming — the
// honest defaults, and the reason this migration can be exact rather than a
// guess.
void MigrateV2ToV3(SaveFields& fields) {
  int count = 0;
  if (!ParseInt(fields, "projects", count)) return;
  for (int i = 0; i < count; i++) {
    fields[ProjKey(i, "clean")] = "1";
    fields[ProjKey(i, "given")] = "";
    fields[ProjKey(i, "fa")] = "0";
    fields[ProjKey(i, "confirmed")] = "-1";
  }
}

// v3 → v4: careers gained the people they know. A v3 career knew nobody,
// which is precisely what an empty list means — no guessing required.
void MigrateV3ToV4(SaveFields& fields) {
  fields["bonds"] = "0";
}

// v5 → v6: shoes and the van. A v5 career wore neither, so both arrive new.
// Generous rather than exact on purpose: the honest alternative would be
// inventing wear nobody earned, and a career that predates a mechanic has
// not been dodging it.
void MigrateV5ToV6(SaveFields& fields) {
  fields["shoes.wear"] = "0";
  fields["shoes.resoles"] = "0";
  fields["shoes.pairs"] = "1";
  fields["van.hours"] = "0";
  for (int i = 0; i < 6; i++) {
    const std::string k = "van." + IntToStr(i) + ".";
    fields[k + "wear"] = "0";
    fields[k + "patches"] = "0";
    fields[k + "failed"] = "0";
  }
}

// v7 → v8: the scene. A v7 career had none to stand with, so it arrives
// where a new one does: nobody's friend, nobody's problem, crag open.
void MigrateV7ToV8(SaveFields& fields) {
  for (int i = 0; i < 4; i++) {
    fields["standing." + IntToStr(i)] = "0";
  }
  fields["standing.closed"] = "0";
}

// v8 → v9: the job. Standing was saved from v8 and the job was not, so a
// v8 career woke up unemployed every time it loaded — which is not a
// migration problem, it is the bug this version exists to fix. A v8 save
// arrives as what it actually was: whatever days it had worked are gone,
// and it is not salaried, because it could not have been.
void MigrateV8ToV9(SaveFields& fields) {
  fields["job.salaried"] = "0";
  fields["job.days"] = "0";
  fields["job.weeks"] = "0";
}

// v9 → v10: the kit. A v9 career owned no pads, no board and no
// membership, which is exactly what a new one owns — so it arrives with
// nothing, and the first thing it can do about that is go to work.
void MigrateV9ToV10(SaveFields& fields) {
  fields["kit.pads"] = "1";   // it always had the one, it just never said so
  fields["kit.hangboard"] = "0";
  fields["kit.membership"] = "0";
}

// v10 → v11: the body's second ledger. A v10 career had no training load
// and could not be hurt, so it arrives exactly as it played: rested, and in
// one piece. Which is honest — it never took a risk it did not know about.
void MigrateV10ToV11(SaveFields& fields) {
  fields["load"] = "0";
  fields["injury.active"] = "0";
  fields["injury.kind"] = "0";
  fields["injury.severity"] = "0";
  fields["injury.days"] = "0";
  fields["physio.last"] = "0";
}

// v11 → v12: the ledger learned which ladder its line is on, and careers
// gained the ones that came before. A v11 save predates the rope crag
// entirely, so every line it ever touched was a boulder — which is not a
// guess, it is the only thing that could have been true.
void MigrateV11ToV12(SaveFields& fields) {
  int count = 0;
  if (ParseInt(fields, "projects", count)) {
    for (int i = 0; i < count; i++) {
      fields[ProjKey(i, "disc")] = "0";   // Discipline::Boulder
    }
  }
  fields["legacies"] = "0";   // it is the first life; nobody came before
}

// v12 → v13: who pays you. A v12 career had nobody calling, which is both
// the truth and what a fresh Sponsorship already is.
void MigrateV12ToV13(SaveFields& fields) {
  fields["sponsor.tier"] = "0";
  fields["sponsor.seasons"] = "0";
  fields["sponsor.lastgrade"] = "-1";
  fields["sponsor.stale"] = "0";
  fields["sponsor.hurtdays"] = "0";
}

// v13 → v14: what you did that nobody saw. A v13 career carried nothing,
// which is the honest answer and the common one.
void MigrateV13ToV14(SaveFields& fields) { fields["secrets"] = "0"; }

// v14 → v15: days hurt since the last review. A v14 career had never been
// reviewed at all — ReviewSeason was written and called by nothing, in the
// engine and in the probe alike — so migrating to zero is not an
// approximation, it is the truth: nobody has ever looked at them.
void MigrateV14ToV15(SaveFields& fields) { fields["sponsor.hurtdays"] = "0"; }

// v15 -> v16: the Dirtbag Year. An old save has no record of whether its
// streak was running, and there is no honest way to reconstruct one -- a
// climber who has never been salaried might be four hundred days in, and
// the file does not say. So every old save starts its first streak today.
// That loses history rather than inventing it, which is the right way round:
// awarding somebody a year they may not have lived would put a line in their
// legacy that never happened.
void MigrateV15ToV16(SaveFields& fields) {
  fields["job.sincesalary"] = "0";
  fields["job.dirtbagyears"] = "0";
  fields["job.longeststreak"] = "0";
}

// v16 -> v17: the crew. An old save has no name, which is the correct
// answer rather than a lossy one -- the town had not said it yet because
// the system did not exist. The bonds that earn one are already saved, so
// an existing career starts its month from today and gets named on the
// far side of it, exactly as a new one would.
void MigrateV16ToV17(SaveFields& fields) {
  fields["crew.name"] = "";
  fields["crew.namedon"] = "0";
  fields["crew.days"] = "0";
  fields["crew.members"] = "0";
}

// v17 -> v18: dreams, and the Rig flag on the van. An old career owns
// nothing and is saving for nothing, which is exactly right -- there was
// nothing to own or save for. `van.rig` is false for the same reason: the
// van they have is the van they started with, whatever they have spent on
// it, because a Rig is a van you bought rather than a van you maintained.
void MigrateV17ToV18(SaveFields& fields) {
  fields["dreams.rig"] = "0";
  fields["dreams.warchest"] = "0";
  fields["dreams.homebase"] = "0";
  fields["dreams.working"] = "0";
  fields["dreams.seasonoff"] = "0";
  fields["van.rig"] = "0";
}

// v18 -> v19: dreams are chosen, not browsed. The old `dreams.working` was
// a free note-to-self; `chosen` is a binding, once-per-career commitment.
// A v18 note does NOT become the commitment -- binding somebody to a thing
// they idly clicked last month is exactly the kind of retroactive promise
// a migration must never make. Every old career arrives unchosen and makes
// the choice for real, keeping whatever it already bought under the old
// rules. The stale `dreams.working` key is simply no longer read.
void MigrateV18ToV19(SaveFields& fields) { fields["dreams.chosen"] = "0"; }

// v19 -> v20: the player's name reaches the sim (the crew hash needs it so
// the town names each generation's crew rather than re-issuing the last
// one's). Old saves arrive unnamed, which costs nothing: a crew already
// named stays named, and an unnamed career simply salts the hash with
// nothing, exactly as every career did before names existed.
void MigrateV19ToV20(SaveFields& fields) { fields["player.name"] = ""; }

// v20 → v21: who your climber is. A v20 career predates the four questions,
// so it arrives **unbuilt** — and that is exact rather than generous,
// because an unbuilt character is neutral in every lane. The climber you
// had keeps the numbers they had, and the creation screen does not ambush
// somebody twenty years into a career.
// v21 -> v22: somebody to beat. A v21 career had nobody, so it migrates to
// an **empty** rival -- and `SleepToNextDay` skips a rival with no name, so
// the career plays exactly as it did rather than having a stranger appear
// twenty years in.
// v22 -> v23: the line they are on. v22 shipped a day before the race
// existed, so a v22 career migrates to nothing running -- which is where a
// career sits most of the time anyway, and the roll starts it again the next
// morning.
// v23 -> v24: national ranking points. A v23 career never entered a comp,
// so it starts unranked -- which is the Local tier, where an unranked
// climber belongs.
void MigrateV23ToV24(SaveFields& fields) { fields["ranking"] = "0"; }

// v24 -> v25: the circuit season. A v24 career had points and no season, so
// it arrives with none started -- and the night tick opens one the next
// morning, which is where a career begins anyway. The points it already had
// are kept: they were earned at comps and a season boundary does not undo
// them.
// v25 -> v26: the national team. A v25 career was never called, so it
// arrives never selected -- exact rather than generous, and it means the
// first review after loading is a first call rather than a re-announcement
// of one that never happened.
// v26 -> v27: the World Cup and the Games. A v26 career had neither, so
// it arrives with no season and no date -- and the night tick opens a
// season and seeds the cycle the next morning, which is where a career
// starts anyway. **Exact rather than generous**: nobody who loads an old
// save was ever on a plane, and giving them a start would be inventing a
// year they did not have.
// v27 -> v28: the ranking became a record instead of a lifetime total.
//
// **The one migration in this project that deliberately throws a number
// away.** A v27 career's `ranking` field is the sum of everything it ever
// scored, on a curve where mid-field paid half a win -- ten-year careers
// were carrying fourteen thousand points against a top tier of two
// thousand two hundred. Carrying that across would hand a migrated save a
// World-Class ranking it could never have earned under the new curve and
// could never lose, since there is no record behind it to age out.
//
// So the record starts empty and the ranking starts at nothing. That is
// harsh and it is the only honest option: the old number is not a smaller
// version of the new one, it is a different measurement. A migrated career
// is Unranked and re-earns its rung over its next season of comps, which
// is about eleven weeks of play.
// v28 -> v29: the league. A v28 career had no Wednesday night, so it
// arrives with none scheduled -- and the night tick schedules the first one
// inside the week, which is where a career starts anyway. **The personal
// best starts at nothing**, exactly, because it is a record of nights you
// climbed and a migrated career climbed none.
// v29 -> v30: the medical file. A v29 career had an injury and a day
// count and nothing else -- no diagnosis, no comeback, no joints, no
// scars, no policy. It arrives with none of those, and **a career that was
// mid-injury when it was saved keeps the injury**: `MedicalDay` finds a
// hurt climber with no comeback running and starts one, which is exactly
// the case that guard exists for.
//
// The joints start clean. That is generous and it is the only honest
// option: a v29 save has no record of what its cortisone history was,
// because there was none, and inventing one would be inventing a career.
// v30 -> v31: the things that are wrong with you that are not the injury.
// A v30 career was never ill, has good teeth, has never done a morning of
// prehab and has never talked to anybody. All four are the honest default:
// none of them existed to have happened.
//
// **The tooth clock starts from the load rather than from day one**, which
// is why `teeth.since` is zero -- the first night after the migration is
// the first roll, and a career twenty years in does not wake up with an
// abscess it has been carrying invisibly.
// v31 -> v32: the trades. A v31 career worked plenty of shifts and got
// no better at any of them, because there was nothing to get better at.
//
// **It arrives with nothing, and that is exact rather than harsh.** Back-
// filling a craft from `job.daysWorked` would be inventing a trade: the
// save records how many days were worked and not one word about what the
// work *was*, so any number here would be a guess wearing a fact's coat.
// A career that has been washing dishes for ten years starts learning to
// wash dishes today, which is wrong, and is less wrong than deciding it
// was a setter all along.
// v32 → v33: the rack. A v32 career owned no gear and could not have —
// there was no trad in the game to place it on — so it loads with an empty
// harness, which is exactly what it had.
// v33 → v34: the logbook and what it made of you. A v33 career climbed
// without anybody counting, so it loads with an empty book and no quirks —
// which is not a loss, because the book only remembers a season anyway and
// a career that keeps climbing the way it has been will earn the same ones
// back inside two.
// v34 → v35: how well you ever knew somebody, which rapport now drifts
// down to a fraction of rather than to nothing.
//
// **The only migration in this file that has to read the save to write it.**
// Bonds are a counted list -- `bonds=N` and then `bond.0.*` -- so there is
// no fixed set of keys to add; how many there are depends on how many
// people this career ever climbed with. The honest reconstruction is that
// you knew them at least as well as you know them now, which is exactly
// what the runtime would derive on the next `BondsFrom` anyway.
// v42 -> v43: the people in the building. A v42 career could own a gym and
// run its books, and there was nobody in it -- so it loads with nobody's
// story started, which is exactly whose story had been started.
//
// **The cohort is the field worth thinking about.** Wave two is keyed to
// the set mix in force the day wave one is lived out, and a v42 save has
// not lived any of it out, so the honest answer is that it has not
// arrived: whatever mix is on the walls when the fourth arc closes is the
// room that fills, and that is a decision this migration must not make on
// the player's behalf.
void MigrateV42ToV43(SaveFields& fields) {
  for (int i = 0; i < kGymRegularCount; i++) {
    fields["floor.stage" + IntToStr(i)] = "0";
  }
  fields["floor.walk"] = "-1";
  fields["floor.comp"] = "-99";
  fields["floor.wave2"] = "0";
  fields["floor.wave2mix"] = "1";   // All-Comers, and unread until it lands
}

// v41 -> v42: the gym, pass two. A v41 career could own the building but
// its two staff were a pair of booleans, it had no wings and nothing ever
// went wrong in it.
//
// **The staff are the interesting half of this migration**, and the source
// names the answer: a save from before `GYM-3` "derives a plain
// 1.0-quality staffer on the base wage", so an existing gym's books do not
// move by a cent. The name is derived from the gym's, which means the
// person who has apparently been on that desk for two years finally gets
// one, rather than being hired this morning by a save loader.
void MigrateV41ToV42(SaveFields& fields) {
  const GymDials fresh;
  const auto named = fields.find("gym.name");
  const std::string gymName = named == fields.end() ? std::string() : named->second;
  int since = 0;
  ParseInt(fields, "gym.day", since);

  for (int role = 0; role < 2; role++) {
    const bool desk = role == 0;
    const std::string key = desk ? "gym.deskwho" : "gym.setwho";
    // The middle of the shortlist is the 1.0-quality trait in both tables,
    // which is what makes this a derivation and not a gift.
    const std::vector<GymStaffer> pool =
        GymCandidates(gymName, desk, 0, fresh);
    std::string who = "Sam";
    std::string trait;
    for (const GymStaffer& c : pool) {
      if (c.quality == 1.0) { who = c.name; trait = c.trait; break; }
    }
    fields[key + ".name"] = who;
    fields[key + ".trait"] = trait;
    fields[key + ".wage"] =
        NumToStr(desk ? fresh.frontDeskWage : fresh.setterWage);
    fields[key + ".quality"] = NumToStr(1.0);
    fields[key + ".hired"] = IntToStr(since);
    fields[key + ".ask"] = IntToStr(since + fresh.raiseDays);
    fields[key + ".refusals"] = "0";
  }

  for (int i = 0; i < kGymWingCount; i++) {
    fields["gym.wing" + IntToStr(i)] = "0";
  }
  fields["gym.passive"] = "0";
  fields["gym.incident"] = "0";
  fields["gym.incidentday"] = "0";
}

// v40 -> v41: where you park. A v40 career had one place to sleep and no
// city writing tickets, so it loads parked in the Lot with a clean record
// -- which is where it was and what it had.
void MigrateV40ToV41(SaveFields& fields) {
  fields["bivy.tonight"] = "0";
  for (int i = 0; i < kSpotCount; i++) fields["bivy.last" + IntToStr(i)] = "0";
  fields["bivy.lotnights"] = "0";
  fields["bivy.tickets"] = "0";
  fields["bivy.booted"] = "0";
}

// v39 -> v40: how you are living. A v39 career washed as often as it liked
// and never ran out of anything, so it loads at the same place a new one
// starts -- which is a fresh climber with most of a bottle, and true enough
// for a career that has never had jugs to empty.
void MigrateV39ToV40(SaveFields& fields) {
  const Living fresh;
  fields["live.grime"] = NumToStr(fresh.grime);
  fields["live.water"] = NumToStr(fresh.water);
  fields["live.propane"] = NumToStr(fresh.propane);
}

// v38 -> v39: the gym. A v38 career never had the option -- gym ownership
// was a recorded cut until 2026-08-24 -- so it loads owning nothing, which
// is exactly what it owned.
void MigrateV38ToV39(SaveFields& fields) {
  fields["gym.owned"] = "0";
  fields["gym.name"] = "";
  fields["gym.day"] = "0";
  fields["gym.price"] = "1";     // Standard, the shape a fresh Gym has
  fields["gym.mix"] = "1";       // All-Comers
  fields["gym.equip"] = "0";
  fields["gym.camp"] = "0";
  fields["gym.campuntil"] = "0";
  fields["gym.desk"] = "0";
  fields["gym.setter"] = "0";
  fields["gym.members"] = "0";
  fields["gym.balance"] = "0";
  fields["gym.debtdays"] = "0";
  fields["gym.tick"] = "0";
}

// v37 -> v38: what you went to bed with. A v37 career woke level every
// morning however it had lived, because hunger reset at dawn -- so it loads
// level, which is exactly the day it was having.
void MigrateV37ToV38(SaveFields& fields) {
  fields["hunger.carried"] = "0";
}

// v36 -> v37: the people behind the counters. A v36 career walked into a
// town with nobody in it, and loads into one with nobody in it -- for about
// a night, because the night tick opens the roster the same way it opens a
// circuit season. Nobody knows you yet, which is true: nobody did.
void MigrateV36ToV37(SaveFields& fields) {
  fields["loc.n"] = "0";
  fields["loc.titles"] = "0";
  fields["loc.tier"] = "0";
}

// v35 -> v36: the life outside it. A v35 career had nothing but climbing
// in it, and loads with nothing but climbing in it -- every thread not
// taken up, which is a truthful reconstruction rather than a loss: there
// was nobody to neglect.
void MigrateV35ToV36(SaveFields& fields) {
  for (int i = 1; i < kThreadCount; i++) {
    const std::string k = "life." + IntToStr(i) + ".";
    fields[k + "going"] = "0";
    fields[k + "warm"] = "0";
    fields[k + "deep"] = "0";
    fields[k + "last"] = "0";
    fields[k + "days"] = "0";
    fields[k + "ended"] = "0";
    fields[k + "endday"] = "0";
  }
  fields["life.grieving"] = "0";
  fields["life.griefw"] = "0";
  fields["life.lost"] = "0";
}

void MigrateV34ToV35(SaveFields& fields) {
  const auto found = fields.find("bonds");
  if (found == fields.end()) return;
  int count = 0;
  try {
    count = std::stoi(found->second);
  } catch (...) {
    // A save whose bond count is not a number is a save the loader is about
    // to reject anyway. Adding nothing here lets it reach the error it
    // deserves rather than turning it into a different one.
    return;
  }
  for (int i = 0; i < count; i++) {
    const std::string rapport = BondKey(i, "rapport");
    const auto had = fields.find(rapport);
    fields[BondKey(i, "knew")] = had == fields.end() ? "0" : had->second;
  }
}

void MigrateV33ToV34(SaveFields& fields) {
  fields["log.asof"] = "0";
  for (int i = 0; i < kDidCount; i++) {
    fields["log.n" + IntToStr(i)] = "0";
  }
  fields["log.burns"] = "0";
  fields["log.days"] = "0";
  fields["quirk.picked"] = "0";
  fields["quirk.n"] = "0";
  for (int i = 0; i < kHabitCount; i++) {
    fields["quirk.h" + IntToStr(i)] = "0";
  }
  fields["quirk.became"] = "0";
}

void MigrateV32ToV33(SaveFields& fields) {
  fields["rack.pieces"] = "0";
  fields["rack.quality"] = "0";
}

void MigrateV31ToV32(SaveFields& fields) {
  fields["craft.n"] = "0";
  fields["craft.taken"] = "0";
  fields["craft.botched"] = "0";
  fields["craft.ducked"] = "0";
  fields["craft.sackings"] = "0";
}

void MigrateV30ToV31(SaveFields& fields) {
  fields["sick.active"] = "0";
  fields["sick.days"] = "0";
  fields["sick.sev"] = "0";
  fields["sick.meds"] = "0";
  fields["sick.caught"] = "0";
  fields["teeth.stage"] = "0";
  fields["teeth.since"] = "0";
  fields["teeth.fixes"] = "0";
  fields["teeth.worst"] = "0";
  fields["teeth.lost"] = "0";
  fields["up.prehabday"] = "0";
  fields["up.streak"] = "0";
  fields["up.prehabdays"] = "0";
  fields["up.shrinkday"] = "0";
  fields["up.shrinks"] = "0";
}

void MigrateV29ToV30(SaveFields& fields) {
  fields["med.diagnosis"] = "0";
  fields["med.treatment"] = "0";
  fields["med.stage"] = "0";
  fields["med.stagestart"] = "0";
  fields["med.stagedays"] = "0";
  fields["med.told"] = "0";
  fields["med.joints"] = "0";
  fields["med.scars"] = "0";
  fields["med.insured"] = "0";
  fields["med.insuredon"] = "0";
  fields["med.premiums"] = "0";
  fields["med.claims"] = "0";
  fields["med.diagnoses"] = "0";
  fields["med.shots"] = "0";
  fields["med.surgeries"] = "0";
  fields["med.rushed"] = "0";
  fields["med.untreated"] = "0";
  fields["med.treated"] = "0";
  // A v29 injury was counted down by `BodyDay` and nothing else, which is
  // exactly what `staged == 0` means -- so the migration is the truth here
  // rather than a default, and the injury keeps ticking as it always did
  // until the medical tick takes it over.
  fields["med.staged"] = "0";
}

void MigrateV28ToV29(SaveFields& fields) {
  fields["league.next"] = "0";
  fields["league.block"] = "1";
  fields["league.weeks"] = "0";
  fields["league.you"] = "0";
  fields["league.best"] = "0";
  fields["league.bestday"] = "0";
  fields["league.nights"] = "0";
  fields["league.wins"] = "0";
  fields["league.lastnight"] = "0";
  fields["league.fields"] = "0";
}

void MigrateV27ToV28(SaveFields& fields) {
  fields["ranking"] = "0";
  fields["rank.results"] = "0";
  // The committee's clock, which did not exist while the review sat once a
  // domestic season. Zero means never, so a migrated career gets its next
  // review at the next season's close rather than waiting a year for a
  // committee that has been meeting all along.
  fields["team.lastday"] = "0";
}

void MigrateV26ToV27(SaveFields& fields) {
  fields["wc.season"] = "0";
  fields["wc.you"] = "0";
  fields["wc.closed"] = "0";
  fields["wc.starts"] = "0";
  fields["wc.missed"] = "0";
  fields["wc.finals"] = "0";
  fields["wc.podiums"] = "0";
  fields["wc.wins"] = "0";
  fields["wc.titles"] = "0";
  fields["wc.best"] = "0";
  fields["wc.last"] = "0";
  fields["wc.rounds"] = "0";
  fields["wc.fields"] = "0";
  fields["og.next"] = "0";
  fields["og.appearances"] = "0";
  fields["og.gold"] = "0";
  fields["og.silver"] = "0";
  fields["og.bronze"] = "0";
  // Not "0". `lastCompeted` at zero means "you have climbed the Games held
  // on day zero", and a migration that quietly says so is an off-by-one
  // nobody notices until a Games will not open.
  fields["og.last"] = "-1";
}

void MigrateV25ToV26(SaveFields& fields) {
  fields["team.status"] = "0";
  fields["team.ever"] = "0";
  fields["team.seasons"] = "0";
  fields["team.cuts"] = "0";
  fields["team.namedday"] = "0";
  fields["team.coach"] = "";
  fields["team.coachfor"] = "";
  fields["team.passed"] = "";
  fields["team.lastpts"] = "0";
  fields["team.lastseason"] = "0";
  fields["team.mates"] = "0";
  fields["team.gone"] = "0";
}

void MigrateV24ToV25(SaveFields& fields) {
  fields["circuit.season"] = "0";
  fields["circuit.done"] = "0";
  fields["circuit.you"] = "0";
  fields["circuit.rival"] = "0";
  fields["circuit.titles"] = "0";
  fields["circuit.dates"] = "0";
  fields["circuit.fields"] = "0";
}

void MigrateV22ToV23(SaveFields& fields) {
  fields["rival.race"] = "";
  fields["rival.raceby"] = "0";
  fields["rival.racefa"] = "0";
}

void MigrateV21ToV22(SaveFields& fields) {
  fields["rival.name"] = "";
  fields["rival.style"] = "0";
  fields["rival.vibe"] = "0";
  fields["rival.gen"] = "0";
  fields["rival.born"] = "1";
  fields["rival.startage"] = "26";
  fields["rival.grade"] = "0";
  fields["rival.laststep"] = "0";
  fields["rival.peak"] = "0";
  fields["rival.rivalry"] = "0";
  fields["rival.allied"] = "0";
  fields["rival.offered"] = "0";
  fields["rival.met"] = "0";
  fields["rival.retired"] = "0";
  fields["rival.fas"] = "0";
  fields["pastrivals"] = "0";
}

void MigrateV20ToV21(SaveFields& fields) {
  fields["char.built"] = "0";
  fields["char.archetype"] = "0";
  fields["char.origin"] = "0";
  fields["char.flaw"] = "0";
  fields["char.temperament"] = "0";
  fields["char.discipline"] = "0";
  fields["char.boldness"] = "0";
  fields["char.social"] = "0";
  fields["char.purism"] = "0";
  fields["char.gift"] = "0";
  fields["char.anti"] = "0";
  fields["char.giftKnown"] = "0";
  fields["char.antiKnown"] = "0";
  fields["char.reps0"] = "0";
  fields["char.reps1"] = "0";
  fields["char.reps2"] = "0";
  fields["char.reps3"] = "0";
  fields["char.reps4"] = "0";
  fields["char.startingCash"] = "0";
  fields["char.agePlus"] = "0";
}

// v6 → v7: what you owe. A v6 career could not owe anything, because there
// was nowhere to owe it — the number was simply missing from cash.
void MigrateV6ToV7(SaveFields& fields) { fields["owed"] = "0"; }

// v4 → v5: the dog. A v4 career never met it, so it migrates to exactly the
// stray a new career finds at the Lot: nobody's, unbonded, and hungry.
void MigrateV4ToV5(SaveFields& fields) {
  fields["dog.name"] = "the dog";
  fields["dog.adopted"] = "0";
  fields["dog.bond"] = "0";
  fields["dog.fed"] = "0.4";
}

}  // namespace

const std::vector<Migration>& DefaultMigrations() {
  static const std::vector<Migration> kMigrations = {
      &MigrateV1ToV2, &MigrateV2ToV3, &MigrateV3ToV4, &MigrateV4ToV5,
      &MigrateV5ToV6, &MigrateV6ToV7, &MigrateV7ToV8, &MigrateV8ToV9,
      &MigrateV9ToV10, &MigrateV10ToV11, &MigrateV11ToV12,
      &MigrateV12ToV13, &MigrateV13ToV14, &MigrateV14ToV15,
      &MigrateV15ToV16, &MigrateV16ToV17, &MigrateV17ToV18,
      &MigrateV18ToV19, &MigrateV19ToV20, &MigrateV20ToV21,
      &MigrateV21ToV22, &MigrateV22ToV23, &MigrateV23ToV24,
      &MigrateV24ToV25, &MigrateV25ToV26, &MigrateV26ToV27,
      &MigrateV27ToV28, &MigrateV28ToV29, &MigrateV29ToV30,
      &MigrateV30ToV31, &MigrateV31ToV32, &MigrateV32ToV33,
      &MigrateV33ToV34, &MigrateV34ToV35, &MigrateV35ToV36,
      &MigrateV36ToV37, &MigrateV37ToV38, &MigrateV38ToV39,
      &MigrateV39ToV40, &MigrateV40ToV41, &MigrateV41ToV42,
      &MigrateV42ToV43};
  return kMigrations;
}

bool ApplyMigrations(SaveFields& fields, int fromVersion, int toVersion,
                     const std::vector<Migration>& migrations) {
  for (int v = fromVersion; v < toVersion; v++) {
    const int index = v - 1;
    if (index < 0 || index >= static_cast<int>(migrations.size())) return false;
    migrations[index](fields);
  }
  return true;
}

std::string SerializeSave(const SaveGame& save) {
  std::ostringstream out;
  out << "version=" << save.version << "\n";
  out << "seed=" << save.seed << "\n";
  // The name is the player's own text, and the format is line-oriented: a
  // newline in it would shear the file in half. '=' is fine -- the parser
  // splits on the first one -- so only line breaks are stripped.
  std::string safeName = save.player.name;
  safeName.erase(std::remove(safeName.begin(), safeName.end(), '\n'),
                 safeName.end());
  safeName.erase(std::remove(safeName.begin(), safeName.end(), '\r'),
                 safeName.end());
  out << "player.name=" << safeName << "\n";
  out << "day=" << save.player.day << "\n";
  out << "cash=" << NumToStr(save.player.cash) << "\n";

  const Climber& c = save.player.climber;
  out << "skills.power=" << NumToStr(c.skills.power) << "\n";
  out << "skills.fingers=" << NumToStr(c.skills.fingers) << "\n";
  out << "skills.technique=" << NumToStr(c.skills.technique) << "\n";
  out << "skills.endurance=" << NumToStr(c.skills.endurance) << "\n";
  out << "skills.head=" << NumToStr(c.skills.head) << "\n";
  out << "morphology=" << IntToStr(static_cast<int>(c.morphology)) << "\n";
  out << "skin=" << NumToStr(c.skin) << "\n";
  out << "psyche=" << NumToStr(c.psyche) << "\n";

  out << "projects=" << static_cast<int>(save.player.projects.size()) << "\n";
  for (size_t i = 0; i < save.player.projects.size(); i++) {
    const ProjectMemory& m = save.player.projects[i];
    const int n = static_cast<int>(i);
    out << ProjKey(n, "name") << "=" << m.routeName << "\n";
    out << ProjKey(n, "grade") << "=" << IntToStr(m.grade) << "\n";
    out << ProjKey(n, "attempts") << "=" << IntToStr(m.attempts) << "\n";
    out << ProjKey(n, "best") << "=" << IntToStr(m.bestHighpoint) << "\n";
    out << ProjKey(n, "beta") << "=" << NumToStr(m.beta) << "\n";
    out << ProjKey(n, "sent") << "=" << (m.sent ? "1" : "0") << "\n";
    out << ProjKey(n, "clean") << "=" << NumToStr(m.cleanliness) << "\n";
    out << ProjKey(n, "given") << "=" << m.givenName << "\n";
    out << ProjKey(n, "fa") << "=" << (m.firstAscent ? "1" : "0") << "\n";
    out << ProjKey(n, "confirmed") << "=" << IntToStr(m.confirmedGrade) << "\n";
    out << ProjKey(n, "style") << "=" << IntToStr(static_cast<int>(m.firstSendStyle))
        << "\n";
    out << ProjKey(n, "disc") << "=" << IntToStr(static_cast<int>(m.discipline))
        << "\n";
  }

  // The ones that came before. Their first ascents are the only reason the
  // guidebook says anything at all about who put a line up.
  out << "legacies=" << IntToStr(static_cast<int>(save.legacies.size()))
      << "\n";
  for (size_t i = 0; i < save.legacies.size(); i++) {
    const Legacy& l = save.legacies[i];
    const std::string k = "legacy." + IntToStr(static_cast<int>(i)) + ".";
    out << k << "name=" << l.name << "\n";
    out << k << "seasons=" << IntToStr(l.seasons) << "\n";
    out << k << "retiredat=" << NumToStr(l.retiredAt) << "\n";
    out << k << "hardest=" << IntToStr(l.hardestSendGrade) << "\n";
    out << k << "hardestname=" << l.hardestSendName << "\n";
    out << k << "hardestdisc="
        << IntToStr(static_cast<int>(l.hardestSendDiscipline)) << "\n";
    out << k << "sends=" << IntToStr(l.totalSends) << "\n";
    out << k << "attempts=" << IntToStr(l.totalAttempts) << "\n";
    out << k << "nemesis=" << l.nemesis << "\n";
    out << k << "nemesisattempts=" << IntToStr(l.nemesisAttempts) << "\n";
    out << k << "open=" << (l.cragOpenAtTheEnd ? "1" : "0") << "\n";
    out << k << "fas=" << IntToStr(static_cast<int>(l.firstAscents.size()))
        << "\n";
    for (size_t j = 0; j < l.firstAscents.size(); j++) {
      const NamedLine& n = l.firstAscents[j];
      const std::string fk = k + "fa." + IntToStr(static_cast<int>(j)) + ".";
      out << fk << "key=" << n.routeKey << "\n";
      out << fk << "given=" << n.givenName << "\n";
      out << fk << "grade=" << IntToStr(n.confirmedGrade) << "\n";
      out << fk << "style=" << IntToStr(static_cast<int>(n.style)) << "\n";
      out << fk << "disc=" << IntToStr(static_cast<int>(n.discipline)) << "\n";
    }
  }

  out << "secrets=" << IntToStr(static_cast<int>(save.player.secrets.size()))
      << "\n";
  for (size_t i = 0; i < save.player.secrets.size(); i++) {
    const Secret& s = save.player.secrets[i];
    const std::string k = "secret." + IntToStr(static_cast<int>(i)) + ".";
    out << k << "act=" << IntToStr(static_cast<int>(s.act)) << "\n";
    out << k << "route=" << s.routeKey << "\n";
    out << k << "done=" << IntToStr(s.dayDone) << "\n";
    out << k << "known=" << (s.known ? "1" : "0") << "\n";
    out << k << "found=" << IntToStr(s.dayFound) << "\n";
  }

  out << "sponsor.tier="
      << IntToStr(static_cast<int>(save.player.sponsor.tier)) << "\n";
  out << "sponsor.seasons=" << IntToStr(save.player.sponsor.seasonsHeld)
      << "\n";
  out << "sponsor.lastgrade="
      << IntToStr(save.player.sponsor.gradeAtLastReview) << "\n";
  out << "sponsor.stale="
      << IntToStr(save.player.sponsor.seasonsWithoutProgress) << "\n";
  out << "sponsor.hurtdays="
      << IntToStr(save.player.sponsor.daysHurtThisSeason) << "\n";

  out << "owed=" << NumToStr(save.player.owed) << "\n";
  for (int i = 0; i < kFactionCount; i++) {
    out << "standing." << IntToStr(i) << "="
        << NumToStr(save.player.standing.with[i]) << "\n";
  }
  out << "load=" << NumToStr(save.player.climber.load) << "\n";
  // Somebody to beat, and the ones who came before. The name goes first
  // because it is the field that says whether there is anybody at all --
  // `SleepToNextDay` skips an empty one, which is what makes a v21 career
  // load unchanged.
  {
    const Rival& rv = save.player.rival;
    out << "rival.name=" << rv.name << "\n";
    out << "rival.style=" << IntToStr(static_cast<int>(rv.style)) << "\n";
    out << "rival.vibe=" << IntToStr(static_cast<int>(rv.vibe)) << "\n";
    out << "rival.gen=" << IntToStr(rv.generation) << "\n";
    out << "rival.born=" << IntToStr(rv.bornOnDay) << "\n";
    out << "rival.startage=" << NumToStr(rv.startAge) << "\n";
    out << "rival.grade=" << NumToStr(rv.grade) << "\n";
    out << "rival.laststep=" << IntToStr(rv.lastStepDay) << "\n";
    out << "rival.peak=" << NumToStr(rv.peakGrade) << "\n";
    out << "rival.rivalry=" << NumToStr(rv.rivalry) << "\n";
    out << "rival.allied=" << IntToStr(rv.allied ? 1 : 0) << "\n";
    out << "rival.offered=" << IntToStr(rv.offered ? 1 : 0) << "\n";
    out << "rival.met=" << IntToStr(rv.met ? 1 : 0) << "\n";
    out << "rival.retired=" << IntToStr(rv.retired ? 1 : 0) << "\n";
    out << "ranking=" << NumToStr(save.player.rankingPoints) << "\n";
    {
      const Circuit& ci = save.player.circuit;
      out << "circuit.season=" << IntToStr(ci.season) << "\n";
      out << "circuit.done=" << IntToStr(ci.compsDone) << "\n";
      out << "circuit.you=" << NumToStr(ci.yourPoints) << "\n";
      out << "circuit.rival=" << NumToStr(ci.rivalPoints) << "\n";
      out << "circuit.titles=" << IntToStr(ci.titles) << "\n";
      out << "circuit.dates="
          << IntToStr(static_cast<int>(ci.schedule.size())) << "\n";
      for (std::size_t i = 0; i < ci.schedule.size(); i++) {
        out << "circuit.date" << IntToStr(static_cast<int>(i)) << "="
            << IntToStr(ci.schedule[i]) << "\n";
      }
      const NationalTeam& tm = save.player.team;
      out << "team.status=" << IntToStr(static_cast<int>(tm.status)) << "\n";
      out << "team.ever=" << IntToStr(tm.everNamed ? 1 : 0) << "\n";
      out << "team.seasons=" << IntToStr(tm.seasons) << "\n";
      out << "team.cuts=" << IntToStr(tm.cuts) << "\n";
      out << "team.namedday=" << IntToStr(tm.namedOnDay) << "\n";
      out << "team.coach=" << tm.coach << "\n";
      out << "team.coachfor=" << tm.coachKnownFor << "\n";
      out << "team.passed=" << tm.passed << "\n";
      out << "team.lastpts=" << NumToStr(tm.lastReviewPoints) << "\n";
      out << "team.lastseason=" << IntToStr(tm.lastReviewSeason) << "\n";
      out << "team.lastday=" << IntToStr(tm.lastReviewDay) << "\n";
      out << "team.mates=" << IntToStr(static_cast<int>(tm.roster.size()))
          << "\n";
      for (std::size_t i = 0; i < tm.roster.size(); i++) {
        const std::string k = "team.mate" + IntToStr(static_cast<int>(i));
        out << k << "n=" << tm.roster[i].name << "\n";
        out << k << "r=" << tm.roster[i].role << "\n";
        out << k << "p=" << NumToStr(tm.roster[i].points) << "\n";
      }
      out << "team.gone=" << IntToStr(static_cast<int>(tm.gone.size()))
          << "\n";
      for (std::size_t i = 0; i < tm.gone.size(); i++) {
        out << "team.gone" << IntToStr(static_cast<int>(i)) << "="
            << tm.gone[i] << "\n";
      }
      out << "circuit.fields="
          << IntToStr(static_cast<int>(ci.fieldPoints.size())) << "\n";
      for (std::size_t i = 0; i < ci.fieldPoints.size(); i++) {
        out << "circuit.field" << IntToStr(static_cast<int>(i)) << "="
            << NumToStr(ci.fieldPoints[i]) << "\n";
      }

      const WorldCupSeason& wc = save.player.worldCup;
      out << "wc.season=" << IntToStr(wc.season) << "\n";
      out << "wc.you=" << NumToStr(wc.yourPoints) << "\n";
      out << "wc.closed=" << IntToStr(wc.closed ? 1 : 0) << "\n";
      out << "wc.starts=" << IntToStr(wc.starts) << "\n";
      out << "wc.missed=" << IntToStr(wc.missed) << "\n";
      out << "wc.finals=" << IntToStr(wc.finals) << "\n";
      out << "wc.podiums=" << IntToStr(wc.podiums) << "\n";
      out << "wc.wins=" << IntToStr(wc.wins) << "\n";
      out << "wc.titles=" << IntToStr(wc.titles) << "\n";
      out << "wc.best=" << IntToStr(wc.bestRank) << "\n";
      out << "wc.last=" << IntToStr(wc.lastRank) << "\n";
      out << "wc.rounds=" << IntToStr(static_cast<int>(wc.schedule.size()))
          << "\n";
      for (std::size_t i = 0; i < wc.schedule.size(); i++) {
        const std::string k = "wc.round" + IntToStr(static_cast<int>(i));
        out << k << "d=" << IntToStr(wc.schedule[i].day) << "\n";
        out << k << "v=" << IntToStr(wc.schedule[i].venue) << "\n";
        out << k << "r=" << IntToStr(wc.schedule[i].resolved ? 1 : 0) << "\n";
        out << k << "f=" << IntToStr(wc.schedule[i].flown ? 1 : 0) << "\n";
      }
      out << "wc.fields="
          << IntToStr(static_cast<int>(wc.fieldPoints.size())) << "\n";
      for (std::size_t i = 0; i < wc.fieldPoints.size(); i++) {
        out << "wc.field" << IntToStr(static_cast<int>(i)) << "="
            << NumToStr(wc.fieldPoints[i]) << "\n";
      }

      // The ranking itself is written too, and it is derived -- the night
      // tick recomputes it from the record every morning. It is in the
      // file so a save loaded and read before the first sleep says the
      // right thing rather than zero.
      const std::vector<RankingResult>& rr = save.player.rankingRecord;
      out << "rank.results=" << IntToStr(static_cast<int>(rr.size())) << "\n";
      for (std::size_t i = 0; i < rr.size(); i++) {
        const std::string k = "rank.r" + IntToStr(static_cast<int>(i));
        out << k << "d=" << IntToStr(rr[i].day) << "\n";
        out << k << "p=" << NumToStr(rr[i].points) << "\n";
      }

      const Medical& mm = save.player.medical;
      out << "med.diagnosis=" << IntToStr(static_cast<int>(mm.diagnosis))
          << "\n";
      out << "med.treatment=" << IntToStr(static_cast<int>(mm.treatment))
          << "\n";
      out << "med.stage=" << IntToStr(static_cast<int>(mm.stage)) << "\n";
      out << "med.stagestart=" << IntToStr(mm.stageStarted) << "\n";
      out << "med.stagedays=" << IntToStr(mm.stageDays) << "\n";
      out << "med.told=" << NumToStr(mm.toldSeverity) << "\n";
      out << "med.joints=" << IntToStr(kInjuryKindCount) << "\n";
      for (int i = 0; i < kInjuryKindCount; i++) {
        out << "med.joint" << IntToStr(i) << "=" << NumToStr(mm.joints[i])
            << "\n";
        out << "med.shot" << IntToStr(i) << "=" << IntToStr(mm.shots[i])
            << "\n";
      }
      out << "med.scars=" << IntToStr(static_cast<int>(mm.scars.size()))
          << "\n";
      for (std::size_t i = 0; i < mm.scars.size(); i++) {
        const std::string k = "med.scar" + IntToStr(static_cast<int>(i));
        out << k << "k=" << IntToStr(static_cast<int>(mm.scars[i].kind))
            << "\n";
        out << k << "w=" << NumToStr(mm.scars[i].weight) << "\n";
        out << k << "d=" << IntToStr(mm.scars[i].fromDay) << "\n";
      }
      out << "med.insured=" << IntToStr(mm.insured ? 1 : 0) << "\n";
      out << "med.insuredon=" << IntToStr(mm.insuredOnDay) << "\n";
      out << "med.premiums=" << NumToStr(mm.premiumsPaid) << "\n";
      out << "med.claims=" << NumToStr(mm.claimsPaid) << "\n";
      out << "med.diagnoses=" << IntToStr(mm.diagnoses) << "\n";
      out << "med.shots=" << IntToStr(mm.shotsTaken) << "\n";
      out << "med.surgeries=" << IntToStr(mm.surgeries) << "\n";
      out << "med.rushed=" << IntToStr(mm.rushedComebacks) << "\n";
      out << "med.untreated=" << IntToStr(mm.untreatedInjuries) << "\n";
      out << "med.treated=" << IntToStr(mm.treatedThisTime ? 1 : 0) << "\n";
      // **Who owns the injury clock**, and losing it is not cosmetic: a
      // loaded save would hand a staged comeback back to `BodyDay`, which
      // would count it down underneath the stages and clear it early.
      out << "med.staged="
          << IntToStr(save.player.climber.injury.staged ? 1 : 0) << "\n";

      const Craftsman& hd = save.player.hand;
      out << "craft.n=" << IntToStr(kCraftCount) << "\n";
      for (int i = 0; i < kCraftCount; i++) {
        const std::string k = "craft" + IntToStr(i);
        out << k << "s=" << NumToStr(hd.skill[i]) << "\n";
        out << k << "r=" << NumToStr(hd.standing[i]) << "\n";
        out << k << "n=" << IntToStr(hd.shifts[i]) << "\n";
        out << k << "x=" << IntToStr(hd.sacked[i] ? 1 : 0) << "\n";
      }
      out << "craft.taken=" << IntToStr(hd.momentsTaken) << "\n";
      out << "craft.botched=" << IntToStr(hd.momentsBotched) << "\n";
      out << "craft.ducked=" << IntToStr(hd.momentsDucked) << "\n";
      out << "craft.sackings=" << IntToStr(hd.sackings) << "\n";

      const Sickness& sk = save.player.sickness;
      out << "sick.active=" << IntToStr(sk.active ? 1 : 0) << "\n";
      out << "sick.days=" << IntToStr(sk.daysLeft) << "\n";
      out << "sick.sev=" << NumToStr(sk.severity) << "\n";
      out << "sick.meds=" << IntToStr(sk.medicated ? 1 : 0) << "\n";
      out << "sick.caught=" << IntToStr(sk.caught) << "\n";

      const Teeth& th = save.player.teeth;
      out << "teeth.stage=" << IntToStr(static_cast<int>(th.stage)) << "\n";
      out << "teeth.since=" << IntToStr(th.sinceDay) << "\n";
      out << "teeth.fixes=" << IntToStr(th.fixes) << "\n";
      out << "teeth.worst=" << IntToStr(th.worstEver) << "\n";
      out << "teeth.lost=" << IntToStr(th.lost) << "\n";

      const Upkeep& up = save.player.upkeep;
      out << "up.prehabday=" << IntToStr(up.lastPrehabDay) << "\n";
      out << "up.streak=" << IntToStr(up.prehabStreak) << "\n";
      out << "up.prehabdays=" << IntToStr(up.prehabDays) << "\n";
      out << "up.shrinkday=" << IntToStr(up.lastShrinkDay) << "\n";
      out << "up.shrinks=" << IntToStr(up.shrinkSessions) << "\n";

      const League& lg = save.player.league;
      out << "league.next=" << IntToStr(lg.nextNight) << "\n";
      out << "league.block=" << IntToStr(lg.block) << "\n";
      out << "league.weeks=" << IntToStr(lg.weeksDone) << "\n";
      out << "league.you=" << NumToStr(lg.yourPoints) << "\n";
      out << "league.best=" << NumToStr(lg.best) << "\n";
      out << "league.bestday=" << IntToStr(lg.bestOnDay) << "\n";
      out << "league.nights=" << IntToStr(lg.nights) << "\n";
      out << "league.wins=" << IntToStr(lg.blockWins) << "\n";
      out << "league.lastnight=" << IntToStr(lg.lastClimbedNight) << "\n";
      out << "league.fields="
          << IntToStr(static_cast<int>(lg.fieldPoints.size())) << "\n";
      for (std::size_t i = 0; i < lg.fieldPoints.size(); i++) {
        out << "league.field" << IntToStr(static_cast<int>(i)) << "="
            << NumToStr(lg.fieldPoints[i]) << "\n";
      }

      const Olympics& og = save.player.olympics;
      out << "og.next=" << IntToStr(og.nextDay) << "\n";
      out << "og.appearances=" << IntToStr(og.appearances) << "\n";
      out << "og.gold=" << IntToStr(og.gold) << "\n";
      out << "og.silver=" << IntToStr(og.silver) << "\n";
      out << "og.bronze=" << IntToStr(og.bronze) << "\n";
      out << "og.last=" << IntToStr(og.lastCompeted) << "\n";
    }
    out << "rival.race=" << rv.race.routeName << "\n";
    out << "rival.raceby=" << IntToStr(rv.race.byDay) << "\n";
    out << "rival.racefa=" << IntToStr(rv.race.forFirstAscent ? 1 : 0)
        << "\n";
    out << "rival.fas=" << IntToStr(static_cast<int>(rv.firstAscents.size()))
        << "\n";
    for (std::size_t i = 0; i < rv.firstAscents.size(); i++) {
      out << "rival.fa" << IntToStr(static_cast<int>(i)) << "="
          << rv.firstAscents[i] << "\n";
    }
    out << "pastrivals=" << IntToStr(static_cast<int>(
                                save.player.pastRivals.size()))
        << "\n";
    for (std::size_t i = 0; i < save.player.pastRivals.size(); i++) {
      const PastRival& p = save.player.pastRivals[i];
      const std::string k = "pastrival" + IntToStr(static_cast<int>(i)) + ".";
      out << k << "name=" << p.name << "\n";
      out << k << "role=" << IntToStr(static_cast<int>(p.role)) << "\n";
      out << k << "style=" << IntToStr(static_cast<int>(p.style)) << "\n";
      out << k << "day=" << IntToStr(p.retiredOnDay) << "\n";
      out << k << "age=" << NumToStr(p.age) << "\n";
      out << k << "peak=" << NumToStr(p.peakGrade) << "\n";
      out << k << "gen=" << IntToStr(p.generation) << "\n";
    }
  }

  // Who you are. `built` first, because it is the field the loader has to
  // believe before any of the others mean anything.
  {
    const Character& ch = save.player.character;
    out << "char.built=" << IntToStr(ch.built ? 1 : 0) << "\n";
    out << "char.archetype=" << IntToStr(static_cast<int>(ch.build.archetype))
        << "\n";
    out << "char.origin=" << IntToStr(static_cast<int>(ch.build.origin))
        << "\n";
    out << "char.flaw=" << IntToStr(static_cast<int>(ch.build.flaw)) << "\n";
    out << "char.temperament="
        << IntToStr(static_cast<int>(ch.build.temperament)) << "\n";
    out << "char.discipline=" << NumToStr(ch.personality.discipline) << "\n";
    out << "char.boldness=" << NumToStr(ch.personality.boldness) << "\n";
    out << "char.social=" << NumToStr(ch.personality.social) << "\n";
    out << "char.purism=" << NumToStr(ch.personality.purism) << "\n";
    out << "char.gift=" << IntToStr(static_cast<int>(ch.gift)) << "\n";
    out << "char.anti=" << IntToStr(static_cast<int>(ch.antiTalent)) << "\n";
    out << "char.giftKnown=" << IntToStr(ch.giftKnown ? 1 : 0) << "\n";
    out << "char.antiKnown=" << IntToStr(ch.antiKnown ? 1 : 0) << "\n";
    for (int i = 0; i < kSkillCount; i++) {
      out << "char.reps" << IntToStr(i) << "=" << NumToStr(ch.reps[i])
          << "\n";
    }
    out << "char.startingCash=" << NumToStr(ch.startingCash) << "\n";
    out << "char.agePlus=" << IntToStr(ch.agePlus) << "\n";
  }
  out << "injury.active="
      << IntToStr(save.player.climber.injury.active ? 1 : 0) << "\n";
  out << "injury.kind="
      << IntToStr(static_cast<int>(save.player.climber.injury.kind)) << "\n";
  out << "injury.severity=" << NumToStr(save.player.climber.injury.severity)
      << "\n";
  out << "injury.days=" << IntToStr(save.player.climber.injury.daysLeft)
      << "\n";
  out << "physio.last=" << IntToStr(save.player.lastPhysioDay) << "\n";
  // The logbook, and what it has made of you so far.
  out << "log.asof=" << IntToStr(save.player.logbook.asOfDay) << "\n";
  for (int i = 0; i < kDidCount; i++) {
    out << "log.n" << IntToStr(i) << "="
        << NumToStr(save.player.logbook.count[i]) << "\n";
  }
  out << "log.burns=" << NumToStr(save.player.logbook.lifetimeBurns) << "\n";
  out << "log.days=" << NumToStr(save.player.logbook.lifetimeDays) << "\n";
  out << "quirk.picked=" << IntToStr(static_cast<int>(save.player.quirks.picked))
      << "\n";
  out << "quirk.n="
      << IntToStr(static_cast<int>(save.player.quirks.held.size())) << "\n";
  for (std::size_t i = 0; i < save.player.quirks.held.size(); i++) {
    out << "quirk." << IntToStr(static_cast<int>(i)) << "="
        << IntToStr(static_cast<int>(save.player.quirks.held[i])) << "\n";
  }
  for (int i = 0; i < kHabitCount; i++) {
    out << "quirk.h" << IntToStr(i) << "="
        << NumToStr(save.player.quirks.heldFor[i]) << "\n";
  }
  out << "quirk.became=" << IntToStr(static_cast<int>(save.player.becameToday))
      << "\n";

  // The life outside it: five threads, and what an ending is still costing.
  for (int i = 1; i < kThreadCount; i++) {
    const Strand& s = save.player.life.strands[i];
    const std::string k = "life." + IntToStr(i) + ".";
    out << k << "going=" << IntToStr(s.going ? 1 : 0) << "\n";
    out << k << "warm=" << NumToStr(s.warmth) << "\n";
    out << k << "deep=" << NumToStr(s.depth) << "\n";
    out << k << "last=" << IntToStr(s.lastGivenDay) << "\n";
    out << k << "days=" << IntToStr(s.daysGiven) << "\n";
    out << k << "ended=" << IntToStr(s.ended ? 1 : 0) << "\n";
    out << k << "endday=" << IntToStr(s.endedDay) << "\n";
  }
  out << "life.grieving=" << NumToStr(save.player.life.grieving) << "\n";
  out << "life.griefw=" << NumToStr(save.player.life.griefWeight) << "\n";
  out << "life.lost=" << IntToStr(static_cast<int>(save.player.lostToday))
      << "\n";

  // The people behind the counters, and what they are holding. A counted
  // list, because how many there are is a fact about the town's venues.
  out << "loc.n="
      << IntToStr(static_cast<int>(save.player.locals.people.size())) << "\n";
  for (std::size_t i = 0; i < save.player.locals.people.size(); i++) {
    const Local& p = save.player.locals.people[i];
    const std::string k = "loc." + IntToStr(static_cast<int>(i)) + ".";
    out << k << "name=" << p.name << "\n";
    out << k << "where=" << IntToStr(static_cast<int>(p.where)) << "\n";
    out << k << "known=" << NumToStr(p.known) << "\n";
    out << k << "knew=" << NumToStr(p.everKnew) << "\n";
    out << k << "holds=" << IntToStr(static_cast<int>(p.holds)) << "\n";
    out << k << "about=" << p.about << "\n";
    out << k << "seen=" << IntToStr(p.lastSeen) << "\n";
  }
  out << "bivy.tonight=" << IntToStr(static_cast<int>(save.player.bivy.tonight))
      << "\n";
  for (int i = 0; i < kSpotCount; i++) {
    out << "bivy.last" << IntToStr(i) << "="
        << IntToStr(save.player.bivy.lastSlept[i]) << "\n";
  }
  out << "bivy.lotnights=" << IntToStr(save.player.bivy.lotNights) << "\n";
  out << "bivy.tickets=" << IntToStr(save.player.bivy.ticketsOwed) << "\n";
  out << "bivy.booted=" << IntToStr(save.player.bivy.booted ? 1 : 0) << "\n";

  out << "live.grime=" << NumToStr(save.player.living.grime) << "\n";
  out << "live.water=" << NumToStr(save.player.living.water) << "\n";
  out << "live.propane=" << NumToStr(save.player.living.propane) << "\n";

  // The gym. A career mostly has none, and `owned` is what says so.
  out << "gym.owned=" << IntToStr(save.player.gym.owned ? 1 : 0) << "\n";
  out << "gym.name=" << save.player.gym.name << "\n";
  out << "gym.day=" << IntToStr(save.player.gym.ownedDay) << "\n";
  out << "gym.price=" << IntToStr(static_cast<int>(save.player.gym.price))
      << "\n";
  out << "gym.mix=" << IntToStr(static_cast<int>(save.player.gym.mix)) << "\n";
  out << "gym.equip=" << IntToStr(static_cast<int>(save.player.gym.equip))
      << "\n";
  out << "gym.camp=" << IntToStr(static_cast<int>(save.player.gym.campaign))
      << "\n";
  out << "gym.campuntil=" << IntToStr(save.player.gym.campaignUntil) << "\n";
  out << "gym.desk=" << IntToStr(save.player.gym.frontDesk ? 1 : 0) << "\n";
  out << "gym.setter=" << IntToStr(save.player.gym.setter ? 1 : 0) << "\n";
  out << "gym.members=" << NumToStr(save.player.gym.members) << "\n";
  out << "gym.balance=" << NumToStr(save.player.gym.balance) << "\n";
  out << "gym.debtdays=" << IntToStr(save.player.gym.debtDays) << "\n";
  out << "gym.tick=" << IntToStr(save.player.gym.lastTickDay) << "\n";
  // GYM-3: and the two people in it. Written whether or not the seat is
  // filled, because an empty seat's record is a well-defined blank and a
  // conditional field is a parse branch waiting to go wrong.
  for (int role = 0; role < 2; role++) {
    const GymStaffer& who =
        role == 0 ? save.player.gym.desk : save.player.gym.routesetter;
    const std::string k = role == 0 ? "gym.deskwho." : "gym.setwho.";
    out << k << "name=" << who.name << "\n";
    out << k << "trait=" << who.trait << "\n";
    out << k << "wage=" << NumToStr(who.wage) << "\n";
    out << k << "quality=" << NumToStr(who.quality) << "\n";
    out << k << "hired=" << IntToStr(who.hiredDay) << "\n";
    out << k << "ask=" << IntToStr(who.nextAskDay) << "\n";
    out << k << "refusals=" << IntToStr(who.refusals) << "\n";
  }
  for (int i = 0; i < kGymWingCount; i++) {
    out << "gym.wing" << IntToStr(i) << "="
        << IntToStr(save.player.gym.wings[i] ? 1 : 0) << "\n";
  }
  out << "gym.passive=" << IntToStr(save.player.gym.passive ? 1 : 0) << "\n";
  out << "gym.incident="
      << IntToStr(static_cast<int>(save.player.gym.incident)) << "\n";
  out << "gym.incidentday=" << IntToStr(save.player.gym.incidentDay) << "\n";

  // GYM-2/GYM-5: the floor's memory. Beside the gym, not inside it.
  for (int i = 0; i < kGymRegularCount; i++) {
    out << "floor.stage" << IntToStr(i) << "="
        << IntToStr(save.player.floor.stage[i]) << "\n";
  }
  out << "floor.walk=" << IntToStr(save.player.floor.lastWalkDay) << "\n";
  out << "floor.comp=" << IntToStr(save.player.floor.lastCompDay) << "\n";
  out << "floor.wave2=" << IntToStr(save.player.floor.waveTwoArrived ? 1 : 0)
      << "\n";
  out << "floor.wave2mix="
      << IntToStr(static_cast<int>(save.player.floor.waveTwo)) << "\n";

  out << "hunger.carried=" << NumToStr(save.player.hungerCarried) << "\n";
  out << "loc.titles=" << IntToStr(save.player.locals.knownTitles) << "\n";
  out << "loc.tier=" << IntToStr(save.player.locals.knownTier) << "\n";

  out << "rack.pieces=" << IntToStr(save.player.rack.pieces) << "\n";
  out << "rack.quality=" << NumToStr(save.player.rack.quality) << "\n";
  out << "kit.pads=" << IntToStr(save.player.kit.pads) << "\n";
  out << "kit.hangboard=" << IntToStr(save.player.kit.hangboard ? 1 : 0)
      << "\n";
  out << "kit.membership=" << IntToStr(save.player.kit.membershipDaysLeft)
      << "\n";
  out << "job.salaried=" << IntToStr(save.player.job.salaried ? 1 : 0) << "\n";
  out << "job.days=" << IntToStr(save.player.job.daysWorked) << "\n";
  out << "job.weeks=" << IntToStr(save.player.job.weeksSalaried) << "\n";
  out << "job.sincesalary=" << IntToStr(save.player.job.daysSinceSalary)
      << "\n";
  out << "job.dirtbagyears=" << IntToStr(save.player.job.dirtbagYears) << "\n";
  out << "job.longeststreak=" << IntToStr(save.player.job.longestStreak)
      << "\n";
  // The crew name is authored -- one of a fixed set in DirtbagCrew.cpp, all
  // plain ASCII with no '=' or newline -- so it needs no escaping, the same
  // way the seed does not.
  out << "crew.name=" << save.player.crew.name << "\n";
  out << "crew.namedon=" << IntToStr(save.player.crew.namedOnDay) << "\n";
  out << "crew.days=" << IntToStr(save.player.crew.daysReadingAsACrew)
      << "\n";
  out << "crew.members=" << IntToStr(save.player.crew.membersWhenNamed)
      << "\n";
  out << "dreams.rig=" << IntToStr(save.player.dreams.has[0] ? 1 : 0) << "\n";
  out << "dreams.warchest=" << IntToStr(save.player.dreams.has[1] ? 1 : 0)
      << "\n";
  out << "dreams.homebase=" << IntToStr(save.player.dreams.has[2] ? 1 : 0)
      << "\n";
  out << "dreams.chosen=" << IntToStr(static_cast<int>(save.player.dreams.chosen))
      << "\n";
  out << "dreams.seasonoff=" << IntToStr(save.player.dreams.seasonOffDaysLeft)
      << "\n";
  out << "van.rig=" << IntToStr(save.player.van.rig ? 1 : 0) << "\n";
  out << "standing.closed=" << IntToStr(save.player.standing.closedDays)
      << "\n";
  out << "shoes.wear=" << NumToStr(save.player.shoes.wear) << "\n";
  out << "shoes.resoles=" << IntToStr(save.player.shoes.resoles) << "\n";
  out << "shoes.pairs=" << IntToStr(save.player.shoes.pairsOwned) << "\n";
  out << "van.hours=" << NumToStr(save.player.van.hoursDriven) << "\n";
  for (int i = 0; i < kVanPartCount; i++) {
    const std::string k = "van." + IntToStr(i) + ".";
    out << k << "wear=" << NumToStr(save.player.van.parts[i].wear) << "\n";
    out << k << "patches=" << IntToStr(save.player.van.parts[i].patches) << "\n";
    out << k << "failed=" << (save.player.van.parts[i].failed ? "1" : "0")
        << "\n";
  }
  out << "dog.name=" << save.player.dog.name << "\n";
  out << "dog.adopted=" << (save.player.dog.adopted ? "1" : "0") << "\n";
  out << "dog.bond=" << NumToStr(save.player.dog.bond) << "\n";
  out << "dog.fed=" << NumToStr(save.player.dog.fed) << "\n";
  out << "bonds=" << static_cast<int>(save.player.bonds.size()) << "\n";
  for (size_t i = 0; i < save.player.bonds.size(); i++) {
    const PartnerBond& b = save.player.bonds[i];
    const int n = static_cast<int>(i);
    out << BondKey(n, "name") << "=" << b.name << "\n";
    out << BondKey(n, "rapport") << "=" << NumToStr(b.rapport) << "\n";
    out << BondKey(n, "knew") << "=" << NumToStr(b.everKnew) << "\n";
    out << BondKey(n, "fas") << "="
        << static_cast<int>(b.firstAscents.size()) << "\n";
    for (size_t f = 0; f < b.firstAscents.size(); f++) {
      out << BondKey(n, ("fa." + IntToStr(static_cast<int>(f))).c_str())
          << "=" << b.firstAscents[f] << "\n";
    }
  }
  return out.str();
}

LoadResult DeserializeSave(const std::string& text, SaveGame& out,
                           const std::vector<Migration>& migrations) {
  // Split into key=value on the first '=' per line. Values run to the line
  // end, so route names may contain '=' but never a newline — BuildRoute
  // names satisfy that by construction.
  SaveFields fields;
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    const size_t eq = line.find('=');
    if (eq == std::string::npos) return LoadResult::BadFormat;
    fields[line.substr(0, eq)] = line.substr(eq + 1);
  }

  int version = 0;
  if (!ParseInt(fields, "version", version)) return LoadResult::BadFormat;
  if (version > kSaveVersion) return LoadResult::FutureVersion;
  if (version < kSaveVersion &&
      !ApplyMigrations(fields, version, kSaveVersion, migrations)) {
    return LoadResult::BadFormat;
  }

  SaveGame save;
  save.version = kSaveVersion;
  int morphology = 0, projectCount = 0;
  Climber& c = save.player.climber;
  if (!ParseString(fields, "seed", save.seed) ||
      !ParseString(fields, "player.name", save.player.name) ||
      !ParseInt(fields, "day", save.player.day) ||
      !ParseDouble(fields, "cash", save.player.cash) ||
      !ParseDouble(fields, "skills.power", c.skills.power) ||
      !ParseDouble(fields, "skills.fingers", c.skills.fingers) ||
      !ParseDouble(fields, "skills.technique", c.skills.technique) ||
      !ParseDouble(fields, "skills.endurance", c.skills.endurance) ||
      !ParseDouble(fields, "skills.head", c.skills.head) ||
      !ParseInt(fields, "morphology", morphology) ||
      !ParseDouble(fields, "skin", c.skin) ||
      !ParseDouble(fields, "psyche", c.psyche) ||
      !ParseInt(fields, "projects", projectCount)) {
    return LoadResult::BadFormat;
  }
  c.morphology = static_cast<Morphology>(morphology);

  save.player.projects.clear();
  for (int i = 0; i < projectCount; i++) {
    ProjectMemory m;
    int sent = 0, style = 0, firstAscent = 0;
    if (!ParseString(fields, ProjKey(i, "name"), m.routeName) ||
        !ParseInt(fields, ProjKey(i, "grade"), m.grade) ||
        !ParseInt(fields, ProjKey(i, "attempts"), m.attempts) ||
        !ParseInt(fields, ProjKey(i, "best"), m.bestHighpoint) ||
        !ParseDouble(fields, ProjKey(i, "beta"), m.beta) ||
        !ParseInt(fields, ProjKey(i, "sent"), sent) ||
        !ParseDouble(fields, ProjKey(i, "clean"), m.cleanliness) ||
        !ParseInt(fields, ProjKey(i, "fa"), firstAscent) ||
        !ParseInt(fields, ProjKey(i, "confirmed"), m.confirmedGrade) ||
        !ParseInt(fields, ProjKey(i, "style"), style)) {
      return LoadResult::BadFormat;
    }
    m.sent = sent != 0;
    m.firstSendStyle = static_cast<Style>(style);
    m.firstAscent = firstAscent != 0;
    // A given name is allowed to be absent and allowed to be empty: an
    // unnamed line is the normal case, not a corrupt one.
    ParseString(fields, ProjKey(i, "given"), m.givenName);
    int disc = 0;
    if (!ParseInt(fields, ProjKey(i, "disc"), disc)) {
      return LoadResult::BadFormat;
    }
    // Clamped rather than trusted: a hand-edited save must not be able to
    // hand the guidebook a ladder that does not exist.
    m.discipline = disc == 2 ? Discipline::Trad
                             : (disc == 1 ? Discipline::Sport
                                          : Discipline::Boulder);
    save.player.projects.push_back(m);
  }

  for (int i = 0; i < kFactionCount; i++) {
    if (!ParseDouble(fields, "standing." + IntToStr(i),
                     save.player.standing.with[i])) {
      return LoadResult::BadFormat;
    }
  }
  if (!ParseInt(fields, "standing.closed", save.player.standing.closedDays)) {
    return LoadResult::BadFormat;
  }
  int hurt = 0, injuryKind = 0;
  {
    Rival& rv = save.player.rival;
    int style = 0, vibe = 0, allied = 0, offered = 0, met = 0, retired = 0,
        fas = 0, pastCount = 0, raceFa = 0, circuitDates = 0,
        circuitFields = 0, teamStatus = 0, teamEver = 0, teamMates = 0,
        teamGone = 0, wcRounds = 0, wcFields = 0, wcClosed = 0,
        rankResults = 0, leagueFields = 0, medDiag = 0, medTreat = 0,
        medStage = 0, medJoints = 0, medScars = 0, medInsured = 0,
        medTreated = 0, medStaged = 0, sickActive = 0, sickMeds = 0,
        toothStage = 0, craftN = 0;
    if (!ParseString(fields, "rival.name", rv.name) ||
        !ParseInt(fields, "rival.style", style) ||
        !ParseInt(fields, "rival.vibe", vibe) ||
        !ParseInt(fields, "rival.gen", rv.generation) ||
        !ParseInt(fields, "rival.born", rv.bornOnDay) ||
        !ParseDouble(fields, "rival.startage", rv.startAge) ||
        !ParseDouble(fields, "rival.grade", rv.grade) ||
        !ParseInt(fields, "rival.laststep", rv.lastStepDay) ||
        !ParseDouble(fields, "rival.peak", rv.peakGrade) ||
        !ParseDouble(fields, "rival.rivalry", rv.rivalry) ||
        !ParseInt(fields, "rival.allied", allied) ||
        !ParseInt(fields, "rival.offered", offered) ||
        !ParseInt(fields, "rival.met", met) ||
        !ParseInt(fields, "rival.retired", retired) ||
        !ParseDouble(fields, "ranking", save.player.rankingPoints) ||
        !ParseInt(fields, "circuit.season", save.player.circuit.season) ||
        !ParseInt(fields, "circuit.done", save.player.circuit.compsDone) ||
        !ParseDouble(fields, "circuit.you",
                     save.player.circuit.yourPoints) ||
        !ParseDouble(fields, "circuit.rival",
                     save.player.circuit.rivalPoints) ||
        !ParseInt(fields, "circuit.titles", save.player.circuit.titles) ||
        !ParseInt(fields, "circuit.dates", circuitDates) ||
        !ParseInt(fields, "circuit.fields", circuitFields) ||
        !ParseInt(fields, "team.status", teamStatus) ||
        !ParseInt(fields, "team.ever", teamEver) ||
        !ParseInt(fields, "team.seasons", save.player.team.seasons) ||
        !ParseInt(fields, "team.cuts", save.player.team.cuts) ||
        !ParseInt(fields, "team.namedday", save.player.team.namedOnDay) ||
        !ParseString(fields, "team.coach", save.player.team.coach) ||
        !ParseString(fields, "team.coachfor",
                     save.player.team.coachKnownFor) ||
        !ParseString(fields, "team.passed", save.player.team.passed) ||
        !ParseDouble(fields, "team.lastpts",
                     save.player.team.lastReviewPoints) ||
        !ParseInt(fields, "team.lastseason",
                  save.player.team.lastReviewSeason) ||
        !ParseInt(fields, "team.lastday",
                  save.player.team.lastReviewDay) ||
        !ParseInt(fields, "team.mates", teamMates) ||
        !ParseInt(fields, "team.gone", teamGone) ||
        !ParseInt(fields, "wc.season", save.player.worldCup.season) ||
        !ParseDouble(fields, "wc.you", save.player.worldCup.yourPoints) ||
        !ParseInt(fields, "wc.closed", wcClosed) ||
        !ParseInt(fields, "wc.starts", save.player.worldCup.starts) ||
        !ParseInt(fields, "wc.missed", save.player.worldCup.missed) ||
        !ParseInt(fields, "wc.finals", save.player.worldCup.finals) ||
        !ParseInt(fields, "wc.podiums", save.player.worldCup.podiums) ||
        !ParseInt(fields, "wc.wins", save.player.worldCup.wins) ||
        !ParseInt(fields, "wc.titles", save.player.worldCup.titles) ||
        !ParseInt(fields, "wc.best", save.player.worldCup.bestRank) ||
        !ParseInt(fields, "wc.last", save.player.worldCup.lastRank) ||
        !ParseInt(fields, "wc.rounds", wcRounds) ||
        !ParseInt(fields, "wc.fields", wcFields) ||
        !ParseInt(fields, "rank.results", rankResults) ||
        !ParseInt(fields, "league.next", save.player.league.nextNight) ||
        !ParseInt(fields, "league.block", save.player.league.block) ||
        !ParseInt(fields, "league.weeks", save.player.league.weeksDone) ||
        !ParseDouble(fields, "league.you",
                     save.player.league.yourPoints) ||
        !ParseDouble(fields, "league.best", save.player.league.best) ||
        !ParseInt(fields, "league.bestday",
                  save.player.league.bestOnDay) ||
        !ParseInt(fields, "league.nights", save.player.league.nights) ||
        !ParseInt(fields, "league.wins", save.player.league.blockWins) ||
        !ParseInt(fields, "league.lastnight",
                  save.player.league.lastClimbedNight) ||
        !ParseInt(fields, "league.fields", leagueFields) ||
        !ParseInt(fields, "med.diagnosis", medDiag) ||
        !ParseInt(fields, "med.treatment", medTreat) ||
        !ParseInt(fields, "med.stage", medStage) ||
        !ParseInt(fields, "med.stagestart",
                  save.player.medical.stageStarted) ||
        !ParseInt(fields, "med.stagedays", save.player.medical.stageDays) ||
        !ParseDouble(fields, "med.told",
                     save.player.medical.toldSeverity) ||
        !ParseInt(fields, "med.joints", medJoints) ||
        !ParseInt(fields, "med.scars", medScars) ||
        !ParseInt(fields, "med.insured", medInsured) ||
        !ParseInt(fields, "med.insuredon",
                  save.player.medical.insuredOnDay) ||
        !ParseDouble(fields, "med.premiums",
                     save.player.medical.premiumsPaid) ||
        !ParseDouble(fields, "med.claims",
                     save.player.medical.claimsPaid) ||
        !ParseInt(fields, "med.diagnoses",
                  save.player.medical.diagnoses) ||
        !ParseInt(fields, "med.shots", save.player.medical.shotsTaken) ||
        !ParseInt(fields, "med.surgeries",
                  save.player.medical.surgeries) ||
        !ParseInt(fields, "med.rushed",
                  save.player.medical.rushedComebacks) ||
        !ParseInt(fields, "med.untreated",
                  save.player.medical.untreatedInjuries) ||
        !ParseInt(fields, "med.treated", medTreated) ||
        !ParseInt(fields, "med.staged", medStaged) ||
        !ParseInt(fields, "craft.n", craftN) ||
        !ParseInt(fields, "craft.taken", save.player.hand.momentsTaken) ||
        !ParseInt(fields, "craft.botched",
                  save.player.hand.momentsBotched) ||
        !ParseInt(fields, "craft.ducked",
                  save.player.hand.momentsDucked) ||
        !ParseInt(fields, "craft.sackings", save.player.hand.sackings) ||
        !ParseInt(fields, "sick.active", sickActive) ||
        !ParseInt(fields, "sick.days", save.player.sickness.daysLeft) ||
        !ParseDouble(fields, "sick.sev", save.player.sickness.severity) ||
        !ParseInt(fields, "sick.meds", sickMeds) ||
        !ParseInt(fields, "sick.caught", save.player.sickness.caught) ||
        !ParseInt(fields, "teeth.stage", toothStage) ||
        !ParseInt(fields, "teeth.since", save.player.teeth.sinceDay) ||
        !ParseInt(fields, "teeth.fixes", save.player.teeth.fixes) ||
        !ParseInt(fields, "teeth.worst", save.player.teeth.worstEver) ||
        !ParseInt(fields, "teeth.lost", save.player.teeth.lost) ||
        !ParseInt(fields, "up.prehabday",
                  save.player.upkeep.lastPrehabDay) ||
        !ParseInt(fields, "up.streak", save.player.upkeep.prehabStreak) ||
        !ParseInt(fields, "up.prehabdays",
                  save.player.upkeep.prehabDays) ||
        !ParseInt(fields, "up.shrinkday",
                  save.player.upkeep.lastShrinkDay) ||
        !ParseInt(fields, "up.shrinks",
                  save.player.upkeep.shrinkSessions) ||
        !ParseInt(fields, "og.next", save.player.olympics.nextDay) ||
        !ParseInt(fields, "og.appearances",
                  save.player.olympics.appearances) ||
        !ParseInt(fields, "og.gold", save.player.olympics.gold) ||
        !ParseInt(fields, "og.silver", save.player.olympics.silver) ||
        !ParseInt(fields, "og.bronze", save.player.olympics.bronze) ||
        !ParseInt(fields, "og.last", save.player.olympics.lastCompeted) ||
        !ParseString(fields, "rival.race", rv.race.routeName) ||
        !ParseInt(fields, "rival.raceby", rv.race.byDay) ||
        !ParseInt(fields, "rival.racefa", raceFa) ||
        !ParseInt(fields, "rival.fas", fas) ||
        !ParseInt(fields, "pastrivals", pastCount)) {
      return LoadResult::BadFormat;
    }
    // Clamped rather than trusted, same as the character's enums: a file is
    // a thing a person can edit and an out-of-range value indexes off the
    // end of a static table.
    const auto pick = [](int v, int count) {
      return (v >= 0 && v < count) ? v : 0;
    };
    rv.style = static_cast<RouteType>(pick(style, 6));
    rv.vibe = static_cast<RivalVibe>(pick(vibe, kRivalVibeCount));
    rv.allied = allied != 0;
    rv.offered = offered != 0;
    rv.met = met != 0;
    rv.retired = retired != 0;
    rv.race.forFirstAscent = raceFa != 0;
    save.player.circuit.schedule.clear();
    for (int i = 0; i < circuitDates; i++) {
      int d = 0;
      if (!ParseInt(fields, "circuit.date" + IntToStr(i), d)) {
        return LoadResult::BadFormat;
      }
      save.player.circuit.schedule.push_back(d);
    }
    save.player.team.status = static_cast<TeamStatus>(pick(teamStatus, 3));
    save.player.team.everNamed = teamEver != 0;
    save.player.team.roster.clear();
    for (int i = 0; i < teamMates; i++) {
      const std::string k = "team.mate" + IntToStr(i);
      Teammate m;
      if (!ParseString(fields, k + "n", m.name) ||
          !ParseString(fields, k + "r", m.role) ||
          !ParseDouble(fields, k + "p", m.points)) {
        return LoadResult::BadFormat;
      }
      save.player.team.roster.push_back(m);
    }
    save.player.team.gone.clear();
    for (int i = 0; i < teamGone; i++) {
      std::string g;
      if (!ParseString(fields, "team.gone" + IntToStr(i), g)) {
        return LoadResult::BadFormat;
      }
      save.player.team.gone.push_back(g);
    }
    save.player.circuit.fieldPoints.clear();
    for (int i = 0; i < circuitFields; i++) {
      double p = 0.0;
      if (!ParseDouble(fields, "circuit.field" + IntToStr(i), p)) {
        return LoadResult::BadFormat;
      }
      save.player.circuit.fieldPoints.push_back(p);
    }

    for (int i = 0; i < craftN && i < kCraftCount; i++) {
      const std::string k = "craft" + IntToStr(i);
      int sacked = 0;
      if (!ParseDouble(fields, k + "s", save.player.hand.skill[i]) ||
          !ParseDouble(fields, k + "r", save.player.hand.standing[i]) ||
          !ParseInt(fields, k + "n", save.player.hand.shifts[i]) ||
          !ParseInt(fields, k + "x", sacked)) {
        return LoadResult::BadFormat;
      }
      save.player.hand.sacked[i] = sacked != 0;
    }

    save.player.sickness.active = sickActive != 0;
    save.player.sickness.medicated = sickMeds != 0;
    save.player.teeth.stage =
        static_cast<ToothStage>(pick(toothStage, kToothStageCount));

    save.player.medical.diagnosis =
        static_cast<Diagnosis>(pick(medDiag, kDiagnosisCount));
    save.player.medical.treatment =
        static_cast<Treatment>(pick(medTreat, kTreatmentCount));
    save.player.medical.stage =
        static_cast<Comeback>(pick(medStage, kComebackCount));
    save.player.medical.insured = medInsured != 0;
    save.player.medical.treatedThisTime = medTreated != 0;
    save.player.climber.injury.staged = medStaged != 0;
    for (int i = 0; i < medJoints && i < kInjuryKindCount; i++) {
      if (!ParseDouble(fields, "med.joint" + IntToStr(i),
                       save.player.medical.joints[i]) ||
          !ParseInt(fields, "med.shot" + IntToStr(i),
                    save.player.medical.shots[i])) {
        return LoadResult::BadFormat;
      }
    }
    save.player.medical.scars.clear();
    for (int i = 0; i < medScars; i++) {
      const std::string k = "med.scar" + IntToStr(i);
      Scar s;
      int kind = 0;
      if (!ParseInt(fields, k + "k", kind) ||
          !ParseDouble(fields, k + "w", s.weight) ||
          !ParseInt(fields, k + "d", s.fromDay)) {
        return LoadResult::BadFormat;
      }
      s.kind = static_cast<InjuryKind>(pick(kind, kInjuryKindCount));
      save.player.medical.scars.push_back(s);
    }

    save.player.league.fieldPoints.clear();
    for (int i = 0; i < leagueFields; i++) {
      double p = 0.0;
      if (!ParseDouble(fields, "league.field" + IntToStr(i), p)) {
        return LoadResult::BadFormat;
      }
      save.player.league.fieldPoints.push_back(p);
    }

    save.player.rankingRecord.clear();
    for (int i = 0; i < rankResults; i++) {
      const std::string k = "rank.r" + IntToStr(i);
      RankingResult r;
      if (!ParseInt(fields, k + "d", r.day) ||
          !ParseDouble(fields, k + "p", r.points)) {
        return LoadResult::BadFormat;
      }
      save.player.rankingRecord.push_back(r);
    }

    save.player.worldCup.closed = wcClosed != 0;
    save.player.worldCup.schedule.clear();
    for (int i = 0; i < wcRounds; i++) {
      const std::string k = "wc.round" + IntToStr(i);
      WorldCupRound r;
      int resolved = 0, flown = 0;
      if (!ParseInt(fields, k + "d", r.day) ||
          !ParseInt(fields, k + "v", r.venue) ||
          !ParseInt(fields, k + "r", resolved) ||
          !ParseInt(fields, k + "f", flown)) {
        return LoadResult::BadFormat;
      }
      // Clamped rather than trusted, same as every other index that comes
      // off a file a person can edit: an out-of-range venue reads off the
      // end of a static table.
      r.venue = pick(r.venue, static_cast<int>(TheVenues().size()));
      r.resolved = resolved != 0;
      r.flown = flown != 0;
      save.player.worldCup.schedule.push_back(r);
    }
    save.player.worldCup.fieldPoints.clear();
    for (int i = 0; i < wcFields; i++) {
      double p = 0.0;
      if (!ParseDouble(fields, "wc.field" + IntToStr(i), p)) {
        return LoadResult::BadFormat;
      }
      save.player.worldCup.fieldPoints.push_back(p);
    }
    rv.firstAscents.clear();
    for (int i = 0; i < fas; i++) {
      std::string key;
      if (!ParseString(fields, "rival.fa" + IntToStr(i), key)) {
        return LoadResult::BadFormat;
      }
      rv.firstAscents.push_back(key);
    }
    save.player.pastRivals.clear();
    for (int i = 0; i < pastCount; i++) {
      const std::string k = "pastrival" + IntToStr(i) + ".";
      PastRival p;
      int role = 0, pstyle = 0;
      if (!ParseString(fields, k + "name", p.name) ||
          !ParseInt(fields, k + "role", role) ||
          !ParseInt(fields, k + "style", pstyle) ||
          !ParseInt(fields, k + "day", p.retiredOnDay) ||
          !ParseDouble(fields, k + "age", p.age) ||
          !ParseDouble(fields, k + "peak", p.peakGrade) ||
          !ParseInt(fields, k + "gen", p.generation)) {
        return LoadResult::BadFormat;
      }
      p.role = static_cast<RivalRole>(pick(role, 3));
      p.style = static_cast<RouteType>(pick(pstyle, 6));
      save.player.pastRivals.push_back(p);
    }
  }
  {
    Character& ch = save.player.character;
    int built = 0, arch = 0, orig = 0, flaw = 0, temper = 0, gift = 0,
        anti = 0, gk = 0, ak = 0;
    if (!ParseInt(fields, "char.built", built) ||
        !ParseInt(fields, "char.archetype", arch) ||
        !ParseInt(fields, "char.origin", orig) ||
        !ParseInt(fields, "char.flaw", flaw) ||
        !ParseInt(fields, "char.temperament", temper) ||
        !ParseDouble(fields, "char.discipline", ch.personality.discipline) ||
        !ParseDouble(fields, "char.boldness", ch.personality.boldness) ||
        !ParseDouble(fields, "char.social", ch.personality.social) ||
        !ParseDouble(fields, "char.purism", ch.personality.purism) ||
        !ParseInt(fields, "char.gift", gift) ||
        !ParseInt(fields, "char.anti", anti) ||
        !ParseInt(fields, "char.giftKnown", gk) ||
        !ParseInt(fields, "char.antiKnown", ak) ||
        !ParseDouble(fields, "char.startingCash", ch.startingCash) ||
        !ParseInt(fields, "char.agePlus", ch.agePlus)) {
      return LoadResult::BadFormat;
    }
    // Clamped rather than trusted. A file is a thing a person can edit, and
    // an out-of-range enum here indexes off the end of a static table.
    const auto pick = [](int v, int count) {
      return (v >= 0 && v < count) ? v : 0;
    };
    ch.built = built != 0;
    ch.build.archetype =
        static_cast<Archetype>(pick(arch, kArchetypeCount));
    ch.build.origin = static_cast<Origin>(pick(orig, kOriginCount));
    ch.build.flaw = static_cast<Flaw>(pick(flaw, kFlawCount));
    ch.build.temperament =
        static_cast<Temperament>(pick(temper, kTemperamentCount));
    ch.gift = static_cast<Talent>(pick(gift, kTalentCount));
    ch.antiTalent = static_cast<Talent>(pick(anti, kTalentCount));
    ch.giftKnown = gk != 0;
    ch.antiKnown = ak != 0;
    for (int i = 0; i < kSkillCount; i++) {
      if (!ParseDouble(fields, "char.reps" + IntToStr(i), ch.reps[i])) {
        return LoadResult::BadFormat;
      }
    }
  }
  if (!ParseDouble(fields, "load", save.player.climber.load) ||
      !ParseInt(fields, "injury.active", hurt) ||
      !ParseInt(fields, "injury.kind", injuryKind) ||
      !ParseDouble(fields, "injury.severity",
                   save.player.climber.injury.severity) ||
      !ParseInt(fields, "injury.days", save.player.climber.injury.daysLeft) ||
      !ParseInt(fields, "physio.last", save.player.lastPhysioDay)) {
    return LoadResult::BadFormat;
  }
  save.player.climber.injury.active = hurt != 0;
  // Clamped rather than trusted: a hand-edited save must not be able to
  // hand the resolver an injury kind that is not one of the four.
  save.player.climber.injury.kind = static_cast<InjuryKind>(
      injuryKind >= 0 && injuryKind < kInjuryKindCount ? injuryKind : 0);

  int hangboard = 0;
  {
    Logbook& book = save.player.logbook;
    if (!ParseInt(fields, "log.asof", book.asOfDay) ||
        !ParseDouble(fields, "log.burns", book.lifetimeBurns) ||
        !ParseDouble(fields, "log.days", book.lifetimeDays)) {
      return LoadResult::BadFormat;
    }
    for (int i = 0; i < kDidCount; i++) {
      if (!ParseDouble(fields, "log.n" + IntToStr(i), book.count[i])) {
        return LoadResult::BadFormat;
      }
    }
    Quirks& q = save.player.quirks;
    int picked = 0, held = 0, became = 0;
    if (!ParseInt(fields, "quirk.picked", picked) ||
        !ParseInt(fields, "quirk.n", held) ||
        !ParseInt(fields, "quirk.became", became)) {
      return LoadResult::BadFormat;
    }
    // Clamped rather than trusted, the same rule the guidebook's discipline
    // and the injury's kind are read under: a hand-edited save must not be
    // able to hand the game a quirk that does not exist.
    const auto asQuirk = [](int v) {
      return static_cast<Quirk>(v > 0 && v < kQuirkCount ? v : 0);
    };
    q.picked = asQuirk(picked);
    save.player.becameToday = asQuirk(became);
    q.held.clear();
    for (int i = 0; i < held; i++) {
      int one = 0;
      if (!ParseInt(fields, "quirk." + IntToStr(i), one)) {
        return LoadResult::BadFormat;
      }
      const Quirk got = asQuirk(one);
      if (got != Quirk::None && !Has(q, got)) q.held.push_back(got);
    }
    for (int i = 0; i < kHabitCount; i++) {
      if (!ParseDouble(fields, "quirk.h" + IntToStr(i), q.heldFor[i])) {
        return LoadResult::BadFormat;
      }
    }
  }

  {
    Locals& town = save.player.locals;
    int count = 0;
    {
      Bivy& bivy = save.player.bivy;
      int where = 0, booted = 0;
      if (!ParseInt(fields, "bivy.tonight", where) ||
          !ParseInt(fields, "bivy.lotnights", bivy.lotNights) ||
          !ParseInt(fields, "bivy.tickets", bivy.ticketsOwed) ||
          !ParseInt(fields, "bivy.booted", booted)) {
        return LoadResult::BadFormat;
      }
      for (int i = 0; i < kSpotCount; i++) {
        if (!ParseInt(fields, "bivy.last" + IntToStr(i), bivy.lastSlept[i])) {
          return LoadResult::BadFormat;
        }
      }
      // Clamped rather than trusted, the same rule every other enum here
      // is read under.
      bivy.tonight =
          static_cast<Spot>(where >= 0 && where < kSpotCount ? where : 0);
      bivy.booted = booted != 0;
    }
    if (!ParseDouble(fields, "live.grime", save.player.living.grime) ||
        !ParseDouble(fields, "live.water", save.player.living.water) ||
        !ParseDouble(fields, "live.propane", save.player.living.propane)) {
      return LoadResult::BadFormat;
    }
    {
      Gym& gym = save.player.gym;
      int owned = 0, price = 0, mix = 0, equip = 0, camp = 0, desk = 0, set = 0;
      if (!ParseInt(fields, "gym.owned", owned) ||
          !ParseInt(fields, "gym.day", gym.ownedDay) ||
          !ParseInt(fields, "gym.price", price) ||
          !ParseInt(fields, "gym.mix", mix) ||
          !ParseInt(fields, "gym.equip", equip) ||
          !ParseInt(fields, "gym.camp", camp) ||
          !ParseInt(fields, "gym.campuntil", gym.campaignUntil) ||
          !ParseInt(fields, "gym.desk", desk) ||
          !ParseInt(fields, "gym.setter", set) ||
          !ParseDouble(fields, "gym.members", gym.members) ||
          !ParseDouble(fields, "gym.balance", gym.balance) ||
          !ParseInt(fields, "gym.debtdays", gym.debtDays) ||
          !ParseInt(fields, "gym.tick", gym.lastTickDay)) {
        return LoadResult::BadFormat;
      }
      const auto named = fields.find("gym.name");
      if (named == fields.end()) return LoadResult::BadFormat;
      gym.name = named->second;
      gym.owned = owned != 0;
      gym.frontDesk = desk != 0;
      gym.setter = set != 0;
      // Clamped rather than trusted, the same rule the quirk, the injury
      // kind and the guidebook's discipline are read under.
      const auto within = [](int v, int n) { return v >= 0 && v < n ? v : 0; };
      gym.price = static_cast<GymPrice>(within(price, kGymPriceCount));
      gym.mix = static_cast<GymSetMix>(within(mix, kGymSetMixCount));
      gym.equip = static_cast<GymEquip>(within(equip, kGymEquipCount));
      gym.campaign = static_cast<GymCampaign>(within(camp, kGymCampaignCount));

      for (int role = 0; role < 2; role++) {
        GymStaffer& who = role == 0 ? gym.desk : gym.routesetter;
        const std::string k = role == 0 ? "gym.deskwho." : "gym.setwho.";
        if (!ParseDouble(fields, k + "wage", who.wage) ||
            !ParseDouble(fields, k + "quality", who.quality) ||
            !ParseInt(fields, k + "hired", who.hiredDay) ||
            !ParseInt(fields, k + "ask", who.nextAskDay) ||
            !ParseInt(fields, k + "refusals", who.refusals)) {
          return LoadResult::BadFormat;
        }
        // The name and the trait are free text, so a missing key is a
        // format error the same way `gym.name` is, and an empty one is a
        // legitimate empty seat.
        const auto n = fields.find(k + "name");
        const auto t = fields.find(k + "trait");
        if (n == fields.end() || t == fields.end()) return LoadResult::BadFormat;
        who.name = n->second;
        who.trait = t->second;
      }

      for (int i = 0; i < kGymWingCount; i++) {
        int built = 0;
        if (!ParseInt(fields, "gym.wing" + IntToStr(i), built)) {
          return LoadResult::BadFormat;
        }
        gym.wings[i] = built != 0;
      }
      int passive = 0, incident = 0;
      if (!ParseInt(fields, "gym.passive", passive) ||
          !ParseInt(fields, "gym.incident", incident) ||
          !ParseInt(fields, "gym.incidentday", gym.incidentDay)) {
        return LoadResult::BadFormat;
      }
      gym.passive = passive != 0;
      gym.incident =
          static_cast<GymIncident>(within(incident, kGymIncidentCount));

      GymFloor& floor = save.player.floor;
      for (int i = 0; i < kGymRegularCount; i++) {
        if (!ParseInt(fields, "floor.stage" + IntToStr(i), floor.stage[i])) {
          return LoadResult::BadFormat;
        }
        // An arc cannot be part-way past its end, however a file got here.
        floor.stage[i] = std::max(0, std::min(kArcStages, floor.stage[i]));
      }
      int wave2 = 0, wave2mix = 0;
      if (!ParseInt(fields, "floor.walk", floor.lastWalkDay) ||
          !ParseInt(fields, "floor.comp", floor.lastCompDay) ||
          !ParseInt(fields, "floor.wave2", wave2) ||
          !ParseInt(fields, "floor.wave2mix", wave2mix)) {
        return LoadResult::BadFormat;
      }
      floor.waveTwoArrived = wave2 != 0;
      floor.waveTwo = static_cast<GymSetMix>(within(wave2mix, kGymSetMixCount));
    }
    if (!ParseDouble(fields, "hunger.carried", save.player.hungerCarried)) {
      return LoadResult::BadFormat;
    }
    if (!ParseInt(fields, "loc.n", count) ||
        !ParseInt(fields, "loc.titles", town.knownTitles) ||
        !ParseInt(fields, "loc.tier", town.knownTier)) {
      return LoadResult::BadFormat;
    }
    town.people.clear();
    for (int i = 0; i < count; i++) {
      const std::string k = "loc." + IntToStr(i) + ".";
      Local p;
      int where = 0, holds = 0;
      if (!ParseInt(fields, k + "where", where) ||
          !ParseDouble(fields, k + "known", p.known) ||
          !ParseDouble(fields, k + "knew", p.everKnew) ||
          !ParseInt(fields, k + "holds", holds) ||
          !ParseInt(fields, k + "seen", p.lastSeen)) {
        return LoadResult::BadFormat;
      }
      const auto named = fields.find(k + "name");
      if (named == fields.end()) return LoadResult::BadFormat;
      p.name = named->second;
      const auto said = fields.find(k + "about");
      if (said != fields.end()) p.about = said->second;
      // Clamped rather than trusted, the same rule the quirk, the injury
      // kind and the guidebook's discipline are read under.
      p.where = static_cast<Service>(where >= 0 && where < 5 ? where : 0);
      p.holds = static_cast<Heard>(holds > 0 && holds < kHeardCount ? holds : 0);
      town.people.push_back(p);
    }
  }

  {
    Life& life = save.player.life;
    for (int i = 1; i < kThreadCount; i++) {
      Strand& s = life.strands[i];
      const std::string k = "life." + IntToStr(i) + ".";
      int going = 0, ended = 0;
      if (!ParseInt(fields, k + "going", going) ||
          !ParseDouble(fields, k + "warm", s.warmth) ||
          !ParseDouble(fields, k + "deep", s.depth) ||
          !ParseInt(fields, k + "last", s.lastGivenDay) ||
          !ParseInt(fields, k + "days", s.daysGiven) ||
          !ParseInt(fields, k + "ended", ended) ||
          !ParseInt(fields, k + "endday", s.endedDay)) {
        return LoadResult::BadFormat;
      }
      s.going = going != 0;
      s.ended = ended != 0;
    }
    int lost = 0;
    if (!ParseDouble(fields, "life.grieving", life.grieving) ||
        !ParseDouble(fields, "life.griefw", life.griefWeight) ||
        !ParseInt(fields, "life.lost", lost)) {
      return LoadResult::BadFormat;
    }
    // Clamped rather than trusted, the same rule the quirk and the injury
    // kind are read under.
    save.player.lostToday =
        static_cast<Thread>(lost > 0 && lost < kThreadCount ? lost : 0);
  }

  if (!ParseInt(fields, "rack.pieces", save.player.rack.pieces) ||
      !ParseDouble(fields, "rack.quality", save.player.rack.quality)) {
    return LoadResult::BadFormat;
  }
  if (!ParseInt(fields, "kit.pads", save.player.kit.pads) ||
      !ParseInt(fields, "kit.hangboard", hangboard) ||
      !ParseInt(fields, "kit.membership",
                save.player.kit.membershipDaysLeft)) {
    return LoadResult::BadFormat;
  }
  save.player.kit.hangboard = hangboard != 0;

  int salaried = 0;
  int dreamRig = 0, dreamWarChest = 0, dreamHomeBase = 0, dreamChosen = 0;
  int vanRig = 0;
  if (!ParseInt(fields, "job.salaried", salaried) ||
      !ParseInt(fields, "job.days", save.player.job.daysWorked) ||
      !ParseInt(fields, "job.weeks", save.player.job.weeksSalaried) ||
      !ParseInt(fields, "job.sincesalary", save.player.job.daysSinceSalary) ||
      !ParseInt(fields, "job.dirtbagyears", save.player.job.dirtbagYears) ||
      !ParseInt(fields, "job.longeststreak", save.player.job.longestStreak) ||
      !ParseInt(fields, "crew.namedon", save.player.crew.namedOnDay) ||
      !ParseInt(fields, "crew.days", save.player.crew.daysReadingAsACrew) ||
      !ParseInt(fields, "crew.members", save.player.crew.membersWhenNamed) ||
      // Required rather than optional even though empty is the common
      // value: a missing key means an unmigrated save, and silently
      // leaving the name blank would look exactly like a career the town
      // has not named yet. `line.substr(eq + 1)` yields "" for "crew.name=",
      // so an unnamed crew round-trips as present-and-empty.
      !ParseString(fields, "crew.name", save.player.crew.name) ||
      !ParseInt(fields, "dreams.rig", dreamRig) ||
      !ParseInt(fields, "dreams.warchest", dreamWarChest) ||
      !ParseInt(fields, "dreams.homebase", dreamHomeBase) ||
      !ParseInt(fields, "dreams.chosen", dreamChosen) ||
      !ParseInt(fields, "dreams.seasonoff",
                save.player.dreams.seasonOffDaysLeft) ||
      !ParseInt(fields, "van.rig", vanRig)) {
    return LoadResult::BadFormat;
  }
  save.player.job.salaried = salaried != 0;
  save.player.dreams.has[0] = dreamRig != 0;
  save.player.dreams.has[1] = dreamWarChest != 0;
  save.player.dreams.has[2] = dreamHomeBase != 0;
  // Clamped rather than trusted, the same way the injury kind is: a
  // hand-edited save must not be able to name a dream that does not exist.
  save.player.dreams.chosen =
      dreamChosen >= 0 && dreamChosen <= kDreamCount
          ? static_cast<Dream>(dreamChosen)
          : Dream::None;
  save.player.van.rig = vanRig != 0;
  int secretCount = 0;
  if (!ParseInt(fields, "secrets", secretCount)) return LoadResult::BadFormat;
  for (int i = 0; i < secretCount; i++) {
    const std::string k = "secret." + IntToStr(i) + ".";
    Secret s;
    int act = 0, known = 0;
    if (!ParseInt(fields, k + "act", act) ||
        !ParseInt(fields, k + "done", s.dayDone) ||
        !ParseInt(fields, k + "known", known) ||
        !ParseInt(fields, k + "found", s.dayFound)) {
      return LoadResult::BadFormat;
    }
    // A staged photo is not on a line, so an empty route key is correct.
    ParseString(fields, k + "route", s.routeKey);
    // Clamped rather than trusted, like every other enum out of a save.
    s.act = (act >= 0 && act < kEthicalActCount)
                ? static_cast<EthicalAct>(act)
                : EthicalAct::ChippedAHold;
    s.known = known != 0;
    save.player.secrets.push_back(s);
  }

  int sponsorTier = 0;
  if (!ParseInt(fields, "sponsor.tier", sponsorTier) ||
      !ParseInt(fields, "sponsor.seasons", save.player.sponsor.seasonsHeld) ||
      !ParseInt(fields, "sponsor.lastgrade",
                save.player.sponsor.gradeAtLastReview) ||
      !ParseInt(fields, "sponsor.stale",
                save.player.sponsor.seasonsWithoutProgress) ||
      !ParseInt(fields, "sponsor.hurtdays",
                save.player.sponsor.daysHurtThisSeason)) {
    return LoadResult::BadFormat;
  }
  // Clamped rather than trusted, like every other enum out of a save file.
  save.player.sponsor.tier =
      (sponsorTier >= 0 && sponsorTier < kSponsorTierCount)
          ? static_cast<SponsorTier>(sponsorTier)
          : SponsorTier::None;

  int legacyCount = 0;
  if (!ParseInt(fields, "legacies", legacyCount)) return LoadResult::BadFormat;
  for (int i = 0; i < legacyCount; i++) {
    const std::string k = "legacy." + IntToStr(i) + ".";
    Legacy l;
    int hardestDisc = 0, open = 0, faCount = 0;
    if (!ParseString(fields, k + "name", l.name) ||
        !ParseInt(fields, k + "seasons", l.seasons) ||
        !ParseDouble(fields, k + "retiredat", l.retiredAt) ||
        !ParseInt(fields, k + "hardest", l.hardestSendGrade) ||
        !ParseInt(fields, k + "hardestdisc", hardestDisc) ||
        !ParseInt(fields, k + "sends", l.totalSends) ||
        !ParseInt(fields, k + "attempts", l.totalAttempts) ||
        !ParseInt(fields, k + "nemesisattempts", l.nemesisAttempts) ||
        !ParseInt(fields, k + "open", open) ||
        !ParseInt(fields, k + "fas", faCount)) {
      return LoadResult::BadFormat;
    }
    // Names are allowed to be empty — a career that never sent anything has
    // no hardest line, and a nemesis is a luxury.
    ParseString(fields, k + "hardestname", l.hardestSendName);
    ParseString(fields, k + "nemesis", l.nemesis);
    l.hardestSendDiscipline =
        hardestDisc == 1 ? Discipline::Sport : Discipline::Boulder;
    l.cragOpenAtTheEnd = open != 0;

    for (int j = 0; j < faCount; j++) {
      const std::string fk = k + "fa." + IntToStr(j) + ".";
      NamedLine n;
      int style = 0, disc = 0;
      if (!ParseString(fields, fk + "key", n.routeKey) ||
          !ParseInt(fields, fk + "grade", n.confirmedGrade) ||
          !ParseInt(fields, fk + "style", style) ||
          !ParseInt(fields, fk + "disc", disc)) {
        return LoadResult::BadFormat;
      }
      ParseString(fields, fk + "given", n.givenName);
      n.style = static_cast<Style>(style);
      n.discipline = disc == 2 ? Discipline::Trad
                               : (disc == 1 ? Discipline::Sport
                                            : Discipline::Boulder);
      n.by = l.name;
      l.firstAscents.push_back(n);
    }
    save.legacies.push_back(l);
  }

  if (!ParseDouble(fields, "owed", save.player.owed) ||
      !ParseDouble(fields, "shoes.wear", save.player.shoes.wear) ||
      !ParseInt(fields, "shoes.resoles", save.player.shoes.resoles) ||
      !ParseInt(fields, "shoes.pairs", save.player.shoes.pairsOwned) ||
      !ParseDouble(fields, "van.hours", save.player.van.hoursDriven)) {
    return LoadResult::BadFormat;
  }
  for (int i = 0; i < kVanPartCount; i++) {
    const std::string k = "van." + IntToStr(i) + ".";
    int failed = 0;
    if (!ParseDouble(fields, k + "wear", save.player.van.parts[i].wear) ||
        !ParseInt(fields, k + "patches", save.player.van.parts[i].patches) ||
        !ParseInt(fields, k + "failed", failed)) {
      return LoadResult::BadFormat;
    }
    save.player.van.parts[i].failed = failed != 0;
  }

  int adopted = 0;
  if (!ParseString(fields, "dog.name", save.player.dog.name) ||
      !ParseInt(fields, "dog.adopted", adopted) ||
      !ParseDouble(fields, "dog.bond", save.player.dog.bond) ||
      !ParseDouble(fields, "dog.fed", save.player.dog.fed)) {
    return LoadResult::BadFormat;
  }
  save.player.dog.adopted = adopted != 0;

  int bondCount = 0;
  if (!ParseInt(fields, "bonds", bondCount) || bondCount < 0) {
    return LoadResult::BadFormat;
  }
  for (int i = 0; i < bondCount; i++) {
    PartnerBond b;
    int faCount = 0;
    if (!ParseString(fields, BondKey(i, "name"), b.name) ||
        !ParseDouble(fields, BondKey(i, "rapport"), b.rapport) ||
        !ParseDouble(fields, BondKey(i, "knew"), b.everKnew) ||
        !ParseInt(fields, BondKey(i, "fas"), faCount) || faCount < 0) {
      return LoadResult::BadFormat;
    }
    for (int f = 0; f < faCount; f++) {
      std::string key;
      if (!ParseString(fields,
                       BondKey(i, ("fa." + IntToStr(f)).c_str()), key)) {
        return LoadResult::BadFormat;
      }
      b.firstAscents.push_back(key);
    }
    save.player.bonds.push_back(b);
  }

  out = save;
  return LoadResult::Ok;
}

}  // namespace dirtbag
