# DECISION — comps and the Olympics are back in scope

**2026-08-23.** Evan: *"wait comps were cut??? ok bring them back and the
olympics. that's a huge part of the game."*

**Reversed.** `DIRTBAG.md` §4's *"cut or defer past 1.0"* line put
*comps/Olympics* at the top of the defer list. That entry is struck and
points here.

## Why it was cut, and why that reasoning was wrong

§4's cut list has one stated justification for all nine entries: *"The 2D
game took years to accrete these; the 3D game earns them the same way."*
That is a good rule and it is right about most of the list — expeditions,
deep-water solo, big-wall, the photography economy and gym ownership really
are accretions, added late to a game that already worked without them.

It is wrong about comps, and the reason it is wrong is visible in the 2D
source rather than arguable:

- **The comp ladder is not a feature, it is the mid-to-late game.** A
  climbing career in the 2D game has two arcs. Outdoors it is grades,
  projects and first ascents, and the port has all of it. Indoors it is
  Local → Regional → National → the national team → the World Cup → the
  Games, and the port has none of it. Cutting comps does not remove a
  system; it removes **the half of the career that has an ending.**
- **It is what the ranking is for.** `natlPts` exists so there is somewhere
  for the numbers to point. Without it, `rep` is banked toward a sponsorship
  and nothing else, and a career's ceiling is "harder grades, more of them".
- **The world already assumes it.** The 2D zone graph has an **Olympic
  Village** as one of only three van-only destinations, alongside the crag
  and the folks' farm. A cut system does not get a zone.
- **Evan named it unprompted.** Describing how travel works, without being
  asked about comps: *"crags and the olympics which you needed to use the van
  to get to."* When the designer describes the world from memory and a cut
  system is in the description, the cut is wrong.

The roadmap re-scope flagged this tension the day before and declined to
resolve it, on the grounds that reversing a `concepts/` decision is a pivot
and not a roadmap edit. This is that pivot.

## What comes back

All of it, as one ladder. Taken from the 2D source rather than described from
memory, because that is the mistake this whole week has been about.

**1. The gym comp.** Announced 3–5 days out with a countdown, $20 entry, six
hours, 30 energy. The format is the interesting part and it is **not a stat
roll**: **five problems, seven attempts across the whole session, an
eighty-second clock.** You spend attempts, not time, and you bank sends
before it runs out. That is a resource-allocation minigame played with the
verbs the session sim already has — *2D minigames, 3D staging*, exactly.
Nerves are a dial: `COMP_PRESSURE = 0.8`, so a comp keeps 80% of your normal
margin, and psych work buys back half of that.

**2. The circuit season.** Five comps, firm dates 6–8 days apart, **the last
is the finals and pays 1.5×**. Season standings, a champion, a runner-up and
a bronze, each with cash, rep and followers. No-showing a scheduled comp
hands the rival 60 points and costs you standing — the schedule is a
commitment, like the job.

**3. Three tiers that mean three different things.** Local (`min: 0`),
Regional (350), National (1200), with field strength and problem grade
rising at each. The 2D game's own balance note is worth carrying over
verbatim, because it is a mistake already made and fixed there: the field
was originally symmetric around your grade, so four entrants sat above you
before every comp *at every tier*, and the measured result was **5th on
average, 2.2% podiums and 0% wins, forever** — which gated three downstream
systems behind an event that happened one comp in fifty. Shifting the field
down two fixed it. **Local is yours to lose, Regional is a fight, National
is the mountain.** National and above run a real quals → semi → final.

**4. A field of people, not a sorted list.** Seven named climbers, each a
stable grade offset from you, and — the part that makes results mean
something — **each has a discipline they are known for and one they are soft
on**, a grade harder and a grade softer respectively. It is a redistribution,
not a buff: the field's average strength is unchanged, but Kai takes the dyno
problem off you every time and you take the crimpy one back off him. Anyone
the game invents later gets the same treatment derived from their name.

**5. The national ranking.** Six tiers — Unranked, Regional Climber (120),
National Prospect (350), **National Team (700)**, **Olympic Hopeful (1200)**,
World-Class (2200) — reading the best of your per-discipline ranks
(boulder / sport / speed), so you build all three and lean into your
strongest.

**6. The national team, which is a roster and not a threshold.** Named when a
circuit season closes above 700; held all the way down to 560, which is the
grace a selection committee actually gives a returning athlete; cut below
that, with the door left open. **Five teammates on the paper with you**, a
head coach with opinions about you, a **$180 taxable stipend that famously
does not cover rent**, teammates who come and go, and a climber you went past
to get on it **who knows.**

**7. The World Cup.** Ten real venues with real cities, disciplines and
travel costs — Salt Lake, Innsbruck, Villars, Briançon, Chamonix, Bern,
Seoul, Koper, Wujiang, Prague. A season schedule, and **an international
field that flies whether you do or not**, so the rounds you skip are counted
against you rather than simply not happening. Career starts, finals,
podiums, wins, titles, best world ranking.

**8. The Olympics.** A 56-day cycle, qualification at 1200, arrival three
days before the event to **declare which disciplines you are entering**
(more than one is allowed, three days apart), **problems a grade above
yours**, medals, and appearances counted across a career. Reached by van,
from its own zone.

## What this costs, honestly

This is the largest single system in the 2D game — it spans the `COMP`,
`CIRCUIT`, `OLY`, `TEAM`, `WC`, `LEAGUE` and `SPEED` tag families and roughly
sixty of the 497 save fields. It is not a phase-sized afternoon; it is a
phase.

Two things make it cheaper than it looks:

- **The session sim already resolves comp attempts.** `COMP_PRESSURE` is a
  margin multiplier on machinery that exists, is tested and has golden
  vectors. The comp format is a wrapper around `ResolveAttempt`, not a
  parallel resolver.
- **It is engine-free.** The whole ladder is sim work — schedules, fields,
  points, selection, medals — with the same shape as the sponsor and faction
  systems already built. The only editor-side item is the Olympic Village as
  a place, and Phase 6 is already building places.

**One real dependency:** the ladder is entangled with the rival throughout —
`circuitRivalPoints`, `compWins`, `compLosses`, `scouted`, the forfeit
points. Comps without somebody to lose to are a leaderboard. **Rivals ship
first** (roadmap Phase 8), and comps take the phase after.

## What stays cut

The rest of §4's defer list is unchanged and the reasoning still holds:
expeditions (El Cap tier — THE WALL's design), deep-water solo, big-wall
multi-day, the filmmaking/photography economy, gym ownership, Solo mode,
Notown/Halloween.

**Leagues are a judgement call and go with comps**, being the same system's
low end — a weekly gym night with a personal best to chase rather than a
ranking. Cheap, and it gives the gym something to be between comps.
