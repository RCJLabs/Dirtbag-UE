# Nothing was listening

*2026-08-25*

The first PIE run of the gym batch. The module built, UHT passed, MSVC
passed, a v20 save migrated forty-five steps to v46, the creation screen
came up — and it would not take a key.

## The shape of it

Every numbered question in this game is asked at a counter, and every
counter binds its own number keys in `OnTriggerBegin`: **when the player
walks into its trigger volume.** That is right for all of them but one.

The four questions that build a climber are asked on the **first frame of a
career**, when the player is standing nowhere at all. No counter was
overlapping, so nothing had bound 1–6, so the keys went to nobody. The
screen that outranks every other screen was the one screen with no listener.

`ADirtbagDaySpot::OnChoose1` carries a comment saying creation is asked
"before anything else and without `bPlayerNear`". That comment is true and
was no help whatsoever: it describes the **first line of a handler that was
never reached**. A note about what a function does first is not a claim that
the function runs.

## The rule that comes out of it

**Input has to be owned by something that exists when the input is
possible.** Not by whatever happens to be nearby, and not by the thing the
question is conceptually about. A counter owning a counter's keys is
correct. Creation is not a counter; it never was one; it borrowed a
counter's keys and inherited a counter's precondition along with them.

The fix puts the six keys on `ADirtbagUEPlayerController`, which is the one
listener that is always there, and two things about how it does that are
load-bearing:

**It never consumes.** Those six keys already belong to every counter in the
game. Where the player controller's own input component sorts against an
actor that has called `EnableInput` is not something this repo can test —
there is no editor here — and a consuming binding that happened to sort
above the gym counter would eat the levers on the one machine that could
find out. So the controller listens and never swallows. When you cannot test
an ordering, do not depend on one.

**One press is one answer.** Two listeners on one key means the same press
can arrive twice, so `UDirtbagGameInstance::LastCreationFrame` stamps the
frame a creation key was spent on and the second call that frame is
recognised as the other listener rather than a second decision. It still
returns *handled*, because the key did belong to creation — letting it fall
through to a counter standing behind the creation screen would be the same
bug wearing a different coat.

## The second one, in the same log

```
Unable to bind delegate to 'OnApproachBegin'
```

`AddDynamic` resolves its target **by name, at runtime, through the
reflection tables**. A target with no `UFUNCTION()` compiles clean, links
clean, and then is simply not there. `OnApproachEnd` had its macro;
`OnApproachBegin` did not. Walking away from a wall worked. Walking up to
one did nothing, and had never done anything.

That is the second failure here in a week whose whole character is being
**quiet**: no error, no warning, a thing that just does not happen. The
missing `UENUM` on `EDirtbagHandoverStep` was the same. Both are statically
checkable and neither was checked, which is the actual finding —
`tools/check-dynamic.py` now starts from the call sites, because every
`AddDynamic` names a function that has to be reflected and there is no
reason to guess.

Verified by reintroducing the exact defect. That is now three checkers this
month that were proved by putting the bug back, and two of the earlier ones
turned out not to work until I did.
