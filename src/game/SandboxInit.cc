// SandboxInit.cc
// Minimal combat sandbox for multiplayer testing.
// Loads a specific map sector, spawns two soldiers on opposing teams,
// and forces the engine into turn-based combat.

#include "SandboxInit.h"

#include "Game_Clock.h"        // InitializeJA2Clock
#include "Logger.h"
#include "Map_Information.h"   // gfWorldLoaded, gMapInformation
#include "Overhead.h"          // TacticalStatusType, CommonEnterCombatModeCode, etc.
#include "RenderWorld.h"       // gsRenderCenterX, gsRenderCenterY
#include "Soldier_Add.h"       // AddSoldierToSector
#include "Soldier_Control.h"   // OUR_TEAM, ENEMY_TEAM
#include "Soldier_Create.h"    // SOLDIERCREATE_STRUCT, TacticalCreateSoldier, TacticalCreateEnemySoldier
#include "Soldier_Profile.h"   // BARRY profile ID
#include "Overhead_Types.h"   // SEEN_CURRENTLY, STATUS_RED
#include "OppList.h"           // AllTeamsLookForAll, NO_INTERRUPTS
#include "Render_Fun.h"        // gubWorldRoomHidden, MAX_ROOMS
#include "Strategic_Movement.h" // CreateNewPlayerGroupDepartingFromSector, AddPlayerToGroup
#include "StrategicMap.h"      // gWorldSector, SGPSector, GetMapFileName
#include "WorldDef.h"          // LoadWorld, InitLoadedWorld, gfWorldLoaded

bool gfSandboxMode = false;

// Sector to load: A10 basement 1 (Omerta, resistance HQ)
// A = 1st row (y=1), column 10 (x=10), underground level 1
static const SGPSector SANDBOX_SECTOR(10, 1, 1);

// Offset applied to the map's center gridno to separate the two fighters.
// Keep this small (< 5) so they stay in the same room and have clear LOS.
static const INT16 PLAYER_OFFSET = -2;
static const INT16 ENEMY_OFFSET  = +2;

// -----------------------------------------------------------------------
static SOLDIERTYPE *SpawnFriendly(INT16 gridno)
{
	SOLDIERCREATE_STRUCT c{};
	c.bTeam             = OUR_TEAM;
	c.ubProfile         = BARRY;           // Barry – AIM merc, profile ID 0
	c.fCopyProfileItemsOver = TRUE;        // copy his default inventory
	c.sSector           = SANDBOX_SECTOR;
	c.sInsertionGridNo  = gridno;
	c.bDirection        = SOUTH;
	c.fVisible          = TRUE;

	SOLDIERTYPE* const s = TacticalCreateSoldier(c);
	if (!s) {
		SLOGE("SandboxInit: failed to create friendly soldier");
		return nullptr;
	}
	AddSoldierToSector(s);

	// Every OUR_TEAM soldier must belong to a strategic group, otherwise
	// the strategic layer crashes when it checks ubGroupID != 0.
	GROUP* const group = CreateNewPlayerGroupDepartingFromSector(SANDBOX_SECTOR);
	AddPlayerToGroup(*group, *s);

	return s;
}

// -----------------------------------------------------------------------
static SOLDIERTYPE *SpawnEnemy(INT16 gridno)
{
	SOLDIERCREATE_STRUCT c{};
	c.bTeam             = ENEMY_TEAM;
	c.ubProfile         = IVAN;            // Ivan - AIM merc, profile ID 6
	c.fCopyProfileItemsOver = TRUE;        // copy his default inventory
	c.sSector           = SANDBOX_SECTOR;
	c.sInsertionGridNo  = gridno;
	c.bDirection        = NORTH;
	c.fVisible          = TRUE;

	SOLDIERTYPE* const s = TacticalCreateSoldier(c);
	if (!s) {
		SLOGE("SandboxInit: failed to create enemy soldier");
		return nullptr;
	}
	AddSoldierToSector(s);
	return s;
}

// -----------------------------------------------------------------------
void InitMultiplayerSandbox()
{
	gfSandboxMode = true;

	SLOGI("SandboxInit: loading Omerta basement (A10-B1)");

	// --- Step 1: Set the world sector --------------------------------------
	gWorldSector = SANDBOX_SECTOR;

	// --- Step 2: Load the map ----------------------------------------------
	// GetMapFileName generates the correctly-cased name, e.g. "A10_b1.dat"
	const ST::string mapFile = GetMapFileName(gWorldSector, FALSE);
	SLOGI("SandboxInit: loading map file: {}", mapFile);
	try {
		LoadWorld(mapFile);
	} catch (...) {
		SLOGE("SandboxInit: LoadWorld({}) failed – check JA2 data files", mapFile);
		return;
	}

	// Mark world as loaded and compile movement costs / wireframe
	gfWorldLoaded = TRUE;
	InitLoadedWorld();

	// Centre the camera on the map's own centre gridno
	gsRenderCenterX = 805;
	gsRenderCenterY = 805;

	// --- Step 3: Game clock (required for AP calculation) ------------------
	InitializeJA2Clock();

	// --- Step 4: Reset battle-start flags ----------------------------------
	InitializeTacticalStatusAtBattleStart();
	gTacticalStatus.fEnemyInSector = TRUE;
	gTacticalStatus.fVirginSector  = FALSE;

	// --- Step 5: Reveal all rooms and spawn both soldiers ----------------
	// Underground maps have all rooms hidden by default. Unhide them so
	// both soldiers (and the player camera) can see the whole level.
	std::fill(std::begin(gubWorldRoomHidden), std::end(gubWorldRoomHidden), FALSE);

	// Spawn soldiers at the map's editor-defined centre ± a small offset
	const INT16 centre = gMapInformation.sCenterGridNo;
	SLOGI("SandboxInit: map centre gridno = {}", centre);

	SOLDIERTYPE* const friendly = SpawnFriendly(centre + PLAYER_OFFSET);
	SOLDIERTYPE* const enemy    = SpawnEnemy   (centre + ENEMY_OFFSET);

	// Select the friendly merc so the UI has a starting selection
	if (friendly) {
		SetSelectedMan(friendly);
	}

	// --- Step 6: Force turn-based combat, player goes first ---------------
	// Call CommonEnterCombatModeCode() directly to bypass the
	// NumCapableEnemyInSector() guard in EnterCombatMode().
	SLOGI("SandboxInit: entering combat mode");
	CommonEnterCombatModeCode();

	// Manually populate the opplist so both soldiers "see" each other,
	// bypassing the LOS check entirely. This is necessary because:
	// 1. Walls between them would block the LOS ray in AllTeamsLookForAll.
	// 2. CheckForEndOfCombatMode exits immediately if nobody sees anybody.
	if (friendly && enemy)
	{
		// Friendly sees enemy
		friendly->bOppList[enemy->ubID] = SEEN_CURRENTLY;
		friendly->bOppCnt = 1;

		// Enemy sees friendly (and is alert / hostile)
		enemy->bOppList[friendly->ubID] = SEEN_CURRENTLY;
		enemy->bOppCnt    = 1;
		enemy->bAlertStatus = STATUS_RED; // ensures AI acts as hostile
		enemy->bNeutral     = FALSE;
	}

	SLOGI("SandboxInit: NumCapableEnemyInSector = {}", NumCapableEnemyInSector());

	StartPlayerTeamTurn(FALSE, TRUE);

	SLOGI("SandboxInit: sandbox ready – player's turn");
	(void)enemy; // enemy is controlled by AI
}
