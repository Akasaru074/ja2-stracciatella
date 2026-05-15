// TwoPlayerTest.cc
// Initializes a "from scratch" two-player hotseat sandbox:
//   - Loads A10 Omerta basement (SGPSector 10,1,1)
//   - Creates two AIM mercs (Barry & Blood) in the center of the room
//   - Merc 1 → Squad 0 (FIRST_SQUAD), Merc 2 → Squad 1 (SECOND_SQUAD)
//   - Both start on OUR_TEAM; the EndTurn logic in TeamTurns.cc handles
//     team-swapping between squads for hotseat PvP.

#include "TwoPlayerTest.h"

// Strategic / world
#include "Game_Init.h"        // InitStrategicLayer()
#include "StrategicMap.h"     // SetCurrentWorldSector(), gWorldSector, SGPSector
#include "Strategic.h"        // INSERTION_CODE_CENTER

// Soldiers & squads
#include "Soldier_Create.h"   // SOLDIERCREATE_STRUCT, TacticalCreateSoldier()
#include "Soldier_Profile.h"  // GetProfile(), LoadMercProfiles(), BARRY, BLOOD
#include "Squads.h"           // AddCharacterToSquad(), FIRST_SQUAD, SECOND_SQUAD
#include "Overhead.h"         // gTacticalStatus, OUR_TEAM, OppList helpers
#include "Overhead_Types.h"   // TacticalStatusType flags

// Misc
#include "SaveLoadGameStates.h"  // ResetGameStates()
#include "Tactical_Save.h"       // InitTacticalSave()
#include "Interface.h"           // SetCurrentInterfacePanel(), TEAM_PANEL

bool gfTwoPlayerSandboxMode = false;

// ---------------------------------------------------------------------------
// The A10 basement sector (x=10, y=1, z=1 == "A10 B1")
static const SGPSector SANDBOX_SECTOR(10, 1, 1);

// ---------------------------------------------------------------------------
// Helper: build a SOLDIERCREATE_STRUCT for a profiled AIM merc and place him
// in SANDBOX_SECTOR using the center insertion point.
static SOLDIERTYPE* CreateSandboxMerc(ProfileID profileID)
{
    SOLDIERCREATE_STRUCT cs{};
    cs.bTeam            = OUR_TEAM;
    cs.ubProfile        = profileID;
    cs.fCopyProfileItemsOver = TRUE; 
    cs.sSector          = SANDBOX_SECTOR;
    cs.bDirection       = NORTHEAST;
    cs.sInsertionGridNo = -1;

    SOLDIERTYPE* const s = TacticalCreateSoldier(cs);
    if (s)
    {
        s->ubStrategicInsertionCode = INSERTION_CODE_CENTER;
        s->usStrategicInsertionData = 0;
    }
    return s;
}

// ---------------------------------------------------------------------------
void InitTwoPlayerSandbox()
{
    // 1. Minimal game-state reset (mirrors what ReStartingGame / LoadSavedGame do)
    ResetGameStates();
    InitTacticalSave();
    LoadMercProfiles();

    // 2. Initialise the strategic layer (squads, clock, campaign structures …)
    InitStrategicLayer();

    // 3. Mark game as started so screens don't try to go to laptop, etc.
    gTacticalStatus.fHasAGameBeenStarted = TRUE;
    gfTwoPlayerSandboxMode = true;

    SOLDIERTYPE* const merc1 = CreateSandboxMerc(BARRY);
    if (merc1)
    {
        AddCharacterToSquad(merc1, FIRST_SQUAD);
    }

    SOLDIERTYPE* const merc2 = CreateSandboxMerc(BLOOD);
    if (merc2)
    {
        AddCharacterToSquad(merc2, SECOND_SQUAD);
        
        merc2->bTeam = ENEMY_TEAM;
        merc2->bSide = Side::ENEMY;
        
        RemoveManFromTeam(OUR_TEAM);
        AddManToTeam(ENEMY_TEAM);
    }

    // 6. Load the sector – this triggers UpdateMercsInSector() which places
    //    both soldiers on the map using their INSERTION_CODE_CENTER setting.
    SetCurrentWorldSector(SANDBOX_SECTOR);


    SetCurrentInterfacePanel(TEAM_PANEL);
}
