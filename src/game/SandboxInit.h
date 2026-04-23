#pragma once

extern bool gfSandboxMode;

// Initializes a minimal combat sandbox for multiplayer testing:
// - Loads sector D13 basement (Drassen resistance base)
// - Spawns one friendly merc (OUR_TEAM) and one enemy soldier (ENEMY_TEAM)
// - Forces the engine into turn-based combat mode
void InitMultiplayerSandbox();
