# 2D spec log

The 2D game is this project's spec (see `CLAUDE.md`). When the spec changes, the change is logged here with what it means for the Unreal version, so this repo never follows a spec it hasn't read. Newest first.

---

## 2026-10-18 — Phase 36.1: liar's dice is Perudo

**Built in the 2D rebuild's Phase 36.1.** Save v77 (the dice table's shape). `app/src/sim/perudo.ts` (`sitDown`, `beats`, `least`, `theirMove`, `settle`, `nextRound`, `theirTurns`, `playedOut`, `callLines`), `sim/content/perudo.ts` (`PERUDO_STYLES`, `PERUDO`), `sim/fire.ts` (`fireOut`), `GAMES.dice` and `GAMES.late` in `sim/dials.ts`, `ui/Perudo.tsx`.

- **The game:** you and everyone at the fire, five dice each under a cup, rolled from the seed each round. Ones count for every face and are never bid. A bid is a count of a face (2 to 6) under all the cups; each beats the last with more of any face, or as many of a higher face, never more than the dice in play. Instead of raising, call the last bid: all cups shown; fewer than bid and the bidder loses a die, as many or more and the caller does. The loser opens the next round (or the next seat with dice); no dice is out; the last cup with dice wins. If you're out, the others play to a winner unseen.
- **The others' bids:** from their own cup and the binomial chance (p = 1/3) that the unseen dice make up the rest. Each person has a style of three numbers: call when the bid on the table looks less likely than `doubt`; raise on a face they hold none of `bluff` of the time; open `caution` under what they expect. Hazel cautious, Sage loose and quick to call, Frank straight and suspicious.
- **Time:** a game costs energy and a round's minutes to start; each round after costs its minutes. No longer once a night: game after game until the fire dies at 1 AM (`GAMES.late`), when no game starts and the next round never comes. Bond with each player once a day, as before. Measured: about 70 minutes a game with two others; a cup-reading player wins 37% of three-handed games.
- **Shown:** others' cups (dice left), the round's bids in order, your dice with those counting for the picked face lit, your bid as a count stepper and a face picker, call; on a call every cup up with the counting dice lit and the loser marked.
- **For Unreal:** the fire's dice game is Perudo with the same rules and the same style numbers per person; seed each round's dice from the save's seed, the day, the game and the round; keep the night's 1 AM end and the once-a-day bond.

## 2026-10-16 — Phase 19.4: the beta kit

**Built in the 2D rebuild's Phase 19.4.** No save change. `app/src/game/trail.ts` (`Moves`, `moveOf`, `TRAIL`), `ui/crash.ts` (`crashReport`'s trail, `crashTrail`), `game/unlock.ts` (`trying`, `Opening.trial`), `sim/demo.ts` (`demoOver`), `sim/save.ts` (the full-save refusal), `docs/BETA.md`.

- **The crash report** ends on where the game was (day, time, place, the save's seed) and the last 12 moves as dispatched (each action's kind and fields; a run of walking steps is one line; any field holding text the player typed is left out). Kept in memory only, never saved.
- **The full game, locked:** a climber who's been past where the demo goes (a place, line, wall, trip or book the demo hasn't got) is over at once: the last card, where the unlock is offered. Only the demo's own build refuses such a save on load; the locked full game loads it (no save is set aside at start-up).
- **A tester's link:** a launch parameter shuts the gate for the session only, as on launch day for a climber not from before; only a store unlock opens it, and it marks nothing. For trying the store purchase before the gate is on.
- **For Unreal:** a crash report with the session's seed, day and last actions (scrub typed text); a demo-locked full build never discards a full save, it holds it at the end screen; a debug or tester flag that previews the locked state without writing anything.

## 2026-10-16 — Phase 19.2: Play's unlock in the Android app

**Built in the 2D rebuild's Phase 19.2.** No save change. `app/src/game/play.ts` (`goods`, `owned`, `priceOf`, `askPlay`, `buyUnlock`, `restoreUnlock`, `inAndroidApp`), `twa/PlayAcknowledger.java` and `twa/acknowledge.mjs`, `game/unlock.ts` (`locked`, `STORES`), `content/demo.ts` (the last card's unlock lines).

- **The product:** one one-time Play product, `full_game`; its price is Play's, shown in the player's currency, never typed into the game.
- **A purchase counts** when Play lists it as the account's, not when the payment sheet closes; then the gate's unlock is kept (`play`) and the game starts again from the same save, unlocked.
- **Restore:** a locked game asks Play at start-up and opens if the account owns it; the last card also has "You've bought it already". A refund doesn't re-lock (honor gate).
- **Acknowledgement:** Play refunds an unacknowledged purchase after three days. The app acknowledges every paid, unacknowledged purchase whenever it starts and whenever the page asks Play anything, on the phone, with no server.
- **Store policy:** the Android app offers only Play's purchase; other builds' last cards may link to store pages.
- **For Unreal:** on Android, a native Play Billing one-time product; acknowledge on the device right after purchase and again at every launch; restore from the account's purchases at launch; never link to other stores in the Play build.

## 2026-10-16 — Phase 19.1: the gate

**Built in the 2D rebuild's Phase 19.1.** No save change. The full game's build can play the demo's rules till it's unlocked. `app/src/game/unlock.ts` (`opened`, `openGate`, `unlockIn`, `keepUnlock`, `GATE`), `sim/edition.ts` (`DEMO_BUILD` for the build, `DEMO` for the rules, `asDemo`), `sim/guides.ts` (`guideCrags()`), `vite.config.ts` (`__GATE__`).

- **Two switches, where there was one:** the demo's build (stripped content, its own save keys, a full save turned away) and the demo's rules (Act I and Dex's race, the demo's places, the last card). The full game's build can run the rules alone, with all its content present and its own keys, so the same save carries on when it's unlocked.
- **The gate:** decided once at start-up, before the game boots, never while it runs. Open on the desktop's full edition (bought), on a remembered unlock (`dirtbag.unlock`: `before`, or Play's `play` from 19.2), or for a climber from before the gate. A build switch, off until launch.
- **From before:** while the gate is off, any climber here (a named save or its fallback, or a v0.956 career) is marked `before`. The first gated start marks anyone it missed, then writes `dirtbag.gate`; after that nothing is marked, even by an older build put back. The unlock is written before the mark.
- **For Unreal:** a desktop-only game has no gate (Steam's ownership is the full edition, the demo its own app). If a mobile or web build comes, keep the rules switch apart from the content strip, decide it before the world loads, and keep one save slot for locked and unlocked, so an unlock continues the same career.

## 2026-10-16 — Phase 15.3: the Steam demo

**Built in the 2D rebuild's Phase 15.3.** No save change, no rule change: packaging. `desktop/edition.js`, `pack.mjs --demo`, `steam.json` (`appId`, `demoAppId`), the bridge's `toStore()` and `mail()`, `content/demo.ts` (`DEMO_END.store`, `storeNote`, `noSteam`).

- **Editions:** one desktop code, two products: the full game ("Dirtbag") and the demo ("Dirtbag Demo"), each its own executable, user-data folder and Steam app ID. The demo's save never shares a file or a Cloud path with the full game's.
- **The demo's end:** its last card adds, on Steam, the full game's store page in the overlay (`ActivateGameOverlayToStore`); without Steam, a line that Steam isn't running.
- **Crash report:** on the desktop, the report's mailto: goes to the system's mail app through the bridge (mailto: only).
- **For Unreal:** ship the demo as its own Steam app (its own app ID, depots and Cloud subfolder) from the same project with a build flag; its end screen opens the full game's store page in the overlay; keep its save folder apart.

## 2026-10-16 — Phase 15.2: the error screen

**Built in the 2D rebuild's Phase 15.2.** No save change, no rule change: the shell. `app/src/ui/crash.ts` (`crashed`, `crashReport`, `notACrash`, `REPORT_TO`), `game/persist.ts` (`rawSave`), `main.tsx`.

- **Crash:** an uncaught error shows an error screen over everything, never a blank page. A render crash (React's root torn down) offers a reload only; an error thrown outside rendering (the frame loop, a handler) also offers to carry on. A rejected promise isn't a crash.
- **The screen:** the save is as it was after the last action (saves are written after each action, whole); a reload; the save as kept, as a file, without loading it; the report to copy, and to email from the player's own mail once `REPORT_TO` is set.
- **The report:** version, demo or full, web or desktop, the user agent, the error's name and message and up to twelve stack lines. Nothing from the save.
- **For Unreal:** a crash path that shows the same screen (the save is safe, restart, the save as a file) and a report the player sends themselves, with no crash uploader, in keeping with the privacy promise; on Steam, Valve's minidumps are a separate decision.

## 2026-10-16 — Phase 14.5c: the demo's own keys, and carrying on from it

**Built in the 2D rebuild's Phase 14.5c.** No save change, no rule change: storage. `app/src/game/storage.ts` (`ours()`, `DEMO_SAVE`), `game/persist.ts` (`demoClimber`), `ui/SaveFiles.tsx`.

- **Keys:** the full game's save, fallback, quarantine, replaced copy, plan and settings stay `dirtbag.save`, `dirtbag.save.prev`, `dirtbag.save.corrupt.<ms>`, `dirtbag.save.replaced`, `dirtbag.plan`, `dirtbag.settings`; the demo's are the same names under `dirtbag.demo.`. v0.956's `dirtbag-` keys are untouched by both.
- **Carry-over:** with no climber of its own, the full game's first screen offers a climber found under `dirtbag.demo.save` (loaded and migrated as any save); taking it copies it into the full game's save (as Import does) and leaves the demo's in place.
- **Deployment practice:** the demo at its own origin and, on the desktop, its own product name (its own user-data folder and Steam Cloud paths).
- **For Unreal:** keep a demo's save slot apart from the full game's (its own slot name or folder), and let the full game offer a demo slot it finds rather than rely on a file alone.

## 2026-10-16 — Phase 14.6b: rebinding, Steam's keyboard, and Phase 14's Done-when

**Built in the 2D rebuild's Phase 14.6b.** No save change, no rule change: input. `app/src/game/binds.ts` (`DEFAULT_BINDS`, `bindPad`, `bindKey`, `readBinds`, `buttonName`, `keyName`), `Settings.binds`, `ui/pad.ts` (roles from bindings, `fresh`), the desktop bridge's `deck()` and `keyboard()`.

- **Roles:** pad: a (press/hold/strum), b (back), y (plan), start (journal), select (map), lb/rb (speed wall hands), defaults 0, 1, 3, 9, 8, 4, 5 in the standard mapping. Keys: press Space, back Escape, map M, journal J, plan P, left F, right J. Fixed: D-pad/stick/arrows move, Enter presses, RT (7) a second hold, guide (16) Steam's. Bindable buttons: 0–6, 8–11.
- **Rebinding:** a role given a button or key takes it; the role that had it takes the old one (keys swap within their group: the speed wall's hands apart from the rest, so J is both). A kept set with two roles on one button falls back to the defaults.
- **Presses:** a role is pressed only on its button's down edge; a button bound while held doesn't press its new role until released and pressed again.
- **Desktop/Steam:** `isSteamRunningOnSteamDeck` switches the button names to the Deck's (L1/R1/L2/R2/L3/R3, View, Menu; else Xbox names); a text field focused by the pad calls `showFloatingGamepadTextInput(SingleLine, rect)`.
- **Phase 14 Done-when:** 1 (controller-only Act I) and 4 (demo content diff) pass; 2 (60 fps on a Deck) and 3 (Steam dev branch) wait on Evan's hardware and app ID.
- **For Unreal:** the same role table and swap rule (Enhanced Input mapping contexts per role, with remapping that swaps conflicts), edge-triggered presses across a rebind, Deck glyph detection, and the floating keyboard on text entry.

## 2026-10-16 — Phase 14.6a: marks, minigames by controller, a frame meter

**Built in the 2D rebuild's Phase 14.6a.** No save change, no rule change: input and presentation. `app/src/ui/Glyph.tsx` + `marks.ts` (`glyphs()`), `ui/PadHint.tsx`, `ui/meter.ts`; `ui/input.ts` (`canSkip`), the busk and speed panels, `game/persist.ts` (`Settings.meter`).

- **Marks the fonts lack:** ✓ ✕ ○ ← ♠ ♥ ♦ ♣ are drawn as vector marks in the text's colour (a 16-unit box), never left to a system font; a test holds every character in code to the bundled fonts' cmaps plus that set.
- **Controller in minigames:** B (keyboard Escape) during cooking before the first stir = skip (plain meal, same as the panel's button). When busking, horseshoes, cooking or the speed wall ends, focus goes to the panel's exit button. Scene hotspots are inert while busking. While the pad is the active input, a minigame's controls show their button: A (strum, stir, throw, hold), B (skip), LB/RB (speed wall hands), Xbox names.
- **Frame meter (setting, off by default):** over the last 1 s: frames per second, the slowest frame (ms), and frames over 25 ms (1.5 × a 60 Hz frame); refreshed every 0.5 s.
- **For Unreal:** the same button roles in minigames (A act, B skip/back, bumpers for the speed wall's hands), focus to the exit on a minigame's end, and on-screen prompts only while a pad is the active input; Unreal's fonts need the same audit for any symbol the 2D UI uses.

## 2026-10-16 — Phase 14.5b: the demo's strip and its content diff

**Built in the 2D rebuild's Phase 14.5b.** No save change; rules move only when the demo flag is set. The full game's content past Roadside and the Gorge sits in `*.full.ts` modules (`sim/content/routes`, `places`, `rock`, `story`, `expeditions`, `hollow`; `view/layout`, `view/paint/scenes`, `wall`, `expeds`), each with a `*.demo.ts` twin of the same exports, empty, that the demo build resolves instead (`app/build/demo.ts`). `sim/demo.ts` (`raceDecided`, `demoOver`, `fromFullGame`); `edition.ts` is now a leaf (`DEMO`, `asDemo`, `inEdition`).

- **Places:** the later places keep their rule fields in the demo under blank words (name, cards, lock and closure lines), so presence and weather can still reckon with them; `crag` lists used for guides, invites and media shoots skip places outside the edition.
- **Presence:** `whereIs` returns null for a place outside the edition (Rico's Cave and Moonstone days, Tam's Mesa, Mara's Cave), so the demo's people are only ever where the player can go.
- **Story:** `STORY` is Act I alone in the demo; the act-end card for Act I is never The Line's (`!DEMO && n === STORY.length`). Retiring is refused in the demo (`DEMO_END.retire`), since the endings are stripped.
- **Saves:** in the demo, `fromSave` refuses (before validation) a state at, unlocked or with a guide for a place outside the edition, with a route or first ascent the demo can't resolve or that lies outside it, on a stripped wall, on an expedition, or with any expedition in its book: `DEMO_END.fullSave`. Demo saves load in the full game unchanged.
- **Check:** a CI content diff reads the full game's strings (later lines, walls, places, rock, Acts II to V, The Line, expeditions, the Hollow, every literal in the `.full` sources) and fails on any found in the demo build, less those the demo's own source also says.
- **For Unreal:** a demo edition is an allowlist plus content that's absent, not hidden: package the later crags' data and art as separate assets the demo build doesn't cook, keep presence inside the allowlist, and refuse a full save in the demo rather than half-load it.

## 2026-10-16 — Phase 14.5a: the demo's build and its rules

**Built in the 2D rebuild's Phase 14.5a.** No save change; rules only when the demo flag is set. `app/src/sim/edition.ts` (`DEMO`, `asDemo`, `inEdition`, `raceDecided`, `demoOver`), `sim/content/demo.ts` (`DEMO_PLACES`, `DEMO_START`, `DEMO_END`); `known()`, `currentGoal()` and `act()` read it.

- **Places:** lot, road, gorge, gym, diner, shop, cafe, warehouse, clinic, lake, market, garage. `known()` is false for every other place (so no pin, no travel).
- **Story:** `currentGoal` is null once the ladder passes Act I (no Act II goals); the race week runs unchanged (Dex met on the first V4+ send, the race the night grade ≥ V4).
- **End:** `raceDecided` = `firsts[rsopen].day` (Dex's, overnight) or `routes[rsopen].sent.day` (yours); `demoOver` = demo ∧ goals ≥ |Act I| ∧ decided ∧ day > decided day. Once over, `act()` refuses everything; the UI pins the last card (export the save, start over).
- **Saves:** the demo writes the full game's save; it carries over by export and import.
- **For Unreal:** a build-time edition flag with a place allowlist, a story cap after Act I, and a terminal state derived from saved facts (no new field).

## 2026-10-16 — Phase 14.4: Steamworks behind a stub

**Built in the 2D rebuild's Phase 14.4.** No save change, no rule change. `desktop/steam.js` (`startSteam`, `appIdIn`, `overlayForSteam`), `desktop/steam.json` (`appId`, 480 for now), IPC `steam:achieve`; `app/src/game/achievements.ts` (`steamName`, `earnedEver`, `steamAchievements`); `desktop/steam/achievements.json`; `desktop/STEAM.md`.

- **Achievement names:** each Record Book id upper-cased (`firstsend` → `FIRSTSEND`), 51 of them; display name = the entry's title, description = its `desc`.
- **When:** on each `record` event (the ids earned), and at every boot the union of `state.record` and every `family.books[].record`; unlocking an already-unlocked one is skipped.
- **Stub:** no app ID, Steam not running, or `steamworks.js` failing to load or init → every call returns 0; a name outside `^[A-Z0-9_]{1,64}$` is refused in the main process.
- **Cloud:** Auto-Cloud on `dirtbag.json` under the per-OS user-data roots (WinAppDataRoaming, MacAppSupport, LinuxXdgConfigHome; subdirectory `Dirtbag`).
- **Overlay:** steamworks.js's Electron switches only when launched by Steam (`SteamAppId`/`SteamGameId` set).
- **For Unreal:** the same achievement API names keep a player's unlocks across the 2D and Unreal builds if they share an app ID.

## 2026-10-16 — Phase 14.3: the desktop build

**Built in the 2D rebuild's Phase 14.3.** No save change, no rule change: a wrapper. `desktop/` (Electron 44.5.1, `@electron/packager` 20.3.0): `main.js`, `preload.cjs`, `store.js`, `window.js`; the game's side `app/src/game/desktop.ts`.

- **Serving:** the built game from `app://game/` (privileged standard, secure scheme) out of `app/dist` or a packed copy; requests outside that folder 404; no service worker; new windows denied, navigation off `app://` blocked; one instance.
- **Save file:** `<userData>/dirtbag.json` = `{format: 'dirtbag-desktop', keys: {key: string}}`, every write to `.tmp` then renamed over; a refused write rolls back and reports false; an unreadable file is renamed `.unreadable-<ms>` and the store starts empty. Reached by sync IPC (`store:all`, `store:set`, `store:remove`), from the game's own frame only; the renderer wraps it as a `Storage` given to the store seam before boot.
- **Window:** 1280×800 default (min 360×600), bounds and full screen kept in `<userData>/window.json`; F11 toggles; Settings has full screen / windowed and Quit. Autoplay allowed without a gesture.
- **For Unreal:** the same envelope (`dirtbag.json` with the save's key) is the file Steam Cloud syncs; an Unreal build reading it can pick up a 2D desktop climber if the state shape is shared.

## 2026-10-16 — Phase 14.2: saves, one seam, export and import

**Built in the 2D rebuild's Phase 14.2.** No save change, no rule change. `app/src/game/storage.ts` (`store`, `keepIn`), `game/savefile.ts` (`saveFileName`, `exportSave`, `readSave`), `persist.replace`, `Game.exportSave/importSave`, `ui/SaveFiles.tsx`.

- **Storage:** the game's own keys (`dirtbag.save`, `.prev`, `.corrupt.<ts>`, `.replaced`, `dirtbag.plan`, `dirtbag.settings`) go through one synchronous Storage-shaped store, swappable before boot. v0.956's `dirtbag-` keys are read from the browser directly, never through it.
- **Export:** the save file exactly as stored (`{format: 'dirtbag', v, app, state}`), named `dirtbag-<name slug>-day-<day>.json`.
- **Import:** `fromSave` on the file's text (4 MB cap); refused with its reason and no change, or confirmed by the player, then: current save → `dirtbag.save.replaced`, `dirtbag.save.prev` removed, the file's save written, plans wiped, the page rebooted from it; no write from the old session after that.
- **For Unreal:** save slots as files with the same envelope make a 2D save importable if the state shape is shared; the "no fallback to someone else" rule applies to any slot swap.

## 2026-10-15 — Phase 14.1: focus moves, by keys and a controller

**Built in the 2D rebuild's Phase 14.1.** No save change, no rule change: input only. `app/src/ui/spatial.ts` (`nearest`), `ui/nav.ts` (`layer`, `stops`, `move`, `focused`), `ui/pad.ts` (`Pad.poll`), `ui/input.ts` (`back`, `hub`, `act`, `onPad`); the key handler in `ui/App.tsx`.

- **Focus:** the layer on top (sheet, else conversation, else the first screen, else the screen: HUD, scene hotspots, map pins) holds the stops (live buttons and fields). A move in a direction picks, among stops wholly past the focused one's centre that way, the least `travel + 2 × off-axis gap + drift/100` (centre-to-centre travel; the gap between spans across, 0 when they overlap). None: no move (the key's default runs). Nothing focused: a sheet's first stop; on the screen, the stop nearest its middle.
- **Controller (standard mapping):** polled per frame. A = press the focused button, else use the nearest scene thing / open the wall's beta; A or RT = climb hold and strum; B = back (talk, then the sheet's close, then the map); LB/RB or D-pad left/right = speed-wall hands; Start = journal, Select = map, Y = plan. Direction repeat 0.4 s then 0.13 s; stick lean on ≥ 0.5, off < 0.35; the stick across walks a scene with nothing open.
- **Keys:** arrows move focus except Left/Right walking a scene with nothing open and mid-go; M/J/P = map/journal/plan, ignored in text fields.
- **First screen:** `NAME_IDEAS` (16 names, none in the cast or the comp rosters), cycled by a button.
- **For Unreal:** a focus-driven UI (UMG's navigation or CommonUI) gives the same model; tall world hotspots (wall lines) sit between short ones (boulders), so directional picks need the "wholly past centre" rule or an explicit nav graph.

## 2026-10-15 — Phase 35.5: the road and the year's content rules; Phase 35's Done-when

**Built in the 2D rebuild's Phase 35.5.** No save change, no rule change. `app/src/sim/content.test.ts` ("the road and the year") holds every line of `KNOCKS`, `HITCHERS` (with `friend`), `PASS_BY`, `STOPS`, `HOLIDAYS`, `HOLIDAY_SCENES` and `HOLIDAY_WITH` to one set of rules.

- **Words:** no digit, no straight quote, no doubled or edge space; the only blanks are `{who}` in a knock (the driveway's host) and `{dish}` in a holiday scene (the pantry's dish).
- **References:** unique ids per pool; `helped` and `away.opt` index real answers and never the same one; every knock with `helped` has a `back-<id>` and every `back-<id>` a knock with `helped`; `HOLIDAY_WITH` keys are cast ids.
- **One line changed:** the headlights knock's "at 2 a.m." reads "at two in the morning".
- **Measured (Phase 35, criterion 2):** no hitchhiker a third time before all once, by the rule, over two years of daily crag drives (unit test), and in sixteen four-year career-bot runs (weakly: a median of three meetings).
- **Carried:** the synthesized music's second pass, waiting on a listen.
- **For Unreal:** text pools validated as data (blanks whitelisted per pool, cross-references checked), not by eye.

## 2026-10-15 — Phase 35.4: holiday nights, a scene each

**Built in the 2D rebuild's Phase 35.4.** Save v76 (an encounter of kind `holiday`, id the season; migration 75 is identity). `HOLIDAY_SCENES` and `HolidayFx` in `app/src/sim/content/folks.ts`; `holidayScene`, `holidayDish` (`DISH` 2), `holidayBlocked`, `holidayNote`, `holidayOpts` in `folks.ts`; the bed act and the answer in `game.ts`.

- **When:** at bed at the Lot (`s.at === 'lot'`, not sleeping rough), `holidayOn(day)` and `crew(s)` non-empty: the encounter opens before any knock (a returning knock waits). Alone, Phase 17.6's night stands (psyche `HOLIDAY.psyche`, the morning card).
- **Answer:** the way's `holidayBlocked` (dish needs two servings of an ingredient; cash needs its amount); then each of `crew(s)`: a day together (`climbedWith`) and `bond` more; the dish takes `DISH` servings of the most-stocked ingredient; cash, energy, skin, fed, psyche applied directly; the line (`{dish}` filled); then the night, without the old holiday block or its card.
- **Ways:** Harvest (appetite / dish / cider), Longest Night (fire / headlamp / lamps), First Dry Rock (toast / tick list / early night), Midsummer (sunrise / the hill / bed). The first needs nothing.
- **For Unreal:** calendar scenes at a hub with the present cast, choice effects including group bond and an inventory cost.

## 2026-10-15 — Phase 35.3: knocks that come back

**Built in the 2D rebuild's Phase 35.3.** Save v75 (`deck.back: {id, opt, day, done?}[]`, migration 74 starts empty). `Knock.helped?: number[]`, `Knock.away?: {opt, line}`; return knocks `back-<id>` with no spots (never rolled); `KnockFx.fed`; `knockBackDue` and `knockOpen` in `app/src/sim/events.ts`; dial `EVENTS.knock.back [14, 28]`.

- **Owed:** answering a knock with a `helped` or `away` answer pushes `{id, opt, day}` unless that knock has an entry already (once a knock, ever).
- **Due:** at bed, the first entry not done with `day ≥ entry.day + int(back)` (seeded `back-<id>-<day>`): `away` → its line, done, the night's knock roll goes on; helped → the knock `back-<id>` as tonight's encounter, done, only when a knock could come (`knockOpen`: day ≥ `EVENTS.from`, no encounter today, past `knock.every`).
- **Who:** jump (0, 1; away 2), drunk (1), headlights (0), sharedspot (0; away 2), scavenge (1). Returns: two answers each (supplies, fed, energy, psyche, cash).
- **For Unreal:** encounter memory with a delayed, seeded payoff; a negative answer remembered in narration, not punished.

## 2026-10-15 — Phase 35.2: roadside stops with a choice

**Built in the 2D rebuild's Phase 35.2.** No save change. `RoadStop.opts: StopOpt[]` (`{label, out, min, fx}`) replaces the stop's single `fx`; `RoadFx.followers` (added through `fromAudience`); `stopNote` (events.ts) formats a way's gains; `EVENTS.stop.min` removed (each way has its own minutes).

- **Answer:** `i < opts.length` takes that way: its minutes, its fx (×`EVENTS.stop.again` once the stop's been found), the stop found; `i === opts.length` drives on. The first way at every stop is the old pull-over (40 min, the old fx).
- **The nine stops' second ways:** a photo for followers (overlook, Balanced Rock, fire lookout); walk round the big boulder; sleep under the Lone Sentinel (90 min, more energy); coffee instead of pie; a quick dip instead of a soak; the mine tailings (more psyche, some energy); a sling left at the shrine (a few dollars, more psyche).
- **For Unreal:** roadside stops as small choices, the card showing the trade from the data.

## 2026-10-15 — Phase 35.1: the hitchhikers' rotation, four more, the old hand at the pull-off

**Built in the 2D rebuild's Phase 35.1.** Save v74 (`deck.meets: Record<hitcher id, n>`, every time one comes, picked up or passed; migration 73 sets each `met` id to 1). Rules in `app/src/sim/events.ts` (`meets`, `hitchersDue`, `driveEncounter`); content in `content/road.ts`; dial `EVENTS.hitch.fresh 3` (replacing `again 2.6`).

- **Rotation:** eligible hitchers are those with `meets ≤ least + 1` (least = the minimum over all twelve); weight `fresh` for those at `least`, else 1; the old hand's ×`HOLLOW.hitch` while his clue is due stands. So none comes a third time before all have come once.
- **The pull-off:** a drive to or from the Gorge, the day without an encounter yet, while `hitchDue` and the old hand is eligible: he's there (no odds, no five-day spacing).
- **Four new hitchers:** guide, setter, stranded (a pair and a dog), ranger: three first answers, two second, a friend line each.
- **For Unreal:** a fair-rotation encounter table (least-met first, bounded lead) and a guaranteed quest-giver on a named road.

## 2026-10-15 — Phase 32.5: the bot finds the Hollow; Phase 32's Done-when

**Built in the 2D rebuild's Phase 32.5.** No save change. The bot (`app/src/sim/bot.ts`, option `hollow`): once asking, Roadside on a weekend morning Ray's there (`whereNow`), the shop's guidebook, the old hand's ask when he's on the road (`hitchOpts`), and with all three the walk in from the Gorge, taking the least-risk option at each stage; once found, the Hollow joins its crags (pull as the story's crag), and the easiest line it could try (`beyond` with the line prepared), affordable, one at a time, is prepared (crack on gear with a rack; face bolted with $140 over its cushion; boulder cleaned). The career harness's Hollow run: four careers, 420 days.

- **Rule fix:** `bolt` no longer refuses a zero-cost preparation (cleaning) when cash is negative.
- **Measured (Phase 32, criterion 1):** 2 of 4 find the Hollow and name first ascents (8 between them, V9–V12; 6 settled by day 420). Of the other two, one never met the old hand in 420 days (he's a weighted hitchhiker: the third clue is luck-bound); one found it on day 282 and never made the 165-minute drive back around its shifts.
- **For Unreal:** the third clue's source wants a surer trigger than a weighted random encounter (an open call for Evan).

## 2026-10-15 — Phase 32.4: grades settle, royalties rise

**Built in the 2D rebuild's Phase 32.4.** Save v73 (`FirstAscent.heard?: number`, repeats told; migration 72 marks the repeats already past as heard). Rules in `app/src/sim/settle.ts` (`repeatDays`, `repeatsOf`, `settled`, `faNote`), `game.ts` (`lineGrade`; `settleNight` in the night), `guides.ts` (`fasIn`, `royaltyOf`, `poorestWeek`, `royalties`); words in `content/settle.ts`; dials `SETTLE {repeats 3, gap [7, 21], wild 2}` and `GUIDE.perFa 3`, `GUIDE.week 5`.

- **Repeats:** only for your FAs (no `by`). Seeded per line (`events` stream, derive `repeats-<id>`): `repeats` days, the first `int(gap)` days after the FA's day, each next `int(gap)` after that; ×`wild` at a wild place. A repeat on day D counts from day D+1 (`d < s.day`).
- **Settled:** `repeatsOf ≥ repeats`: `lineGrade` = the listed grade; before, listed + call.
- **Heard:** each night, per FA, if `repeatsOf > heard`: set `heard`; say the settle line (by call −1/0/+1) if settled, else the first-repeat line if `heard` was 0 (repeater seeded from a pool, derive `repeater-<id>`; a wild place has its own line). Middle repeats are silent.
- **Royalties:** per guide out, `royalty + perFa × (your FAs among its lines with day ≤ out)`; the week's total `min(Σ, poorestWeek)`, `poorestWeek` = the least single-shift first-rank job pay × `week` ($28 × 5 = $140).
- **For Unreal:** a consensus-grade model fed by seeded repeat events, and a capped passive income per guide.

## 2026-10-15 — Phase 32.3: first ascents in the Hollow: clean, bolt or gear

**Built in the 2D rebuild's Phase 32.3.** Save v72 (`boltedCracks: string[]`, trad lines in a wild place bolted; each also in `bolted`; migration 71 starts empty). `PlaceDef.wild`; `bolted`/`boltBlocked` in `app/src/sim/business.ts`; `asSport` in `content/routes.ts`, applied by `wallAt` (`content/gym.ts`) and `routeOfId` (`game.ts`); the `bolt` action takes `how?: 'gear' | 'bolts'`; dial `HOLLOW.scene {bolt {gym 2}, crack {old −8, gym 4}, gear {old 2}}`; stance `hollowbolts` (`content/scene.ts`, `crack: true`); text `GEAR_CLEANED` in `content/hollow.ts`.

- **Preparing:** a wild place's lines are closed till prepared, with no ownership needed (at the place is enough). Boulder: clean (`LAND.clean`). Sport: bolt (`LAND.bolt`). Trad: `how` = `gear` (default; `LAND.clean`; stays trad, needs a rack) or `bolts` (`LAND.bolt`; added to `boltedCracks`).
- **A bolted crack:** read everywhere as `asSport(r)`: disc sport, bolts from its moves as a sport line's, stances dropped; grade, moves, cruxes, rock and id unchanged (the route log carries over). The topo (`ROUTES`) is untouched.
- **The scene:** on a wild place's preparation, shift by `HOLLOW.scene.gear` / `.crack` / `.bolt`; boulders none.
- **The stance:** eligible only with `boltedCracks` non-empty; when a stance rolls and it's eligible it's chosen first. Answer 1 ("Pull the bolts") clears `boltedCracks` (lines stay prepared, back to trad); fx old +12, gym −4, psyche −2, energy −20.
- **For Unreal:** a per-line preparation choice whose result changes how the line climbs (protection model), and a faction reaction plus a forced stance.

## 2026-10-15 — Phase 32.2: the Hollow's quest and the walk-in

**Built in the 2D rebuild's Phase 32.2.** Save v71 (`hollow: { clues: ('old'|'hitch'|'book')[], tries }`; migration 70 starts empty). Rules in `app/src/sim/hollow.ts` (`hardestOutside`, `asking`, `oldWordDue`, `hitchDue`, `bookBlocked`, `walkBlocked`), `events.ts` (the old hand's weight and ask), `game.ts` (the `hollow` action: `book`, `walk`; clues from talk and road effects; the walk-in's ending); words in `content/hollow.ts`; the epic `hollow` in `content/epics.ts`; Cond `oldWord`; dial `HOLLOW {from 9, hitch 4, book 20, walk {by 12:00, energy 30}}`; Record entry `hollow`.

- **Asking:** not found, and your hardest outdoor send (not gym, not expedition) is at least `from`.
- **Clues, any order:** `old` from Ray's talk beat (Frank's once `RAY_GONE`); `hitch` from an extra answer to the old hand (first or later meeting), whose pick weight is ×`hitch` while due; `book` from a row at the gear shop (`book` minutes). The third says so.
- **The walk-in:** at the Gorge, all three clues, before `walk.by`, energy ≥ `walk.energy`: starts epic `hollow` (three stages; risks 0/3/−1, 3/1/0, −1/1/2; `EVENTS.epic` thresholds clean ≤ 2, rough ≤ 6). Clean or rough: `unlocked` += hollow, you're at the Hollow. Bad: `tries`++, no injury, at the Gorge.
- **For Unreal:** a three-source clue quest gating a hidden place, and a branching walk-in whose safe choices the clues name.

## 2026-10-15 — Phase 32.1: the Hollow, the place

**Built in the 2D rebuild's Phase 32.1.** No save change. `PlaceDef.hidden`; `known(s, id)` in `app/src/sim/hollow.ts` (not hidden, or in `unlocked`); `PLACES.hollow` and its road (`gorge`–`hollow`, 45 min, $6); seven lines in `content/routes.ts`; their moves in `content/rock.ts`; the painter in `view/paint/scenes.ts`, `wall.ts`, `boulder.ts`; the layout, pin and a hidden side road in `view/layout.ts` and `valley.ts`.

- **Hidden:** travel to a place you don't know refuses before any other check ("There’s no road there you know of."); its pin, its dirt road and taps on the map wait till it's in `unlocked`; the day planner, guides, the Record Book's circuit, a sponsor's shoot and the crew invite list skip it. No `unlock` price: finding it (32.2) is the only way in.
- **The place:** sandstone, shaded, `minGrade` 8, no crowd, `invite` 5.
- **The lines (all `open`):** boulders hcrow V9 technical, hstone V10 crimp, hgap V12 dyno; trad hsplit (grade 8, crack/endurance), hcorner (10, crack/power); sport hwall (9, crimp/technical), hlean (11, power/endurance). Sandstone first ways not repeated in the crag.
- **For Unreal:** a hidden-place flag on the world map and travel; the Hollow's lines and canyon look.

## 2026-10-15 — Phase 31.4: the heir in the harness and the e2e

**Built in the 2D rebuild's Phase 31.4.** No save change; no rule change. The bot (`app/src/sim/bot.ts`): `playDays` takes `from` (a handover state; the kid is made in it); `forebearNight` in every evening at the Lot (sit with the forebear when at the fire, ask about a line; asks recorded with whether the line was theirs); the social bot answers `kin-` talk beats. The career harness's family run: four careers hung up at day 200, each kid played 112 days as the social career bot.

- **Measured (Phase 31, criterion 1):** 4 of 4 kids meet the forebear the first night at the Lot and ask about lines they sent (19 to 30 asks in two years). Frank's word on the family reached every kid; Ray's none (weekend mornings at Roadside, gone after year six). One kid sent the quest (the forebear's hardest, V11) at V8 all round.
- **For Unreal:** a family run in balancing: careers to retirement, then their heirs.

## 2026-10-15 — Phase 31.3: the name and the quest

**Built in the 2D rebuild's Phase 31.3.** No save change (`kin-<who>`, `forebear-quest` in `seen`). Rules in `app/src/sim/kin.ts` (`kinBand`, `kinBeatDue`), `heir.ts` (`questOf`, `questDone`, `hardestOf`), `forebear.ts` (quest words; the quest among the beta lines), `game.ts` (the quest given and heard at the fire; the send's line), `record.ts` (aim `quest`), `epilogue.ts`; words in `content/kin.ts`, `content/forebear.ts`, `content/epilogue.ts` (`QUEST_END`); Cond `kin`, `kinBeat`.

- **The old guard's word:** a talk beat on Ray's and Frank's trees, after their meeting, once (`seen: kin-<who>`), keyed by the forebear's `career.old` band: ≥ `FACTION.bands[2]` high, < `bands[1]` low, else mid (and for a pre-phase heir). Crowd and keeper pools gain `{forebear}` lines under `when: { kin: true }`.
- **The quest:** `career.project` (most-tried unsent) → else the hardest outdoor send → else `kinLine` (pre-phase heirs). Given at the first sit (after the meeting line); the forebear gives beta on it (added to the beta candidates once met). The send: a line; the next sit, once, their reaction; Record entry `kinquest` ("Unfinished Business", aim `{ quest: true }`, given and sent); an epilogue line for the project and hardest kinds.
- **For Unreal:** two old-guard dialogue beats keyed by a standing band; a single named quest derived from the family record; an achievement.

## 2026-10-15 — Phase 31.2: the old climber at the fire

**Built in the 2D rebuild's Phase 31.2.** No save change (the first meeting and the lines heard go in `seen`; once a night each in `today`). Rules in `app/src/sim/forebear.ts` (`forebearAtFire`, `theirLines`, `betaLine`, `forebearSays`, `forebearBlocked`) and `game.ts` (the `forebear` action, `talk` or `beta`); words in `content/forebear.ts`; dials `FAMILY.fire 0.5, talk 30, beta 20`.

- **Presence:** a family's climber, at the Lot, at night: always until the first sit (`forebear-met` in `seen`), then a seeded roll per day under `fire`. Not in `atFire` (no games with them); on the fire card and the van's fire row by name. Not drawn (a drawn, talkable figure would be a person with a People-list bond).
- **Talk:** `talk` minutes, once a night, sets the `fire` flag. First time: the meeting line. Then the first unheard line whose need the career meets (hardest send, FA, project, scar, summit, crew, how it ended, old-guard standing high or low, or none), in order; once all heard, walked by the seed and day.
- **Beta:** `beta` minutes, once a night. Their lines = `career.sent` ∪ `family.lines`. The pick: one you haven't sent with beta left (`sprayable`), most goes first, then easiest. Learns the next beta as told (a first go after is a flash).
- **For Unreal:** the forebear as a fire-side presence with two interactions; the line conditions read the family record.

## 2026-10-15 — Phase 31.1: what the old climber leaves

**Built in the 2D rebuild's Phase 31.1.** Save v70 (`family.career?`, `family.books`; migration 69 splits an heir's Record Book). Rules in `app/src/sim/heir.ts` (`careerOf`, `moneyLeft`, `leaveCap`, `oldStart`, the handover), `record.ts` (`day` aims count from the kid's first morning); words in `content/heir.ts` (`HEIR_MONEY`); dial `FAMILY {share 0.25, shifts 10, old 0.5}`.

- **The career, kept at the handover:** the outdoor lines sent (not the gym's, not expedition pitches); the project (most goes, unsent, outdoor; ties to the harder); age at retirement; how (`chose`, `body`, `home`); scar marks; summits by name; the crew's name; `scene.old`; the money left.
- **The money (Evan's call: a share, capped):** `min(cafe.shift pay × shifts, floor(share × max(0, cash + dream pot)))`, added to the kid's starting cash. Said once at the start (`HEIR_MONEY`). The cap stays under the cheapest dream.
- **The old guard (Evan's call: lines plus standing; the lines come in 31.3):** the kid's `scene.old` = `round(start + old × (forebear's − start))`, either way.
- **The family book (Evan's call):** the kid's Record Book starts empty; `books` holds each earlier generation's `{name, record}`, oldest first. Ownership entries (dog, dream, land, guide in print) come with the keys on the first morning; `day` aims read `day − life.held` for a family.
- **Old heir saves:** entries dated up to `life.held` (the forebear's last day) go to the forebear's page; no career, so only their first ascents are known.
- **For Unreal:** a family record at the handover; the money and standing formulas; per-generation achievement pages.

## 2026-10-15 — Phase 30.4: the epilogue at 30 and 45, and the age table

**Built in the 2D rebuild's Phase 30.4.** No save change. Rules in `app/src/sim/epilogue.ts` (the stop line; the late bloomer's chosen stop); words in `content/epilogue.ts` (`STOP_END`, `LATE_CHOSE`); the bot's day record carries `aged` and `agedPower` (levels of the aged skills); the ending harness's age table.

- **The stop line:** a retired career (not gone home) gets one line, keyed on how and when: the body's call (`forced`) → `body`; chosen before `AGEING.power.from` (33) → `early`; chosen after → `chose`. Filled with the age at retirement. It sits after the people, where going home's line sits for a career that went home.
- **A late bloomer who chose the day** gets `LATE_CHOSE` in place of "ran out of time first", which stays for one the body ended.
- **The age table (Phase 30, criterion 1):** for each ending career, at the first day of 40, `levelOf(average(agedSkills))` against its best before then, and the same on power lines (`mix(aged, 'power')`). The target: within 1 grade of the best, all round. Measured: 16 of 16, the most below 0.09 all round and 0.35 on power lines; training bots outgain the curve, so a climber who stops training is the case the table doesn't show.
- **For Unreal:** an epilogue line on the retirement's age and cause; an aged-level readout per day for balancing.

## 2026-10-15 — Phase 30.3: the runway

**Built in the 2D rebuild's Phase 30.3.** Save v69 (a sponsor task `{ kind: 'clinic', done }`; identity migration). Rules in `app/src/sim/runway.ts` (`clinicHeads`, `clinicPay`, `clinicBlocked`, `compTomorrow`, `setCompBlocked`), `media.ts` (`cycleTasks`: the ambassador's clinic) and `game.ts` (the `clinic` and `setComp` actions); words in `content/runway.ts`; dial `RUNWAY`.

- **From `RUNWAY.from` (35)** (Evan's call: all three, from 35).
- **A clinic:** at the Cave, on a weekend (day % 7 ∈ {6, 0}), by 12:00, once a day; a Coach or better (`rankAt(s,'coach')` ≥ 1) or anyone sponsored. Heads = min(`most` 6, `base` 2 + floor(hardest send's grade / `per` 3)); pay = heads × `fee` ($11); 240 min, 15 energy. Paid as a shift (`paid: 'job'`). Marks a sponsor's `clinic` task done.
- **Setting a comp:** at a venue with a comp tomorrow (`compHere(s, day + 1, at)`), a Senior setter or better (`rankAt(s,'set')` ≥ 2), once a day: 360 min, 20 energy, $85, paid as a shift.
- **Ambassador terms:** `cycleTasks` from 35 puts `{ kind: 'clinic' }` where it put the send at your grade − 1; such a cycle pays `RUNWAY.ambassador` (0.5) × the stipend (`cyclePay`). At the full stipend the media bots never lost a sponsor after 35 and one media career's media reached 105% of its shifts.
- **Never more than a job's:** the most a clinic pays ($66) is under a Head coach's shift ($68); comp setting's hourly ($14.17) is under a Head setter's ($15). Tested against the jobs' data.
- **Bots:** a sponsored bot runs the asked clinic the first weekend morning it can; the business bot sets League night the day before at Send City from 35.
- **For Unreal:** two age-gated paid actions and a sponsor-task swap at an age.

## 2026-10-15 — Phase 30.2: recovery, healing and insurance by age

**Built in the 2D rebuild's Phase 30.2.** No save change. Rules in `app/src/sim/ageing.ts` (`ageCost`, `healDays`, and the note's second sentence), `game.ts` (the night's energy and skin; three injury rolls), `body.ts` (`rollInjury`), `tonight.ts` (the evening's preview) and `clinic.ts` (`premiumOf`, moved from `identity.ts`, which keeps `premiumMult` for the origin's share); dial `AGE_COST`.

- **`ageCost(s, what)`** = 1 + `a` × max(0, age − `from`): `recover` −0.01 from 30; `heal` +0.03 from 35; `premium` +0.03 from 30. Integer age.
- **A night:** the night's energy (van, ledge or expedition) and skin are each × `recover`, rounded, before sickness. The Tonight preview uses tomorrow's age, as the night does.
- **An injury:** every roll's days (`INJURY.days` picked by the seeded roll) × `heal`, rounded (`healDays`). `injuryChance` doesn't read age (Evan's call).
- **Insurance:** a plan's weekly premium × the origin's factor × `premium`, rounded.
- **For Unreal:** three age multipliers on a night's recovery, an injury's length and the premium.

## 2026-10-15 — Phase 30.1: the body's curve

**Built in the 2D rebuild's Phase 30.1.** No save change (age is read off the day). Rules in `app/src/sim/ageing.ts` (`ageOf`, `startAge` and `dayAtAge` moved here from `age.ts`, which re-exports them; `ageMult`, `agedSkills`, `ageingNote`); the go's reads in `climb.ts` (`paceFor`, the pump), `reach.ts` (`reachOf`: crux windows and the cruxes a line gains over your grade) and `game.ts` (the speed wall's time); dial `AGEING`.

- **Phase 30 opened** (Evan's calls, 15 Oct 2026): a gentle curve with skill offsets; recovery and healing slower with age, not injuries likelier; the runway (clinics, setting a comp, ambassador terms) from 35; insurance priced by age.
- **The curve:** each skill × `ageMult(age, skill)` = 1 + `a` × max(0, age − `from`), clamped at `limit`: power and fingers `a` −0.01 from 33 (limit 0.85); endurance −0.007 from 35 (0.85); technique and head +0.005 from 28 (1.08). Integer age, so it moves on birthdays. First proposed at −1.5%/+1%, halved after measuring: above V10 (the curve's linear top, 26 skill points a grade) it came to 1.6 grades on power lines at 40.
- **Used by a go, not the shown grade:** the displayed grade stays `gradeOf(skills as trained)`. For a V13 climber at 40 against 25: power lines −1.07 grades, crimps and dynos −0.38, endurance −0.03, cracks +0.22, technical +0.92; at 45, power −1.84, technical +1.04.
- **Shown:** the You page's age row adds the years' line, formatted from the dials.
- **Bots:** the career bot's wall-pitch choice reads the aged margin.
- **For Unreal:** a per-skill age multiplier applied to a go's skill inputs only; the UI grade from raw skills.

## 2026-10-15 — Phase 34.4: trade shows, a film you pay for, and the money table

**Built in the 2D rebuild's Phase 34.4.** Save v68 (`media.film: { route, day } | null`; a sponsor task `{ kind: 'show', day, done }`). Rules in `app/src/sim/media.ts` (`cycleTasks` with the stand, `showToday`, `showBlocked`, `filmLine`, `filmBlocked`) and `game.ts` (the `show` and `film` actions; a `paid` event wherever money comes in); words in `content/media.ts` (`SHOW_SAYS`, `FILM_SAYS`); dials `SHOW`, `FILM`; Record Book entry `commissioned` (aim `{ commissioned: true }`).

- **The stand** (Evan's call: trade shows, light): from sponsor tier `SHOW.tier` (1, Apex) up, each cycle's asks include a day on the stand, `before` (4) days before the cycle's end. Worked that day, started by `by` (10:00), it takes `min` (600) and `energy` (30), pays `fee[tier]` ($60 Apex, $120 Meridian: about a shift's pay, and a double's; at $100 and $200 a media career's media reached 90% of its shifts') and `followers` (600, through the audience cap). Not worked, it's an undone ask: the cycle ends on a warning, unpaid, as any other.
- **A film you pay for** (Evan's call: a film you commission, light): once a life, of your hardest first-sent line outside at V`FILM.from` (10) or harder, not a wall pitch or an expedition pitch, for `price` ($2,500); it brings `followers` (8,000, through the audience cap) and the Record Book's "Paid for the Light". Ines's doc stays the media ladder's top rung.
- **Money tagged by source:** an event `{ k: 'paid', from, cash }` for a shift's pay (with tips), a sponsor's cycle, an ad, a photo, a piece and a stand. The harness's money table (Phase 34, criterion 1) sums it per media career.
- **Bots:** any sponsored bot works the stand and books no shift over it; the media bot pays for the film once it has the price, its cushion and $500 more.
- **For Unreal:** a dated sponsor ask with a fee; a one-off purchasable film keyed to the best outdoor send; an income ledger by source.

## 2026-10-15 — Phase 34.3: photos that sell, and a magazine piece

**Built in the 2D rebuild's Phase 34.3.** Save v67 (`media.photos: { route, day, by, sold? }[]`, `media.piece: { asked, due, wrote? } | null`). Rules in `app/src/sim/media.ts` (`photoWorthy`, `photoPrice`, `photoBlocked`, `unsoldPhotos`, `lastSold`, `photosSold`, `pieceAsks`, `pieceOpen`, `pieceBlocked`) and `game.ts` (the photo on a first send, the `photo` and `piece` actions, the piece's ask and lapse at night); words in `content/media.ts` (`PHOTO_SAYS`, `PIECE_SAYS`); dials `PHOTO`, `PIECE`, `KIT.camera`; acts `shop.camera`, `shop.sellCamera`.

- **A photo** (Evan's call: your sends sell): a first send (not a repeat), outside, at V`from` (5) or harder, not a wall pitch or an expedition pitch (their first summits pay already) and not indoors, makes one when you own a camera (`by: 'camera'`) or one of the crew is at the crag (`by` = that partner, the first of `crewHere`).
- **The price**, read when sold: `base` (15) × (grade − `from` + 1) × (1 + followers / `fans` (20,000)) × (`phone` (0.5) if a partner shot it), rounded, capped at `cap` ($100). The cap is held under the poorest week a job pays (first-rank pay × shifts posted a week, the lowest across jobs: Setting, $120) by a test.
- **Selling:** `{ t: 'photo', route }` from the feed, any photo of yours not yet sold, one every `every` (7) days.
- **The piece:** at night, once `after` (2) photos have sold and `every` (56, a year) days have passed since the last ask, an editor asks; due `days` (14) on. Written at the van (`min` 180, `energy` 15) for `pay` ($150) and `followers` (1,000, through the audience cap). Unwritten by the due day, a lapse line.
- **Bots:** only the media-focused bot: it buys the camera past its cushion, pitches its best unsold photo whenever allowed, and writes the piece at the van in the morning.
- **For Unreal:** a photo record per first send with a shooter, a weekly sale with a capped price formula, and a yearly commission gated on sales.

## 2026-10-15 — Phase 34.2: the costs of ambition

**Built in the 2D rebuild's Phase 34.2.** Save v66 (`retainers: { physio, coach }`, each the day taken on or null). Rules in `app/src/sim/retainers.ts` (`retained`, `dueTonight`, `retainerBlocked`, `retainerNote`), `game.ts` (the `retainer` action, the nightly charge, physio's days), `clinic.ts` (`carePrice`), `sessions.ts` (`sessionGains`) and `comps.ts` (`compCost`, `compBlocked`); words in `content/retainers.ts`; dials `RETAINER`; `travel` on the comp tiers.

- **Retainers** (Evan's call: opt-in): taken on at their place (the physio at the clinic, a coach at the Cave), the first week's `weekly` paid then, and each later week on the night that ends a week from the day taken on; let go, the payments stop with nothing back.
  - **Physio** ($35 a week): physio costs nothing, and a session takes `days` (2) off an injury instead of 1.
  - **Coach** ($45 a week): a training session's gains × (1 + `gain`), 1.25.
- **Comp travel** (Evan's call): Nationals $100, the World Series $250, the Games $500, paid with the fee (`compCost`); entry is refused when cash is short of both, saying what they come to. The comp bots count it.
- **Bots** take no retainers. The comp bot saves for the dearest comp in the next 14 days (with the World Series on top before a Nationals it's strong enough for) and holds that back from trips and van upgrades; without it, 1 of 4 comp bots reached the Games.
- **For Unreal:** two weekly subscriptions with a start day, a per-tier travel cost on comp entry.

## 2026-10-15 — Phase 34.1: rank perks

**Built in the 2D rebuild's Phase 34.1.** No save change (a perk is read off the rank). Rules in `app/src/sim/perks.ts` (`hasJobPerk`, `perkMult`, `perkPasses`, `perksNote`) and the price hooks: `game.ts` (`actCost`: the gear shop's staff price; the café's coffee and the diner's special; the morning's passes), `van.ts` (`gasFor`), `clinic.ts` (`carePrice`); words in `content/jobs.ts` (`PERK_SAYS`); dials `PERKS`.

- **Phase 34 opened** (Evan's calls, 15 Oct 2026): perks on the jobs there are (no clerk or courier); photos of your own sends sell; the physio and a coach as opt-in retainers, plus comp travel; trade shows and a film you pay for, both light.
- **The perks**, each `{ job, from, off }`: Setting rank 1 (Setter), Send City free; rank 2 (Senior setter), every gym free (the passes go in the day each morning). Guiding rank 1, the gear shop's acts at `1 − 0.2`. Warehouse rank 2, gas at `1 − 0.25`. Coaching rank 1, physio at `1 − 0.5`. Café rank 1, its coffee free; diner rank 1, its special at `1 − 0.5`.
- **Shown:** a shift's row lists the perks held and the next with its rank; every perk's text is formatted from its dial (Phase 34's criterion 2), with a test that the templates carry no numbers.
- **Bots:** the business-focused bot minds its gym before story trips away until the gym has peaked at the top's members (perks made it rich early, and it left its gym for expeditions).
- **For Unreal:** a perk table keyed by job and rank, applied where each price is computed.

## 2026-10-14 — Phase 33.4: selection and the calendar

**Built in the 2D rebuild's Phase 33.4.** No save change (the team is read off the results). Rules in `app/src/sim/comps.ts` (`lastEdition`, `nextEdition`, `onTeam`, `picksFor`, `compBlocked`, `rungOpen`) and `game.ts` (the team's lines in `finishComp`; a round's pass in the morning); content `content/comps.ts` (`team` on a tier, `TEAM_SAYS`, the calendar).

- **The calendar** (Evan's call: Nationals first): Nationals moves to day 8 of every 28 (rounds 8–9), the World Series to day 15 (15–17). No two comps share a day: League nights (4, 11, 18, 25), the Circuit (10, 24), the Games (day 50 of 56, 50–51) and the dyno comp (13 of 56) all clear.
- **Selection** (Evan's call): the World Series has `team: { from: Nationals, top: 3 }` in place of its points gate (`need` 0, and `rungOpen` skips it). You're on the team when a result at the last Nationals before the day placed 3rd or better; it holds till the next Nationals. Making it says so with the World Series' day; missing the podium while on the team says you're off it. The Nationals entry row says the podium picks the team; the ladders page says the World Series is for the team.
- **A round's morning:** waking on the day of a round you're through to puts that wall's pass in your day, as entering did (the desk no longer offers a finalist a pass).
- **The e2e** plays a Nationals qualifier (eight problems on the climb panel, the card handed in, through) and the final the next evening (four problems, the card handed in, placed): Phase 33's criterion 2.
- **For Unreal:** comp dates, a team flag derived from the last selection event's podium, and the gate on the World Series.

## 2026-10-14 — Phase 33.3: the field that comes back

**Built in the 2D rebuild's Phase 33.3.** No save change (the rivals are derived from the seed and the year). Rules in `app/src/sim/comps.ts` (`rivalGrade`, `rivalTier`, `rivalsIn`, `rostered`, the field's entrants, `fieldSays`); content `content/comps.ts` (`ROSTER`, `FIELD_SAYS`); dials `RIVALS`.

- **Rosters** (Evan's call: a roster per tier): from the Circuit up, each rung has 5–7 named rivals, each with a first-year grade and a trend a year (rising, steady, fading) that runs for `RIVALS.years` (5) years, then holds; each year's grade is the trend's ± `wobble` (0.5), seeded per career. Each rung's favourites sit at or just over its top grade, so its top grade no longer wins it every time.
- **Moving up:** a rival goes up a rung for good in the first year their grade is `up` (1) over the top of their rung's grades, and leaves the one below (Nico Hale, Circuit to Nationals, by year 3 in 12 of 12 careers; Mae Okafor, Nationals to the World Series, 9 of 12 by year 3; Kai Mendez and Arjun Mehta by year 7).
- **Skye Maddox** (the media rival) is on the World Series roster and enters Nationals too.
- **A field** is the rung's rivals that year plus fillers from the old pool to make up the numbers, drawn as before. Fillers are never named. League night, the A league and the dyno comp are unchanged.
- **Shown:** the desk's entry row names the three favourites and anyone up from the rung below this year. A comp handed in names its winner (or "somebody nobody had on their sheet"), or who was second if you won, and the rival one place ahead of you; out before the final, only the last.
- **The field's tops roll go by go** (`rollTops`, every comp): each go's chance is `1 − (1 − p)^(1/goes)`, where `p` is the old chance of a top in the round (the logistic on grade), so tops come as often as before but a climber well over a problem tops it early. (A top's goes were uniform from 1 to the budget, so the top grade always won on goes.) Measured: a V13 wins Nationals 83% (100%), a V15 the World Series 92% (100%); a V10 makes the Nationals final 25% (58%).
- **For Unreal:** a rival table (name, rung, grade, trend), a per-year grade function and a promotion rule, all derived from the seed; the field built from roster plus fillers. The field's goes rolled per go.

## 2026-10-14 — Phase 33.2: comp rules, the clock and isolation

**Built in the 2D rebuild's Phase 33.2.** No save change. Rules in `app/src/sim/comps.ts` (`clockFor`, `goesFor(tier, round)`, `goesLeft`, `compClock`, `inComp`), `game.ts` (`goBlocked`, `lessonAt`, the watch, `ask`) and `crowds.ts` (`sprayable`); words in `content/comps.ts` (`COMP_RULES`); dials `COMP.clock` and `COMP.perGo` (replacing `COMP.goes`).

- **The clock** (Evan's call: a go budget): a problem is on the clock `clock` minutes a round, [5, 4] (the qualifier, every round after), and a go with its rest is `perGo` (1) minute, so a problem gets 5 goes in a qualifier and 4 after. The goes are all you get: a go past them is refused (`goesLeft` = 0), and a top in any of them counts. (18.4's rule was a top counting in its first 5 goes, with more goes allowed.) Every ladder comp has the clock; the dyno comp keeps its 3 throws a dyno.
- **The field** rolls each round's goes against that round's budget.
- **Isolation:** on a comp problem (`cp-…`), nobody gives you beta: the crowd's shout, asking around and the crew's go all find nothing (`sprayable` is empty), and asking is refused. A lesson (Sage's watch, a hitchhiker's tip) reads the wall as you find it and skips comp problems; this fixes a lesson during a comp being taken from the week's set. Asked in a comp at its wall, Sage says so. Your own falls still unlock beta.
- **Shown:** the clock's minutes and goes at the desk, on entry, on the scorecard and on the round you're through to; a comp problem's sheet counts "Go n of m on the clock", and its locked beta says isolation. The Training Center's painted clock reads the qualifier's minutes.
- **For Unreal:** a per-round go cap on comp problems, a beta-source filter on them, and the clock's minutes as a stated rule.

## 2026-10-14 — Phase 33.1: comps in rounds

**Built in the 2D rebuild's Phase 33.1.** Save v65 (`comps.on.round`). Rules in `app/src/sim/comps.ts` (`roundOf`, `startOf`, `roundName`, `roundField`, and `compSet` by round) and `game.ts` (`finishComp`: through, or placed); content `content/comps.ts` (`rounds` on a tier, `ROUND_NAMES`, `ROUND_SAYS`); dial `COMP.roundProblems`.

- **Rounds** (Evan's call: small finals): a tier's `rounds` are the cuts, one round a day from its calendar day. Nationals [6] (16, then a final of 6); the World Series [12, 6] (20, a semi-final of 12, a final of 6); the Games [8]. Other tiers are one round.
- **A round's set:** the qualifier as before (the wall's slots, across the grades); later rounds `roundProblems` (4), from `lo + (hi − lo) × round / (2 × (rounds − 1))` up to `hi`: the final from the middle, a semi-final from a quarter. A round's problems are its own day's (`cp-tier-day-n`), so the ids, the wall and the scorecard need nothing else.
- **The field:** drawn once from the comp's first day; each round rolls tops (seeded per round) for whoever's still in it. The field in a round you're in is the previous round's ranking cut to `cut − 1`: the place you took is yours.
- **Finishing a round:** placed at or above its cut, you're through to tomorrow's round (no fee, no pass; the day's sleep doesn't hand it in). Below it, or in the last round, your place in that round is your place in the comp of the whole field, with the points and the purse.
- **Bots:** a round they're through to is the day's climbing; the day stays free of shifts and crew days.
- **For Unreal:** a round index on the entry, a set per round day, a field carried through cuts, and a placing rule.

## 2026-10-14 — Phase 29.4: cooking at the van

**Built in the 2D rebuild's Phase 29.4.** No save change (a today flag, `meal-good`). Rules in `app/src/sim/game.ts` (a recipe act taking `play: { cook: number[] }`), `psyche.ts` and `spots.ts`; words in `content/food.ts`; dials `COOK` and `PSYCHE.meal`.

- **Cooking a recipe** plays the beat panel (the busking and horseshoes beat): `beats` (5) stirs, each scored 1 in the band, a half near it, 0 off it. Skipped before the first stir, it's the plain meal it always was. Ramen stays a tap.
- **A good meal** (the stirs' mean at `good`, 0.7, or better): `COOK.energy` (5) more from tonight's sleep wherever you sleep, and `PSYCHE.meal` (2, the fire's) inside the day's cap. Once a day. Under `burnt` (0.3) it's said to be scorched, with no other effect.
- **Bots** eat plain meals, as before.
- **Phase 29's Done-when** all pass: the free pile against the cans (29.1), the van's verdict by year three (29.3), cooking in the e2e (29.4). Closing is the developer's call.
- **For Unreal:** a timing minigame on a recipe, a day flag, and two small buffs.

## 2026-10-14 — Phase 29.3: the van ages, and the decision

**Built in the 2D rebuild's Phase 29.3.** Save v64 (on the van: `miles`, `rebuilt`, `verdict`). Rules in `app/src/sim/van.ts` (`engineCap`, `vanWorth`, `verdictDue`, `usedPrice`, `FITTED`, and `repairCost` against the cap) and `game.ts` (miles on every drive, the verdict at the garage, the `swap` acts, the Rig's new van); words in `content/garage.ts`; dials `VAN_AGE`.

- **Miles** (Evan's call: the van ages with miles driven): every drive adds its minutes × `mph`/60 (45 mph) to the clock; a ride from a friend adds none. A new climber's van has `start` (160,000) on it. Old saves start the clock there too (Evan's call).
- **The engine's cap:** a service brings the engine back only to `100 − (miles − rebuilt − from) / per` (from 156,250, a point every 250 miles; 85 at the start). Tires and the battery still go back to new. A repair is priced by the points it puts back (`price × (cap − now) / 100`), never under `least`; a bodge or a friend's fix can't pass the cap.
- **The van's worth** is `value` ($1,500) × the cap. **Dale's verdict:** at the garage, once the worth is under a rebuild ($900, so at a cap under 60), Dale says so once (`verdict` holds the day; cleared by a rebuild or a new van), and the garage offers:
  - **Rebuild** ($900, 8 h): `rebuilt` += `off` (8,000 miles), so the cap rises 32; everything fitted stays.
  - **Trade in for a used van** ($1,200 less your van's worth): `used.miles` (158,750, a cap of 90), the parts at a new climber's. What's fitted stays with the old van, Dale's upgrades and the build-outs; the toolkit comes along (Evan's call).
  - **The Dream Rig**, from the dream jar as before. Claiming it now also gives a new van (`rig.miles`, every part at 100), fitted *and built out*, and Dale pays the old van's worth.
  - **Limp on:** do nothing. Servicing still works, to a cap that keeps falling, and a low engine breaks down more.
- **Bots:** once the verdict's due, a bot goes to hear it, and saves for the cheaper way out. At the garage it rebuilds when its cash covers a rebuild plus its cushion, trades in when that covers the used van, and limps on otherwise.
  - **Measured:** all 16 career bots heard it by year three, between days 91 and 136 (median 105), and all traded in (median day 115). One rebuilt its second van, on day 221.
  - **The cost:** the trade-in lands as many careers reach the Moonstone, and Act II ends later (median day 156, against 100.5). Over 32 careers, 20 saw the story through, against 21.
- **For Unreal:** an odometer and a rebuild offset on the van, a derived cap and worth, a once-per-van verdict, two garage actions and the Rig's new van.

## 2026-10-14 — Phase 29.2: the Board's bounties

**Built in the 2D rebuild's Phase 29.2.** Save v63 (on the board: each job's `cash` and `parts`, and `pick`). Rules in `app/src/sim/board.ts` (`postBoard`, `bountyPay`, `bountyProgress`, `partWorth`) and `game.ts` (the `bounty` action, the payout); content `content/board.ts` (each job's `worth`); dials `BOARD.pay` and `BOARD.split`.

- **The Board's three weekly jobs are bounties.** You take one (from the Journal's Board page); the other two come down. Only the one you took pays.
- **It counts from when you take it** (Evan's call): progress is what's done after the pick (`base` holds the count at the pick), so taking a job you've already finished gains nothing. It pays the moment it's done; it lapses with the week.
- **Pay** (Evan's calls): each job has a `worth` (8–18), and its bounty is worth `pay` (2) times that. Each week the seed deals the three one of each way to pay:
  - **cash:** the whole worth;
  - **parts:** the worth in parts, at `partWorth` ($4: the cans' mean take over the pile's mean find, so an hour at either is worth the same), rounded down;
  - **split:** `split` (half) in cash, the rest in parts, rounded down.

  So every week offers money now or the van later. The best a week can pay is $36, under the harness's cap of two days of the worst shift ($56).
- **Bots** take a bounty the moment the week's board goes up (or when they're next free to): the one whose cash, times how much of the job they got done the week before, is most. With nothing to go on, the one that pays most. They count parts as nothing: a bot that took parts at what they'd fetch reached the top of Outdoors in 13 of 32 careers, against 20 before the change and 21 on cash alone.
- **For Unreal:** a pick on the weekly board, a progress baseline, and per-bounty pay in cash and parts.

## 2026-10-14 — Phase 29.1: scavenging and build-outs

**Built in the 2D rebuild's Phase 29.1.** Save v62 (`parts`). Rules in `app/src/sim/hustle.ts` (the `scrap` take), `content/places.ts` (`lot.scrap`, `buildOut`), `spots.ts` and `game.ts` (the effects); dials `HUSTLE.scrap` and `BUILD`. The download budget is 460 KB (Evan's call).

- **The free pile** (`lot.scrap`), a new hustle at the Lot, by day, once a day. It costs the cans' time and energy (120 min, 8 energy) and finds 1–2 parts (seeded), not money. A broke morning is now cash or parts: the hustle's choice with a trade-off (Phase 29's criterion 1).
- **Build-outs from parts,** at the van (Evan's call: new ones; the garage's cash upgrades stay). Each is built once, as a gear id:
  - **a shelf** (2 parts, 1 h): supplies drop ×0.75 a night;
  - **a solar panel** (5 parts, 2 h): the battery doesn't drain overnight;
  - **door seals** (3 parts, 1.5 h): a winter night's cold ×0.7, on top of insulation;
  - **a bed platform** (3 parts, 2 h): +3 energy from a night in the van, on top of the mattress.
- **Bots:** on a day it didn't climb, with energy to spare, a bot picks over the free pile toward the next build-out in a set order (shelf, solar, seals, platform), and builds it when it has the parts. (Corrected in 29.2: this entry first said every evening, a rule that cost the media bots their rest and was changed before the merge.)
- **For Unreal:** a parts count, one hustle, four owned items and their effects on the night.

## 2026-10-14 — Phase 28.4: renown

**Built in the 2D rebuild's Phase 28.4.** No save change: renown is derived. Rules in `app/src/sim/crew.ts` (`renownTier`, `renownBeatDue`), with new conditions `renown` and `renownBeat`. Words in `content/renown.ts`; dials `RENOWN`.

- **Renown** (Evan's call: lines only, never a number) is derived from the crew's record:
  - `train` (3) for each project sent with the crew there;
  - `month` (1) for each 30 days since you named the crew.

  It falls into tiers at `tiers` [3, 8, 15], so the first tier comes with the first train. With no name, there's no renown.
- **Where it's said:**
  - **The threads:** once the crew's known (tier 1 or more), the heat lines (`HEAT_SAYS`) include two about the crew by name (`CREW_HEAT`).
  - **The old guard:** Ray on the rail and Frank by his stove each have a line per tier, said once (talk beats `renown-1..3`, kept in `seen`). A jump past a tier skips that tier's line.
  - **The strangers:** six crowd lines gated by tier (`RENOWN_CROWD`).
  - **The shops:** one each at the gear shop and the diner, at tier 2 (`RENOWN_KEEPERS`).
- **Filling the name:** `{crew}` is filled with the crew's name where the line is said: `crowdLine`, `keeperLine`, the heat line, and `talkValues`.
- **Bots:** the social bot answers the old guard's beats, so they don't stand in front of its sits with Ray and Frank.
- **Phase 28's criterion 1** is now a harness target: a social career names a crew, sends its project, and trades favours both ways.
- **For Unreal:** a derived tier, two new conditions, and line pools; no state.

## 2026-10-14 — Phase 28.3: favours, both ways

**Built in the 2D rebuild's Phase 28.3.** Save v61 (on a person: `owed`, `favour`, `lent`). Rules in `app/src/sim/favours.ts` (`favourAsk`, `clashWith`, `riding`) and `game.ts` (`favourBlocked`, `coverable`, the `favour` action, the asks answered in `say`, loans repaid at night, rides in `travel`); words in `content/favours.ts`; dials `FAVOUR`.

- **You ask** one of the crew, from the crew's texts ("Ask a favour", then who, then what).
  - Once a day each, not while they're away, and not when you owe them `FAVOUR.cap` (2; Evan's call).
  - **Cover a shift:** your next scheduled shift, today's or tomorrow's, comes off the schedule. No strike, no pay, no rank credit.
  - **Lend a rack:** only from Hazel, Mara or Tam (`PersonDef.rack`), and only if you own none. `has(s, 'rack')` is true for the day.
  - **Drive you:** only from Hazel, Sage, Mara or Tam (`PersonDef.wheels`). Today's drives cost no gas or wear, roll no breakdown, and ignore a flat battery or a worn-out part. Permits and the walk-out still apply.
  - Asking adds one to what you owe them. If they owed you, it squares it instead.
- **They ask:** each of the crew, once in every `every` (14) days, on a seeded day, until answered. It comes as a talk beat (`crew-favour-belay | ride | loan`), with the kind seeded per window.
  - **Yes** costs the ask's price (the button shows it): an hour and some energy for a belay; an hour and gas money for a ride; or `loan` ($40), repaid on the night `loanDays` (7) later. Yes is hidden when you can't afford it.
  - A yes takes one off what you owe, or banks one they owe you (down to −cap).
  - **No** costs `no` (1) bond.
- **Two want your day:** if nobody's fallen out yet (and it's past `CREW.from`), an ask for your time from one of Sage, Mara, Rico or Tam, after you've said yes to another today, becomes crew drama's pick or keep (`crew-favour-clash-<b>`).
- **Shown:** the People list ("You owe Hazel one"; "Mara owes you two") and the favour sheet.
- **Bots:** every bot answers asks (the first allowed option, so yes when affordable); Sage's beat must not block her lessons.
  - The social bot also asks: a ride when its van can't go, a rack at a crag with trad and none of its own, and a covered shift on a crew day.
  - **Measured:** favours went both ways in 4 of 4 social careers in a quick run.
- **For Unreal:** person fields for the debt, the last answer and a loan; a favour screen; a talk beat per partner per kind; and the exemptions in the drive and the rack check.

## 2026-10-14 — Phase 28.2: the crew project and the send train

**Built in the 2D rebuild's Phase 28.2.** Save v60 (`crewProject`, `crewDay`, `crewSends`). Rules in `app/src/sim/game.ts` (`projectable`, `hardestSend`, `crewDayAnswers`, the `crewProject`, `crewDay` and `crewGo` actions, the train in `done`) and `crew.ts` (`crewHere`, `crewDayFor`, `crewDayFrom`); words in `content/crew.ts`; dials `CREW_PROJECT`.

- **The project** (Evan's call: you pick it):
  - **What qualifies:** with the crew named and no project on, any line that is unsent, not a wall pitch or expedition, at a crag with an invite bond, and `CREW_PROJECT.over` (2) grades or more above your hardest send of its kind (boulders or ropes; 0 with none).
  - One at a time, and it can be dropped.
- **A crew day,** called from the crew's texts. It's once every `every` (7) days, across projects, so dropping and re-picking doesn't buy another.
  - It's for today before `BOND.inviteBefore`, otherwise tomorrow from `from` (9:00).
  - Each of the crew answers, closest first. It's a no if they're away or on a stint, if their bond is under the crag's invite, or if they'd be with someone they're keeping clear of (`avoid`, either way round). A yes sets their `invite`.
- **The crew goes first:** at the project's crag, each of the crew there who hasn't today has a go.
  - Each go costs `watch` (10) minutes and shows you the next earned way you don't know, as told beta (`sprayable`), in their voice.
  - Told beta makes a first-go send a flash.
- **The send train** (Evan's call: your send starts it): a first send of the project ends it.
  - Each of the crew there follows you up it, a line each. The send goes in `crewSends` with its train, and that night gets `PSYCHE.train` (4).
  - Sent alone, it ends with an empty train.
- **Record Book:** "Send Train" (`{ train: 1 }`), 48 entries.
- **Bots:** only the social bot runs a project.
  - It picks the lowest qualifying line at a career crag, and calls a crew day for tomorrow once a week when the crag will be dry.
  - On the day it goes there, watches the crew go, and works the project first.
- **Measured:** the social bot sent a project with the crew in 4 of 4 careers. The first came on day 30.5 at the median (29–93), then about 4.5 a career by day 300.
- **For Unreal:** three actions, a texts row, the beta sheet's rows, state for the project, the last crew day and the sends, and the train in the send.

## 2026-10-14 — Phase 28.1: the crew's name

**Built in the 2D rebuild's Phase 28.1.** Save v59 (`crewName`). Rules in `app/src/sim/crew.ts` (`crewOf`, `crewNameable`, `crewNameDue`) and `game.ts` (`nameCrew`, `crewSuggestions`); words in `content/crew.ts`.

- **The crew** is the partners (Hazel, Sage, Mara, Rico, Tam) you're a Regular with or closer (`CREW.tier`), closest first. Tam leaves it once he's stopped coming out for good.
- **Naming:** once there are `CREW_NAME.min` (2) of them and no name yet, whichever of them you talk to first brings it up, once, as a talk beat (`calls: true`), in their own words. {other} in the text is the next of the crew (`talkValues`).
  - **Name it:** this opens the naming sheet.
  - **Later:** the beat is spent (`seen: 'crew-name'`). The People list then holds a "Name the crew" button until it's named.
- **The name:** trimmed, 2 to `CREW_NAME.max` (28) characters, and set for good: there's no renaming. It's stored as `crewName: { name, day }`.
- **Names on offer:** four, from the record, in order:
  - a nickname for the place you've sent most at (`CREW_NAMES.place`);
  - your dog's ("Scout’s Pack");
  - the crowd you're in with (`FACTION.bands[2]`), or "No Fixed Abode";
  - then "The Van Crew" and "Rest Day Never".
- **Psyche:** that night gets `PSYCHE.crewNamed` (4, inside the day's cap).
- **Shown:** the People list ("The crew: X, named on day N. A, B and you.") and the crew's texts ("You send the same text to X").
- **Bots:** every bot names it with the first suggestion. The beat on Sage's tree is answered by every bot, so it can't block her lessons.
- **For Unreal:** a state field, a talk beat on five partners' trees, a naming screen like the first-ascent one, and the suggestion rule. Renown (28.4) will read the name.

## 2026-10-14 — Phase 27.6: more at the Big Stone

**Built in the 2D rebuild's Phase 27.6.** No save change. Content in `app/src/sim/content/routes.ts` and `rock.ts`; placement in `app/src/view/layout.ts`.

- **Four new single lines at the Big Stone**, all granite, with names and grades [proposed]:
  - Meadow Arête, V8 technical boulder, 14 ft: the crag's first V8, matching its `minGrade`;
  - The Erratic, V11 power boulder, 18 ft: a highball, so it gets the 27.5 top-out;
  - The Big Stone project, V13 crimp boulder, 15 ft, **open** (Evan's call): the first to send it names it, which also makes a Big Stone guide of your own possible;
  - The Apron, sport 9 (5.13b), technical slab, 22 moves, 90 ft, with a rest at 11, "The polished bit" and "The water streak".
- **Moves:** granite pool moves not yet used at the Big Stone, plus one new top-out pair for The Erratic (a polished mantel and a frost crack). The per-crag rule still holds: no shared first way or pair.
- **Measured:** each new line's half-send grade sits where its style's existing lines do (Meadow −0.5, Erratic −1.3, project −0.6, Apron −0.4).
- **For Unreal:** four rows of content and their placement. No rule changes.

## 2026-10-13 — Phase 27.5: highball top-outs, and where the pad goes

**Built in the 2D rebuild's Phase 27.5.** No save change. Rules in `app/src/sim/game.ts` (`cruxFallMoves`, `fallCrux`, `padUnder`, `landingChance`, `rollLanding`); the top-out in `content/routes.ts` (`topOut`, `TOP_OUT`); moves in `content/rock.ts` (`TOP_OUTS`).

- **Top-outs:** every highball (boulder, ≥ 16 ft, not DWS or board) gets a second crux `B`, "The top-out":
  - `moves` + 2;
  - B spans `oldMoves + 0.4` to `oldMoves + 1.6`;
  - `heightFt` is unchanged, so crux A's fall is now lower;
  - crux A's default win becomes "Through it. Now the top.";
  - B plays as LIBRARY's technical pair, with the window widened by `HIGHBALL.topOut` (first w .22, need 1.1; earned w .24, need 0.9).
- **Difficulty:** measured, the grade at which a balanced climber sends half the time moves by at most 0.2 per highball; most don't move. The top-outs' 36 moves are rock-specific, through `ROCK_LINES`.
- **Renamed:** four cruxes now that a top-out follows them: Highball Arête's "The top" is "The arête"; Tall Arête's "The top" is "The rounded arête"; Moonstone Mantel's "The mantel" is "The lip" (its top-out is the mantel). Moonstone Open's "The last hard move" is "The hard move".
- **One fall height:** a fall off a crux comes from move `floor(from) + 1`, which is the high point the climb model records. The landing roll, the beta sheet and the bot now all use it; before, the sheet and the bot used the crux's midpoint.
- **Pads:** with extra pads (your own second pad, or the haul's at Moonstone), they halve the landing odds under **one** crux only:
  - that crux is the one the `go` names (`pad: 'A' | 'B'`, kept as today flag `pad-<route>-<crux>` for the day's goes), or by default the worse fall;
  - a fall's crux is `fellAt`, else the crux nearest its high point;
  - lines with one crux keep pads everywhere.
- **The spotter** (the first partner at the crag) still halves every fall, and the sheet names them.
- **Bots** gate on the worse of the two falls, with the pads under it (the default).
- **For Unreal:** a second crux on tall boulders, one shared "fall move" function, and a per-go pad placement.

## 2026-10-13 — Phase 27.4: calling it

**Built in the 2D rebuild's Phase 27.4** (Phase 27's old 27.4 split: the highball top-outs and pad placement are now 27.5). Save v58.

- **The call:** the `go` action takes `call: true`. It counts only on go 1 of a line never sent; then today gets `called-<route>`. On any other go it is dropped silently.
  - The UI label is "Call the onsight", or "Call the flash" if the route log's `told` is non-empty.
- **At `done`,** when it's go 1 and `called-<route>` is set:
  - A send writes `SendRecord.called = true` and flags `called-sent`.
  - A fall flags `called-fell`.
  - The style is still decided as before (onsight/flash by `told`). A crowd spray after the call turns a called onsight into a called flash.
- **Psyche:** `psycheDay` adds `PSYCHE.called` (4) for `called-sent` and subtracts `PSYCHE.calledFell` (4) for `called-fell`. Both are applied **after** the daily cap (±8), because a normal good day already hits the cap.
- **Record Book:** `called` ("Called It"), aim `{ called: 10 }`, counting first sends with `called`. 47 entries.
- **Save v58:** `SendRecord.called?: true`; the 57 → 58 migration is the identity. The validator rejects any other value.
- **Bots** never call.
- **For Unreal:** one optional bool on the send record, a go-time flag, and a bet settled outside the psyche cap.

## 2026-10-13 — Phase 27.3: wall pitches by kind

**Built in the 2D rebuild's Phase 27.3.** No save change. Shapes in `PITCH_KINDS` (`app/src/sim/content/routes.ts`); moves in `content/rock.ts`.

- **A wall pitch has a `kind`:** face, crack, corner, chimney, traverse, roof or slab. Pitches had been one shape: 16 moves, crux 9.2–11.4, no rest.
- **The shapes**, as moves, crux from–to, and rest at:
  - face 16, 9.2–11.4, none;
  - crack 16, 8.6–10.8, none;
  - corner 20, 13.4–15.6, rest at 4;
  - chimney 20, 17.2–19.2, rest at 9;
  - traverse 14, 7.8–10, none;
  - roof 15, 10–12.2, none;
  - slab 18, 13.6–15.8, rest at 5.
- **Each kind also has a line:** "Pitch n. " plus the kind's sentence, and a win line. Crux names stay the pitch's name.
- **Rule of thumb:** each crux comes after roughly the old shape's climbing (about 8.5–10 moves), counted from the belay or the rest. Measured, the grade at which a balanced climber sends half the time moves by at most 0.16 a pitch.
- **Moves:** 48 granite BetaDefs, a first and an earned per pitch, through `ROCK_LINES` and `inRock` like any outdoor line (same verb and numbers as the replaced library beta).
  - The crag invariant (no shared first-move name or pair) now counts wall pitches with their crag's single lines.
- **Kinds per wall:** Long Prow slab/face/roof/crack; Golden Buttress slab/corner/face/roof/crack; Obsidian Tower corner/slab/roof/face/crack/traverse/face; the Ascendant chimney/crack/slab/roof/face/traverse/crack/face. Every wall has at least 3 kinds.
- **View:** the topo follows the kind (a traverse goes across, a roof goes out to its lip at the crux, a corner leans), with the feature drawn under the line.
- **Unchanged:** expedition pitches keep the single library shape.
- **For Unreal:** a kind enum on pitches, a seven-row shape table, and the topo hint per kind.

## 2026-10-13 — Phase 27.2: the rock's beta

**Built in the 2D rebuild's Phase 27.2.** No save change. The moves are in `app/src/sim/content/rock.ts`, applied by `inRock` in `content/routes.ts`.

- **Outdoor lines no longer use the plastic library.** Each crag's lines take their cruxes from their rock's pool of moves: 158 BetaDefs, including the signature lines'. Each crux is assigned by hand in `ROCK_LINES` (route → per crux, [first id, earned id]).
- **Invariant:** within a crag, no two cruxes share a first move's name, and no two share a (first, earned) pair.
- **Difficulty-neutral:**
  - A move must have the replaced beta's verb.
  - The line keeps the replaced beta's window (`w`), `rate`/`need` and `skin`.
  - A roped line's second crux keeps falls 2 to earn.
  - What changes is the name, short, note, hint, unlock line, fall lines and **style**. Style decides which skills the window reads, and `crimpy` now follows a crimp style.
- **Unchanged:** indoor sets (gym, Cave, Center, comps), and beta ids (`A1`…`B2`), which saves hold.
- **For Unreal:** the move pools and the assignment table are content to port as they are. The rule is "same verb and numbers, new words and style".

## 2026-10-13 — Phase 27.1: rock as a rule

**Built in the 2D rebuild's Phase 27.1** (Phase 26 closed by Evan's call; Phase 27, the rock, by hand, is next). No save change.

- **The rock:** `PlaceDef.rock` = granite (Roadside, the Gorge, the Big Stone, Wind River), quartzite (Moonstone), sandstone (the Mesa), gneiss (the Crucible), limestone (the Cove, the Bluff).
- **Skin per go outdoors:** style × 1.35 × `ROCK.skin`, where sandstone 1.25, quartzite 1.12, granite 1, gneiss 0.95, limestone 0.85. These are v0.956's per-rock multipliers.
- **Sandstone shut the day after rain:** a sandstone crag that's `seeping` (dry today, rain yesterday, on its own sky) has `closed` = "The sandstone’s soft after rain. Holds snap when it’s wet". Seasonal closures win.
- **Pads:** the Moonstone haul's pads halve landings only at Moonstone. They had been counting at every crag once unlocked. A second pad of your own counts everywhere.
- **For Unreal:** a rock enum per crag, a skin multiplier, and one closure rule.

## 2026-10-13 — Phase 26b: money in a day

**Built in the 2D rebuild (Evan's call, from playing it: once the card's in the red, a shift only treads water).** No save change. Numbers [proposed] in `GIG`, `TOURISTS`, `PLASMA` and `KIT.sell`; rules in `app/src/sim/gigs.ts`.

- **Second shift:** shift acts need not `worked2` and not `shift-<job>`.
  - A job act pushes `shift-<job>`, plus `worked2` if `worked` was already set. The double and the warehouse set both.
  - Sign-ups stay one a day, so a second shift is a walk-in: paid, not counted.
- **Day labor:**
  - Stream `events`, `gig-<day>`: chance 0.43, then a gig from those in season.
  - At the market, start by 8:00. 540 min, energy −40, fed −20, +$80.
  - It sets `worked` and `worked2`, and counts toward no rank.
- **Crag tourists:**
  - Offered at a crag (not private land) whose crowd is busy or packed, with a rope and a sent roped line there (not a wall or expedition pitch, not DWS), when stream `events` `tourists-<day>-<place>` chance 0.5 comes up, before 17:00, once a day.
  - 60 min, energy −6, +$20 plus a tip of 0–10 (`tourist-tip-<day>`). The easiest such line is named.
  - Said on arrival once (the `tourists-asked` flag).
- **Selling kit:** any day, rack, rope, second pad, hangboard or headlamp, for round(0.4 × price), and the gear goes to 0.
- **Plasma:**
  - On day % 7 ∈ {2, 5} (Tuesday, Friday), once a day. 120 min, energy −15, fed −10, +$30.
  - The flag `plasma` multiplies the day's windows by 0.92, in `dayFactor`.
- **For Unreal:** all five are actions, plus a seeded daily roll. The useful rule is the second-shift flag pair.

## 2026-10-08 — Phase 26: a tune-up from the measurements

**Built in the 2D rebuild's Phase 26.** Save v57: `struck: Record<job, number>`, the day of a job's last warning. Rules in `app/src/sim/presence.ts`, `comps.ts`, `mentee.ts` and `game.ts`; numbers [proposed] in `WORK.forgive`, `LEAGUE.prizeA`, `MENTEE.heir` and the comp tiers.

- **Mara and Tam's fallbacks.** "Open" means dry and not shut for the season. Before, Mara stood at a raptor-closed Gorge and Tam at a summer-shut Mesa.
  - Mara, on a day her roll turns her up (unchanged): the Gorge when it's open, else the Cave.
  - Tam: the Mesa when it's open, else the Gorge when that's open, else nowhere.
  - The four's own schedules now cross: Mara and Rico in the Cave, Tam and Mara at the Gorge. So crew drama's avoid rule now bites off invites too.
- **The A league.** A comp tier with `division: 'A'`:
  - League night's nights. Problems V3–V8, a field of 10, fee 15, purse 80/50/30, 15 ladder points. Never a rung.
  - Yours in year y if a settled season of any earlier year has place 1. No relegation, and nothing new stored.
  - Its field is drawn per year from `league-a-<year>`, and its nights roll from `night-a-<day>`. League night's streams are unchanged.
  - Its season's top pays 500 (League night's 250).
- **The dyno comp grows.** Its set and field grades rise by min(3, year), counting the year from 0: V2–V10 the first year, V5–V13 from the fourth. Every other tier is unchanged (`gradesFor`).
- **The heir's head start.** Each skill gets max(0, floor(need(round(0.4 × mentee grade)) − average) + 1), so a V5 mentee makes a V2 heir, V8 a V3, V10 a V4 and V12 a V5. It was +4 skill per mentee grade.
- **Warnings lapse.** A job's warnings clear on the night that ended − struck ≥ 28. A loaded v56 save's warnings count from the day it loads into.
- **The Record Book has 46 entries.** "Everybody Else's Summits" is for team summits on every objective. "Olympic Medalist" is now "Games Medalist", with its id unchanged.
- **The bot** answers Sage's romance beats (the social bot: first option; the others: "friends"). They had starved it of her lessons from Partner on.
- **The crowd's placement (view).** Strangers stand in the gaps between taps that are at least 32 wide: the van, lines ±26, boulders ±w/2 and cast spots ±20. With fewer gaps than the crowd, fewer strangers show.
- **For Unreal:** the presence fallbacks and the A league (derived, not stored) are the rule changes. The rest is numbers.

## 2026-10-04 — Music, made in code; Phase 25 closed

**Built in the 2D rebuild (Evan's call: synthesize it for now and see how it turns out).** No save change; presentation only.

- **Moods by place and hour:** the Lot by day → camp; the Lot at night, or a crag after dark or in winter → fire; a crag → crag; gyms and shops → town; the map or driving → road; an expedition → big. Each: a key, a scale, four chords looped, a tempo, how busy its tune is, and whether a bass and a beat play.
- **Shape:** stretches of 16–32 bars, 25–60 s of quiet between; a new mood starts at the bar after a short gap. It ducks under dialogue and during a go. A Music on/off setting.
- **For Unreal:** the mood map and the stretch-and-quiet shape are the spec; the synthesis is the 2D build's stopgap until there's real music.
- **Phase 25 closed by Evan's call;** the 2D build's next milestone is Phase 14, the desktop and Steam build.

## 2026-10-04 — Phase 25.7: polish

**Built in the 2D rebuild's Phase 25.7.** No save change; presentation only.

- **Trip keepsake card:** the send card's layout for a trip in the book with high ≥ 1. Drawn on pitch `high` of the objective (the summit pitch for a summit). Name: the objective, plus ", pitch N" short of the top; the style line is "Summit" or "High point, N of M"; who: you, and your partner if one came. An objective's card uses its summit trip, else the trip with the highest high point.
- **Glossary links:** a sheet's text marks the first lowercase use of each glossary term (acronyms as written; a plural s/es allowed), three per text at most, longest term first; never go, send, crag, soft, nut, lip or dirtbag. A tap opens the term's entry, returning to the sheet.
- **Miller's Bluff:** its own close-up face (grey bedded limestone; the tufa, the corner, the pillar, the roof at about a third of the height), and a drawn line for each of its six sport lines, which had none.
- **For Unreal:** the link rule is the useful part: names are capitalized, terms in prose aren't.

## 2026-10-04 — Phase 25.6: the people

**Built in the 2D rebuild's Phase 25.6.** Save v56: `crew: { a, b, day, stage: 'rift' | 'mended' | 'set' } | null`, and `PersonLog.avoid?: string`. Numbers [proposed] in `CREW`; rules in `app/src/sim/crew.ts`, `texts.ts`, `keepers.ts`, `crowdtalk.ts`.

- **Crew drama** (once a life):
  - The pair: of `CREW.who` (Sage, Mara, Rico, Tam), those with tier ≥ 2 and not away, sorted by bond; the top two, if two.
  - Ask (`crew: "A/ask/B"`): `crew` null, day ≥ 28, and the pair is (A, B). A talk beat on A's tree, `calls`.
  - Pick A (`fx.crew {do: 'pick', with: B}`) or keep your word to B (`do: 'keep'`): `crew = { a: chosen, b: let down, day, stage: 'rift' }`. The let-down loses 2 bond, gets `avoid` = the chosen, `away` = day + 7, and loses any invite.
  - `whereIs(…, people)`: someone with `avoid` is nowhere at a minute their avoided one is at the same place. The four never share a place on their own schedules, so in practice it's invites.
  - Mend (`crew: "A/mend/B"`): stage rift and day − crew.day ≥ 21, on the chosen one's tree. Try: stream `events`, `crew-mend-<day>`, next() < min(0.95, 0.4 + 0.15 × tier(let-down's bond)) → mended (avoid cleared, both +1 bond), else set. Leave: crew.day = today.
- **Texts back** (no state): a partner's first stay from now (`whereIs` minute by minute); none and it's night → tomorrow's from wake. Else a line: a sulk (avoid and away) → 3 short lines; a stint → its kind's 3; away → 3; else their own 6. Pools walked by `walk(seed, 'text-<who>', pool, day)` (a seeded shuffle, walked in order).
- **Shops' small talk** (no state): the place card's line at the diner, shop, café, market, garage and clinic: their pool filtered by each line's `Cond`, walked by `keeper-<place>` and the day. `Cond` gains `season`.
- **The crowd** (no state): strangers' placement is shared by the renderer and the tap targets (view-side `mulberry32`, unchanged). A tap: the pool filtered by `Cond`, walked by `crowd` at index day × 5 + the taps today; past five, a fixed line.
- **For Unreal:** crew drama is a talk beat pair and one avoid rule in presence; the rest is text selection.

## 2026-10-04 — Phase 25.5: comps outside the ladder

**Built in the 2D rebuild's Phase 25.5.** Save v55: `comps.seasons: { year, place, of }[]`. Numbers [proposed] in `content/comps.ts` and `LEAGUE`; rules in `app/src/sim/comps.ts`.

- **The dyno comp:** a sixth `COMP_TIERS` entry, last, so saved tier indices hold. Send City, `every` 56, `on` 13, grades V2–V10, field 12, fee $15, purse 300/150/75, `pts` 0, `need` 0, and `side: { n: 5, type: 'dyno', moves: [3, 4], goes: 3 }`.
  - Its set: 5 boulders, grades spread lo→hi as any comp's, all dyno, moves `int(3, 4)`, crux "The throw".
  - A top counts in its first `goesFor(tier)` goes (3; every other comp `COMP.goes`, 5); the field's goes roll `int(1, goesFor)`.
  - Side comps add no ladder points. `COMP_LADDER` (the non-side indices) is what `rungOpen`, the ladders screen and the comp bot read; `GAMES_TIER` is the ladder's last.
- **League night's field** (tier 0 only): names and grades from stream `events`, `league-<year>` (year = floor((day − 1) / 56)), a shuffle, then a grade each, with no rolls between. Each night's tops roll from `night-<day>`. Every other comp's field is unchanged, draw for draw.
- **The season:** the year's League nights (8). A night's places: the field's by tops desc, goes asc, then draw order; if you entered, your stored place, and the field from that place down shifts one. A night is worth `field + 1 − place`. Your table counts the best `LEAGUE.best` (6). Ties go your way.
- **Settled** in the night after the year's last League night, after that night's scorecard: if you're on the table, push `{ year, place, of }`; 1st pays `LEAGUE.prize` ($250).
- **Record Book:** `dynocomp` (place 1 at the dyno comp), `league` (any season at place 1).

## 2026-10-04 — Phase 25.4: an expedition team

**Built in the 2D rebuild's Phase 25.4.** Save v54: `team: { id, names: [lead, second], left, back, odds } | null`, `teams: { id, day, summit, high }[]`. Numbers [proposed] in `TEAM`; rules in `app/src/sim/team.ts`.

- **Fund** (`{t:'team', id}`): no team away, a trip in the book to `id` that ended `summit`, and cash ≥ the cost. 10 min.
  - Cost: round(your own trip's price there × 1.5).
  - Names: `TEAMS[(seed.length + day) % 5]`; the lead is your mentee instead when their level ≥ 8.
  - `back` = the last day your own trip leaving today would be away (day + out + days + home − 1).
  - Odds, fixed when paid: min(0.9, (1 − stormOdds) × (0.45 + 0.2 × min(2, your summits there) + 0.05 × min(2, your other trips there) + 0.1 if the mentee leads)).
  - A mentee who leads: `last` = back (no drift while away), and coaching is refused until they're home.
- **Nightly**, when the day that ended ≥ back (stream `events`, `team-<id>-<left>`): summit if next() < odds, high = the pitches; else high = int(0, pitches − 1). A line, the trip appended to `teams`, team null. A summit adds 5 to the old crowd's standing (`scene.old`, capped at 100).
- **Record Book:** `patron` when any of `teams` summited.
- **For Unreal:** nothing new in the expedition sim; this reads the trip book and rolls once.

## 2026-10-04 — Phase 25.3: the mentee

**Built in the 2D rebuild's Phase 25.3.** Save v53: `mentee: { name, level, sessions, since, last, told: grade[] } | null`, `family.mentee?: { name, grade }`. Numbers [proposed] in `MENTEE`; rules in `app/src/sim/mentee.ts`.

- **Take** (`{t:'mentee', do:'take'}`): at Send City with grade ≥ 7. Name `MENTEES[(seed.length + day) % 6]`, level 1.
- **Coach** (`do:'coach'`):
  - At Send City with its pass, once a day, energy ≥ 8; 120 min, −8 energy.
  - level' = min(max(level, your grade), level + 0.35 / (1 + level / 8)).
  - Lines at sessions 1, 5, 10, 20 and 35, and the first time `floor(level)` reaches 5, 8 or 10.
- **Let** (`do:'let'`): mentee null.
- **Nightly:** on day − last = 14, a warning; at ≥ 28, the mentee's gone. On the bills night, −$15.
- **Heir:** `family.mentee = { name, grade: floor(level) }`. At the heir's create, each skill gets + 4 × grade, and a line.
- **Record Book:** `mentor` at level ≥ 8.

## 2026-10-04 — Phase 25.2: guiding

**Built in the 2D rebuild's Phase 25.2.** Save v52: `outfit: { since, guides, till, last } | null`. Numbers [proposed] in `GUIDING`; rules in `app/src/sim/guiding.ts`.

- **Job `guide`** (place `shop`):
  - Ranks at 0 / 8 / 20 shifts, grade gates V6 / V8 / V10, raise $18, 3 posts a week.
  - Act `shop.guide`: 300 min, $50, −26 energy, −12 fed, before 10:00, technique and head +1.
- **Guests** (stream `events`, `guide-<day>`):
  - Count `[2, 3, 3][rank]`.
  - Goal `int(2, min(top, 4 + rank))`, where `top` is the hardest line on offer.
  - Level `max(0, goal − int(1, 2))`.
  - Nervous below 0.3.
- **Lines:** Roadside's, not on an expedition or wall, not hidden, not open, grade ≤ 9.
- **Value** of a line for a guest:
  - Under level − 1: 0.4.
  - Over goal: 0.15, or 0 if nervous.
  - Under goal: 1.
  - At goal: p × 3 + (1 − p) × 0.2, where p = 1/(1 + e^(−1.5 × (level + 1 − grade − nervous))).
- **Play** (`{guide: ids}`): distinct lines, one per guest. Score = Σ value / the best over all distinct assignments. The bonus is `playBonus(pay, score, 0.9)`.
- **Outfit** (`{t:'outfit', do}`):
  - `start`: at the shop, top rank, $3,000; guides 1; signed-up guide shifts dropped.
  - `hire` / `fire`: 1 to 3 guides.
  - `draw` and `sell` (60% of the price plus the till).
  - Nightly: if Roadside was open and not closed and it isn't winter, guides × (160 − 100); minus 25 every night.
  - Record Book `outfitter`.

## 2026-10-04 — Phase 25.1: guides, giving, and a trait

**Built in the 2D rebuild's Phase 25.1.** Save v51: `giving: { total, food, access }` (the total given this life, and the day each was last given), `guides: Record<crag, { pages, out: day | null }>`. Numbers [proposed] in `GIVING` and `GUIDE`; rules in `app/src/sim/giving.ts` and `guides.ts`.

- **Give** (`{t:'give', to}`):
  - `food`, at the market: $50, +5 psyche.
  - `access`, at the gear shop: $40, the old crowd's standing +2.
  - Each once per 7 days, 10 minutes.
  - `total` adds the cash given. An heir starts at 0.
- **Guide** (`{t:'guide', place}`):
  - **Lines:** a crag's lines are its routes, minus expeditions and walls, minus a myth not yet readable, minus a land line not yet bolted.
  - **To write:** every line sent and at least one first ascent of yours there, at the Lot, once a day, energy ≥ 10. A session is 180 min and −10 energy, and adds a page.
  - **Out:** at 5 pages, `out = day`.
  - **Royalties:** on the weekly bills night, cash += $15 × the number of guides out.
  - **Heir:** keeps the guides that are out.
- **Record Book:**
  - `guidebook`: any guide out.
  - `trait`: a quirk named, so traits are Phase 23.4's quirks.
  - `giving`: `total` ≥ 1,000.
  - No entry waits on a system any more.

## 2026-10-03 — Phase 18.7: the grade curve's knee, and the tuning (Evan's calls)

**Changed in the 2D rebuild's Phase 18.7.** No save change. Numbers [proposed].

- **Grade curve (`CURVE` = { knee: 10, top: 26 }):**
  - `needFor(g) = 8g + 2.4g²` for g ≤ 10.
  - Past that, `320 + 26 × (g − 10)`.
  - `levelOf` is its inverse: the quadratic's up to 320, then `10 + (skill − 320) / 26`.
  - Everything that reads a level (margins, windows, `gradeOf`, reach) follows.
  - **Save-compatible:** the skills are the same numbers, read as higher grades past V10.
- **Sponsors:** a sponsor dropping you sets `lost = day`. Any sponsor offer waits until `day − lost ≥ 28` (before, only the top tier waited, and only after the rival won).
- **Film:** `doc.over` 1 → 0 (the line at your grade). The offer text no longer says "harder than anything you've done".
- **Send City:** $4,500 (was $6,000).
- **The Games:** need 200 ladder points (was 260).

## 2026-10-03 — Phase 18.7: the ladders, and 18.5's audience

**Built in the 2D rebuild's Phase 18.7.** No save change. Rules in `app/src/sim/ladders.ts`; numbers [proposed] in `MEDIA`.

- **Ladders** (each `at(s)` = rungs climbed, the last rung the capstone):
  - outdoors = acts ended (`actOf(s) − 1`, capped at 5), The Line on top
  - comps = max over results of tier + 1 (5); the Games, entered, is the top, and a Games podium is a Record Book entry only
  - media = `mediaRung` (followers ≥ `MEDIA.known` 1,000 → 1; a sponsor of tier t → 2 + t; the film aired → 5)
  - business = 0 off the job, setting rank + 1 on it (1–4), 5 owning Send City, 6 with its peak ≥ 200 members
  - They show where you stand now: dropped by a sponsor or sold up, you're back down.
- **18.5 changed:**
  - A post's worth is the day's best send's (it was the sum over every first send).
  - Every follower gain (posts, an answered thread's +25%, the film's +50%) is × max(0, 1 − followers / 150,000).
  - A shelved film is re-offered once ≥ 56 days have passed (`doc.again`), with the same other conditions as the first offer.
- **Why:** uncapped and summed, a bot posting daily reached 1.6 million followers by day 415 against the rival's 60,000.

## 2026-10-03 — Phase 18.6: your own gym, and a crag

**Built in the 2D rebuild's Phase 18.6.** Save v50: `gym: { since, members, quality, till, last, setter, upgrades, peak, set } | null`, `bolted: string[]`. Numbers [proposed] in `OWN_GYM`, `GYM_SET` and `LAND`; rules in `app/src/sim/business.ts`.

- **Buy** (`{t:'gym', do:'buy'}`): the top setting rank (Head setter) and $6,000 in hand. Starts at 60 members, quality 0.45, till 0. Signed-up setting shifts go; setting acts and sign-ups are refused while you own it. `today` gets `pass` now and every morning while owned (the wall and the shower).
- **The night** (in sleep, after the media night):
  - quality = max(0.55 if a setter's hired else 0, quality − 0.04)
  - target = round(30 + 140 × quality + 25 with the board + 45 with the wall)
  - members = round(members + (target − members) × 0.1)
  - net = round(members × (1.6 + 0.5 with the café) + members × 0.12 × 14) − 120 − 70 − (55 with a setter)
  - till += net; last = net; peak = max(peak, members)
  - The morning's line follows. Under 0, a warning. Under −600, the bank sells: cash += max(0, round(3000 + till)), gym null.
- **Actions:**
  - `draw`: the whole till to cash.
  - `pay`: min(cash, −till) into a short till.
  - `sell`: cash += max(0, round(4200 + till)).
  - `hire` / `fire`: the setter.
  - `upgrade`: from cash, once each. The board $1,500, the café $1,200, the wall $2,500.
  - `set`: at Send City, once a day, energy ≥ 22; 240 min, −22 energy. quality = max(quality, `scoreSet(setBrief(s), set).score` if a set is given, else 0.45).
- **Land:** `PlaceDef.land`. Miller's Bluff is bought through the existing `{t:'unlock'}` for $9,000, 35 min and $5 from the Lot. Its nine lines are all `open`: six sport (V7, V8, V10, V11, V13, V15) and three boulders (V6, V9, V12). A land line is climbable and revealed only once it's in `bolted`.
- **Bolting** (`{t:'bolt', route}`): the land owned, at the place, not done yet. A sport line costs $140, 300 min and 35 energy; a boulder 120 min and 15 energy. An heir keeps `bolted` (with `unlocked`), but not the gym.
- **Ladder rung, for 18.7:** the setting rank 0–3; owning the gym, 4; peak ≥ 200 members, 5.
- **Opened:** the Record Book's Your Own Four Walls (`{gym: true}`) and The Back Forty (`{land: true}`).

## 2026-10-03 — Phase 18.5: media

**Built in the 2D rebuild's Phase 18.5** (Evan's call: followers back). Save v49: `media: { followers, engagement, posted, bait, sponsor: { tier, terms, due, tasks, strikes } | null, offer, heat: { grade, due } | null, doc, lost }`. Numbers [proposed] in `MEDIA` and `app/src/sim/content/media.ts`; rules in `app/src/sim/media.ts`.

- **Post** (`{t:'post', style}`, once a day): worth = Σ over lines first sent today of (max(0, grade) + 2)^1.4 × 1.6 outside × 1.3 not a redpoint × 2 a first ascent, + 400/12 for a podium today. Followers += round((5 for a story + 12 × worth × (0.5 + engagement / 100) × style) × reach), style straight 1, story 0.6 (+4 psyche), bait 1.6 (old crowd −1, threads likelier a week), ad 0.3 (pays the tier's ad). Engagement +6 a post (cap 100); −3 a night after 3 quiet days; under 25, followers −0.5% a night. Followers never pay cash.
- **Sponsors:** tiers at 2,000 / 8,000 / 30,000 followers: stipend $60 / $160 / $420 a 14-day cycle (brand ×1.5), ad $15 / $35 / $90. Asks: a send posted at grade − 1 (all); a shoot at a crag (`shoot-<day>`, tiers 2–3; 3 h, 20 energy); a comp result in the cycle (tier 3); an ad (brand). Offered overnight one at a time; at the top tier, if the rival's followers `60000 / (1 + e^(−(day − 140) / 35))` exceed yours, she's signed and it's offered again no sooner than 28 days on. At the cycle's end: all done, paid; else a strike; two, dropped.
- **Heat:** nightly if followers ≥ 2,000 and none pending, chance min(0.05, 0.012 × (1 + followers / 25,000) × 1.5 brand × 1.8 baited in the last week) (`heat-<day>`): send at your grade within 7 days for +25% followers, else −5%.
- **Film:** offered at the top tier with 40,000 followers; a line of grade + 1, outside, within 28 days: +50% followers, aired; else shelved.
- **Opened:** the Influencer (reach 1.25; 2,000 / 15,000 / 60,000 followers); Record Book entries at 5,000 followers, any sponsor, the top tier, and the film.

## 2026-10-03 — Phase 18.4: the comp ladder

**Built in the 2D rebuild's Phase 18.4.** Save v48: `comps: { on: { tier, day, tops } | null, points: { day, pts }[], results: { tier, day, place, of }[] }`. Numbers [proposed] in `COMP` and `app/src/sim/content/comps.ts`; rules in `app/src/sim/comps.ts`.

- **Rungs** (venue, on day `on` of every `every`, grades, field, fee, purse, win points, points needed): League night (Send City, 4/7, V0–5, 10, $10, 40/25/15, 10, 0); the Circuit (the Cave, 10/14, V4–10, 12, $25, 150/90/50, 25, 20); Nationals (the Training Center, 24/28, V7–13, 16, $40, 400/240/120, 60, 60); the World Series (TC, 17/28, V10–15, 20, $60, 1000/600/300, 120, 140); the Games (TC, 50/56, V12–17, 20, free, 3000/1800/1000, 200, 260).
- **Problems:** the wall's slot count (6 at Send City, 8 elsewhere), grades spread evenly lo→hi, ids `cp-<tier>-<day>-<n>` (stream `worldgen`, `comp-<tier>-<day>`). They stand in for the week's set only for someone signed up that day (`wallAt`); everyone else, bots included, climbs the week's set.
- **Sign-up** (`{t:'comp', do:'enter'}`): at the venue on the day, before 13:00, ladder points ≥ the rung's need, the fee paid; the day's pass comes with it. A top counts if sent (not a lap) in the problem's first 5 goes; recorded as the goes it took.
- **Field** (`events`, `field-<tier>-<day>`): field − 1 names from a pool of 24, grade `lo − 1 + float(0, hi − lo + 1.5)`; each problem topped if a roll < `1 / (1 + e^(−1.4 × (grade − problem + 0.5)))`, in `int(1..5)` goes. Place: rivals with more tops, or as many in fewer goes, ahead; ties to you. Points `round(pts × (field + 1 − place) / field)`; the ladder is their sum, each × 0.5^(age / 56 days). The card's handed in at the desk or at bed.
- **Opened:** the Scene path (Competitor: 3 comps, comp cruxes 4% kinder; Finalist: 2 podiums, 8%; Headliner: 6 podiums, 12% and indoors 3%); the Comp Beast quirk (two podiums; comp cruxes 10% kinder); the Record Book's Game On, Top Step and a podium at the Games.

## 2026-10-03 — Phase 18.3: the coach's roster and the warehouse's picks

**Built in the 2D rebuild's Phase 18.3.** Save v47: `coach: { clients, next, last } | null`, `haul: { day, picks, fatigue, pay } | null`. Numbers [proposed] in `COACH` and `HAUL`; content in `app/src/sim/content/setting.ts`; rules in `app/src/sim/work.ts`.

- **Roster** (`play.coach`, a focus per client, on a coach shift): size `[2, 3, 4][rank]`; a new client n (stream `events`, `client-<n>`): name `CLIENTS[n % 12]` skipping names on the roster, style of four, gap `int(1..3)`, scared at 25%. Today: tired − (day − last), floored at 0; scared also if `scare-<name>-<day>` < 0.25. Burns: +26 − 8 × tired (scared: +4), tired +2. Drill: +12, tired +1. Head: +5, not scared. Rest: tired −2. Progress ≥ 100 sends: +$6 × gap, a new client in the slot.
- **Picks** (`play.haul` starts them on a warehouse shift, which pays as ever; `{t:'haul', pick?}` picks or stops): offer for pick n = 3 of 7 (`haul-<day>-<n>`); weight × 0.75 at rank ≥ 2 × heat `[0.8, 1, 1.3]` (`haulday-<day>`); drop if `drop-<day>-<n>` < min(0.95, 2.2 × (fatigue / 100)²), costing `[0.4, 0.7, 1]` of the pay so far and ending the run; at six picks, or on stop, or at bed, the pay's banked.
- **Unreal:** each pick is its own action (the roll is the sim's, never the view's).

## 2026-10-03 — Phase 18.2: the café's rush and the diner's floor

**Built in the 2D rebuild's Phase 18.2.** No save change. Numbers [proposed] in `RUSH` and `FLOOR`; content in `app/src/sim/content/setting.ts`; rules in `app/src/sim/work.ts`. Bonus as 18.1, `round(pay × 0.4 × clamp((score − from) / (1 − from)))`, with each game's `from`.

- **Plays:** `play.queue` (every order's index, in the order made) on a café shift; `play.tables` (per wave, the tables taken) on a diner shift. Not on the double.
- **Rush** (stream `events`, `rush-<day>`): `[5, 5, 6, 6, 7][rank]` orders; drink from six `{make, tip}`; waits `int(2..9)`, +2 for a regular (30% of the first four, tip ×2). Made in order, a drink's done at the running sum of makes and tips if that's ≤ its wait. Score = tips / the best over every order. `from` 0.8.
- **Floor** (`floor-<day>`): 3 waves of `[3, 3, 4, 4][rank]` tables, party `int(1..6)`, mood easy (tip 2, juggle 0.08), fussy (4, 0.3) or grumpy (1, 0.04); kitchen quick (×0.6), steady (×1) or slow (×1.6). On in wave w = taken in w + taken in w−1; a table from wave w tips `party × tip × max(0, 1 − juggle × kitchen × (max(on[w], on[w+1]) − 1))`. Score = tips / the best over every choice of subsets. `from` 0.65.
- **Unreal:** the play rows sit beside the plain shift; the player sees one wave at a time (the sim takes the whole floor).

## 2026-10-03 — Phase 18.1: the setter's puzzle, and leave

**Built in the 2D rebuild's Phase 18.1.** Save v46: `leave: Record<jobId, day[]>`. Numbers [proposed] in `PLAY` (`from: 0.4`, `top: 0.4`), `SETTING` and `JobDef.leave`; content in `app/src/sim/content/setting.ts`; rules in `app/src/sim/work.ts`.

- **A played shift:** the `act` action takes an optional `play`. Today only the setting shift has one (`play.set`, five move ids). Refused, before any cost, unless it's five distinct moves from today's hand. The shift pays as ever, plus `round(pay × 0.4 × clamp((score − 0.4) / 0.6))`. Working it without a play pays the shift, so playing never pays less.
- **The brief** (stream `events`): wall from `set-<day>` (slab, vertical, overhang, roof, each with the move kinds that suit it); crowd from `set-w<week>` (beginners, regulars, comp team, kids); wanted grade `2 × rank + int(0..2)`; a hand of `[7, 8, 9, 10][rank]` moves from 13, always with one rest.
- **Grade** `max(0, round(0.6 × Σhard + 0.5 × max − 2))`. **Score** `0.35 × accuracy (1 − |grade − want| / 3) + 0.3 × flow (one crux at max ≥ 2; a rest before it; no dyno after a dyno; ≥ 3 kinds) / 4 + 0.2 × fit (non-rest moves of the wall's kinds) + 0.15 × crowd`. Crowd: beginners no 3 and a rest; regulars grade = want; team a dyno and a 3; kids a dyno and grade ≤ want.
- **Not solved:** across 60 briefs, more than half have a different best set, and none is best on more than a tenth (`work.test.ts`).
- **Leave:** at the night's reckoning, a missed signed-up shift with leave left (days a 56-day year at your rank, by job) is recorded and called in, not a warning.

## 2026-10-03 — Phase 17.7: the lines players read most

**Built in the 2D rebuild's Phase 17.7.** Save v45: `PersonLog.heard` and `PersonLog.ledge` (optional ints). Numbers [proposed] in `FIRE_TALK` (`open: 10`, `every: 7`); text in `app/src/sim/content/ambient.ts`; rules in `app/src/sim/ambient.ts`.

- **Fire line** (`lot.sit`): the speaker is `here[day % here.length]` of the people at the fire who have a pool (hazel, sage, frank). Open count = `min(pool, 10 + floor((day − since) / 7))`. If `heard < open`, say `pool[heard]` and `heard += 1`; otherwise walk a seeded shuffle of the open lines (stream `events`, derive `ambient-fire-<who>`) at index `floor(day / here.length)`. Replaces the act's fixed line.
- **Send cheer:** on a first send (not a lap), whoever was with you (the belayer if roped, else the first of hazel, sage, mara, rico, tam, then dex, met and present) says a line from their pool, walked by the number of sent routes; once a day each (day flag `cheer-<who>`).
- **Portaledge:** on `camp`, the expedition partner says `pool[ledge]` and `ledge += 1` while lines remain, then a walk indexed by day.
- **Unreal:** the walk is the shared `Rng`'s Fisher–Yates over indices; the pools are content, keyed by person id.


## 2026-10-03 — Phase 17.6: holidays and the family

**Built in the 2D rebuild's Phase 17.6.** Save v44: `folks: { calls }` and `life.retired.home`. Numbers [proposed] in `HOLIDAY` (`on: [7, 21, 35, 49]` of the 56-day year, `psyche: 10`) and `FOLKS` (`from: 1`, `every: 2`, in age years); rules in `app/src/sim/folks.ts`.

- **Holiday:** overnight, if the day that ended is a holiday and you slept at the Lot: +10 psyche; a day together (+1 bond, once a day) with each person met at tier ≥ 2 who isn't away, on a stint, or past Ray's or Tam's end; an event `{ k: 'holiday', id: season, who }` for the card. No choice.
- **Calls:** call n (0–4) is due when `day ≥ (1 + 2n) × 18 + 1`, fewer than five have been had, and you aren't retired. Overnight, the night one comes due, a line. The `call` action opens the `folks` encounter; an answer counts the call, may cost cash or lift psyche, and the last call's yes retires you (`home: true`). The first answer never goes home.
- **Epilogue:** home for good is its first line.

## 2026-10-03 — Phase 17.5: giving

**Built in the 2D rebuild's Phase 17.5.** Save v43: `PersonLog.gave` (the day of the last gift) and `FirstAscent.for` (a person id). Numbers [proposed] in `GIVE` (`every: 7`, `named: 2`) and `content/gifts.ts`.

- **Gifts:** one option per core person (hazel, sage, dex, mara, rico, tam, ray, frank) in their ordinary talk, listed last; due when no gift in the last 7 days and the cost can be paid. Costs cash, +1 bond, sets `gave`.
- **Named for:** the `name` action takes `for`, honoured only for someone met; +2 bond and a line. The first-ascent screen offers people at Regular (tier 2) or closer, each with a suggested name. The epilogue lists each own first ascent with a `for`.
- **Frank at the fire:** a talk option at night sets the day flag `fire-frank`; while it's set and he's at the Lot, he's at the fire (after Hazel, so she still deals). He has no hold'em style, so hold'em skips him.
- **Dex:** the "Come climbing?" ask and invite places, as for partners.

## 2026-10-03 — Phase 17.4: romance

**Built in the 2D rebuild's Phase 17.4** (Evan's call: Sage and Mara). Save v42: `romance: { who, stage, since, beatDay, over? } | null`, and `PersonLog.sparked`. Rules in `app/src/sim/romance.ts`, text in `content/romance.ts`; numbers [proposed] in `ROMANCE`.

- **Spark** (stage 1) due for sage or mara when `romance` is null, `sparked` unset and bond ≥ 5; listed after the person's arc entries, so a due arc beat plays first. Answers: start (`romance = { who, stage: 1, since: day, beatDay: day }`) or friends; both set `sparked`.
- **Beats 2–5** due when together, `stage = n − 1` and `day − beatDay ≥ 7`; each sets `stage = n`, `beatDay = day`. Forks: a fight dug in on, away 3; their chance taken, away 21.
- **End:** together and `day − max(last, away, beatDay) ≥ 21` → the `love-end` node first; it sets `over = day`, bond to at most 3, away 10. A romance that's over blocks any new spark.
- **Anniversary:** overnight, when `floor((day − since) / 18)` rises to 1 or more while together, a line.
- **Epilogue:** a line for together, another for over.

## 2026-10-03 — Phase 17.3: lives that change

**Built in the 2D rebuild's Phase 17.3.** Save v41. Numbers [proposed] in `LIFE` (`app/src/sim/dials.ts`); rules in `app/src/sim/lives.ts`. A year is `AGE.days` (18), counted from day 1 (`year = floor((day − 1) / 18)`).

- **Ray:** last season from the start of year 5 (day 91) to year 6 (day 109); from day 109 he's never present. Talk beat `ray/1` (`last-season`) due in that window if met. Crossing day 109 overnight, if met: a line, and every non-first beta of every Roadside route learned as told.
- **Tam,** from `since` (the day met): from `since + 3 years` his daily odds' base is halved, and a talk beat `tam/1` (`slowing`) is due; from `since + 5 years` he's never present, with a line the night it's crossed.
- **Frank,** from `since`: from `since + 4 years` he's at the Lot every evening (17:00–22:00), a line the night it's crossed, and talk beat `frank/1` (`parked`).
- **Stints:** for sage, mara, rico and tam, each year y ≥ 1, `events` stream, `stint-<who>-<y>`: chance 0.35; start a uniform day in the year; length 4–12 days; kind hurt or away at even odds. Last year's may run over. Present nowhere during one. Overnight lines for met people as one starts and ends.
- **Save v41:** `PersonLog.talked` (spoken to them; `notMet` now means not spoken to, so a meeting isn't skipped by a belay) and `PersonLog.life` (moments heard). Migration: everyone in an old save is `talked: true`.

## 2026-10-03 — Phase 17.2: milestones

**Built in the 2D rebuild's Phase 17.2.** No save change. Content in `app/src/sim/content/arcs.ts`; numbers [proposed] in `ARC`.

- **Arcs for Hazel, Dex, Mara, Rico, Tam, Ray and Frank,** on Sage's machinery: four beats each, beat n due at bond `ARC.bonds[n-1]` (1, 3, 5, 7) and `ARC.spacing` (5) days after the last; every option advances the arc; the fourth sets bond to at least 7. Hazel's beats open in her crag talk, not the Lot's.
- **Forks with stints away** (`away` days): Hazel 5, Dex 5, Mara 3 (a no) or 12 (her finger, either answer), Rico 4 or 14, Tam 4, Frank 7. Presence now returns nowhere for Hazel and Dex while away, as it did for partners.
- **Dex's bond:** a go at a place where Dex is, once met, counts as a day together (+1 a day), as for partners.
- **Epilogue:** close and far lines for Mara, Rico, Tam, Ray and Frank, by bond tier (Regular or closer is close).
- **Size budget:** 360 KB (Evan's call).

## 2026-10-03 — Phase 17.1: the cast

**Built in the 2D rebuild's Phase 17.1.** No save change. Numbers [proposed], in `CAST` (`app/src/sim/dials.ts`).

- **Five more people** (v0.956's), each placed by the seed, the day and the clock like Hazel, Sage and Dex, nothing stored:
  - `ray`: Roadside, weekends (day % 7 is 6 or 0), 8:00–14:00, dry days only. A local, not a partner.
  - `frank`: the Lot, 17:00–22:00, from day 3, on 40% of days (20% in winter), drawn from the `events` stream with `frank-<day>`. A local.
  - `mara`: the Gorge, 9:00–17:00, dry days, on 0.34 + 0.07 a bond tier. A partner.
  - `rico`: 11:00–20:00 on 0.55 + 0.07 a tier, at Moonstone on 40% of the days it's dry, the Cave otherwise. A partner.
  - `tam` (Tam Okonkwo): the Mesa, 7:00–14:00, dry days, 0.30 + 0.07 a tier. A partner.
  - Partners honour an invite and a stint away as Sage does; their daily draw is `<who>-<day>` on `events`.
- **Partners** are now hazel, sage, mara, rico and tam: any of them at your place belays, and climbing near them counts as a day together.
- **Locals' talk:** once a day, a sit (Ray 20 min, Frank 30) for a bond.
- **Names:** no two people share a first name, including the town's Wren and Otis; no two looks match.
- **Life timers** (17.3) run on the age clock (18 days to a year): Evan's call.

## 2026-10-03 — Phase 16: climbing above your grade

**Changed in the 2D rebuild, closing Phase 16** (Evan's call: a limit, tighter windows, and more cruxes above your grade). No save change. All numbers [proposed], in `OVER` (`app/src/sim/dials.ts`).

- **Reach:** a crux's margin is your level in the beta's style, capped at your overall level (the curve's inverse of the average of all five skills) + 1.5. Before, the style's own level counted however far past your grade it was, so a V12 power specialist climbed power at V15.
- **Windows:** at or under your level as before. Over it, `e^(0.9 m)` to one grade over, then an extra `e^(0.6 (m + 1))`. At three grades over the factor is 0: no window, and every hit test requires a window above zero. (The meter steps at 60 Hz, so before, a window narrower than a frame still passed whenever a frame landed in it.)
- **The limit:** a line whose own style is three or more grades over your reach, at its *listed* grade (a sandbag keeps its secret), refuses the go: "Beyond you, for now". A new climber (about V0.8) is refused the gym's and Roadside's V4–V5 until about V1.1.
- **Added cruxes:** from 1.5 grades over, in the line's style at its true grade, a go gets one more crux, two from 2.5. They go in the longest stretches with no crux or rest (0.4 moves clear of each), above the first bolt or stance on a rope, up to 1.2 moves long, and borrow the beta, and the pick, of the line's own crux nearest them. A fall at one counts as a fall at the crux it borrowed from, for beta unlocks. They exist only in the go; the topo shows the line's own.
- **The Cave** (Evan's call, with the limit): its weekly eight are V5–V11 with two at V7 (`[5, 6, 7, 7, 8, 9, 10, 11]`), where they were V5–V12, so a V6 on a wet day has something inside the limit.
- **Measured:** a career bot at V11 with power at V15 now sends the Crucible’s V15 (true V14) 42% of goes, and its V16, V17 and both V18 myths never, careful hands included. Before, at V12: 95%, 53%, 17% and 9%, and every time with careful hands.

## 2026-10-02 — Phase 16.6: a wall partner stays the night

**Changed in the 2D rebuild's Phase 16.6** (a fix). No save change.

- **Bivy:** when you bivy on a wall, the partner who was at the wall's crag that day (the latest half hour before dark that finds one) gets an invite for the next day at that crag from wake-up, so they're there to belay the next pitch. Before, an invite lasted only its day and a multi-day wall had no belayer after the first night.
- **Pace for criterion 1** (harness only): 4 s a tap and 15 s a go [proposed]; career bots take 18.4–20.4 h from creation to the forced retirement at 45.
- **Found, not changed:** the Crucible's V15–V18 boulders (5–8 moves, one crux) go for a V12 climber 95% to 9% of goes with human hands, and every time with careful hands. Worth knowing before the Unreal version copies their crux windows.

## 2026-10-02 — Phase 16.5: the next generation

**Built in the 2D rebuild's Phase 16.5** (Evan's call 4). Save v40: `family: { gen, forebear, lines, coach } | null`.

- **`heir` action,** retired only: a fresh state on the same seed, day + 1, wake time, name empty (the creation screen makes the kid). Carries: `firsts` (your own get `by: "kin:<name>"`), `record`, `van`, `dream.owned`, `unlocked`, `dog`, `dogs`, `mode`. Fresh: cash, people, routes, gear, scene, the expedition book, goals. `life.held = day − 1` so the kid's age starts at 22; `year.recapped` set to the years done. `family.lines` is the forebear's own first ascents; `family.coach` is Dex at Regular or closer.
- **Creation for a family:** talents dealt on `seed:gen`; lines for the forebear and Dex's coaching; no Free Solo choice (the mode carried).
- **Story:** a family climber's ladder swaps Act I for Their Shadow (five stages: a send; two outside; a regular; V4; `{ kin: true }`, a go on the forebear's line at the crag reachable soonest, then the easiest, met at once if there's none), then Acts II–V.
- **Coaching:** an edge, `{ train: 1.15 }` [proposed].
- **Display:** a first ascent's `by` resolves to a person's name or the part after `kin:`.

## 2026-10-02 — Phase 16.3: The Line

**Built in the 2D rebuild's Phase 16.3.** No save change.

- **The last act's end** (the `act` event for act 5) shows a four-step sequence instead of a card: the naming (the latest own V18 first ascent's name, grade and crag; Act V's pay); who was there (Act V's `with` lines for Hazel, Sage and Dex at Regular or closer, plus a line for the dog; an alone line if none); the credits roll under the title "The Line"; then a choice: retire now, or keep climbing (the clock stays on hold).
- **Retiring after The Line** is allowed at any age; before it, from 30 as before.

## 2026-10-02 — Phase 16.4: epilogues, and Evan's calls after 16.2

**Built in the 2D rebuild** (Phase 16.4, and calls on 16.1–16.2). No save change.

- **Epilogue:** a list of lines, each flagged `did` (names a deed) or not. Sources, in order: the origin's line (not a deed); the hardest own first ascent by name and place; expedition summits; the furthest path (tier 3: its title; else path and tier name); the quirk's name; masteries; the calling by rungs made (0–3, a line each; rungs > 0 is a deed); the first and last stance answers, quoted; a crowd at Beloved or Distrusted; Hazel, Sage and Dex at Regular or closer (a deed) or met and not (not); the dog, living or the last lost; the Homecoming if had; the latest Record Book entry other than retiring. Shown before the tally; the tally no longer lists people or dogs.
- **Criterion 2:** the ending names at least five deeds. Career bots at day 224: 6 to 11.
- **Act IV:** one expedition (Cerro Torre); Trango's stage becomes `{ outside: 1, grade: 12 }`; stage order Crucible, Cerro Torre, V12, Obsidian Tower, V14.
- **Late Bloomer:** `fx.age` 4 (starts at 26, forced on day 343); the spring cost (0.88 on power and fingers gains) is removed. It's the speed-run origin: Act V before day 343 needs a player about 10% faster than the bots.

## 2026-10-02 — Phase 16.2: Acts II to V

**Built in the 2D rebuild's Phase 16.2** (Evan's call 3, and a new call on Act V). Save v39: no new fields; `goals` now counts stages across all five acts.

- **Content:** `STORY`, five acts, each `{ title, goals, end: { title, text, cash, with } }`; counts 5, 5, 5, 5, 4. New aims: `{ outside, grade }` (distinct sends outside at the grade or harder, expedition pitches excluded), `{ at, grade? }` (a send at that crag), `{ wall }` (its last pitch sent), `{ summit }` (an expedition summit in the book), `{ reveal }` (a myth's `hiddenUntil` line sent), `{ myth }` (your first ascent of a V18).
- **Acts:** II V4–V7 (the Gorge, Moonstone, a V7); III V8–V10 (The Big Stone, El Cap, Wind River V9, a V10); IV V11–V14 (The Crucible, the Obsidian Tower, Cerro Torre, Trango, a V14); V V15–V18 (V16 at The Crucible, a V17, a myth read, a myth's first ascent). Act pay $40, $120, $200, $300, $500 [proposed].
- **An act's end:** its text, plus a line for each of Hazel, Sage and Dex at Regular or closer, on the card.
- **Act V puts the clock on hold (Evan's call):** from Act V there's no countdown, no forced retirement and no 43/44 lines; retiring stays allowed. Without it, forced retirement at 45 (~V14) came long before V18 (~day 900).
- **Migration:** a save with Act I done advances past later stages it already meets, with no events or pay.

## 2026-10-02 — Phase 16.1: age and retirement

**Built in the 2D rebuild's Phase 16.1** (Evan's call 2). Save v38: `life: { held, told, retired: { day, forced } | null }`.

- **Age:** `22 + origin age + floor((day − 1 − held) / 18)`. The Late Bloomer's `fx.age` is 10 (starts at 32). Call it age, never a year: a year is still 56 days. No decline with age.
- **Retiring:** a `retire` action, allowed from age 30, not on a wall or an expedition. Forced on the first van night at 45 (day 415; 235 for the Late Bloomer): never on a ledge or away. Morning lines at 30, 43 and 44 (`told` keeps them to once); from 43 the days left are shown.
- **Retired:** every action is refused, as for a Free Solo death. The tally reads the save: sends (inside and out, crags), the hardest, own first ascents (a V18 one counts as a myth), summits, the calling's rungs, paths taken and the open ones not taken, scars, people at Regular or closer, dogs, Record Book entries. An epitaph picks the first that fits: a myth first ascent, a summit, any first ascent, V11+, else "a life lived on rock".
- **Record Book:** `retired` (The Long Game) is earned by retiring.
- **Migration:** a save past the countdown's start (day 379 for a 22-year-old) has `held` set so it loads at 43; the age on loading counts as told.

## 2026-10-02 — Phase 23: each origin's first morning

**Built for Phase 23's criterion 1** (Evan's call A). No save change. Each origin has a `begin`, applied at creation after its skills and cash: `at` (where you wake), `min`, `today` flags (a day pass), `gear` merged in, a `shift` signed up for today, `insurance`, and a line said once.

- Sold It All: a café shift today. Gym Rat: Send City at 8:30, day pass on. Desert Local: Roadside at 6:40. Ex-Gymnast: Send City at 9:00, day pass on. Late Bloomer: full insurance. Trust-Fund Kid: shoes at 100, a crash pad, a headlamp.
- The client enters the start place's scene after creation.

## 2026-10-02 — Phase 23.8: when calls and echoes come

**Changed in the 2D rebuild's Phase 23.8** (amends 23.5). No save change.

- **When:** checked after a `travel` or a `done` (the end of a go), not only on arriving by road, unless an encounter is already up, the van's broken down, you're on an expedition, or dead. The roll is seeded by day and crag, so repeated checks on one crag-day don't add chances.
- **Where:** an echo can come anywhere (several are set off the rock: a gym car park, an access meeting); a new call only at a crag.
- **Naming a first ascent** (`{ t: 'name' }`) is allowed while an encounter waits, since it finishes the go that came before it. The client keeps a go's own card (fall, send, first ascent, the send card) up until it's closed, then shows the encounter.
- **Measured:** bots answering each call the way its echo comes back for see every echo land within a 224-day career (most on the first seed; the latest at day 155).

## 2026-10-02 — Phase 23.7: the year and the Homecoming

**Built in the 2D rebuild's Phase 23.7.** Save v37: `year` (`recapped`: years recapped; `home`: `'none'`, `'armed'`, or the day it was had). Action `{ t: 'home', arm }`; events `{ k: 'year', n }`, `{ k: 'homecoming', route, who }`.

- **A year is 56 days** (`YEAR.days`). After every action, when `floor((day − 1) / 56)` exceeds `recapped`, it's recapped: a card for that year, built from state by day ranges (first sends excluding expedition pitches, the hardest of them, crags sent at, your own first ascents, Record Book entries, expeditions home, calls answered, cash in hand). Migration marks years already done as recapped, with no card.
- **The Homecoming,** once a life: available when at least `HOME.people` (2) people are at bond tier ≥ `HOME.tier` (2, Regular), counting people (v0.956 counted diary entries). Armed (or put off) from the You page; on the next action that sends a line outside (not a repeat, not an expedition pitch) while armed, it pays `HOME.cash` (200) and `HOME.psyche` (20), records the day, and names who came.

## 2026-10-02 — Phase 23.6: the Board

**Built in the 2D rebuild's Phase 23.6.** Save v36: `board` (`week`, `jobs`: `{id, grade, paid}`, `shifts0`: shifts worked when it went up, `sessions`: sessions since).

- **One weekly board** replaces v0.956's café board, daily and weekly challenges and club jobs (`content/board.ts`, 14 jobs, de-duplicated). After every action, if `weekOf(day)` differs from `board.week`, a new board is posted: `BOARD.jobs` (3) jobs, a seeded shuffle on `derive('board-{week}')`, each stamped with `gradeOf(skills)` at posting.
- **Criteria:** sends anywhere; at ≥ grade + `over`; outside; outside at ≥ grade + `over`; of a route type; training sessions (counted in `train`); shifts (sum of `jobs` minus `shifts0`). Sends count first sends with `sent.day` in the board's week; expedition pitches don't.
- **Pay:** automatic, once, the moment progress reaches `n`, after every action: `cash` (8–18) and a line. Unfinished jobs lapse with the week; no penalty.
- **Harness:** the three best-paying jobs together pay under `BOARD.capDays` (2) days of the worst one-shift job's pay.

## 2026-10-02 — Phase 23.5: the scene, factions and stances

**Built in the 2D rebuild's Phase 23.5.** Save v35: `scene` (`old`, `gym`: standing 0..100 from 50; `stances`: `{id, opt, day}`; `echoes`: `{id, turned, day}`; `last`, `echoLast`: days of the last call and echo). Encounter kinds gain `stance` and `echo`.

- **Two factions:** the old guard (v0.956's trad, and the locals) and the gym crowd (its gym and comp). v0.956's media, purism, rep, followers and access fund are dropped. Words at 25 / 45 / 56 / 76: Distrusted, Skeptical, Neutral, Respected, Beloved.
- **What moves them:** answers to calls and echoes only, plus taking up a calling (Purist old +15; Send-or-Bust gym +13; Lifer old +8, gym +5). Sends don't (v0.956's per-send drift would saturate both).
- **Calls** (`content/scene.ts`, v0.956's five): on arriving at a crag (not a gym), with no road encounter, if at least `FACTION.gap` (14) days since the last call, a seeded `FACTION.chance` (0.22) picks one not yet faced whose `after` day has come (chip, closure, trashed 14; retrobolt, fa 28; fa also needs a first ascent of your own). Three answers; a fourth (the elder's) when `scene.old ≥ FACTION.elder` (65). Each answer: standing deltas, psyche, energy, and a journal line.
- **Echoes** (six): on arrival, before a call, the first echo whose stance and answer match a call made at least `FACTION.echoAfter` (30) days ago, if `FACTION.echoGap` (25) days since the last echo. Two answers: hold or turn.
- **Perks of standing** (v0.956's clubs folded in), as edges: old guard 56: crux windows outdoors ×1.02; 76: another ×1.02, and permits cost nothing. Gym crowd 56: session gains ×1.15; 76: another ×1.13.

## 2026-10-02 — Phase 23.4: paths, mastery and a quirk

**Built in the 2D rebuild's Phase 23.4.** Save v34: `paths` (`tiers`: path id → tiers claimed; `told`: "id:tier" already announced), `mastery` (styles mastered), `quirk` (id or null), `habits` (counts of goes: `goes`, `outdoor`, `fresh`, `tired`, `evening`, `dawn`, `easy`, `power`).

- **One edge type** (`content/paths.ts`) feeds every source: `windows` (crux windows ×, on lines of `style` if given), `firstGo` (× on a line never tried and never sent), `gym` (× indoors), `fresh` / `tired` (× at energy ≥ 70 / < 35), `evening` / `dawn` (× from 17:00 / before 09:00), `injury` ×, `gain` (one skill's learning ×), `gainAll` ×, `gas` ×, `living` ×. Edges multiply. v0.956's "+n% odds" became windows × (1 + n).
- **Paths:** Crusher (power, dyno), Tendon (crimp), Nerve (first go), Mover (technical; new), Dirtbag (trips); Scene waits for comps. Three tiers each; tier i needs `gradeOf(skills) ≥ PATH.gates[i]` (4, 9, 14) and its deed (sends in its styles 4 / 12 / 25 for the style paths, with Tendon's 2nd at 12 crimps and Crusher's 2nd 6 dynos; Nerve 3 / 8 / 15 flashes or onsights; Dirtbag 6 / 25 / 60 trips). Claimed by action `{ t: 'path', id }`, a tier at a time, at most `PATH.max` (2) paths. Each newly claimable tier is announced once. Tier 3 gives a title and line, no active ability.
- **Mastery,** one track, automatic, checked after every action: a style at `MASTERY_AT.sends` (40) lines sent in it (v0.956's style-mastery edges), and v0.956's ten hybrids, each two skills both at `needFor(MASTERY_AT.hybrid)` (V8's) with v0.956's hybrid edges. Ids share the `mastery` list (`hy_*` for hybrids).
- **Quirk:** each go counts into `habits` before its costs. Once `goes ≥ 30` and no quirk, the first of v0.956's tests that passes names it (Glass Cannon, Stone Purist, Weekend Warrior, Grinder, Night Owl, Sandbagger, Slow Burn at day 150; Comp Beast waits). One quirk, not v0.956's two.
- **Migration:** paths and quirk empty, habits zero, mastery backfilled quietly from sends.

## 2026-10-02 — Phase 23.3: callings

**Built in the 2D rebuild's Phase 23.3.** Save v33: `calling` (`id` or null, `since`: the day taken up, `rungs`: the day each was met, `offered`).

- **The offer:** after any action, a climber with no calling, never offered, and at least `CALLING.offerAt` (10) lines sent is offered once (event `{ k: 'calling' }`, `offered` set). Taking one up (`{ t: 'calling', id }`) is permanent; only the open callings can be taken.
- **Callings** (`content/callings.ts`), fx [proposed]:
  - Purist: `gainOut` 1.15 (skill from goes outside ×).
  - Send-or-Bust: `firstGo` 1.05 (crux windows × on a line never tried and never sent; shown on the beta bars), `injury` 1.1.
  - Lifer: `living` 0.8 (the night's spot and lifestyle ×), `injury` 0.75 (on any line, unlike tendon talents).
  - Influencer: no fx, `wait`: not offered until followers and sponsors exist.
- **Rungs,** counting only sends whose `sent.day >= since`: Purist, a line sent outside at V8 / V11 / V14; Send-or-Bust, flashed or onsighted outside at V6 / V9 / V12; Lifer, 5 / 9 / 13 grades each with `CALLING.consolidate` (5) lines sent. Checked after every action, in order (a rung only after the one below); each met adds the day to `rungs`, psyche `CALLING.psyche[i]` (8, 12, 16) and a note.
- v0.956's personality seeds and faction shifts from callings are cut.

## 2026-10-02 — Phase 23.2: origins and talents

**Built in the 2D rebuild's Phase 23.2.** Save v32: `origin` (an id, `'across'` for a v0.956 climber, or null) and `talents` (`ids`, `known`, `from`: the skills at creation).

- **Six origins** (`content/origins.ts`), picked at creation with a name and a start. Each adds `skills` to the start's (floored at 1) and has `fx`, all [proposed]: `pay` (shift pay ×, after rank raises), `gainIn` / `gainOut` (skill from goes indoors / outside ×), `gainTrain` (from sessions, the speed wall's too ×), `spring` (power and fingers from anything ×), `living` (the night's spot and lifestyle ×), `premium` (insurance ×), `shop` (kit and food bought ×), `bill` (dollars a week on the weekly bills), `cash` (on top of the start). Prices round after the multiplier, and the UI shows the same functions' results.
  - Sold It All: pay 1.12, +$150, $8/week; endurance +2, head +2.
  - Gym Rat: indoors 1.12, outside 0.9; power +3, fingers +3, head −3, endurance −1.
  - Desert Local: living 0.85, −$25; head +4, technique +3, power −3.
  - Ex-Gymnast: sessions 1.2; power +5, fingers +2, head −5, technique −2.
  - Late Bloomer: premium 0.65, +$120, spring 0.88; head +4, technique +3, power −3, fingers −2.
  - Trust-Fund Kid: shop 0.7, +$250, pay 0.85; technique +2.
- **Talents** (`content/talents.ts`, v0.956's 11): with an origin, one good and one bad are dealt on `Rng.fromStream(seed,'events').derive('talents')` (good list index, then bad). Each multiplies all gains of one skill (1.35 / 1.1 good, 0.7 bad); Bomber and Glass Tendons also multiply injury chance on crimp and crack lines (0.55 / 1.7). Hidden until that skill is `TALENT.reveal` (6) above `from`, or, for a tendon talent, on a finger injury; then a note and `known`.
- **A carried v0.956 climber** gets `origin: 'across'`, no fx, no talents. No origin (old saves, tests, bots) means no fx and no talents.
- **Act I's first goal** ($60 in hand) is met at creation for the richer origins, as in v0.956.

## 2026-10-02 — Phase 23.1: the Record Book

**Built in the 2D rebuild's Phase 23.1.** Save v31: `record` (entry id → the day earned).

- **One book** replaces v0.956's feats, milestones and story cards: 37 entries, each carrying the v0.956 ids it merged (`from`). 27 have an `aim` and are earnable now; 10 `wait` for systems not yet built (comps, media, sponsors, guidebooks, giving, retirement, traits) and are hidden until then.
- **Earning:** after every action (once a climber exists), every entry not yet in the book whose aim is true goes in, dated today. Aims read existing state only: sends (count, highest route grade, flash/onsight, outside), first ascents, every open crag sent, grade above Dex's, injury or scar, day > 56, a shift worked (walk-ins included), a job rank ≥ 1, ever let go, cash ≥ N, kit kinds ≥ N, a dream owned, a breakdown, a bond tier ≥ 3, an expedition summit, a dog.
- **One moment, one card:** all entries earned by one action make one event, `{ k: 'record', ids }`; the card shows the first entry's story and names the rest.
- **No reward** beyond the entry. Ids are stable for Steam achievements (Phase 14).
- **Migration:** an older save's record is filled with every entry its state already meets, dated the load day, with no cards.

## 2026-10-02 — Phase 24.9: a summit pays once

**Built in the 2D rebuild's Phase 24.9**, confirmed by Evan (2 Oct 2026). No save change.

- **Pay:** an objective's `pays` is paid on its first summit only: if the book holds a summit of that objective, a summit pays 0. (Before, every summit paid, and a climber two grades past El Cap netted about +$1,175 a trip.)
- **No-farm target:** for each objective at every grade from `gradeReq` to `grade + 2`, the bots' mean net cash (pay − trip − food; bills excluded) per calendar day away, for a repeat trip, must be under the worst-paid shift's daily pay.
- **UI rule:** while a go is under way on an expedition pitch (and its stamp shows), no sheet covers the wall; the day's card returns after.

## 2026-10-02 — Phase 24.8: Trango's scene

**Built in the 2D rebuild's Phase 24.8.** No rule or save change; what the scene shows is spec.

- **While away on Trango,** the backdrop is base camp on the Trango glacier: the Nameless Tower (golden, square-topped) on its snowy shoulder, Great Trango left, the Monk and the Pulpit right, Karakoram snow peaks behind, the glacier with medial moraines and base camp's tents. Variants: day, night (one tent lit), storm (towers in cloud, snow), storm at night.
- **Your position:** the portaledge marker follows Eternal Flame (approach gully, snow ledge, south face) by `pitch / pitches`.
- **A pitch's wall view** is golden granite with a snowy belay ledge and the glacier below.
- **All three objectives** now have their own scene and wall view; storms snow wherever the objective's water is snow.

## 2026-10-02 — Phase 24.7: Cerro Torre's scene

**Built in the 2D rebuild's Phase 24.7.** No rule or save change; what the scene shows is spec.

- **While away on Cerro Torre,** the backdrop is the view from above Laguna Torre: the needle with its rime mushroom, the Southeast Ridge as its left skyline from the Col of Patience (El Mocho beside it), Torre Egger and Standhardt right, the ice cap behind, the Torre glacier, moraine and lake with bergs below. Variants: day (lenticular clouds), night, storm (summit plume, cloud on the towers), storm at night.
- **Precipitation:** storms on objectives whose water is snow (`melt`) fall as snow, on the scene and the wall view; El Cap's are rain.
- **Your position:** the portaledge marker climbs the Southeast Ridge by `pitch / pitches`.
- **A pitch's wall view** is rimed granite with ice in the corner, a snowy belay ledge, the glacier below; your partner belays from the ledge.

## 2026-10-01 — Phase 24.6: El Capitan's scene

**Built in the 2D rebuild's Phase 24.6.** No rule or save change; what the scene shows is spec.

- **While away on El Cap,** the backdrop is El Cap Meadow: the wall (the Nose between the lit southwest face and the shaded southeast; the Heart; the North America diorite), the forest at its foot, the meadow. Variants: day, night (moon, stars, other parties' headlamps), storm (summit in cloud, rain), storm at night. Weather is the trip's `stormOn` for the day.
- **Your position:** a portaledge marker at `pitch / pitches` of the way up the line, with the count beside it.
- **A pitch's wall view** is El Cap granite with a belay ledge at its foot, the haul bag and portaledge hanging below, and the meadow far down. Your partner belays from the ledge (not whoever is at the Lot).

## 2026-10-01 — Phase 24.5: coming home

**Built in the 2D rebuild's Phase 24.5.** Save v30: `book` (a list of trip logs).

**The rules:**
- **Every end of a trip** (summit, bail, out of water, out of days) runs one path: expedition cleared; bond with the partner moved by +2 (summit), −1 (water), +1 (otherwise, if pitches fixed × 2 ≥ the objective's pitches), else 0; the `home` travel nights; then a log appended: `{id, day (calendar, after travel), end, high (pitches fixed), partner, nights, seen, told: false}`, and a `home` event (the UI's trip card).
- **High point:** per objective, the max `high` over its logs, and whether any ended in a summit.
- **The story** (`story` action): only for the newest log, untold; at the Lot after dark with someone at the fire. Costs 30 min; psyche +8 for a summit, +4 otherwise (clamped to 100); each person at the fire counts it as a day together (the once-a-day bond); marks it told.

## 2026-10-01 — Phase 24.4: what happens up there

**Built in the 2D rebuild's Phase 24.4.** Save v29: `expedition.seen` (ids of this trip's events).

**The rules:**
- **Events as data:** six, each two options with effects `{energy, food (days), pitch, ledge: false, bond, psyche, day}`. `stove` only where the objective's water is snow and a stove was packed; `party` only with a partner.
- **When:** after each portaledge night that doesn't end the trip, if fewer than 2 have come: roll `Rng.fromStream(seed, 'events').derive('wall-{objective}-{calendar day}')`; under 0.15, pick uniformly from the eligible, unseen events with the same stream's next draw. The event is an encounter (`kind: 'wall'`) and blocks everything but its answer.
- **Answer:** apply the effects (energy and psyche clamped to 0–100, food and pitch floored at 0); `day` runs another portaledge night, which can bring the next event.
- **Odds:** the DP carries the count of events so far; on any day after the trip's first, other than today, with fewer than 2, an event comes with p = 0.15 and is modelled as the day lost (no climbing, no night's cost beyond the day).

## 2026-10-01 — Phase 24.3: getting there and back

**Built in the 2D rebuild's Phase 24.3.** No save change.

**The rules:**
- **Travel:** each objective has `out` and `home` days (El Cap 1/1, Cerro Torre 2/2, Trango 8/6). `go` runs `out` away-nights (bills, psyche, the day count) before the wall's day 1; every way a trip ends (summit, bail, out of water, out of time) runs `home` away-nights back to the Lot.
- **Trip window:** from the booked day to day + out + days + home − 1. Sign-ups inside it are refused while booked. Booking ≥ 7 days ahead removes signed-up shifts inside it; otherwise they stay and are missed (a warning each, as any missed shift).
- **Leads per day:** at most 3 goes on expedition pitches a day (counted across the wall's pitches).
- **Odds:** the wall's day 1 is the booked day + out for the forecast. Each pitch's send chance is played at an estimated skin: skin − (trip day − today) × max(0, 3 × the wall's mean skin per go − 22) − 2 × that per-go skin, and the DP's goes a day are capped at 3.

## 2026-10-01 — Phase 24.2: planning and packing

**Built in the 2D rebuild's Phase 24.2** (`dirtbag/app/src/sim/expeditions.ts`; `EXPED` in `dials.ts`). Save v28. Evan confirmed 24.1's partner grades (yours −1 Hazel, +1 Sage) and handing over a pitch.

**The rules:**
- **Plan:** `{ day, partner, food, ledge, stove }`. Book (`exped do: 'book'`) up to 7 days ahead; pays the trip cost (War Chest halves it) + $12 × food. Refused under the grade gate, roped without an eligible partner, solo with one, food outside 1..days−1, or a bag over 80 kg. `booked` holds it; `cancel` refunds half (rounded); a sleep past the day with no expedition drops it, no refund. `go` only on the booked day, at the Lot.
- **Bag:** kg = food × 3 × (melt objective without stove ? 2 : 1) + 9 (portaledge) + 2 (stove). Cerro Torre and Trango are melt objectives.
- **Nights:** a camp needs food > 0 (else: down, trip over); food −1; energy back = max(0, round(nightBack(nights) × (ledge ? 1 : 0.5) − max(0, kg − 35) × 0.4)), kg after eating.
- **Forecast:** for a day `lead` days from today, a call of storm/clear = truth, flipped if a seeded roll (`fc-{id}-{day}-{lead}`) ≥ accuracy, accuracy = 1 at lead ≤ 0, else max(0.5, 1 − lead / 20). Storm chance used by the odds = the Bayes posterior of the call given stormOdds and the accuracy (today: the truth).
- **Odds:** the 24.1 DP, with each day's storm chance from the forecast, and the trip ending when food runs out.

**What it means here:** the expedition card is a planner whose every choice moves the shown summit odds.

## 2026-10-01 — Phase 24.1: expeditions climbed

**Built in the 2D rebuild's Phase 24.1** (`dirtbag/app/src/sim/expeditions.ts`, pitches in `sim/content/expeditions.ts`; `EXPED` in `dials.ts`). Replaces 21.5's dice-per-pitch model. Save v27.

**The rules:**
- **Pitches:** each objective has `pitches` lines (6/8/11), each a single-crux route like a valley wall pitch (`libraryPitch`), graded up to the objective's grade, with `exped: id`. Ids `elcap-1`…
- **State:** `expedition = { id, day, pitch, partner | null, nights }`. While set, only `exped` actions and goes on the current expedition pitch are allowed.
- **Go:** needs the grade gate, cash in hand (War Chest halves), and on a rope a partner: the highest-bond person with a `grade` at bond ≥ BOND.tiers[2]. Free Solo: partner null.
- **Blocks:** pitch i is yours if floor(i / 2) is even (solo: all yours). A go on your current pitch costs 150 min, 15 energy, 6 fed; refused in a storm (seeded `storm-{id}-{day}` < stormOdds), after dark (19:00), or under 10 energy. A send advances the pitch; the last pitch pays and teaches head +2.
- **Follow** (their block, or yours handed over; once a day; not in a storm): the partner tries up to 2 times on consecutive pitches, each fixing it if a seeded roll (`lead-{id}-{day}-{t}`) < clamp((0.4 + 0.1 × (partnerGrade − pitchGrade)) × windows, 0.05, 0.95). Partner grade = your grade + offset (Hazel −1, Sage +1). Costs you 15 energy.
- **Camp:** a portaledge night: energy + round(45 × 0.92^nights), fed raised to 60, day + 1. Past `days`: home, nothing paid.
- **Windows up there:** body factors × thin (El Cap 0.6, Cerro Torre 0.65, Trango 0.65) × max(0.82, 1 − 0.015 × (day − 1)). No valley weather, sun or crowd.
- **Odds:** each of your pitches' send chance per go = the fraction of 24 seeded plays with human-ish hands (bot `humanHands`), at trip days start/middle/end (interpolated), with food as of the day's third go. Then an exact DP over (day, pitch, energy, nights): storm with stormOdds; your block = goes until energy/light; partner's day = the 2-try distribution; on your block, the better of leading and handing over.

**What it means here:** an expedition is a long wall of real climbing, and the odds are a model of the climbing, not a separate roll. Cerro Torre's objective is now the Southeast Ridge.

## 2026-10-01 — Phase 22 closed: Evan's calls

**Rule change (2D 0.984.0):** a night at cards (blackjack or hold'em played today) counts as psyche's "company", once a day, the same as climbing with someone. It moves no bond.

**Confirmed as built** (no longer proposals): the fire games' bond at most +1 per person per day however many games; no win bonuses from v0.956; Sage at the fire 40% of nights at tier ≥ 2; blackjack pays ceil(1.5 × bet) on a blackjack ($8 on $5); no insurance; the hold'em styles; v0.956's history trivia stays out; the dreams keep v0.956's prices.

**Next:** the 2D build's CURRENT MILESTONE is Phase 24, expeditions as trips.

## 2026-10-01 — Phase 22.9c: the glossary

**Built in the 2D rebuild's Phase 22.9c** (`dirtbag/app/src/sim/content/glossary.ts`). Content only, no rules or save change.

**The spec:** v0.956's trivia game is gone. Its technique, gear and culture terms are entries in a glossary, with the rebuild's own climbing words: 45 terms in five groups (on the wall, how it went, kit, grades, the life), each a term and a short definition, shown as a journal page. Nothing unlocks or pays. v0.956's history questions are not carried.

**What it means here:** a reference screen in the journal, reading the same data. The text is the spec; keep it in sync with the 2D content file.

## 2026-10-01 — Phase 22.9b: blackjack and hold'em

**Built in the 2D rebuild's Phase 22.9b** (`dirtbag/app/src/sim/cards.ts`, styles in `sim/content/cards.ts`; `GAMES.bj`, `GAMES.holdem` in `dials.ts`). Evan's calls on stakes and shape. Save v26. Simulated gambling: store content ratings must say so.

**The rules:**
- **State:** `cards = { game: 'bj' | 'holdem', who, chips, hands, bj, he } | null` and `reads: Record<who, { hands, caught }>`. While `cards` is set, only that game's actions are allowed.
- **Sit:** at the Lot at night with someone at the fire, ≥3 energy, cash ≥ one hand's need ($5 blackjack; $11 hold'em, the ante plus every bet); 60 min, 3 energy, once a night per game. Chips = min(floor(cash), $40), taken from cash; leave returns them.
- **Blackjack:** `{ t: 'bj', do: sit | deal | hit | stand | double | leave }`, dealt by the first at the fire. Deck per hand from `bj-{day}-{hand}`, a full 52 shuffled; you get cards 0 and 2, the dealer 1 and 3. Either blackjack settles at once (dealer peek). Hit to 21 or bust settles. Double: two cards only, bet doubled, one card, then stand. Dealer draws below 17 and stands on all 17s. Pays: blackjack bet + ceil(1.5 × bet); win 2×; push 1×. 10 hands a night, $5 a hand.
- **Hold'em:** `{ t: 'holdem', do: sit | deal | fold | call | bet | leave, who }`, heads-up with a chosen person at the fire who has a style. Deck per hand from `he-{who}-{day}-{hand}`: you 0 and 2, them 1 and 3, board 4–8. Ante $1 each. Streets 0 (no board), 1 (flop, 3 cards), 2 (turn and river, all 5); one bet a street of $2/$4/$4. With no bet facing you: check (call), bet or fold. They respond to your bet by calling or folding, and to your check by betting (you then call or fold) or checking. No raises. Showdown after street 2; ties split.
  - **Their play:** equity e = a 60-deal seeded sample of their hand against a random one, given the visible board. Facing a bet: call if e ≥ style.call − 0.15 × theyKnow. Else: bet if e ≥ style.bet, or on a seeded bluff roll < style.bluff. Hazel 0.50 / 0.68 / 0.05; Sage 0.40 / 0.56 / 0.30.
  - **Reads:** every hand played increments `reads[who].hands`. A loss at showdown after you bet street 2 increments `caught`. theyKnow = min(1, caught / hands / 0.25), and 0 below 5 hands. Reads unlock at 5 hands (how they bet) and 12 (bluff rate as "one in N", from style.bluff).
- **Pay:** money only. No bond.

**What it means here:** two money games, kept near even. A sensible player wins about $1 a night at hold'em and blackjack is roughly break-even; the hold'em opponents adapt to a player who bluffs too much.

## 2026-10-01 — Phase 22.9a: horseshoes and liar's dice

**Built in the 2D rebuild's Phase 22.9a** (`dirtbag/app/src/sim/fire.ts`; `GAMES` in `dials.ts`). Two of v0.956's fire games, rebuilt with a time cost and a bond cap. Save v25.

**The rules:**
- **Who's there:** at the Lot after dark. Hazel always; Sage if bond tier ≥ 2, not away, on a seeded 40% of nights (`sage-fire-{day}`).
- **Each game:** 60 min, 3 energy, once a night (`today` gets `shoes` / `dice`). Refused away from the Lot, by day, with nobody there, or under 3 energy.
- **Bond:** each player at the table gets the same call as climbing together: +1 bond and `last = day`, at most once a day per person, however many games. `last = day` is psyche's "company".
- **Horseshoes:** `{ t: 'shoes', throws: [4 × 0 | 0.5 | 1] }`, scored by the player's timing on the busking beat: 1 = ringer (3 pts), 0.5 = leaner (1), 0 = miss. Each other player's 4 throws come from `shoes-{who}-{day}`: ringer 25%, leaner 35%. A line says the score against each.
- **Liar's dice:** `{ t: 'dice', do: 'deal' | 'call' | 'raise' }` against the first at the fire. Deal (from `dice-{who}-{day}`): 5 dice each; their bid is on their most-held face (ties to the higher), at held + 0/1/2 (35/40/25%). State `table = { who, mine, theirs, bid }` blocks every other action until resolved.
  - call: you're right if both cups hold fewer than the bid.
  - raise: your most-held face at the bid's count if that face is higher, else count + 1. They call if it needs more than 1 of your dice past what they hold; otherwise they let it go (yours).
  - Nothing is staked; the result is a line.

**What it means here:** v0.956 paid +1 bond to Sage, Rico and Mara per game with no time and no cap (the audit's bond farm). The rebuild prices a game in an hour and caps bond at a day's worth per person. Liar's dice's opener is tuned so the player's own dice are the tell (calling holding none of the face is right ~64% of the time).

## 2026-09-30 — Phase 22.8: dreams

**Built in the 2D rebuild's Phase 22.8** (`dirtbag/app/src/sim/dreams.ts`, content in `sim/content/dreams.ts`; `DREAM` in `dials.ts`). v0.956's dreams and prices. Save v24.

**The rules:**
- **State:** `dream = { pick, pot, owned[] }`.
- **Actions:** `{ t: 'dream', do: 'pick' | 'stash' | 'take' | 'claim' }`.
  - pick: any dream not owned.
  - stash: an amount from cash in hand, refused if cash < amount (never the card).
  - take: the whole pot back to cash.
  - claim: pot ≥ the picked dream's cost; the pot pays it, the dream is owned, and pick is cleared.
- **The pot:** bills and card spending never touch it. The broke check (the hustle's teach line) uses cash + pot.
- **Dream Rig ($2,800):** every spot's price × 0.5 (rounded; gas unchanged), and every van upgrade set owned.
- **War Chest ($5,000):** expedition cost × 0.5 (rounded).
- **Home Base ($9,000):** the Lot's spot price 0 and Lot ticket odds 0, and a kitchen owned. The weekly bills are unchanged.

**What it means here:** long-horizon savings goals with a protected pot and permanent cost modifiers. At the 2D build's worker saving rate, they take roughly 170, 300 and 550 days (a tuning note, not a rule).

## 2026-09-30 — Phase 22.7: Scout's life

**Built in the 2D rebuild's Phase 22.7** (`dirtbag/app/src/sim/scout.ts`, text in `sim/content/dog.ts`; numbers in `DOG` in `dials.ts`). v0.956's lifecycle, words and numbers. Save v23.

**The rules:**
- **Perks by bond** (and only while fed, ≥30). Each is said the day bond crosses it:
  - settle, ≥20: the fire's psyche lift +2;
  - find, ≥45: each van night, `events`, `scout-find-{day}`, p 0.25, +1 serving of a random ingredient;
  - watch, ≥75: Lot ticket odds × 0.5.
- **Age:** floor((day − since) / 18) dog-years.
- **At the night's turn over** (age at the ended day < n ≤ age at the new day):
  - n = 8: the gray line;
  - n = 12: the senior line;
  - n = 10: The Limp, −$70; n = 13: The Lump, −$130 (flat cash, not insurance).
  - Gray and older: the crag lines are the slower set.
- **The end:** on a van night when age ≥ 16, encounter `farewell` (id = the dog's name). Every action but the answer is refused.
  - Three answers, text only. After one: the closing line (best-friend version if bond ≥ 70) and a log entry "{name}, {years} years. {legacy}. Best dog in the valley and everybody knew it."
  - Then `dogs.push({ name, years, day })` and `dog = null`.
- **After:** the stray offer needs 30 days since the last dog went. Names go Scout, Moss, Juniper, Biscuit, by how many you've lost.

**What it means here:** a companion with bond-gated passive perks, a fixed aging timeline with scripted beats and costs, and a permanent, player-shaped ending.

## 2026-09-30 — Phase 22.6c: walk-outs

**Built in the 2D rebuild's Phase 22.6c** (`dirtbag/app/src/sim/events.ts`, content in `sim/content/epics.ts`; numbers in `EVENTS.epic` in `dials.ts`). v0.956's text and numbers; the headlamp's price is [proposed]. Save v22.

**The rules:**
- **Trigger:** a `travel` from a crag, checked before the drive. Needs: day ≥ 3, not injured, no encounter today, ≥10 days since the last walk-out.
- **Risk:**
  - Late: +2 if min ≥ 19:00, +1 more if ≥ 21:00, and +3 more if late without a headlamp.
  - Conditions: +2 for rain at the crag, +1 in winter.
  - The body: +2 if energy < 25, else +1 if < 45; +1 if food ≤ 15; +1 if supplies ≤ 0.
  - +1 if you climbed with nobody today.
- **Odds:** clamp((risk − 4) × 0.055, 0, 0.45) against `events`, `epic-{day}`.
- **Kind:** rain → storm; on a wall → stuck; late in winter → cold; late → dark; else lost.
- **State while it runs:** `encounter = { kind: 'epic', id: kind, stage, tally }`. Each answer adds its option's risk, energy, fed, skin, psyche and hours (1 when unspecified); three stages.
- **After the last stage:** the time passes (hours × 60 min) and energy, fed and skin change by the tally; psyche changes by (tally + 5) × 0.5. The end is clean if risk ≤ 2, rough if ≤ 6, else bad.
  - A bad end is an injury: 50/50 tier 1 or 2 ("jammed" or "rolled ankle"), days from the injury table, billed as any injury.
  - The ending's line is said, and the story goes in the log.
- **Afterwards:** you're still at the crag and drive normally.
- **The headlamp:** owned gear, $18 at the gear shop.

**What it means here:** a multi-stage choice sequence gated by a risk score from the day's state, resolving by thresholds into outcomes with a persistent story line.

## 2026-09-30 — Phase 22.6b: hitchhikers and roadside stops

**Built in the 2D rebuild's Phase 22.6b** (`dirtbag/app/src/sim/events.ts`, content in `sim/content/road.ts`; numbers in `EVENTS` in `dials.ts`). v0.956's text and hitchhiker numbers; stop odds and the day-3 start [proposed]. Save v21.

**The rules:**
- **No encounter of any kind before day 3** (this now applies to knocks too).
- **Trigger:** a `travel` to or from a crag that doesn't break down, with no encounter yet today. Roll: `events`, `drive-{day}-{min}` (min on arrival).
  - First draw < 0.14, and ≥5 days since the last hitchhiker: a hitchhiker, picked by weight (2.6 if met, else 1) with a second draw.
  - Else, if the first draw is in [0.14, 0.34) and ≥3 days since the last stop: a stop, drawn from the unseen ones (all, once every one is seen).
- **The encounter is set after arrival;** every action but `answer` is refused until it's answered.
- **Hitchhiker, first meeting:** their 3 answers plus "drive past" (index 3: no effect, not remembered). Picking an answer records `met[id] = index`.
- **Hitchhiker, later meetings:** their 2 `again` answers, minus any whose `when` differs from `met[id]`.
- **Effects:** psyche × 0.5; cash; energy; `beta`, the most useful unknown beta at the destination (the one a watching partner would teach); `guitar` (+sets).
- **Stop:** 0 = pull over (40 min; effects × 1 the first time, × 0.4 after, psyche then × 0.5), 1 = keep driving.
- **Breakdown:** with no friend to call, a met hitchhiker other than the grifter or thief comes along on a seeded 50% (`events`, `friend-{day}`), and works as the friend fix.

**What it means here:** a travel-triggered encounter layer with persistent memory of each NPC's first choice, feeding a later rescue.

## 2026-09-30 — Phase 22.6a: the event deck, and knocks at night

**Built in the 2D rebuild's Phase 22.6a** (`dirtbag/app/src/sim/events.ts`, content in `sim/content/knocks.ts`; numbers in `EVENTS` in `dials.ts`). v0.956's numbers and text. Save v20.

**The rules:**
- **State:** `deck = { last, knock, seen[] }` (day of the last encounter of any kind, night of the last knock, knock ids heard, last 10) and `encounter = { kind: 'knock', id } | null`.
- **At most one encounter a day:** no knock on a day with `deck.last === day`.
- **A knock** can come when you sleep in the van (not the pullout): odds 0.11 × spot (lot 1.4, truckstop 1.2, trailhead 0.9, ridge 0.6, driveway 0.5), 0 if fewer than 4 nights since the last. Roll: `events`, `knock-{day}`, first draw against the odds; the knock is drawn from the spot's knocks not among the last 3 heard (all of the spot's if none are left).
- **While an encounter is pending,** every action but the answer is refused, and the night hasn't happened.
- **The answer** applies its effects (cash; energy, clamped; supplies; psyche × 0.5, rounded, clamped; bond ± with the driveway's host), says its line, then runs the night as a normal van sleep.
- 10 knocks × 3 answers, by spot; the driveway's is the friend's.

**What it means here:** a seeded, spot-weighted interrupt on the sleep action with a three-way choice, cooldowns, and a shared one-a-day cap that 22.6b's drive events and 22.6c's walk-outs will share.

## 2026-09-30 — Phase 22.5b: busking

**Built in the 2D rebuild's Phase 22.5b** (`dirtbag/app/src/sim/busk.ts`, the beat in `ui/BuskPanel.tsx` and `game/game.ts`; numbers in `BUSK` in `dials.ts`). Evan's calls; numbers [proposed]. Save v19.

**The rules:**
- **A set:** at the café, 08:00–20:00, not on a rain day, once a day (`busked` flag), ≥5 energy. Costs 60 min and 5 energy.
- **The beat** (UI): 8 notes. A marker sweeps 0→1 across a bar in 1.3 s; a tap scores 1 within 0.08 of 0.5, 0.5 within 0.16, else 0; a sweep that ends untapped scores 0. The sim gets `{ t: 'busk', acc }`, acc = mean score, and trusts it (it's the player's hands, like the speed wall's time).
- **Playing:** `guitar` += 0.5 + 0.5 × acc per set. Skill = 1 − e^(−guitar/150). Ranks at skill 0.25, 0.55, 0.85 (busker, regular, local legend), each with a line when reached.
- **Crowd** = hour factor (08:00 0.6, 11:30 1.2, 14:00 0.8, 17:00 1.3) × 1.3 on weekends × sky (prime 1.1, fair 1, hot 0.8) × luck (1 ± 0.2, `events`, `busk-{day}`).
- **Tips** = round((4 + 20 × skill) × crowd × (0.3 + 0.7 × acc)). **Heads** = round((2 + 28 × skill) × crowd).

**What it means here:** a skill-by-repetition side income with a rhythm minigame, starting below every job's hourly pay and ending above all of them, capped at one short set a day so it never replaces a day's work.

## 2026-09-30 — Phase 22.5a: the hustle (cans, bins, foraging)

**Built in the 2D rebuild's Phase 22.5a** (`dirtbag/app/src/sim/hustle.ts`; numbers in `HUSTLE` in `dials.ts`). All numbers [proposed]. Save v18.

**The rules:**
- Three acts, each once a day, the take drawn at the act from `events`, `hustle-{kind}-{day}` (so the same day always gives the same take):
  - **Cans** (the van, by day): 120 min, −8 energy, cash int[3, 9].
  - **Bins** (the market, night only): 60 min, −4 energy; p 0.78 of food int[24, 39], else nothing. Food found counts as the meal `bins` in the last-meals list.
  - **Forage** (the lake, by day): 120 min, −6 energy, food int[18, 29], ×0.4 rounded in winter; the meal `forage`.
- **Teaching:** after any action, if food < 35 and cash < $10 and `seen` lacks `hustle`, one line names all three and `hustle` is added to `seen` (a list of one-time lines, new in v18).
- **Balance rule:** each hustle's best hour, food priced at ramen's cost per point, stays under the worst-paid shift's first-rank hourly pay.

**What it means here:** a small free-money/free-food safety net, gated by time and a daily flag, with a one-time tutorial prompt keyed to a hungry-and-broke state.

## 2026-09-30 — Phase 22.4d: psyche

**Built in the 2D rebuild's Phase 22.4d** (`dirtbag/app/src/sim/psyche.ts`; numbers in `PSYCHE` in `dials.ts`). All numbers [proposed]. Save v17.

**The rules:**
- **Psyche** (0–100): a new climber starts at 60; a migrated one at 50. It's settled at every night (van, ledge or expedition), from the day that's ending:
  - Lifts: +4 for a first send of any line that day; +6 for arriving at a crag you'd never been to, else +2 for any crag day (a ledge or expedition night counts as one); +3 for climbing, watching or belaying with anyone; +2 for sitting at the fire at night.
  - Wears: −3 for a shift worked. A day with no lift is stale; a run of stale days costs 1 less than its length, today included, up to −4 (so 0, −1, −2, −3, −4, −4…). Any lift ends the run.
  - The day's net is capped at ±8. Then it drifts 10% of the way to 50 and rounds.
  - A crag you'd logged a line at before the save moved to v17 counts as visited.
- **Effect:** every window × (1 + 0.05 × (psyche − 50) / 50): 0.95 to 1.05.
- **Words:** under 25 low, under 45 flat, under 60 steady, under 80 keen, else psyched. A line the morning it tips into low or psyched. Tonight shows the word by morning and the day's reasons.

**What it means here:** a slow, bounded mood stat fed by a day's variety and company, pulled back to even every night so no single habit holds it up, with a small multiplier on every skill check.

## 2026-09-30 — Phase 22.4c: supplies and sickness

**Built in the 2D rebuild's Phase 22.4c** (`dirtbag/app/src/sim/sick.ts`; numbers in `SUPPLIES` and `SICK` in `dials.ts`). All numbers [proposed]. Save v16.

**The rules:**
- **Supplies** (0–100, start 80): −8 per van night, +20 per night at the truck stop.
  - Refills: lake +40 (not in winter), market $4 for +30, gym shower +30 (needs a day pass, once a day).
  - Low is under 25.
- **Sickness:** at each van night, if not already sick, roll (`events`, `sick-{day}`) against p:
  - p = 0.01, +0.03 on a winter night without the heater lit, +0.04 on low supplies, +0.04 if food is under 20, +0.03 if the last 4 meals were the same.
  - On a hit, if supplies are low, a second draw under 0.4 makes it a toothache. Otherwise it's a cold (winter) or a bug. A cold or bug lasts 2–4 days.
  - While sick: −20 energy on each night's rest, and every window ×0.85.
  - A toothache persists until a doctor ($30 × the plan's share, 60 minutes). A doctor halves a cold or bug's remaining days (ending it at 1 day or less).
- Propane stays separate: the heater's fuel, by the tank.

**What it means here:** a consumable hygiene/water meter with multiple refill sources, and a nightly illness roll driven by several state conditions, with a lingering debuff.

## 2026-09-30 — Phase 22.4b: old injuries and fear

**Built in the 2D rebuild's Phase 22.4b** (`dirtbag/app/src/sim/scars.ts`; numbers in `SCARS` and `FEAR` in `dials.ts`). All numbers [proposed]. Save v15.

**The rules:**
- **Marks:** when an injury heals, a seeded roll (`session`, `scar-{day}-{kind}`) leaves a mark on its area. The chance is 0 / 0.35 / 0.7 by tier.
  - The area comes from the injury's name: fingers, shoulder, forearm, leg, or ankle for a landing.
  - A mark multiplies injury chance on styles that load that area ×1.3, and highball landing chance ×1.3 for an ankle.
- **Flares:** each go on a marked style without an injury rolls 6% (`flare-{day}-{go}`) to flare the mark for 3 days. Only one flare at a time.
  - While flaring, that area's styles have windows ×0.9.
  - Physio ends a flare.
- **Fear:** a landing or deck injury, or any tier-3 injury on a go, adds the line's style to a fear set.
  - Feared styles have windows ×0.9.
  - Sending any line of that style removes it.

**What it means here:** persistent per-area risk modifiers, a temporary per-area window penalty, and a per-style confidence penalty cleared by a send.

## 2026-09-30 — Phase 22.4a: insurance plans and the clinic

**Built in the 2D rebuild's Phase 22.4a** (`dirtbag/app/src/sim/clinic.ts`; numbers in `PLANS` and `CLINIC` in `dials.ts`). All numbers [proposed]. Save v14.

**The rules:**
- **Plans:** chosen at any time and paid with the weekly bills (registration $45 + premium).
  - None: $0, clinic bills ×2.
  - Catastrophic: $25, bills ×1. This is the default.
  - Full: $45, every bill $20, and physio and cortisone at 25%.
  - Clinic bills are 0/45/210 by tier before the multiplier. The first injury is free.
- **Physio:** 120 minutes, $40 × the plan's share, once a day, with an injury. The injury ends 1 day sooner.
- **Cortisone:** 30 minutes, $60 × the share. It removes floor(days left / 2), and marks the day of the shot.
  - Within 21 days of a shot, the next injury rolled is one tier worse (up to 3).
  - Its time off grows by the gap between the tiers' shortest durations. The mark then clears.

**What it means here:** an insurance setting modulating injury costs, and two treatment actions, one with a delayed penalty.

## 2026-09-30 — Phase 22.3b: the lake

**Built in the 2D rebuild's Phase 22.3b** (`dirtbag/app/src/sim/lake.ts`; numbers in `LAKE` in `dials.ts`). All numbers [proposed]. No save change.

**The rules:**
- **Fishing:** 120 minutes and −6 energy, once a day.
  - Three tries at a fish, each with a chance p.
  - p is 0.4 in spring, 0.5 in summer, 0.5 in fall and 0.15 in winter. It's ×1.5 before 9:00 or from 17:00, capped at 0.9.
  - The roll is seeded from `events`, `fish-{day}-{minute cast}`.
  - Each fish is +15 food, eaten at once. No cash.
- **A swim:** 45 minutes, +8 energy, once a day, not in winter.

**What it means here:** a seeded yield activity whose output is food, bounded below a meal's worth, with season and time-of-day modifiers.

## 2026-09-30 — Phase 22.3a: the pantry and the kitchen

**Built in the 2D rebuild's Phase 22.3a** (`dirtbag/app/src/sim/content/food.ts`, `game.ts`, `climb.ts` `dayFactor`; numbers in `FOOD` in `dials.ts`). All numbers [proposed]. Save v13.

**The rules:**
- **The pantry** counts servings per ingredient. The Market sells packs of 4 servings: oats, rice, beans, tortillas and pasta at $4; eggs, cheese and greens at $6.
- **Recipes** need the camp kitchen (an owned upgrade, $80) and one serving of each ingredient:

  | Recipe | Ingredients | Time | Food | Other |
  |---|---|---|---|---|
  | Oatmeal | oats | 15 min | +30 | +4 energy |
  | Rice and beans | rice, beans | 30 min | +45 | |
  | Burritos | tortillas, eggs, cheese | 30 min | +50 | fuels |
  | Pasta | pasta, greens | 40 min | +45 | +3 skin, fuels |

- **Fueled:** the day a fueling meal is eaten, every crux window ×1.05 (inside the day factor). Nothing carries over, and no skill is gained.
- **Variety:** the last 4 meals are kept (recipes, ramen, the diner special). When all 4 match, it's shown on the night summary. 22.4's sickness will read it.
- **Coffee:** the third and later cups in a day give −6 energy instead of their normal gain.
- **Hungry bedtime:** if food is under 20 at bed, eat up to 2 servings of anything in the pantry, +15 food each, until it's 20 or more.

**What it means here:** an inventory of ingredients, recipes as data, a per-day buff multiplier in the window model, and a short meal history.

## 2026-09-30 — Phase 22.2c: van upgrades and winter

**Built in the 2D rebuild's Phase 22.2c** (`dirtbag/app/src/sim/spots.ts` `nightAt`, `van.ts`, `content/gear.ts`; numbers in `UPGRADE` and `WINTER` in `dials.ts`). All numbers [proposed]. No save change.

**The rules:**
- **Upgrades** are owned items, each fitted once at the garage for a price and a time:
  - bed: $70, 60 min, +5 energy each van night (not in the pullout);
  - curtains: $60, 30 min, Lot ticket odds ×0.3;
  - tool kit: $85, 5 min, bodge odds 0.9 instead of 0.6;
  - tune-up: $90, 90 min, gas ×0.8, rounded;
  - heater: $45, 90 min (was $150; Evan's call, so it's affordable before the first winter on day 15);
  - insulation: $60, 120 min, cold ×0.5, rounded.
  - None produces income.
- **Winter nights:** in the winter season (the second 14 days of every 56), a van night's cold is −10 plus the spot's winter value. It's not the spot's in the pullout.
  - With a heater and propane > 0, the cold is 0 and one propane is used.
  - Otherwise, insulation halves the cold.
- **Propane:** a consumable, $18 for 10 at the gear shop.
- **The warning:** a line on the night before day (winter start − 5).

**What it means here:** the van carries owned modifiers, and the night's rest gets a seasonal cold term with a fuel-burning counter.

## 2026-09-30 — Phase 22.2b: where you sleep

**Built in the 2D rebuild's Phase 22.2b** (`dirtbag/app/src/sim/spots.ts`, `game.ts` `sleep`; numbers in `SPOTS` and `SPOT` in `dials.ts`). All numbers [proposed]. Save v12.

**The rules:**
- A standing choice of spot, applied on every night at the van. Each spot has a price, gas, a morning drive (minutes added to the wake time), and an energy change, with an extra winter change:

  | Spot | Price | Gas | Drive (min) | Energy | Extra in winter |
  |---|---|---|---|---|---|
  | The Lot | $18 | $0 | 0 | 0 | 0 |
  | Upper Trailhead | $0 | $6 | 25 | −10 | −10 |
  | Truck stop | $6 | $2 | 10 | −12 | 0 |
  | Friend's driveway | $0 | $2 | 10 | +5 | 0 |
  | The Ridge | $0 | $8 | 40 | +5 | −15 |

- **Tickets at the Lot:** with n nights in a row there so far, the chance tonight is min(0.25, 0.05 × (n + 1 − 3)), never below 0. The fine is $25, on the card. The roll is seeded from `events`, `ticket-{day}`. Any other spot resets the count.
- **The driveway:** needs a partner at bond tier 3 or more who isn't away, and 4 days since the last visit.
- **The Ridge:** needs 25 trips out.
- **Fallback:** a spot that won't have you tonight becomes the Lot. A card that can't cover the price and gas is the pullout: nothing paid, no drive, the rough night's energy.
- You always wake at the Lot. Lifestyle is paid after the spot.

**What it means here:** the night gets a location with its own costs and rest, plus a consecutive-nights counter for tickets.

## 2026-09-30 — Jobs pay in more than money (decided)

**Decided by Evan; built in the 2D rebuild** (`dirtbag/app/src/sim/content/jobs.ts`, `content/places.ts`, `jobs.ts` `tipsFor`).

**The rules:**
- A job that pays less an hour trains something: coaching +2 head and +1 technique a shift, setting +3 technique, the warehouse +2 endurance. The café and the diner train nothing.
- The café pays least a shift ($28) and has the shortest shifts (3 h). Setting went from $28 to $30 to keep that true.
- A new job, the Diner: 4 h at $30 base, raise $6 a rank, ranks Busser, Server, Head server, Floor manager at 0/10/24/42 shifts, and 5 shifts posted a week.
  - Tips on top: a seeded integer from 4 to 10 (`events` stream, `tips-{job}-{day}`), ×1.5 rounded on a weekend.

**What it means here:** a job's reward is pay plus a flat skill gain, and a job can carry a seeded tip roll.

## 2026-09-30 — Phase 22.2a: the van's parts, breakdowns and the garage

**Built in the 2D rebuild's Phase 22.2a** (`dirtbag/app/src/sim/van.ts`, `game.ts`, `content/van.ts`; numbers in `VAN` in `dials.ts`). Three parts by Evan's call (Decision 5); the numbers are [proposed]. Save v11.

**The rules:**
- **Parts:** tires, engine and battery, each 0–100. A new climber's van starts at 85; a migrated one at 70.
- **Wear:** tires lose 0.015 and the engine 0.01 per minute driven, and the battery 1 a night. A climber driving to the nearest crag every day needs tires about every 55 days and an engine service about every 80: about $5 a day, battery included. At 0 the battery needs a jump: drives across town only (under 20 minutes), plus home and the garage, so town and a shift are always in reach.
- **Breakdowns:** only on drives of 20 minutes or more. The chance is (minutes / 60) × 0.14 × w², where w is the worst road part's wear (0 new, 1 dead), capped at 0.5. A van kept up never breaks down.
  - The roll is seeded from the `events` stream, derived `breakdown-{day}-{minute}-{from}-{to}`.
  - The part that goes is picked weighted by wear + 0.05, and drops to 5.
  - The breakdown happens halfway: half the drive's wear and time, then nothing else can happen until you take a way out.
- **Ways out:**
  - **Tow:** $70 and 90 minutes, to the garage.
  - **Bodge:** 60 minutes, holds 60% of the time (its own seeded roll). If it holds, the part goes to 35 and you drive on. One try per breakdown.
  - **Limp on:** the rest of the drive at double time and −15 energy. The part stays at 5.
  - **Friend:** a partner at bond tier 2 or more who isn't away. 120 minutes, the part to 30, and you drive on.
- **Unsafe:** a road part under 15 refuses any drive of 20 minutes or more, except home to the Lot or to the garage.
- **The garage (Midtown):** each part back to 100 for its price × wear (tires $140, engine $180, battery $90, never under $15), in 60, 120 and 20 minutes.

**What it means here:** the van becomes state with three condition meters, a travel-time breakdown roll and a blocking breakdown state with four resolutions. Parking spots, upgrades and winter come in 22.2b and 22.2c.

## 2026-09-30 — Phase 22.1: the week, the jobs and how you live

**Decided by Evan (jobs differ, start low, promotion to earn more; a warehouse job added); built in the 2D rebuild's Phase 22.1** (`dirtbag/app/src/sim/content/jobs.ts`, `jobs.ts`, `game.ts`; numbers in `WORK` and `LIFESTYLE` in `dials.ts`). Save v10.

**The rules:**
- **Four jobs**, each with a shift length, a first-rank pay, ranks by shifts worked, and a raise per rank (top rank 1.7 to 2 times the first):
  - Coffee Shop: 3 h, $28, raise $5, five ranks at 0/12/30/54/84 shifts. Posts every day.
  - Coaching (The Cave): 4 h, $34, raise $17, three ranks at 0/8/20 shifts and V5/V6/V8. Posts 4 days a week.
  - Setting (Send City): 4 h, $28, raise $10, four ranks at 0/6/16/30 shifts and V0/V3/V5/V7; trains technique. Posts 4 days a week.
  - Warehouse [proposed]: 8 h from before 9 AM, $52, raise $17, four ranks at 0/10/25/45. Posts 3 days a week.
- **Posting:** a job that doesn't post every day picks its days per 7-day week off the `worldgen` stream, derived `shifts-{job}-w{week}` (partial Fisher–Yates over the week's days). Day 1 is a Monday.
- **Sign-up:** a posted shift from tomorrow up to 7 days ahead, one shift a day, the job's first-rank grade needed. Today's posted shifts are walk-ins: they pay the same and don't count toward promotion. Only a shift you signed up for counts.
- **No-shows:** a signed-up shift not worked by the end of its day is a warning. The third costs the job: shifts there reset to 0, future sign-ups there dropped, and 7 days before it takes you back.
- **How you live:** dirtbag (free), comfortable ($12 a night, +8 energy, +4 skin by morning), plush ($28, +15, +8) [proposed]. Paid at the van after the spot, only while the card covers it; otherwise that night is a dirtbag's. Not on a ledge or an expedition.

**What it means here:** jobs become data with a weekly posting draw, the save carries a shift schedule, warnings, a let-go date per job and a lifestyle, and sleep settles no-shows and the lifestyle's cost.

## 2026-09-30 — The Cave raised, and the Training Center (decided)

**Decided by Evan; built in the 2D rebuild** (`dirtbag/app/src/sim/content/gym.ts`, `content/places.ts`).

**The rule:**
- The Cave's weekly set is now V5 to V12 (v0.956's V3 to V10, raised).
- The Training Center, v0.956's third gym: eight comp-style problems a week, V7 to V14, seeded by week. Styles weighted dyno 3, technical 2, power 2, crimp 1 in 8. Its own day pass ($20 [proposed], against Send City's $14); specialty power and technique (×1.2, as the other gyms' are). You need V7 to go there, like Sandstone Mesa.
- Phase 21 is closed: with these, the 2D game's career bots reach V10 in every run and never run out of things to try.

**What it means here:** a third gym level: a comp hall, and a weekly set generated from its own table.

## 2026-09-30 — Phase 21.6: the speed wall, and Free Solo

**Built in the 2D rebuild's Phase 21.6** (`dirtbag/app/src/sim/speed.ts`, `solo.ts`, `game.ts`; numbers in `SPEED` and `FREESOLO` in `dials.ts`). Save v9.

**The speed wall** (Send City, with the day pass):
- A run is 20 holds. The start is three lights a second apart, go on the third; a grab before it is a false start. Then alternate hands; the same hand twice is a slip that loses 0.35 s.
- The clock time = the player's real seconds (at least 1.5) × a factor: 2.8 at V0, minus 0.1 per grade of (0.6 power + 0.4 technique), never under 1.45. A PB is kept.
- A run costs 10 min, 7 energy, 2 skin and 2 food, and loads you like a short go (×0.8). The first 3 runs a day (false starts count) teach power 0.14 and technique 0.06 of a training session's base, each slowed by hi(); after that, nothing.

**Free Solo** [proposed]:
- Chosen at creation, and never changed afterwards.
- Every outdoor sport line and wall pitch is climbed without a rope: no belayer or rope needed, and every crux window ×0.82. Trad and boulders are unchanged.
- A solo send teaches head +2. A fall ends the run: the state is marked dead and nothing else can happen to that climber.
- The state records the solo in progress from the moment you pull on. Loading a state with a solo in progress resolves it as a fall, so reloading can't undo it.

**What it means here:** the sim needs a mode flag, a "dead" end state and a solo-in-progress marker in the save. The speed wall is a separate minigame with its own time model.

## 2026-09-30 — Phase 21.6: highballs by height (decided)

**Decided by Evan; built in the 2D rebuild** (`dirtbag/app/src/sim/content/routes.ts` `isHighball`, `HIGHBALL.fromFt` in `dials.ts`).

**The rule:** every outdoor boulder 16 ft or taller is a highball; deep-water solos and gym problems never are. It isn't set by hand. The landing rule itself is unchanged (Phase 10.3b): nothing up to 8 ft of fall, +1.2% a foot above that, halved by the haul's pads and again by a spotter. Seventeen problems qualify, twelve of them new.

**What it means here:** the highball flag is derived from height, so the Unreal content pipeline can compute it rather than author it.

## 2026-09-30 — Phase 21.6: crowds and spray beta

**Built in the 2D rebuild's Phase 21.6** (`dirtbag/app/src/sim/crowds.ts`; numbers in `CROWD` in `dials.ts`, each crag's draw as `crowd` in `content/places.ts`). No save change: a crowd is worked out, not stored.

**The rule:**
- A crag's crowd level = its draw (0.2 at The Crucible to 0.6 at Roadside and The Big Stone) × 1.7 at the weekend × the hour (0 before 7:00 and from 19:00; 0.5 from 7:00, 1 from 9:00, 0.7 from 16:00) × the sky (prime 1.25, fair 1, hot 0.6; wet or closed rock 0) × the day's luck (0.7–1.3, seeded per crag and day). Read off as empty (<0.25), quiet, busy (≥0.7) or packed (≥1.2).
- A queue before each go: busy, 10 min for a rope and none for a boulder; packed, 20 and 10. A wall's pitches have none.
- Asking around (busy or packed, a line you haven't sent, beta you don't know): 15 min, and you learn the line's next unknown beta as told, so a first-go send is a flash.
- Packed, on your first go on a line: a 40% seeded chance someone shouts that beta at you before you start. The same outcome.

**What it means here:** crowds at the crag base to stage, and a queue that takes time.

## 2026-09-30 — Phase 21.5: walls and expeditions

**Built in the 2D rebuild's Phase 21.5** (`dirtbag/app/src/sim/content/routes.ts`, `content/expeditions.ts`, `expeditions.ts`, `game.ts`; numbers in `WALL` and `EXPED` in `dials.ts`). Save v8.

**Walls:**
- v0.956's four, pitch by pitch, as sport pitches of 100 ft and 16 moves: The Long Prow at Roadside (4 pitches, sport grade 5; v0.956's "The Prow", renamed because the Mesa's boulder has the name), Golden Buttress (5, grade 9), Obsidian Tower (7, grade 13) and The Ascendant (8, grade 18) at The Big Stone.
- Needs a rope of your own ($150 at the gear shop) and dry rock. You start the wall, then climb its pitches in order: each is a normal go through beta-then-send, costing 60 min, 8 energy, 4 food. A fall leaves you at the same pitch.
- After dark, once off the ground, you can bivy on the ledge: no van fee, +40 energy, −20 food, and you wake on the wall. Rapping off or driving away ends the attempt; the next one starts at pitch 1.
- The first summit pays 40 + 12 × grade + 10 × pitches ($140 for the Prow); later summits pay nothing.

**Expeditions:**
- v0.956's three: El Capitan (V7 to go, 6 pitches in 10 days, storms 18%, $900, pays $2,400), Cerro Torre (V10, 8 in 16, 44%, $2,200, $6,000), Trango Tower (V12, 11 in 24, 56%, $4,500, $12,000). Paid in cash from the Lot; nothing else happens while you're away.
- Each day is one choice: lead (−22 energy), dig deep (−36), rest (+48), or bail. A storm day (seeded per expedition and day) allows only rest or bail. Every night gives +10 energy, up to 100.
- A pitch succeeds with p = clamp(0.45, 0.97, 0.68 + 0.1 × endurance margin over the objective's grade (+0.25 digging deep)), rolled on a seeded stream per day and pitch.
- The summit's odds are shown before you pay and every day after: an exact calculation over storms and falls, for leading every fair day and for digging deep every one. For a V9 climber on El Cap that's about 48% and 81%.
- The summit pays and trains head +2. Running out of days or bailing pays nothing, and the cost is spent.

**What it means here:** walls reuse the attempt as-is, so they need a wall-length scene with ledges and an overnight on the rock. An expedition is a turn-based layer over days, with its odds in the UI.

## 2026-09-30 — Phase 21.4: The Cave, a second gym

**Built in the 2D rebuild's Phase 21.4** (`dirtbag/app/src/sim/content/gym.ts`).

**The rule:**
- Indoor places are a set, each with its own day pass, closing time (10 PM) and specialty: Send City brings on technique and endurance ×1.2, The Cave power and fingers ×1.2.
- The Cave sets eight problems a week, V3 to V10, seeded by week, drawn mostly from power and crimp styles.
- Coaching there: 4 hours, $34, trains head +2 and technique +1; needs V5; ranks at 8 and 20 shifts (V6, V8), +$8 a shift each.

**What it means here:** a second gym level with its own weekly set and job.

## 2026-09-30 — Phase 21.4: Psicobloc Cove, and deep-water solo

**Built in the 2D rebuild's Phase 21.4** (`dirtbag/app/src/sim/content/routes.ts`, `game.ts`, `weather.ts`).

**The rule:**
- A route may be a deep-water solo (`dws`): a boulder problem over the sea. A fall is into the water from wherever you were: no pad needed, no landing roll, no strain (overuse) roll. It still loads you and costs skin like any go.
- A place may be closed for several seasons: the Cove is open in summer only ("Cold, rough seas till summer").
- The Cove: V4, two hours from the Lot, no fee, its own weather. v0.956's eight lines: Tide Pool Traverse V2, The Plunge V3, Saltwater Slab V4, Barnacle Crimps V5, Leap of Faith V6 (climbs V7), Overhanging Tide V7, Psicobloc Arête V8, The Deep End V9.

**What it means here:** a fall resolution with no injury path, and a water landing to stage.

## 2026-09-29 — Phase 21.4: The Crucible, and myths

**Built in the 2D rebuild's Phase 21.4** (`dirtbag/app/src/sim/content/routes.ts`, `game.ts`).

**The rule:**
- A crag opening at V11, four hours from the Lot, shaded, with its own weather and no fee. v0.956's lines: The Reckoning V13, The Vise V14, Apparition V15 (climbs V14), Event Horizon V17; Crucible Crux 5.14b, The Lifeline 5.14d, Threshold 5.15d. Added: The Anvil V16, so every grade V0–V18 has a line.
- A route may name another it's hidden behind (`hiddenUntil`). Until that one is sent, the hidden line can't be read or tried: the V18 boulder myth behind Event Horizon, the 5.16a sport myth behind Threshold. Both are open (unclimbed); the first to send one names it.

**What it means here:** a route's visibility and availability depend on another route's send; the 3D wall should show nothing on a myth until then.

## 2026-09-29 — Phase 21.4: Wind River Walls

**Built in the 2D rebuild's Phase 21.4.** Content, not a new rule.

**The rule:** a crag opening at V9 after an $800 trip paid once, with a $35 permit every trip; four hours from the Lot, reached past the Gorge; shaded, its own weather, closed in winter. v0.956's nine lines: boulders Alpine Crimps V9, The Diamond V11 (climbs V10), Offwidth Horror V12, Thin Air V13, an open V15; sport Glacier Point 5.13c, Skyline Traverse 5.14a, Astroman 5.14c; trad Alpine Trad 5.14b. A partner needs bond 7.

## 2026-09-29 — Phase 21.4: The Big Stone

**Built in the 2D rebuild's Phase 21.4.** Content, not a new rule.

**The rule:** a crag opening at V8 after a $600 trip paid once, four hours from the Lot, shaded, with its own weather. v0.956's five single pitches: Base Camp Boulder V6, The Warm-Up Wall V9 (climbs V10), The Splitter Pitch V10 (highball), The Trad Pitch 5.13d (trad), Valley Classic 5.14a (sport). A partner needs bond 7 to come out. Its multi-pitch walls come with Phase 21.5.

## 2026-09-29 — Promotions, and inviting crew to any crag (decided)

**Decided by Evan** after the harness found bots going broke at V5; built in the 2D rebuild (`dirtbag/app/src/sim/jobs.ts`, `content/jobs.ts`, `content/people.ts`). Save v7.

**The rule:**
- Each job counts shifts worked (a double is two). A rank comes at a shift count and, for setting, a grade: café ranks at 12, 30, 54, 84 shifts, +$4 a shift each; setting at 6, 16, 30 shifts and V3, V5, V7, +$7 a shift each. The shift that earns a rank pays the old rate.
- Each crag carries the bond a partner needs to come out there when asked: Roadside 1, the Gorge 3, the Mesa 5, Moonstone 5. Asking is once a day, before 1 PM, to Hazel or Sage; the crag must be dry, your grade must open it, and a paid-for crag must be paid for. They're there for the day from the minute you ask.

**What it means here:** partner presence gains an invite override for every partner, not just Sage; jobs gain a rank ladder the 3D game's work screens need to show.

## 2026-09-29 — Phase 21.4: Sandstone Mesa

**Built in the 2D rebuild's Phase 21.4** (`dirtbag/app/src/sim/content/routes.ts`, `places.ts`). Content, not a new rule.

**The rule:**
- A crag opening at V7, three hours from the Lot, with no unlock fee; desert (windows ×0.92), its own weather, closed in summer.
- v0.956's eleven lines, names and grades kept: boulders Desert Varnish V7, Sandstone Crimps V8, The Prow V9 (climbs V10), Desert Splitter V9, Powerhouse V10, The Megaproject V11, and an open V13; sport Desert Lap 5.13a, Desert Enduro 5.13b, The Big Link 5.13d; trad Desert Trad Line 5.13d.
- Sage can be invited there to belay, as to the Gorge (bond 3, V7, the crag open that day).

**What it means here:** the Mesa is a level with the same route data. v0.956's multi-pitch "The Prow" at Roadside (Phase 21.5) will need another name, since the Mesa's boulder has this one.

## 2026-09-29 — Phase 21.3: training

**Built in the 2D rebuild's Phase 21.3** (`dirtbag/app/src/sim/training.ts`, `sessions.ts`, `content/training.ts`, numbers in `TRAIN` in `dials.ts`). Save v6.

**The rule:**
- Six protocols, each with a place (the van with a hangboard, or the gym with a pass), a cost (minutes, energy, food, skin), two skill weights, an injury style and an intensity: max hangs, repeaters, pull-ups and core (van); campus from V4, 4x4s, ARC (gym). Plus prehab at the van.
- A session teaches `(2 + 0.35 × grade) × phase × (0.6 if the load ratio is over 1.5)`, split by weight, each skill through `hi()`. One a day.
- It loads you like a go: `goLoad(energy, grade, 1) × intensity`, and rolls for injury on the same model as a go, with its style's risk.
- Refused while tapering, injured (prehab excepted), fried (ratio over 1.7), empty, or short of the session's energy.
- Phases: base (×1 everything; no lock), build (sessions ×1.25, injury ×1.2), peak (every crux window ×1.04, sessions ×0.7, injury ×1.3), deload (sessions ×0.5, injury ×0.6, acute load ×0.8 each night). A chosen phase holds 6 days; peak turns into deload after 7.
- Taper: 3 days with no training, windows ×1.03 then ×1.06 on day 3; 14 days from its end before the next.
- Prehab: injury chance ×0.6 for 8 days.
- Target: at every grade reached, the best protocol's skill an hour stays under climbing's.

**What it means here:** training is a sim action beside `ResolveAttempt`, with the same load and injury model. The phase and taper scale the per-move windows alongside skill, conditions and kit.

## 2026-09-29 — Phase 21.2: trad

**Built in the 2D rebuild's Phase 21.2** (`dirtbag/app/src/sim/climb.ts`, `content/routes.ts`, numbers in `TRAD` in `dials.ts`). No save change.

**The rule:**
- Trad is a third discipline beside boulder and sport. A trad line has stances (in moves) instead of bolts: one low, then about every 3.2 moves, one at the rest, and none inside a crux.
- Leading trad needs a rack (owned kit, $280, or 55% at the swap meet) and a belayer. A go costs 35 minutes, 12 energy and 7 food.
- Between cruxes, letting go within a stance's reach (0.5 moves below it to 0.8 above) places a piece after 1.1 s. While placing, pump rises 2/s where hanging would pay back 4.5/s. Climbing on before 1.1 s places nothing. Climbing through a stance runs it out.
- A fall catches on the highest piece placed below you, exactly as sport's does on the last bolt clipped: fall = 2 × distance over it + 3 ft of slack.
- If that fall is at least the height you're at, you hit the ground: a deck of that height. Nothing under 6 ft hurts; above it, each foot adds 3.6% to an injury roll (ankle names; tiers from 6 and 12 ft over), with no pads or spotter to reduce it.
- A trad send adds 0.4 × the go's learning rate to head, on top of what the go teaches.
- Pieces never pull, and every piece holds the same.

**What it means here:** `ResolveAttempt` needs protection as part of the attempt's state: the moves where pieces went in, chosen during the go, not before it (v0.956's single rack choice is gone). Fall length and the deck check read it. In 3D, a stance is a spot on the wall where the climber can stop, and placing is an animation the player waits through while pump climbs.

## 2026-09-29 — Phase 21.1: your kit

**Built in the 2D rebuild's Phase 21.1** (`dirtbag/app/src/sim/kit.ts`, `content/gear.ts`, numbers in `KIT` in `dials.ts`). Save v5.

**The rule:**
- The state carries a kit: a number per item. Shoes have a condition (0–100); chalk and tape have uses left; a second pad is owned or not.
- A new climber starts with worn shoes (60) and half a bag of chalk (30 goes). The rope on a sport go is still the belayer's.
- Every go wears the shoes by `0.25 × (1 + 0.035 × grade)`, uses one go of chalk, and a wrap of tape on a crack if you have tape.
- The kit scales every crux window, in the same product as skills and the day's conditions (and so the windows the player sees before a go): shoes under 40 ×0.95, under 15 ×0.87, the penalty doubled on technical lines; no chalk ×0.95.
- Tape halves the skin a crack costs, the go's base cost included. A second pad halves a highball's landing risk, as a paid haul's pads do.
- A gear shop sells a resole (to 90, only when under 90), new shoes (100), chalk, tape and a pad; a swap meet on weekends (the last two days of each seven) sells used shoes (60) and pads at 55%.

**What it means here:** `ResolveAttempt`'s per-move execution windows take the kit factor alongside skill and conditions, and a go's outcome writes the kit's wear back. The 3D game needs somewhere to buy and see the kit; the numbers are the 2D game's dials.

## 2026-09-29 — Phase 12.1: the journal, and long lines on a card

**Built in the 2D rebuild's Phase 12.1** (`dirtbag/app/src/ui/Sheet.tsx`, `src/game/game.ts`). A UI feature, not a rule: the sim's log is unchanged.

**The rule:**
- The sim's log (the last 200 lines, in the save) is the player's message log, shown newest first, by day, beside the climber's own page.
- No transient notice runs over 30 words. A longer line goes on a card the player dismisses, which waits until nothing else is open.
- The navigation Phase 12 proposed (five tabs) is dropped: scenes and the map stay the way in, and the journal is the one place for the rest.

**What it means here:** whatever the 3D HUD does for notices, the same cap applies, and the log is a view of the sim's log, not a second one.

## 2026-09-29 — A send after the first is a repeat (decided)

**Decided by Evan; built in the 2D rebuild** (`dirtbag/app/src/sim/game.ts`, the `done` action).

**The rule:**
- A line's first send is an onsight (first go), a flash (first go, having been told the beta) or a redpoint (a later go). The route log keeps that one.
- Any send after the first is a **repeat**. The 2D rebuild had named every send after go one "Redpoint", sends of lines already done included; v0.956 called them repeats, as climbers do.
- A repeat gets no send card and can't be a first ascent. It already trained less (a lap of a sent line teaches less), and that doesn't change.

**What it means here:** send styles are four, not three, in whatever reports a send; the stored first-send style stays three. No save change in the 2D game.

## 2026-09-29 — Phase 11.4: the daily plan

**Built in the 2D rebuild's Phase 11.4** (`dirtbag/app/src/game/plan.ts`; the runner in `src/game/game.ts`). It's a UI feature, not a rule: each step runs through `act()` as a tap would.

**The rule:**
- A plan is a list of steps: a place, and an act there or a climb. It runs in one go: drives, then acts. It waits at a climb while you climb, and stops at the first thing the day refuses, saying why. Bed ends it.
- Yesterday, as played, is the default plan: every act, and a climb once a visit.
- What can be planned comes from the data: a place's acts, and climbing where there's something to climb.
- It's stored beside the save, not in it.

**What it means here:** the day's logistics (shifts, meals, drives, bed) can be one decision, and the climbing stays the part you play. For "2D minigames, 3D staging", that's a shape worth keeping: plan the day, then play the session.

---

## 2026-09-29 — Phase 11.3: roads are a graph

**Built in the 2D rebuild's Phase 11.3** (`dirtbag/app/src/sim/content/places.ts`, `ROADS` and `road`).

**The rule:**
- Each place has roads to its neighbours only, and a drive anywhere else is the quickest way through them: least time, then least gas. Adding a place means adding its neighbours' roads, not one to every other place.
- The valley's network, 11 roads:
  - town's six streets;
  - the highway to Roadside from the Lot, the café and the diner;
  - the Gorge's dirt road (60 min, $10) and the desert road to Moonstone (120 min, $18), both leaving the highway at Roadside.
- Kept: every daily trip, and v0.956's drives from the Lot: Roadside an hour and $12, the Gorge two hours and $22, Moonstone three hours and $30.
- Changed: nine rare trips to or from the Gorge or Moonstone, by −10 to +15 minutes and −$4 to +$1. Roadside to the Gorge is the biggest: an hour and $10, was 70 minutes and $12.

**What it means here:** if the Unreal version has places joined by roads (a map, road trips), model the network the same way, as edges between neighbours with a shortest-path trip. A hand-typed table for every pair doesn't scale, and it drifted into numbers no road network could produce.

---

## 2026-09-29 — Phase 11.2: place cards say who's around when you'd get there

**Built in the 2D rebuild's Phase 11.2** (`dirtbag/app/src/sim/presence.ts`, `staysAt` and `knows`; `src/ui/who.ts`; `src/view/header.ts`). It sits on top of the existing presence rules: nobody's schedule changed.

**The rule:**
- A place card reads the place at the hour you'd arrive: now if you're there, otherwise now plus the road's minutes.
- It lists everyone who'll be there between then and the end of the day, each with from and till. A second stay the same day is listed after the first.
- Someone you haven't met shows as "someone you haven't met". Hazel is known from the start (`PersonDef.known`).
- At a place with sport lines or highballs, one line says what that means: a belayer and/or a spotter (a partner there), till when (overlapping partners joined), from when, or nobody.
- Every card has a header: the place's scene at that hour, or a front drawn for a place without a scene.

**What it means here:** a destination the Unreal version offers (a map pin, a road-trip pick) should read the presence functions at the arrival time, not the current one. Whether there'll be a belayer or a spotter is the question to answer before a drive.

---

## 2026-09-29 — Phase 11.1: bed once it's dark, and lying around in one tap

**Built in the 2D rebuild's Phase 11.1** (`dirtbag/app/src/sim/dials.ts`, `DAY.nightFrom`; `content/places.ts`, the `until` field on acts; `game.ts`, `actCost`). It is *[proposed]*, pending Evan's call.

**Why:** the rebuild's e2e bot now counts a player's taps. A day-3 loop (a shift, the crag, three goes, back, dinner, bed) took 32. 12 of them were spent waiting for bed at 6 PM after getting home at 1 PM: an hour of lying around per tap, then a walk to the fire to fill the last hour. Phase 11's budget is 20.

**The rule:**
- **Bed opens at dark**, 5 PM, when the Lot turns to night and the fire's lit. It was 6 PM, "so a bad day is harder to skip". It only made one harder to skip by taps. A skipped day still costs the $18 spot, a shift not worked and a day of the season.
- **"Lie around till dark"** takes the whole afternoon in one act, at the old rate of 4 energy an hour. An act can now run till a time of day, with its cost given per hour and prorated.
- Sleep is unchanged: you still wake at 7:10, and a night still adds to your energy rather than setting it.
- The 2D harness doesn't move: all four season targets pass with the same figures.

**What it means here:** the day's end is a player's choice once it's dark, with one action to get there from any afternoon. If the Unreal version gets a time-skip, it should work the same way: one action to the next beat, at the same per-hour rates as waiting, never a loop of small waits.

---

## 2026-09-29 — The board runs a grade stiff (decided)

**Evan's call**, closing the question the 10.1 entry left open. Every board problem at Send City climbs like the grade above its label: `trueGrade = grade + BOARD_STIFF` (1), as real boards run.
- On your first go it says so, with its own line rather than the crags' sandbag one: "Board grades: that's no V5. Nobody on the mats is surprised."
- The 2D harness: with the V7 now a month-two project, 0 runs in 144 have a day in the first month with nothing new to try.
- The season targets pass. A few runs don't tie into a V5 by day 28, because the board's V5 climbs like a V6.

**What it means here:** the generator from the 10.1 entry gains one field, `trueGrade = grade + 1`. Board grades are labels; the sim climbs the true grade.

---

## 2026-09-29 — Phase 10 follow-up: Moonstone's own sky

**Built in the 2D rebuild** (`dirtbag/app/src/sim/weather.ts`, `skyAt`; the place flag `ownSky`). This follows v0.956, which rolled every crag's weather from its climate.

**The rule:**
- **Valley places** (the Lot, town, Roadside, the Gorge) keep the one valley sky, `skyOn`. R2's season was tuned on it, though v0.956 rolled the Gorge apart.
- **Moonstone is out of the valley**, so it rolls its own sky:
  - the season's odds, shifted by v0.956's desert terms: hot +0.12, rain −0.10 (floored at 0), prime −0.02;
  - rolled on `worldgen`, derived `sky-{place}-{day}`;
  - day 1 is prime everywhere, as in the valley.
  - (v0.956's shaded terms, rain +0.08 and hot −0.06, are in the function for a future shaded crag out of the valley.)
- **Its conditions** (open, seeping, when the sun comes, windows) come from its own sky. v0.956's desert after-rain nuances ("soft after rain", "washed clean") aren't carried: it seeps the day after rain as the valley does.

**What it means here:** a crag's weather is either the region's or its own. This repo can take the same split: one valley sky, plus per-destination skies for road trips, so a wet day at home can be a dry one three hours away.

---

## 2026-09-29 — Phase 10.3b: highballs, and landings that can hurt

**Built in the 2D rebuild's Phase 10.3b** (`dirtbag/app/src/sim/game.ts`, `landingChance` and `rollLanding`; the `HIGHBALL` dials; the `highball` route flag). All of it is *[proposed]*. v0.956 had no highball rule, only a generic close call for climbing without a pad.

**The rule:**
- **Which boulders:** tall ones carry a `highball` flag. Roadside's Highball Arête (18 ft), and Moonstone's Tall Arête (22), Splitter (20) and project (22).
- **How far you fall:** `heightFt × moves reached / moves`.
- **The chance of a bad landing:**
  - Up to `safeFt` (8), it's a normal boulder fall.
  - Above that, the chance is `perFoot` (0.012) × the feet over.
  - × `pads` (0.5) if you own the Moonstone haul's pads.
  - × `spotter` (0.5) if a partner is there (the same check as a belayer).
- **When it rolls:** only on a failed go, and only if the go's overuse roll didn't already hurt you. It uses the `session` stream, label `landing-{day}-{n}`, where n numbers the go within the day as the injury roll's does.
- **What it costs:** an ankle, by feet over the safe height: jammed (tier 1), rolled (tier 2 from 6 ft over), broken (tier 3 from 12). The days off and clinic bills are the injury table's.

**The 2D harness:**
- Careful bots wait for a spotter while a crux fall is worse than 1 in 20.
- With Hazel spotting at Roadside, 7% of moderate first months now end with a jammed ankle (0% before). Reckless bots: 10%.
- The season targets pass.

**What it means here:**
- In a watched 3D session, a highball's height is visible, and so is the fall. This rule gives the sim a stake to show when the climber comes off high: the landing is decided by the sim, and the staging acts it out.
- Pad placement isn't in, because the 2D boulders have one crux each. If this repo's boulders get two places to fall, placement becomes a real choice.
- No new seed streams (a new label in `session`). No change to `ResolveAttempt` or its golden vectors.

---

## 2026-09-29 — Phase 10.3a: Moonstone Boulders, and trips you pay for once

**Built in the 2D rebuild's Phase 10.3a** (`dirtbag/app/src/sim/content/places.ts`, `content/routes.ts`, the `unlock` and `travel` actions in `game.ts`, save v4 in `save.ts`). The source of truth is the dirtbag repo's `docs/ROADMAP.md`, Phase 10, "As built (10.3a)".

**The place**, on v0.956's terms:
- **Access:** desert quartzite, open at V6.
- **The haul:** $400, paid once in cash in hand (not on the card), opens the trip for good. That's v0.956's `unlockCost`. In the rebuild's words it buys "pads, water jugs and a guidebook".
- **The permit:** $20 on every trip in (v0.956's `permit`). Gas can go on a maxed card; the permit can't.
- **Distance:** three hours and $30 of gas from the Lot, two hours from Roadside.
- **Desert rock:** every window is scaled by `CLIMB.desertFactor` (0.92), v0.956's desert −0.08 on the odds translated the same way seeping's −0.07 was.
- **Sun:** the sun crosses it as at Roadside (the 10.2 entry), with the far project first and the arête by the van last.

**The lines** (v0.956's nine):
- Boulders:
  - Tall Arête V6 (v0.956 named it Highball Arête, like a Roadside line; renamed so logs can't mix them up);
  - Moonstone Mantel V7, The Egg V8;
  - Hueco Pockets V9, which climbs at V8 (`trueGrade`), as in v0.956;
  - Moonstone Splitter V8 (crack), Lunar Roof V11;
  - an open V12 project.
- Sport lines on a spire: Desert Spire (grade 8, 5.13a) and Moonlight Arête (grade 10, 5.13c).
- Each one uses the shared one- and two-crux templates.

**State:** save v4 adds `unlocked: string[]`, the place ids paid for. v3 saves migrate with it empty.

**What it means here:**
- A crag can carry a one-time cost and a per-trip cost, both data on the place. This repo's travel and economy can take both as they are.
- The spire's sport lines need a belayer, and nobody's schedule brings one to Moonstone yet. The 2D game will need road-trip partners for them. That's a known gap, not a rule.
- Highballs (pad and spotter decisions) come in 10.3b and get their own entry.
- No new seed streams. No change to `ResolveAttempt` or its golden vectors.

---

## 2026-09-29 — Phase 10.2: the sun crosses the crag a line at a time

**Built in the 2D rebuild's Phase 10.2** (`dirtbag/app/src/sim/weather.ts`, `sunOn`; the sun's path in `content/places.ts`; the dial `CLIMB.sunSweep` in `dials.ts`; the staging in `src/view/sun.ts`). The rule is *[proposed]*: Evan hasn't ruled on it. The source of truth is the dirtbag repo's `docs/ROADMAP.md`, Phase 10, "As built (10.2)".

**What changed in the rules:**
- **Before,** every line at a sunny crag greased at the same minute (`greaseFrom`): 3 PM on a prime day, 2 on a fair one, noon on a hot one.
- **Now** the sun crosses the crag in `CLIMB.sunSweep` minutes (120), and each line greases when it arrives:
  - `sunOn = greaseFrom + sunSweep × i / (n − 1)`, rounded to the minute;
  - `i` is the line's place in the crag's sun path of `n` lines.
- **`greaseFrom` changes meaning.** It is now the sun's first minute on the wall, half a sweep before the old single time: 2 PM prime, 1 PM fair, 11 AM hot. The path's middle line keeps the old time, so the average doesn't move.
- **Roadside's path** runs from the boulder field's far end to the road:
  - rsopen, project, highball, fingercrack, testpiece, pump, crimpfest, roadside, dyno, warmup, warm;
  - projects lose their shade first, and warm-ups keep it longest.
- **Unchanged:** shaded crags (the Gorge) never get the sun, and a sunny crag without a path greases all at once.
- **The go's once-a-day line** is now "Sun's on this line now. Everything feels greasy."

**Staging** (2D only):
- The scene's sun edge crosses the crag and passes each line's foot at its sun time.
- Wet rock shows after rain.
- The map tags closed and soaked crags.
- The beta sheet says when a line gets the sun.

**What the 2D harness measured** (12 seeds × 28 days):
- All four season targets pass.
- The first V5 go comes at day 19 for every strategy.
- It's balance-neutral within noise. The bots don't plan around the shade, so the gain goes to a player who does.

**What it means here:**
- A crag's sun is data: an ordered list of route ids, and one dial.
- A 3D crag can drive its light from the same numbers: a terminator that crosses each line's base at `sunOn`. In a watched session, the shade line is the prime window made visible, as the 2D game has it.
- No new seed streams. Nothing here touches `ResolveAttempt` or its golden vectors.

---

## 2026-09-29 — Phase 10.1: the board at Send City

**Built in the 2D rebuild's Phase 10.1** (`dirtbag/app/src/sim/content/gym.ts`; the board's painter in `src/view/paint/gym.ts`). The source of truth is the dirtbag repo's `docs/ROADMAP.md`, Phase 10, "On the rebuild".

**The board** (v0.956's "persistent gym walls"):
- Send City gains a steep board beside the weekly wall. It holds four problems, V4, V5, V6 and V7, left to right.
- They stay up four weeks (`BOARD_WEEKS`), then all four reset together. The block is `floor((day - 1) / 28) + 1`, so a block runs days 1–28, 29–56, and so on.
- Each problem is a library boulder: one crux, two beta, 6–9 moves, 12 ft. Styles lean steep: each problem draws from power, crimp, power, crimp, dyno, technical (power and crimp twice as likely).
- Ids are `bd-{block}-{n}`. An old id still resolves, so a route log keeps its name after the reset.
- The gym's lines are the week's six, then the board's four. The wall's rules apply unchanged: a day pass, and closed from 22:00.
- No change to the state's shape: board problems are logged by id like any other line.

**New seed stream:** `worldgen`, derived as `sendcity-board-{block}`. Per problem, in order, it draws:
1. the style: `int(0, 5)` into the list above;
2. the name: `int` into that style's unused names (or, once they run out, any unused board name);
3. the moves: `int(6, 9)`;
4. the crux's start: `moves × float(0.4, 0.6)`, rounded to 2 places;
5. the crux's length: `float(1.4, 2)`, rounded to 2 places.

**What the 2D harness measured** (12 seeds × 28 days × every start and strategy):
- **Before the board**, 4–10 of every 12 runs hit a day with nothing new to try, from about day 25. That day was always wet, with the week's set done.
- **With it**, 3 runs of 144 do, on days 26–27. In each, the climber had already sent all four board problems.
- The median climber ends the month at V4, not V3: the board fills wet days with hard climbing.
- The first V5 go comes about a day later (days 19–20, from 18–18.5): bots work the board's V4 first.
- All four of R2's season targets still pass.
- Board grades climb like the wall's, not stiffer:
  - board V4s and wall V4s are both sent at a median climber grade of V3;
  - 3 runs sent the V7 at V4.
  - Real boards run stiff; whether this one should is open *[proposed: Evan's call]*.

**What it means here:**
- The board is content: a set generator, like the weekly wall's. This repo can take it as a DataTable plus a generator on the `worldgen` stream.
- The derivation label above is the whole contract. Match it, and a seed sets the same board in both games.
- No change to `ResolveAttempt` or its golden vectors.

---

## 2026-09-29 — R2's end: Act I, pace on the wall, the send card, and the season's targets

**Built in the 2D rebuild's R2** (`dirtbag/app/src/sim`: `content/story.ts`, `story.ts`, `climb.ts`, `harness.ts`; the card in `src/view/paint/card.ts`). The source of truth is the dirtbag repo's `docs/ROADMAP.md`, R2 "Plan", whose *As built* notes mark the deviations and the calls still *[proposed]*.

**Act I, the demo's goal ladder** (v0.956's "Your First Season" quest, in its words):
- Five stages, in order:
  1. have $60 in hand;
  2. send 3 lines anywhere (+1 technique);
  3. send 2 outside;
  4. be regulars with someone (v0.956 wanted 15 reputation, which the rebuild doesn't have);
  5. climb V4 (+2 head).
- Stages are checked after every action, in order, and several can complete at once.
- The act ends with v0.956's closing text and $40. Dex's race starts the same night.

**Pace** *[proposed]*: a go climbs at the base rate × `clamp(1 + 0.08 × margin, 0.8, 1.25)`.
- The margin is your level in the route's style against its grade, fixed at the tie-in with the other mods.
- Fewer seconds on the wall means less pump for the same moves.
- v0.956 had no climb speed. The 2D harness shows no measurable change to the season.

**On the wall** (Phase 9). This is staging, not rules:
- the fall sheet draws the go as a bar against your best before it;
- a chalk band follows what the go climbed, and an X marks where it came off;
- past 55 pump the climber shakes, and past 45 the screen's edges close in.

**The send card** (2D only): a first send offers a PNG drawn on the device from the line's topo. It carries the name, grade, how it went ("Redpoint, go 6"), the crag, the day and season, the climber, and any first-ascent credit. Nothing is uploaded.

**The first season, as the 2D harness measures it** (12 seeds × 28 days × every start and strategy):
- A balanced climber's runway is 2.3–3.4 days at days 7–28, and no run is ever stuck.
- First-month injuries: 0% for moderate bots, 3% for reckless ones.
- An hour on the rock teaches 5.8–12.4 skill points (V0–V4); a setting shift, 0.75.
- The first V5 go comes around day 18.

**What it means here:**
- Act I is data: stages with an aim, a text and rewards. This repo's onboarding can take the same ladder as a DataTable.
- Pace matters to the sim only if this repo adopts the 2D attempt model, which is still undecided (see the R1 entry). If it does, the speed mod joins the pump factor at the tie-in.
- In a watched 3D session, pace is also staging: a stronger climber visibly moves faster. That is one answer to Phase 9's "two climbers play the same route differently". The 3D wall needs its own answer to "how close was that go".
- The season numbers above are what this repo's sim should reproduce if it ports the 2D rules.
- No new seed streams. Nothing here touches `ResolveAttempt` or its golden vectors.

---

## 2026-09-29 — R2 people: bonds, Sage's arc, Dex and the race, Scout

**Built in the 2D rebuild's R2** (`dirtbag/app/src/sim`: `presence.ts`, `curves.ts`, `content/people.ts`, `content/dog.ts`). The source of truth is the dirtbag repo's `docs/ROADMAP.md`, R2 "Plan", whose *As built* notes mark the deviations from v0.956.

**Bonds.**
- v0.956's tiers: Stranger 0, Acquaintance 1, Regular 3, Partner 5, Ride-or-Die 7.
- A day counts once toward the bond: climbing where a partner you've met is, or their belaying you. This is Phase 6's cap on v0.956's +1 per go.
- Each tier adds 0.07 to a partner's daily odds of turning up. Sage's base is 0.55, so Ride-or-Die Sage turns up five days in six.
- A Regular comes to the Gorge if asked before 1 PM on a dry day, and belays there.

**Arcs.**
- v0.956's four beats, at bonds 1, 3, 5 and 7, at least 5 days apart. The first beat waits a day past meeting.
- Sage's second beat sends her guiding for 7 days (v0.956: 16).
- The last beat needs a crag.

**Grades on curves of their own** (Phase 6: no rubber band):
- Sage's level is 4.3 + day/20, capped at 13 (v0.956's rate for her).
- Dex's level:
  - It is `min(peak, 5 + 0.045 × days trained) + 0.4 × form`.
  - The peak is 9–11.
  - He trains every day except a hurt stretch of 14–24 days that starts on a day from 35 to 75.
  - Form is a per-week value in [−1, 1], eased between weeks.

**Dex.**
- He appears where you send your first V4 (any V4+), for the rest of that day.
- After that he's out 2 days in 5, from 10:00 to 17:00: the crag on 70% of dry days, else the gym; never while hurt.
- The race: the night you're climbing V4 (displayed), if the open project is unclaimed and unsent, you get 5 days.
  - Send it and the first ascent is yours.
  - Otherwise he takes the first ascent, with a name from v0.956's list of 14.

**Scout.**
- He picks you on your 10th crag trip.
- Kibble costs $6, fills his bowl and adds 3 bond (on a 0–100 scale).
- Play is 60 minutes, once a day, for 10 bond. Each drive adds 2.
- Each night takes 22 food.

**What it means here:**
- **The seed means more again.** New derived streams, all on `events`:
  - `dex`: peak, hurt start, hurt length, in that order;
  - `dex-form-{week}`;
  - `dex-{day}`: out, then crag.
  - Sage's `sage-{day}` draw is unchanged. Only its threshold now reads the bond.
  - Picks that hold for a day hash `"{seed}:{key}:{day}"` with FNV-1a; Dex's first-ascent name hashes `"{seed}:{route}"`.
  - If the Unreal sim schedules people, draw them the same way, or a seed stops meaning the same thing in both games.
- Presence now reads state (bond, time away, invites) as well as the seed and the clock. It's still a pure function of both, so nothing about tomorrow is stored.
- None of this touches `ResolveAttempt` or its golden vectors.

---

## 2026-09-29 — R2 so far: windows close faster above your grade; load and injury; sandbags, first ascents and the Gorge

**Built in the 2D rebuild's R2** (`dirtbag/app/src/sim`). The source of truth is the dirtbag repo's `docs/ROADMAP.md`, R2 "Plan"; deviations from v0.956 are marked there.

**The window curve changes (replaces R1's formula below).** R1 scaled a beta's window by clamp(1 + 0.24·margin, 0.4, 1.4). The R2 harness showed the 0.4 floor let a V4 send the Gorge's V9: past two and a half grades over, every line was equally hard, and patient goes got there. Now:
- margin ≥ 0: min(1.4, 1 + 0.24·margin), as before;
- margin < 0: max(0.002, e^(0.9·margin)), about 40% of the window per grade over.
- With the harness's human-ish hands (throws and taps scattered about 50 ms, tension tracked with a lag of about 200 ± 70 ms), a one-crux boulder goes about 60% a go one grade over, 15% at two, 5% at three and never at four. A two-crux sport line, with pump, goes 10–30% a go one grade over.
- v0.956 made the same point with a hidden cliff: odds capped at 13% 1.5 grades short and 3% at 2.5. This keeps the slope without the step.

**Load and injury** (v0.956's acute:chronic model, with Phase 6's fixes):
- Each go's load = its energy cost × (1 + grade/9) × (0.6 + 0.4 × share of the line climbed). At sleep, acute and chronic load move toward the day's total (α 1/4 and 1/14). Both start at 20, so a first week can't spike.
- The ratio is read as if you slept now: today's goes count straight away, and a morning with no goes reads like a rest day.
- Over 1.3, each go risks an injury: 0.3 per 0.1 of ratio (capped at 40%) × style (crimp 1.5, crack 1.3, power and dyno 1.2) × 1.6 if cold × hunger × the go's size against a v0.956 go. Over 1.5, skill gains ×0.6. Over 1.7 you can't climb.
- Cold: a line at or above your grade before your first go of the day. Its windows are ×0.9.
- Injuries last 2–4, 6–9 or 13–18 days by tier. The clinic copay is $0, $45 or $210, and your first injury is free.

**Sandbags and first ascents:**
- A line can climb at a grade other than its label (`trueGrade`). Windows, pump and skill gains all use the true grade, and your first go on it says so.
- Open lines have no ascent. Send one first and you name it, and call its grade soft (−1), true or stout (+1), as in v0.956. From then on, the line goes by your name and your grade.

**Granite Gorge.** It opens at V4, closes in spring, and is shaded: no afternoon grease, and heat doesn't narrow the windows. Six boulders from V5 to an open V9 (Gorge Dyno is a V7 that climbs as V8) and three sport lines.

**What it means here:**
- The curve only matters if this repo adopts the 2D attempt model, which is still undecided (see the R1 entry). If it does, adopt this curve, not R1's.
- Load is a pure sim rule and ports as it stands. `SessionDials` would carry its numbers.
- **The seed means more again:** injury rolls draw from `session` → `injury-{day}-{go n}`. If the Unreal sim rolls injuries, draw them the same way.

---

## 2026-09-28 — R1: the climber's skills set the windows; the week's rules

**Built in the 2D rebuild's R1** (`dirtbag/app/src/sim`), ported from v0.956 where it had an answer. The source of truth is the dirtbag repo's `docs/ROADMAP.md`, R1 "Plan"; deviations from v0.956 are marked there.

**The resolved attempt model (settles the open difference in the entry below).** In 2D there is still no dice roll. Your skills set how wide each crux's window is; the verb's execution decides whether you're through.
- v0.956's five skills and grade curve: grade = floor((−8 + √(64 + 9.6·average)) / 4.8). The four starts are v0.956's archetypes.
- Each beta has a style (crimp, power, endurance, technical, dyno, crack). The style leans 65/35 on two skills, as in v0.956.
- margin = level(mix) − route grade, in grades. A beta's window = its base width × clamp(1 + 0.24·margin, 0.4, 1.4) × the day (prime rock 1.1, hot 0.9, seeping 0.92, sun on the wall 0.8, hunger down to 0.7).
- Pump rate × clamp(1 − 0.12·margin(endurance mix), 0.6, 1.5).
- The window scale is fixed at the tie-in. Pump and thin skin still narrow a window as the go goes on.
- Skill growth: v0.956's formula, but the route's style takes 60% and the beta you tried takes 40%. Goes are shorter than v0.956's, so ×0.6, and a lap of a sent line teaches ×0.3 (standing in for v0.956's staleness).

**What it means here:** `ResolveAttempt` rolls odds, with execution as one input. The 2D model has no roll: skills scale the window and execution is the whole outcome. Adopting it here would change `ResolveAttempt`, so it moves the golden vectors: save-breaking by this repo's rules. **Not decided here.** It needs a logged decision when this repo reaches its attempt work. Until then, `ResolveAttempt` and its vectors stay as they are.

**The week's rules, now spec:**
- **Money:**
  - What you can't cover goes on a card with a $150 limit. Past it, purchases are declined and the van spot becomes a cold night in the pullout. The van runs on fumes, so you're never stranded.
  - Bills ($70) land every seventh night regardless. Work needs energy but never food.
  - No game over: v0.956's hard wipe is gone.
- **Body:**
  - Fed (v0.956's hunger), −15 a night. Under 35, windows shrink; at 0 you can't climb.
  - Energy and skin come back with sleep.
- **Weather:**
  - One sky a day from the seed, with v0.956's 14-day seasons and weights (fall first).
  - Rain closes the crag, and it seeps the day after. Prime friction; hot days grease early.
- **People keep hours**, from the seed, the day and the clock:
  - Hazel belays at the crag from 8 to 5 on dry days.
  - Sage turns up about half the days, and always at the gym on day two.
  - A rope needs a belayer who's there.
  - Watching Sage is the third way to earn beta, after falling and asking.
- **Send City** resets six problems (V0–V5) each week from the seed. A problem's id carries its week.

**The seed now means more.** These derived streams are part of what a seed is, like the golden vectors:
- `worldgen` → `sky-{day}` and `sendcity-w{week}`;
- `events` → `sage-{day}`.

If the Unreal sim draws weather or schedules, draw them the same way, or a seed stops meaning the same thing in both games.

---

## 2026-09-28 — The 2D game is being rebuilt: beta then send, scrappy survival, side-view scenes

**Decided by Evan** after three look-book rounds and a playable feel slice. The source of truth is the dirtbag repo's `docs/ROADMAP.md`, section "Direction and the rebuild". The 2D game is being rebuilt in a new codebase (`dirtbag/app/`); v0.956 stays live until the rebuild's first season replaces it.

**What changed in the spec:**
1. **Climbing: beta, then send.**
   - Before a go, you choose beta for each crux. The beta sets that crux's verb and window:
     - a tension hold: hold and release to keep a marker in a moving band;
     - a timing tap: tap twice on the beat;
     - hold-to-load: charge, then let go in the band.
   - HOLD TO CLIMB stays the backbone between cruxes, with pump, and rests pay pump back.
   - Better beta is earned: by falling at a crux, by watching someone, or by asking the right person.
   - Pump, afternoon sun and thin skin narrow every window.
2. **Tone: scrappy survival with a sport core.** Money stays tight all season, not just in week 1. The feel slice starts you at $41, and a crag day without a shift ends in debt at bedtime. The attempt is the star.
3. **The clock moves only on actions and travel**, never while the player stands still.
4. **2D only, and not carried here:**
   - Side-view scenes joined by a valley map.
   - The look: park-poster landscapes with comic people, drawn in code.

**What it means here:**
- **The session model survives.** "2D minigames, 3D staging" still holds, and so does `ResolveAttempt`'s contract: per-move execution in [0, 1], and the sim decides. A crux's verb result maps onto that move's `execution`.
- **What's new for the sim:**
  - Beta becomes per-crux and discrete: a chosen sequence per crux, with its own verb and window. That replaces the single `beta` scalar in `AttemptInput`.
  - Unlocking beta is a sim rule: falls per crux, and what people know.
- **A difference still to settle.** In the 2D rebuild so far, the verb alone decides a crux; there is no dice roll. `ResolveAttempt` rolls odds, with execution as one input.
  - The 2D rebuild's next milestone (R1) adds the climber's stats to the windows. When it does, record the resolved model here.
  - Until then, don't change `ResolveAttempt`: its golden vectors are frozen.
- **The RNG is shared.** The rebuild's sim uses this repo's algorithm (mulberry32, FNV-1a over UTF-16 code units, named streams) and checks the same golden vectors, so a seed means the same thing in both games.
- **No change to Phase 0.** Its question — is a minigame-driven session tense and legible on a 3D wall? — still stands. The 2D rebuild will answer the 2D half of it first.
