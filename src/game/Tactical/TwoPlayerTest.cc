#include "TwoPlayerTest.h"
#include "Soldier_Control.h"
#include "Interface.h"
#include "Overhead.h"
#include "TeamTurns.h"
#include "OppList.h"

bool   TwoPlayerTest::bActive = false;
UINT8  TwoPlayerTest::ubCurrentPlayer = 0;
UINT8  TwoPlayerTest::ubMerc1ID = 0;
UINT8  TwoPlayerTest::ubMerc2ID = 0;

void TwoPlayerTest::Init() {
	SOLDIERTYPE* pMerc1 = NULL;
	SOLDIERTYPE* pMerc2 = NULL;
	int count = 0;

	FOR_EACH_IN_TEAM(s, OUR_TEAM) {
		if (!s->bInSector) continue;
		if (count == 0) pMerc1 = s;
		else if (count == 1) pMerc2 = s;
		else break;
		count++;
	}

	if (count < 2) return;

	ubMerc1ID = pMerc1->ubID;
	ubMerc2ID = pMerc2->ubID;

	pMerc2->bTeam = ENEMY_TEAM;
	pMerc2->bSide = Side::ENEMY;

	pMerc1->bOppList[ubMerc2ID] = NOT_HEARD_OR_SEEN;
	pMerc2->bOppList[ubMerc1ID] = NOT_HEARD_OR_SEEN;

	if (!(gTacticalStatus.uiFlags & INCOMBAT)) {
		EnterCombatMode(OUR_TEAM);
	}

	bActive = true;
	ubCurrentPlayer = 0;
}

void TwoPlayerTest::OnEndTurn() {
	if (!bActive) return;

	SOLDIERTYPE& igor = GetMan(ubMerc1ID);
	SOLDIERTYPE& sdoba = GetMan(ubMerc2ID);

	if (ubCurrentPlayer == 0) {
		gTacticalStatus.ubCurrentTeam = ENEMY_TEAM;
		sdoba.bActionPoints = sdoba.bInitialActionPoints;
		g_selected_man = &sdoba;
		SetCurrentTacticalPanelCurrentMerc(&sdoba);
		ubCurrentPlayer = 1;
	}
	else {
		gTacticalStatus.ubCurrentTeam = OUR_TEAM;
		igor.bActionPoints = igor.bInitialActionPoints;
		SelectSoldier(&igor, SELSOLDIER_NONE);
		ubCurrentPlayer = 0;
	}
}

bool TwoPlayerTest::IsActive() {
	return bActive;
}
