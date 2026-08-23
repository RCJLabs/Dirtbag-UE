// The Phase 0 staged-replay wall: walk up, press E, watch the sim's attempt
// acted out on the hold spline. Presentation only — every outcome comes from
// the sim (AttemptInSession); this actor just stages the returned timeline:
// odds → hesitation, success → reach to the next hold, failure → the fall.
//
// Editor setup (notes/phase0-blockout.md): place in front of a wall mesh,
// drag the HoldLine spline points up the wall, assign a skeletal mesh and
// the four animations in the Details panel. Everything else is handled.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DirtbagGameInstance.h"
#include "DirtbagSimTypes.h"

#include "DirtbagClimbWall.generated.h"

class UAnimSequence;
class UAudioComponent;
class USoundBase;
class UBoxComponent;
class UCameraComponent;
class UDirtbagGameInstance;
class UInstancedStaticMeshComponent;
class USkeletalMeshComponent;
class USplineComponent;

UCLASS()
class ADirtbagClimbWall : public AActor
{
	GENERATED_BODY()

public:
	ADirtbagClimbWall();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Which place this wall belongs to. Read by the game mode at BeginPlay
	 *  so a level whose walls agree knows where it is before the player has
	 *  walked up to anything. */
	EDirtbagVenue GetVenue() const { return Venue; }
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	// --- Components -----------------------------------------------------

	UPROPERTY(VisibleAnywhere, Category = "Dirtbag")
	TObjectPtr<USceneComponent> Root;

	/** The route line: one point per hold, bottom to top. Edit in-level. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dirtbag")
	TObjectPtr<USplineComponent> HoldLine;

	/** Small spheres rendered at each spline point so holds read at a glance. */
	UPROPERTY(VisibleAnywhere, Category = "Dirtbag")
	TObjectPtr<UInstancedStaticMeshComponent> HoldMarkers;

	/** Walk into this to get the prompt; E starts the session. */
	UPROPERTY(VisibleAnywhere, Category = "Dirtbag")
	TObjectPtr<UBoxComponent> ApproachTrigger;

	/** The watched-session frame. Position it to see the whole wall. */
	UPROPERTY(VisibleAnywhere, Category = "Dirtbag")
	TObjectPtr<UCameraComponent> SessionCamera;

	/** The breath, attached to the climber so it moves up the route with
	 *  them. Never auto-activates: silence is the correct state of a wall
	 *  nobody is on. */
	UPROPERTY(VisibleAnywhere, Category = "Dirtbag")
	TObjectPtr<UAudioComponent> Breath;

	/** The climber puppet. Assign the template's Quinn/Manny mesh. Not a
	 *  Character — it is placed, never simulated. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dirtbag")
	TObjectPtr<USkeletalMeshComponent> Climber;

	// --- Assets (assign in Details) -------------------------------------

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Anims")
	TObjectPtr<UAnimSequence> HangIdleAnim;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Anims")
	TObjectPtr<UAnimSequence> ReachLeftAnim;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Anims")
	TObjectPtr<UAnimSequence> ReachRightAnim;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Anims")
	TObjectPtr<UAnimSequence> FallAnim;

	/** Optional: played once when the session starts (grab wall from ground). */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Anims")
	TObjectPtr<UAnimSequence> MountAnim;

	/** Optional: played on a send (climb up over the top). */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Anims")
	TObjectPtr<UAnimSequence> TopOutAnim;

	UPROPERTY(EditAnywhere, Category = "Dirtbag")
	TObjectPtr<UStaticMesh> HoldMarkerMesh;

	/** True: the player drives each move with HOLD TO CLIMB (Space).
	 *  False: the bot-staged replay, as before. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag")
	bool bInteractive = true;

	// --- Route / climber config -----------------------------------------

	/** Which place this wall is part of, and the only switch that moves a
	 *  wall outdoors — there is no global one, because the game instance is
	 *  not an actor and has no details panel to put it in. Gym walls take a
	 *  problem off the board; crag walls take a line out of the guidebook
	 *  and get weather, dirt and lines nobody has climbed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Route")
	EDirtbagVenue Venue = EDirtbagVenue::Gym;

	/** With a DirtbagGameInstance present, this wall carries the route at
	 *  #BoardIndex in whichever venue it belongs to — the gym board indoors,
	 *  the guidebook outdoors (where 25-27 are the open projects) — and the
	 *  fields below become fallbacks (used only in levels without the game
	 *  instance, e.g. isolated test maps). */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Route")
	int32 BoardIndex = 0;

	/** Which way the climber faces, in the wall's own space. -90 is into
	 *  the wall, which is where a climber looks.
	 *
	 *  The wall's face points along +Y: the approach trigger sits at +250Y
	 *  and the session camera at +450Y looking back down -Y. The climber has
	 *  to agree with that, and until now nothing set its rotation at all —
	 *  only SetWorldLocation, every move — so it kept whatever the mesh
	 *  happened to be authored with and climbed with its back to the rock.
	 *
	 *  **This is no longer a fixed angle, and the two wrong guesses are
	 *  why.** -90 put the climber side-on; 180 did too. Both were computed
	 *  from an assumed level layout -- wall in the actor's XZ plane, camera
	 *  looking down -Y -- and the holds actually come from a spline drawn
	 *  by hand, on an actor that may itself be rotated. Any fixed number is
	 *  a guess about somebody else's level.
	 *
	 *  So the facing is derived instead: the climber turns its back on the
	 *  session camera, which is what "facing the rock" means from the only
	 *  viewpoint that matters. That is correct for any spline, any actor
	 *  rotation, and any camera nudge, with nothing to retype.
	 *
	 *  What remains is a fact about the *mesh* rather than the level: which
	 *  way the skeletal mesh points when its rotation is zero. The UE5
	 *  mannequins are authored facing +Y, which is 90. That is the one
	 *  number here, it is a property of the asset, and it is the only thing
	 *  to change if a climbing pack ships a mesh built down +X (0) or -X
	 *  (180). */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Staging")
	float MeshForwardYaw = 90.f;

	/** Turn the derivation off and pin the facing by hand. For a wall the
	 *  session camera never looks at squarely -- or to prove which way is
	 *  which by eye. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Staging")
	bool bFaceAwayFromCamera = true;

	/** Used when bFaceAwayFromCamera is off. Relative to the actor. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Staging")
	float ClimberYaw = 180.f;

	/** Point the climber at the rock: back to the session camera, upright.
	 *  Safe to call before BeginPlay and in the editor. */
	void FaceTheRock();

	// --- Fallbacks -------------------------------------------------------
	// Used only in levels with no game instance (isolated test maps). With
	// one present these are overwritten at BeginPlay from the board or the
	// guidebook, so whatever the panel shows here is last run's answer, not
	// this wall's route. Collapsed under Advanced so they stop reading as
	// settings you are supposed to keep in sync.

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Dirtbag|Route")
	FString WorldSeed = TEXT("gym-1");

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Dirtbag|Route")
	FString SessionSeed = TEXT("gym-1");

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Dirtbag|Route")
	FString RouteName = TEXT("First Blood");

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Dirtbag|Route")
	int32 Grade = 4;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Dirtbag|Route")
	int32 TrueGrade = 4;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Dirtbag|Route")
	EDirtbagRouteType RouteType = EDirtbagRouteType::Power;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Route")
	FDirtbagClimber ClimberStats;

	// --- Staging dials (feel, not sim — sim dials live in Sim/) ---------

	/** Seconds at a hold before a sure move; risk stretches it. */
	// --- The sound ------------------------------------------------------
	//
	// Phase 5 item 4. There was none at all. `concepts/DIRTBAG.md` section 7
	// names *"climbing sound design (chalk, breath, rubber on rock)"* on the
	// custom-work list, and those three are exactly the slots below.
	//
	// **Every one of these is optional and every play site is guarded.**
	// With nothing assigned the game behaves precisely as it did — which is
	// the state it ships in from this container, because there is no editor
	// here to assign an asset and no way to make one.
	//
	// What the container *can* build is the half that is not foley: **where
	// a sound fires, and what the sim makes it do when it does.** A cue
	// fired flat is a sound effect; a cue whose pitch and volume come off
	// the same numbers the camera reads is staging. All of these take their
	// parameters from the session, so the audio cannot drift away from what
	// the shot and the bars are saying.

	/** Chalking up. Fires at a stance, and **only before a move worth
	 *  chalking for** — see ChalkBelowOdds. Nobody chalks for a jug, and a
	 *  sound that happens every move says nothing. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Sound")
	TObjectPtr<USoundBase> ChalkSound;

	/** The odds at or below which the next move is worth chalking for. This
	 *  is the same signal the camera tightens on, deliberately: the shot
	 *  coming in and the chalk going on say the same thing about the next
	 *  move, one to the eye and one to the ear. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Sound")
	float ChalkBelowOdds = 0.7f;

	/** Rubber on rock: one per move, pitched and levelled by how well the
	 *  move was executed. A latched move is quiet and precise; a scrappy
	 *  one is neither. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Sound")
	TObjectPtr<USoundBase> MoveSound;

	/** Coming off. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Sound")
	TObjectPtr<USoundBase> SlipSound;

	/** Hitting the mat — louder from higher up, because it was. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Sound")
	TObjectPtr<USoundBase> LandSound;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Sound")
	TObjectPtr<USoundBase> TopOutSound;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Sound")
	TObjectPtr<USoundBase> BrushSound;

	/** Breathing. A **loop**, running for the whole attempt, with its
	 *  volume and pitch driven by dirtbag::PumpShows — the same curve the
	 *  camera sway reads.
	 *
	 *  This is the one that matters. Pump is the resource the whole session
	 *  turns on and the player has been reading it off a bar; breath is how
	 *  a pumped climber actually sounds, and it is the ear's version of
	 *  Phase 0's gate. Assign a calm loop and let the pitch do the work. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Sound")
	TObjectPtr<USoundBase> BreathLoop;

	/** What the breath does between fresh and spent. Volume rises from
	 *  quiet to full; pitch rises to this. Modest on purpose — past about
	 *  1.3 a breath loop reads as a chipmunk rather than as somebody in
	 *  trouble. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Sound")
	float BreathQuietVolume = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Sound")
	float BreathPitchAtLimit = 1.22f;

	// --- The shot -------------------------------------------------------
	//
	// Phase 5 item 3. The session camera was a component placed at a fixed
	// relative offset and never touched again: a locked-off tripod for the
	// whole attempt, so on any line taller than the frame **the climber
	// simply left it** and the watcher spent the crux looking at rock.
	//
	// The rule learned the hard way from the facing bug: **the container
	// cannot see the level, so it must not guess at it.** Nothing here
	// invents an angle. The camera as Evan placed it *is* the wide shot --
	// its authored offset gives both the distance and the direction out
	// from the wall -- and everything below only moves in from there. Place
	// the camera where the shot looks right and the defaults leave it
	// alone.

	/** Let the shot follow the climber and tighten on hard moves. Off puts
	 *  the locked-off tripod back, exactly as it was. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Shot")
	bool bCameraFollows = true;

	/** How far the shot comes in on a desperate move, as a fraction of the
	 *  distance you placed it at. A crux gets a closer shot; that is the
	 *  oldest sentence in the language and it costs nothing to speak it. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Shot")
	float CameraTightenBy = 0.32f;

	/** And how much narrower the lens goes with it, in degrees off the
	 *  authored FOV. Small on purpose: past about ten degrees it reads as a
	 *  zoom rather than as tension. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Shot")
	float CameraNarrowBy = 7.f;

	/** Where the climber sits in frame, in centimetres below the shot's
	 *  centre. Positive keeps rock above their hands — which is where they
	 *  are going, and the half of the frame that says how much is left. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Shot")
	float CameraLead = 55.f;

	/** How fast the shot catches up, and how fast it reads a change of
	 *  difficulty. The second is slower than the first on purpose: the
	 *  framing should settle into a crux rather than snap to it. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Shot")
	float CameraEase = 4.f;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Shot")
	float TensionEase = 1.8f;

	/** How far the shot breathes at full pump, in centimetres.
	 *
	 *  This is the pump bar said without a bar: fresh, the camera is
	 *  locked; pumped, it will not quite hold still. Scaled by the *square*
	 *  of pump so it is invisible for the first half of a route and
	 *  unmistakable at the top, which is also how being pumped works. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Shot")
	float PumpSway = 7.f;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Shot")
	float PumpSwaySpeed = 1.7f;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Staging")
	float BaseHesitation = 0.4f;

	/** Extra seconds of hesitation at odds 0 — a 40% move visibly gathers. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Staging")
	float HesitationPerRisk = 1.5f;

	/** Seconds a hold-to-hold move takes. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Staging")
	float MoveDuration = 0.6f;

	/** Seconds from peel-off to the mat. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Staging")
	float FallDuration = 0.7f;

	/** Seconds the result card lingers before the camera hands back. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Staging")
	float EndPause = 2.0f;

	/** Hours of brushing per press of the clean key. Half an hour is small
	 *  enough that cleaning a line is several deliberate presses rather than
	 *  one, which is roughly what it feels like. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dirtbag|Verb")
	float CleanHoursPerPress = 0.5f;

	// --- The HOLD TO CLIMB verb (interactive mode; presentation dials — the
	// --- sim only ever sees the resulting 0..1 execution scalar) -----------

	/** Seconds of holding Space for the grip meter to reach full. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Verb")
	float ChargeTime = 0.9f;

	/** Sweet window on the grip meter: release inside it to latch the move.
	 *  Release below the window = shake out instead; charge past 1.0 =
	 *  over-grip, the move fires itself with OvergripExecution. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Verb")
	float SweetWindowStart = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Dirtbag|Verb")
	float SweetWindowEnd = 0.95f;

	/** Execution for a release dead-center in the window... */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Verb")
	float PerfectExecution = 0.95f;

	/** ...tapering to this at the window's edges. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Verb")
	float EdgeExecution = 0.6f;

	/** The death grip: what holding on too long costs you. */
	UPROPERTY(EditAnywhere, Category = "Dirtbag|Verb")
	float OvergripExecution = 0.35f;

private:
	enum class EPhase : uint8 { Idle, Mounting, AtStance, Moving, Falling, Ending };

	UFUNCTION()
	/** What is here and what the keys do about it, rebuilt on every change
	 *  and drawn for as long as you are stood at the bottom of the line.
	 *  Was three toasts on two keyed slots, one of which silently ate the
	 *  other. See notes/phase5-toast-triage.md. */
	void PushPrompt();

	/** Open or close the crag's page. The wall is where you actually reach
	 *  for a guidebook — standing under something, deciding. */
	void OnGuidebook();

	/** The shortcut nobody would see. T offers what this line will take;
	 *  1/2/3 does it.
	 *
	 *  A prompt state rather than a screen, deliberately: this is a thing
	 *  you do in a moment at the bottom of a route, not a menu you open. */
	void OnShortcut();
	bool TakeShortcut(int32 Which);
	void OnShortcut1();
	void OnShortcut2();
	void OnShortcut3();

	/** Drive the shot: follow the climber, tighten on a hard move, breathe
	 *  with the pump. Reads the same FDirtbagSessionReadout the HUD reads,
	 *  so the camera and the bars can never disagree about how hard this
	 *  move is — and so a replayed attempt is framed exactly like a driven
	 *  one, because the readout already answers for both. */
	void UpdateCamera(float DeltaSeconds);

	/** Keep the breath in step with the pump. Same source as the camera
	 *  sway, so the ear and the eye cannot disagree. */
	void UpdateBreath();

	/** Start and stop the breath loop with the attempt. Safe to call with
	 *  no BreathLoop assigned, which is how it ships from the container. */
	void StartBreath();
	void StopBreath();

	/** Chalk, but only when the next move is worth chalking for. Nobody
	 *  chalks for a jug, and a cue that fires every move says nothing at
	 *  all — the silence between them is what makes one mean something. */
	void ChalkUpIfItIsWorthIt();

	/** One-shot, guarded, at the climber. Every sound in this file goes
	 *  through here so that "no asset assigned" is handled in exactly one
	 *  place rather than at nine call sites. */
	void PlayCue(USoundBase* Cue, float Volume = 1.f, float Pitch = 1.f);

	/** Put the shot back where it was placed. Called when a session ends,
	 *  so nothing the camera did during an attempt survives it. */
	void RestCamera();

	void OnApproachBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                     bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnApproachEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OnInteract();
	void OnClean();
	void OnAskBeta();
	void OnHoldPressed();
	void OnHoldReleased();
	void StartAttempt();
	void BeginSessionBody();
	void ScheduleNextMove();
	void BeginMove();
	void CommitMove(double Execution);
	void StageMoveResult(bool bSuccess);
	void FinishLiveAttempt();
	void FinishAttempt();
	void EndSession();
	void UpdateHud();

	// Re-read what the book calls this line. Only display: Route.Name is the
	// ledger key every attempt is recorded against and must never move,
	// which is exactly why the name on screen is a separate field.
	void RefreshBookName();

	void PlayAnim(UAnimSequence* Anim, bool bLoop);
	FVector HoldLocation(int32 Index) const;

	// Central state when present; the wall then reads the board and commits
	// through it. Null in game-instance-less test maps → legacy standalone.
	UPROPERTY()
	TObjectPtr<UDirtbagGameInstance> Game;

	FDirtbagRoute Route;
	FDirtbagSessionState Session;
	FDirtbagProjectMemory Memory;
	FDirtbagAttemptResult Current;

	// Interactive-session state: the live attempt the player is driving.
	dirtbag::Route SimRoute;
	dirtbag::LiveAttempt Live;
	bool bLiveSession = false;
	bool bCharging = false;
	float Charge = 0.0f;

	EPhase Phase = EPhase::Idle;

	/** The shot as authored, captured once at BeginPlay and never
	 *  re-read.
	 *
	 *  Captured rather than recomputed because the camera moves during a
	 *  session: re-reading its transform would feed the shot its own
	 *  output and the authored framing would be gone by the second
	 *  attempt. This is the same class of mistake as a fixed climber yaw,
	 *  from the other end — there the container guessed a number it could
	 *  not see, here it would have quietly overwritten one it could. */
	FVector RestOffset = FVector::ZeroVector;
	FRotator RestRotation = FRotator::ZeroRotator;
	float RestFov = 90.f;
	bool bShotCaptured = false;

	/** 0 while the move is a gimme, 1 while it is desperate. Eased rather
	 *  than set, so the framing settles into a crux. */
	/** True while T has offered and nothing has been picked. Cleared by
	 *  walking away and by taking one, so an offer cannot sit open across
	 *  a session. */
	bool bShortcutOffered = false;

	float Tension = 0.f;
	float SwayTime = 0.f;

	/** The shot's own smoothed position, in actor space. Kept here rather
	 *  than read back off the camera because the camera also carries the
	 *  pump sway, and smoothing towards a value that already contains last
	 *  frame's sway turns the sway into a drift. */
	FVector ShotLocal = FVector::ZeroVector;
	int32 TimelineIndex = 0;
	int32 HoldIndex = 0;
	bool bReachLeft = false;
	bool bPlayerNear = false;
	bool bBoundInput = false;
	float MoveAlpha = 0.0f;
	FVector MoveFrom = FVector::ZeroVector;
	FVector MoveTo = FVector::ZeroVector;
	FTimerHandle PhaseTimer;
};
