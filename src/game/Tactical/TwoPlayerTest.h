#ifndef TWOPLAYERTEST_H
#define TWOPLAYERTEST_H

#include "Types.h"

class TwoPlayerTest {
public:
	static void Init();
	static void OnEndTurn();
	static bool IsActive();

private:
	static bool   bActive;
	static UINT8  ubCurrentPlayer;
	static UINT8  ubMerc1ID;
	static UINT8  ubMerc2ID;
};

#endif
