/*

  Dune II - The Maker

  Author : Stefan Hendriks
  Contact: stefan@fundynamic.com
  Website: http://dune2themaker.fundynamic.com

  2001 - 2021 (c) code by Stefan Hendriks

  */

#include "include/d2tmh.h"

bool bDoDebug = false;
int	iRest = 1;	// default rest value

// the ultimate game variable(s)
cGame          				game;

cTimeManager   				TimeManager;
cStructureUtils 			structureUtils;
cMap           				map;
cMapEditor	  				mapEditor;
cRandomMapGenerator 		randomMapGenerator;

cAbstractStructure     	*	structure[MAX_STRUCTURES];
cUnit          				unit[MAX_UNITS];                // units in the game (max MAX_UNITS amount)
cMapCamera				*	mapCamera;
cPlayer        				players[MAX_PLAYERS];             // player is
cParticle      				particle[MAX_PARTICLES];
cBullet        				bullet[MAX_BULLETS];
cRegion        				world[MAX_REGIONS];
cDrawManager   			*	drawManager = NULL;

cAllegroDrawer          *   allegroDrawer = nullptr;

// Structs of all kinds of objects (*info)
s_PreviewMap     			    PreviewMap[MAX_SKIRMISHMAPS];
s_HouseInfo         			sHouseInfo[MAX_HOUSES];
s_StructureInfo    			    sStructureInfo[MAX_STRUCTURETYPES];
s_UnitInfo         			    sUnitInfo[MAX_UNITTYPES];
s_UpgradeInfo                   sUpgradeInfo[MAX_UPGRADETYPES];
s_SpecialInfo                   sSpecialInfo[MAX_SPECIALTYPES];
s_BulletInfo        			sBulletInfo[MAX_BULLET_TYPES];
s_ParticleInfo        			sParticleInfo[MAX_PARTICLE_TYPES];
sReinforcement  			    reinforcements[MAX_REINFORCEMENTS];

// palette
PALETTE general_palette;

// bitmap(s)
BITMAP *bmp_screen;
BITMAP *bmp_backgroundMentat;
BITMAP *bmp_throttle;
BITMAP *bmp_winlose;
BITMAP *bmp_fadeout;

// datafile(s)
DATAFILE *gfxdata;		// graphics (terrain, units, structures)
DATAFILE *gfxaudio;		// audio
DATAFILE *gfxinter;		// interface graphics
DATAFILE *gfxworld;		// world/pieces graphics
DATAFILE *gfxmentat;	// mentat graphics

// FONT stuff
ALFONT_FONT *game_font; 	// arrakeen.fon
ALFONT_FONT *bene_font;		// benegesserit font.
ALFONT_FONT *small_font;	// small font.

// MP3 STUFF
ALMP3_MP3   *mp3_music; // pointer to mp3 music

/** Allegro specific timer creation starts here **/
#ifdef ALLEGRO_H
	volatile int allegro_timerSecond = 0;
	volatile int allegro_timerGlobal = 0;
	volatile int allegro_timerUnits = 0;
#endif

int handleArguments(int argc, char *argv[]) {
    game.bDisableAI = false;
    game.bDisableReinforcements = false;
    game.bDrawUsages = false;
    game.bDrawUnitDebug = false;
    game.bOneAi = false;
    game.windowed = false;
    game.bNoAiRest = false;

	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
            std::string command = argv[i];
			if (command.compare("-game") == 0) {
				if ((i + 1) < argc) {
					i++;
					game.game_filename = std::string(argv[i]);
				}
			} else if (command.compare("-windowed") == 0) {
				// Windowed flag passed, so use that
				game.windowed = true;
			} else if (command.compare("-nomusic") == 0) {
				game.bPlayMusic = false;
			} else if (command.compare("-nosound") == 0) {
			    // disable all sound effects
				game.bPlayMusic = false;
				game.bPlaySound = false;
			} else if (command.compare("-debug") == 0) {
			    // generic debugging enabled
                bDoDebug = true;
			} else if (command.compare("-debug-units") == 0) {
                // unit debugging enabled
                game.bDrawUnitDebug = true;
			} else if (command.compare("-noai") == 0) {
                game.bDisableAI = true;
            } else if (command.compare("-oneai") == 0) {
                game.bOneAi = true;
            } else if (command.compare("-noreinforcements") == 0) {
                game.bDisableReinforcements = true;
            } else if (command.compare("-noairest") == 0) {
                game.bNoAiRest = true;
            } else if (command.compare("-usages") == 0) {
                game.bDrawUsages = true;
            }
		}
	} // arguments passed

	return 0;
}

/**
	Entry point of the game
*/
int main(int argc, char **argv) {
	game.game_filename = "game.ini";
    set_config_file("d2tm.cfg");

    if (handleArguments(argc, argv) > 0) {
        return 0;
    }

    if (game.setupGame()) {
		game.run();
	}

	game.shutdown();

	return 0;
}
END_OF_MAIN();

