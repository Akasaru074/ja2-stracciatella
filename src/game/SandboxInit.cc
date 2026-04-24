// SandboxInit.cc
// Minimal combat sandbox for multiplayer testing.
// Loads a save game and enables sandbox mode (e.g. for hotseat multiplayer).

#include "SandboxInit.h"
#include "Logger.h"
#include "SaveLoadGame.h"

bool gfSandboxMode = false;

void InitMultiplayerSandbox()
{
	gfSandboxMode = true;

	SLOGI("SandboxInit: loading save game for multiplayer...");

	try {
		// Change the save name here to the one you want to load.
		LoadSavedGame("2026-04-23t05-02-18z-marina");
	} catch (...) {
		SLOGE("SandboxInit: failed to load save game. Check if it exists.");
	}
}
