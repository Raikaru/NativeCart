#include "../../../include/global.h"
#include "../../../include/script.h"
#include "../../../include/mystery_event_script.h"

extern bool8 MEScrCmd_nop(struct ScriptContext *ctx);
extern bool8 MEScrCmd_checkcompat(struct ScriptContext *ctx);
extern bool8 MEScrCmd_end(struct ScriptContext *ctx);
extern bool8 MEScrCmd_setmsg(struct ScriptContext *ctx);
extern bool8 MEScrCmd_setstatus(struct ScriptContext *ctx);
extern bool8 MEScrCmd_runscript(struct ScriptContext *ctx);
extern bool8 MEScrCmd_initramscript(struct ScriptContext *ctx);
extern bool8 MEScrCmd_setenigmaberry(struct ScriptContext *ctx);
extern bool8 MEScrCmd_giveribbon(struct ScriptContext *ctx);
extern bool8 MEScrCmd_givenationaldex(struct ScriptContext *ctx);
extern bool8 MEScrCmd_addrareword(struct ScriptContext *ctx);
extern bool8 MEScrCmd_setrecordmixinggift(struct ScriptContext *ctx);
extern bool8 MEScrCmd_givepokemon(struct ScriptContext *ctx);
extern bool8 MEScrCmd_addtrainer(struct ScriptContext *ctx);
extern bool8 MEScrCmd_enableresetrtc(struct ScriptContext *ctx);
extern bool8 MEScrCmd_checksum(struct ScriptContext *ctx);
extern bool8 MEScrCmd_crc(struct ScriptContext *ctx);

ScrCmdFunc gMysteryEventScriptCmdTable[] =
{
    MEScrCmd_nop,
    MEScrCmd_checkcompat,
    MEScrCmd_end,
    MEScrCmd_setmsg,
    MEScrCmd_setstatus,
    MEScrCmd_runscript,
    MEScrCmd_initramscript,
    MEScrCmd_setenigmaberry,
    MEScrCmd_giveribbon,
    MEScrCmd_givenationaldex,
    MEScrCmd_addrareword,
    MEScrCmd_setrecordmixinggift,
    MEScrCmd_givepokemon,
    MEScrCmd_addtrainer,
    MEScrCmd_enableresetrtc,
    MEScrCmd_checksum,
    MEScrCmd_crc,
};

ScrCmdFunc gMysteryEventScriptCmdTableEnd[1] =
{
    NULL,
};
