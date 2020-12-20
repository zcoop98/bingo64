// falling_rising_platform.c.inc
#include "game/bingo.h"
#include "game/bingo_tracking_collectables.h"

void bhv_squishable_platform_loop(void) {
    o->header.gfx.scale[1] = (sins(o->oPlatformTimer) + 1.0) * 0.3 + 0.4;
    o->oPlatformTimer += 0x80;
}

void bhv_bitfs_sinking_platform_loop(void) {
    o->oPosY -=
        sins(o->oPlatformTimer)
        * 0.58; //! f32 double conversion error accumulates on Wii VC causing the platform to rise up
    o->oPlatformTimer += 0x100;
}

// TODO: Named incorrectly. fix
void bhv_ddd_moving_pole_loop(void) {
    copy_object_pos_and_angle(o, o->parentObj);
    if (o->oTimer == 0) {
        o->oBingoId = get_unique_id(BINGO_UPDATE_GRABBED_POLE, o->oPosX, o->oPosY, o->oPosZ);
    }
}

void bhv_bitfs_sinking_cage_platform_loop(void) {
    if (o->oBehParams2ndByte != 0) {
        if (o->oTimer == 0)
            o->oPosY -= 300.0f;
        o->oPosY += sins(o->oPlatformTimer) * 7.0f;
    } else
        o->oPosY -= sins(o->oPlatformTimer) * 3.0f;
    o->oPlatformTimer += 0x100;
}
