# Who the gym is for

*2026-08-25*

`Sim/DirtbagGymFloor.{h,cpp}` new, SAVE v43. `GYM-2` and `GYM-5` ported from
the 2D source.

Two passes made the gym a business — pricing, staffing, a town pushing
back, a year with a shape, a bank that takes it away. What neither of them
said is **who the business is for**. The source's own comment on `GYM-2` is
the argument in one line: *"The business sim is untouched — this layer is
who the business is FOR."*

Members were a number on a dashboard. Now there are thirteen people, and a
gym meets seven of them.

## The lever that finally does something permanent

`GYM-5` exists because `GYM-2`'s four arcs end and then the floor goes
quiet: *"every walk after that fires the same summary line forever, and the
four people you spent a career on become a row of stars."* Its first fix is
the one worth the port:

**Wave two is keyed to the set mix in force the day wave one is lived out,
and then it is sticky.** Beginner walls collect Marisol's after-work three,
Ade who is terrified and here anyway, and Horace who fixes things. Hardcore
walls collect Kestrel with her notebook, Dom learning not to hurt himself,
and Rafferty who has opinions about the coffee bar. All-comers gets Nell
back after twenty years, Tobias with the headphones, and Esperanza who
hates her business.

Which people a gym collects is the most honest consequence a set mix has,
and this is **the only place in the port where a free, reversible identity
lever writes something permanent.** Pricing and the mix cost nothing and can
be changed every morning; this is the one thing either of them leaves
behind. Re-taping the place six months later does not swap the people out,
because they are people.

## Two bugs the tests caught, one of them in the source

**The setter's line of the week does not have a hundred names in it.** The
source builds it from two ten-word lists, striding the second by three:
`A[week % 10]` and `B[(week * 3 + 1) % 10]`. Both indices are linear in the
week modulo ten, so the *pair* repeats every ten weeks — ten route names,
which a thirty-year career sees about a hundred and fifty times each. The
stride cannot help; no linear map mod ten can. Indexing the second word by
the *decade* of weeks gives a real hundred, which is about two years.

Found because the test asserted a hundred and got ten.

**A gate that opened by accident.** `WhyNotACompNight` returns empty for a
gym you do not own — there is no button, so there is nothing to explain —
and `HostACompNight` gated on that empty string meaning yes. A career with
no gym could throw a comp night in a building it did not have. Owning it is
now checked in the verb rather than borrowed from the refusal.

## What it measured

`gym=life` in the probe is `gym=run` plus the people: walk the floor every
day it will let you, throw a comp night whenever one is allowed. Six seeds,
thirty years:

| | `gym=run` | `gym=life` |
|---|---|---|
| floor walks | — | 10,629 |
| comp nights | — | 1,062 |
| arcs lived out | 0 | 7 of 7 |
| the cohort arrived on day | — | ~13 of ownership |
| days climbed | 386 | **327** |
| working days | 71.5% | 61.7% |

**Comp night is priced sensibly.** Takings are `round(members × 0.6) × $8`,
so at forty members it is $320 against $150 of costs — **+$170 every ten
days, or about $17 a day against a base overhead of $180.** A tenth of the
rent, earned by filling the room you built. It pays out of the business
rather than out of nowhere, which is the right shape.

## The finding worth Evan's attention

**The whole of `GYM-2` and `GYM-5`'s content is spent in three weeks.**

Four regulars, three stages each, one stage per walk, one walk a day: twelve
days. The cohort lands on the thirteenth, and its three arcs take nine more.
Twenty-one days into owning a gym, every story is told, and every walk after
that is an ambient line — for the remaining ten thousand six hundred days of
the career.

That is the source's shape, faithfully ported: the only limiter on an arc is
your own patience. But it is **the only relationship system in this port
that works that way.** `Thread::warmth` cools per day. `Local::known` needs
days between. `PartnerBond::rapport` moves with days actually climbed
together. All three age on a clock; this one ages on a click.

The fiction agrees with the clock, too. Dale's second stage says *"He's in
four mornings a week now"*; June's says *"first words in three months"*;
Dom's says *"unhurt for a year, which for him is unprecedented."* These are
arcs that read as years and resolve in a fortnight.

And the cost does not stop when the content does. The probe pays an hour a
day for ten thousand days and it shows: **59 fewer climbing days over a
career**, for twenty-one days of story and then flavour. A real player would
simply stop walking, which is the same as saying the verb has no reason to
exist after week three.

**The obvious fix is a gap between stages** — a `momentGapDays` dial, at
which point four arcs spread across a year or two instead of a fortnight,
and the walk stays worth taking because there is periodically something
there. It is one dial and one comparison.

**I have not made that change.** It is a balance pivot against the source,
and this project's convention is that those are Evan's and get logged in
`concepts/` rather than slipped into a port. The measurement is here; the
call is his.

## Cut, still

`GYM-8`, `GYM-9`, `GYM-10` — the youth team, hosting a circuit round, the
league you run — and with them the wing table's `youth` and `bid` fields.
Piper's second stage ends *"Her mother asked if the youth team has a
waitlist. There is no youth team. There might have to be."* That line is the
hook `GYM-8` was written to answer, and it is now sitting in the game with
nothing behind it. That is the source's own arrangement, and it is the
strongest argument for what the next gym pass should be.
