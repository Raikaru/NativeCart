#include "global.h"
#include "constants/moves.h"
#include "constants/battle.h"
#include "constants/battle_move_effects.h"
#include "constants/battle_script_commands.h"
#include "constants/battle_anim.h"
#include "constants/items.h"
#include "constants/abilities.h"
#include "constants/species.h"
#include "constants/pokemon.h"
#include "constants/songs.h"
#include "constants/game_stat.h"
#include "constants/battle_string_ids.h"

#define ASM_STR2(x) #x
#define ASM_STR(x) ASM_STR2(x)

asm(
".set NULL, 0\n"
".set FALSE, 0\n"
".set TRUE, 1\n"
#ifdef BATTLE_TYPE_OLD_MAN_TUTORIAL
".set BATTLE_TYPE_OLD_MAN_TUTORIAL, " ASM_STR(BATTLE_TYPE_OLD_MAN_TUTORIAL) "\n"
#endif
#ifdef BATTLE_TYPE_POKEDUDE
".set BATTLE_TYPE_POKEDUDE, " ASM_STR(BATTLE_TYPE_POKEDUDE) "\n"
#endif
#ifdef BATTLE_TYPE_SAFARI
".set BATTLE_TYPE_SAFARI, " ASM_STR(BATTLE_TYPE_SAFARI) "\n"
#endif
#ifdef BS_ATTACKER
".set BS_ATTACKER, " ASM_STR(BS_ATTACKER) "\n"
#endif
#ifdef BS_OPPONENT1
".set BS_OPPONENT1, " ASM_STR(BS_OPPONENT1) "\n"
#endif
#ifdef BS_PLAYER2
".set BS_PLAYER2, " ASM_STR(BS_PLAYER2) "\n"
#endif
#ifdef BS_TARGET
".set BS_TARGET, " ASM_STR(BS_TARGET) "\n"
#endif
#ifdef B_ANIM_BAIT_THROW
".set B_ANIM_BAIT_THROW, " ASM_STR(B_ANIM_BAIT_THROW) "\n"
#endif
#ifdef B_ANIM_ROCK_THROW
".set B_ANIM_ROCK_THROW, " ASM_STR(B_ANIM_ROCK_THROW) "\n"
#endif
#ifdef B_ANIM_SAFARI_REACTION
".set B_ANIM_SAFARI_REACTION, " ASM_STR(B_ANIM_SAFARI_REACTION) "\n"
#endif
#ifdef B_OUTCOME_CAUGHT
".set B_OUTCOME_CAUGHT, " ASM_STR(B_OUTCOME_CAUGHT) "\n"
#endif
#ifdef B_OUTCOME_NO_SAFARI_BALLS
".set B_OUTCOME_NO_SAFARI_BALLS, " ASM_STR(B_OUTCOME_NO_SAFARI_BALLS) "\n"
#endif
#ifdef B_OUTCOME_RAN
".set B_OUTCOME_RAN, " ASM_STR(B_OUTCOME_RAN) "\n"
#endif
#ifdef B_WAIT_TIME_LONG
".set B_WAIT_TIME_LONG, " ASM_STR(B_WAIT_TIME_LONG) "\n"
#endif
#ifdef B_WAIT_TIME_MED
".set B_WAIT_TIME_MED, " ASM_STR(B_WAIT_TIME_MED) "\n"
#endif
#ifdef CMP_EQUAL
".set CMP_EQUAL, " ASM_STR(CMP_EQUAL) "\n"
#endif
#ifdef CMP_NOT_EQUAL
".set CMP_NOT_EQUAL, " ASM_STR(CMP_NOT_EQUAL) "\n"
#endif
#ifdef GAME_STAT_POKEMON_CAPTURES
".set GAME_STAT_POKEMON_CAPTURES, " ASM_STR(GAME_STAT_POKEMON_CAPTURES) "\n"
#endif
#ifdef HITMARKER_IGNORE_SUBSTITUTE
".set HITMARKER_IGNORE_SUBSTITUTE, " ASM_STR(HITMARKER_IGNORE_SUBSTITUTE) "\n"
#endif
#ifdef ITEM_SAFARI_BALL
".set ITEM_SAFARI_BALL, " ASM_STR(ITEM_SAFARI_BALL) "\n"
#endif
#ifdef MUS_POKE_FLUTE
".set MUS_POKE_FLUTE, " ASM_STR(MUS_POKE_FLUTE) "\n"
#endif
#ifdef SE_FLEE
".set SE_FLEE, " ASM_STR(SE_FLEE) "\n"
#endif
#ifdef SE_USE_ITEM
".set SE_USE_ITEM, " ASM_STR(SE_USE_ITEM) "\n"
#endif
#ifdef STRINGID_DONTBEATHIEF
".set STRINGID_DONTBEATHIEF, " ASM_STR(STRINGID_DONTBEATHIEF) "\n"
#endif
#ifdef STRINGID_EMPTYSTRING3
".set STRINGID_EMPTYSTRING3, " ASM_STR(STRINGID_EMPTYSTRING3) "\n"
#endif
#ifdef STRINGID_GIVENICKNAMECAPTURED
".set STRINGID_GIVENICKNAMECAPTURED, " ASM_STR(STRINGID_GIVENICKNAMECAPTURED) "\n"
#endif
#ifdef STRINGID_GOTCHAPKMNCAUGHT
".set STRINGID_GOTCHAPKMNCAUGHT, " ASM_STR(STRINGID_GOTCHAPKMNCAUGHT) "\n"
#endif
#ifdef STRINGID_GOTCHAPKMNCAUGHT2
".set STRINGID_GOTCHAPKMNCAUGHT2, " ASM_STR(STRINGID_GOTCHAPKMNCAUGHT2) "\n"
#endif
#ifdef STRINGID_ITDODGEDBALL
".set STRINGID_ITDODGEDBALL, " ASM_STR(STRINGID_ITDODGEDBALL) "\n"
#endif
#ifdef STRINGID_MONHEARINGFLUTEAWOKE
".set STRINGID_MONHEARINGFLUTEAWOKE, " ASM_STR(STRINGID_MONHEARINGFLUTEAWOKE) "\n"
#endif
#ifdef STRINGID_OLDMANUSEDITEM
".set STRINGID_OLDMANUSEDITEM, " ASM_STR(STRINGID_OLDMANUSEDITEM) "\n"
#endif
#ifdef STRINGID_OUTOFSAFARIBALLS
".set STRINGID_OUTOFSAFARIBALLS, " ASM_STR(STRINGID_OUTOFSAFARIBALLS) "\n"
#endif
#ifdef STRINGID_PKMNDATAADDEDTODEX
".set STRINGID_PKMNDATAADDEDTODEX, " ASM_STR(STRINGID_PKMNDATAADDEDTODEX) "\n"
#endif
#ifdef STRINGID_PKMNSITEMRESTOREDHEALTH
".set STRINGID_PKMNSITEMRESTOREDHEALTH, " ASM_STR(STRINGID_PKMNSITEMRESTOREDHEALTH) "\n"
#endif
#ifdef STRINGID_PLAYERUSEDITEM
".set STRINGID_PLAYERUSEDITEM, " ASM_STR(STRINGID_PLAYERUSEDITEM) "\n"
#endif
#ifdef STRINGID_POKEDUDEUSED
".set STRINGID_POKEDUDEUSED, " ASM_STR(STRINGID_POKEDUDEUSED) "\n"
#endif
#ifdef STRINGID_POKEFLUTE
".set STRINGID_POKEFLUTE, " ASM_STR(STRINGID_POKEFLUTE) "\n"
#endif
#ifdef STRINGID_POKEFLUTECATCHY
".set STRINGID_POKEFLUTECATCHY, " ASM_STR(STRINGID_POKEFLUTECATCHY) "\n"
#endif
#ifdef STRINGID_RETURNMON
".set STRINGID_RETURNMON, " ASM_STR(STRINGID_RETURNMON) "\n"
#endif
#ifdef STRINGID_THREWBAIT
".set STRINGID_THREWBAIT, " ASM_STR(STRINGID_THREWBAIT) "\n"
#endif
#ifdef STRINGID_THREWROCK
".set STRINGID_THREWROCK, " ASM_STR(STRINGID_THREWROCK) "\n"
#endif
#ifdef STRINGID_TRAINER1USEDITEM
".set STRINGID_TRAINER1USEDITEM, " ASM_STR(STRINGID_TRAINER1USEDITEM) "\n"
#endif
#ifdef STRINGID_TRAINERBLOCKEDBALL
".set STRINGID_TRAINERBLOCKEDBALL, " ASM_STR(STRINGID_TRAINERBLOCKEDBALL) "\n"
#endif
#ifdef STRINGID_YOUTHROWABALLNOWRIGHT
".set STRINGID_YOUTHROWABALLNOWRIGHT, " ASM_STR(STRINGID_YOUTHROWABALLNOWRIGHT) "\n"
#endif
"	.include \"pokefirered_core/generated/src/data/battle_script_macros_portable.inc\"\n"
"\n"
"	.set NULL, 0\n"
"	.set FALSE, 0\n"
"	.set TRUE, 1\n"
"	.section .rdata,\"dr\"\n"
"	.align 2\n"
"\n"
"	.global gBattlescriptsForBallThrow\n"
"gBattlescriptsForBallThrow:\n"
"	.4byte BattleScript_ThrowBall\n"
"	.4byte BattleScript_ThrowBall\n"
"	.4byte BattleScript_ThrowBall\n"
"	.4byte BattleScript_ThrowBall\n"
"	.4byte BattleScript_ThrowBall\n"
"	.4byte BattleScript_ThrowSafariBall\n"
"	.4byte BattleScript_ThrowBall\n"
"	.4byte BattleScript_ThrowBall\n"
"	.4byte BattleScript_ThrowBall\n"
"	.4byte BattleScript_ThrowBall\n"
"	.4byte BattleScript_ThrowBall\n"
"	.4byte BattleScript_ThrowBall\n"
"	.4byte BattleScript_ThrowBall\n"
"\n"
"	.global gBattlescriptsForUsingItem\n"
"gBattlescriptsForUsingItem:\n"
"	.4byte BattleScript_PlayerUseItem\n"
"	.4byte BattleScript_AIUseFullRestoreOrHpHeal\n"
"	.4byte BattleScript_AIUseFullRestoreOrHpHeal\n"
"	.4byte BattleScript_AIUseStatRestore\n"
"	.4byte BattleScript_AIUseXstat\n"
"	.4byte BattleScript_AIUseGuardSpec\n"
"\n"
"	.global gBattlescriptsForRunningByItem\n"
"gBattlescriptsForRunningByItem:\n"
"	.4byte BattleScript_UseFluffyTail\n"
"	.4byte BattleScript_UsePokeFlute\n"
"\n"
"	.global gBattlescriptsForSafariActions\n"
"gBattlescriptsForSafariActions:\n"
"	.4byte BattleScript_WatchesCarefully\n"
"	.4byte BattleScript_ThrowRock\n"
"	.4byte BattleScript_ThrowBait\n"
"	.4byte BattleScript_LeftoverWallyPrepToThrow\n"
"\n"
"	.global BattleScript_ThrowBall\n"
"BattleScript_ThrowBall:\n"
"	jumpifbattletype BATTLE_TYPE_OLD_MAN_TUTORIAL, BattleScript_OldManThrowBall\n"
"	jumpifbattletype BATTLE_TYPE_POKEDUDE, BattleScript_PokedudeThrowBall\n"
"	printstring STRINGID_PLAYERUSEDITEM\n"
"	handleballthrow\n"
"\n"
"	.global BattleScript_OldManThrowBall\n"
"BattleScript_OldManThrowBall:\n"
"	printstring STRINGID_OLDMANUSEDITEM\n"
"	handleballthrow\n"
"\n"
"	.global BattleScript_PokedudeThrowBall\n"
"BattleScript_PokedudeThrowBall:\n"
"	printstring STRINGID_POKEDUDEUSED\n"
"	handleballthrow\n"
"\n"
"	.global BattleScript_ThrowSafariBall\n"
"BattleScript_ThrowSafariBall:\n"
"	printstring STRINGID_PLAYERUSEDITEM\n"
"	updatestatusicon BS_ATTACKER\n"
"	handleballthrow\n"
"\n"
"	.global BattleScript_SuccessBallThrow\n"
"BattleScript_SuccessBallThrow:\n"
"	jumpifhalfword CMP_EQUAL, gLastUsedItem, ITEM_SAFARI_BALL, BattleScript_SafariNoIncGameStat\n"
"	incrementgamestat GAME_STAT_POKEMON_CAPTURES\n"
"	.global BattleScript_SafariNoIncGameStat\n"
"BattleScript_SafariNoIncGameStat:\n"
"	printstring STRINGID_GOTCHAPKMNCAUGHT\n"
"	trysetcaughtmondexflags BattleScript_CaughtPokemonSkipNewDex\n"
"	printstring STRINGID_PKMNDATAADDEDTODEX\n"
"	waitstate\n"
"	setbyte gBattleCommunication, 0\n"
"	displaydexinfo\n"
"	.global BattleScript_CaughtPokemonSkipNewDex\n"
"BattleScript_CaughtPokemonSkipNewDex:\n"
"	printstring STRINGID_GIVENICKNAMECAPTURED\n"
"	waitstate\n"
"	setbyte gBattleCommunication, 0\n"
"	trygivecaughtmonnick BattleScript_CaughtPokemonSkipNickname\n"
"	givecaughtmon\n"
"	printfromtable gCaughtMonStringIds\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	goto BattleScript_CaughtPokemonDone\n"
"\n"
"	.global BattleScript_CaughtPokemonSkipNickname\n"
"BattleScript_CaughtPokemonSkipNickname:\n"
"	givecaughtmon\n"
"	.global BattleScript_CaughtPokemonDone\n"
"BattleScript_CaughtPokemonDone:\n"
"	setbyte gBattleOutcome, B_OUTCOME_CAUGHT\n"
"	finishturn\n"
"\n"
"	.global BattleScript_OldMan_Pokedude_CaughtMessage\n"
"BattleScript_OldMan_Pokedude_CaughtMessage:\n"
"	printstring STRINGID_GOTCHAPKMNCAUGHT2\n"
"	setbyte gBattleOutcome, B_OUTCOME_CAUGHT\n"
"	endlinkbattle\n"
"	finishturn\n"
"\n"
"	.global BattleScript_ShakeBallThrow\n"
"BattleScript_ShakeBallThrow:\n"
"	printfromtable gBallEscapeStringIds\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	jumpifnotbattletype BATTLE_TYPE_SAFARI, BattleScript_CatchFailEnd\n"
"	jumpifbyte CMP_NOT_EQUAL, gNumSafariBalls, 0, BattleScript_CatchFailEnd\n"
"	printstring STRINGID_OUTOFSAFARIBALLS\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	setbyte gBattleOutcome, B_OUTCOME_NO_SAFARI_BALLS\n"
"	.global BattleScript_CatchFailEnd\n"
"BattleScript_CatchFailEnd:\n"
"	finishaction\n"
"\n"
"	.global BattleScript_TrainerBallBlock\n"
"BattleScript_TrainerBallBlock:\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	printstring STRINGID_TRAINERBLOCKEDBALL\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	printstring STRINGID_DONTBEATHIEF\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	finishaction\n"
"\n"
"	.global BattleScript_GhostBallDodge\n"
"BattleScript_GhostBallDodge:\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	printstring STRINGID_ITDODGEDBALL\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	finishaction\n"
"\n"
"	.global BattleScript_PlayerUseItem\n"
"BattleScript_PlayerUseItem:\n"
"	moveendcase 15\n"
"	end\n"
"\n"
"	.global BattleScript_AIUseFullRestoreOrHpHeal\n"
"BattleScript_AIUseFullRestoreOrHpHeal:\n"
"	printstring STRINGID_EMPTYSTRING3\n"
"	pause B_WAIT_TIME_MED\n"
"	playse SE_USE_ITEM\n"
"	printstring STRINGID_TRAINER1USEDITEM\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	useitemonopponent\n"
"	orword gHitMarker, HITMARKER_IGNORE_SUBSTITUTE\n"
"	healthbarupdate BS_ATTACKER\n"
"	datahpupdate BS_ATTACKER\n"
"	printstring STRINGID_PKMNSITEMRESTOREDHEALTH\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	updatestatusicon BS_ATTACKER\n"
"	moveendcase 15\n"
"	finishaction\n"
"\n"
"	.global BattleScript_AIUseStatRestore\n"
"BattleScript_AIUseStatRestore:\n"
"	printstring STRINGID_EMPTYSTRING3\n"
"	pause B_WAIT_TIME_MED\n"
"	playse SE_USE_ITEM\n"
"	printstring STRINGID_TRAINER1USEDITEM\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	useitemonopponent\n"
"	printfromtable gTrainerItemCuredStatusStringIds\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	updatestatusicon BS_ATTACKER\n"
"	moveendcase 15\n"
"	finishaction\n"
"\n"
"	.global BattleScript_AIUseXstat\n"
"BattleScript_AIUseXstat:\n"
"	printstring STRINGID_EMPTYSTRING3\n"
"	pause B_WAIT_TIME_MED\n"
"	playse SE_USE_ITEM\n"
"	printstring STRINGID_TRAINER1USEDITEM\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	useitemonopponent\n"
"	printfromtable gStatUpStringIds\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	moveendcase 15\n"
"	finishaction\n"
"\n"
"	.global BattleScript_AIUseGuardSpec\n"
"BattleScript_AIUseGuardSpec:\n"
"	printstring STRINGID_EMPTYSTRING3\n"
"	pause B_WAIT_TIME_MED\n"
"	playse SE_USE_ITEM\n"
"	printstring STRINGID_TRAINER1USEDITEM\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	useitemonopponent\n"
"	printfromtable gMistUsedStringIds\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	moveendcase 15\n"
"	finishaction\n"
"\n"
"	.global BattleScript_UseFluffyTail\n"
"BattleScript_UseFluffyTail:\n"
"	playse SE_FLEE\n"
"	setbyte gBattleOutcome, B_OUTCOME_RAN\n"
"	finishturn\n"
"\n"
"	.global BattleScript_UsePokeFlute\n"
"BattleScript_UsePokeFlute:\n"
"	checkpokeflute BS_ATTACKER\n"
"	jumpifbyte CMP_EQUAL, cMULTISTRING_CHOOSER, 1, BattleScript_PokeFluteWakeUp\n"
"	printstring STRINGID_POKEFLUTECATCHY\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	goto BattleScript_PokeFluteEnd\n"
"\n"
"	.global BattleScript_PokeFluteWakeUp\n"
"BattleScript_PokeFluteWakeUp:\n"
"	printstring STRINGID_POKEFLUTE\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	fanfare MUS_POKE_FLUTE\n"
"	waitfanfare BS_ATTACKER\n"
"	printstring STRINGID_MONHEARINGFLUTEAWOKE\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	updatestatusicon BS_PLAYER2\n"
"	waitstate\n"
"	.global BattleScript_PokeFluteEnd\n"
"BattleScript_PokeFluteEnd:\n"
"	finishaction\n"
"\n"
"	.global BattleScript_WatchesCarefully\n"
"BattleScript_WatchesCarefully:\n"
"	printfromtable gSafariReactionStringIds\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	playanimation BS_OPPONENT1, B_ANIM_SAFARI_REACTION\n"
"	end2\n"
"\n"
"	.global BattleScript_ThrowRock\n"
"BattleScript_ThrowRock:\n"
"	printstring STRINGID_THREWROCK\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	playanimation BS_ATTACKER, B_ANIM_ROCK_THROW\n"
"	end2\n"
"\n"
"	.global BattleScript_ThrowBait\n"
"BattleScript_ThrowBait:\n"
"	printstring STRINGID_THREWBAIT\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	playanimation BS_ATTACKER, B_ANIM_BAIT_THROW\n"
"	end2\n"
"\n"
"	.global BattleScript_LeftoverWallyPrepToThrow\n"
"BattleScript_LeftoverWallyPrepToThrow:\n"
"	printstring STRINGID_RETURNMON\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	returnatktoball\n"
"	waitstate\n"
"	trainerslidein BS_TARGET\n"
"	waitstate\n"
"	printstring STRINGID_YOUTHROWABALLNOWRIGHT\n"
"	waitmessage B_WAIT_TIME_LONG\n"
"	end2\n"
);
