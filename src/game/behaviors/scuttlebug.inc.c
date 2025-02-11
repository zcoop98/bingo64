// scuttlebug.c.inc

struct ObjectHitbox sScuttlebugHitbox = {
    /* interactType: */ INTERACT_BOUNCE_TOP,
    /* downOffset: */ 0,
    /* damageOrCoinValue: */ 1,
    /* health: */ 1,
    /* numLootCoins: */ 3,
    /* radius: */ 130,
    /* height: */ 70,
    /* hurtboxRadius: */ 90,
    /* hurtboxHeight: */ 60,
};

s32 func_802C5A64(s32 *a0) {
    if (o->oMoveFlags & 0x200) {
        *a0 = o->oWallAngle;
        return 1;
    } else if (o->oMoveFlags & 0x400) {
        *a0 = o->oMoveAngleYaw + 0x8000;
        return -1;
    }
    return 0;
}

void bhv_scuttlebug_loop(void) {
    UNUSED s32 unused;
    f32 sp18;
    obj_update_floor_and_walls();
    if (o->oSubAction != 0 && o->oSubAction != 1
        && obj_set_hitbox_and_die_if_attacked_bingo(&sScuttlebugHitbox, SOUND_OBJ_DYING_ENEMY1,
                                              o->oScuttlebugUnkF4, BINGO_UPDATE_KILLED_SCUTTLEBUG))
        o->oSubAction = 4;
    if (o->oSubAction != 2)
        o->oScuttlebugUnkF8 = 0;
    switch (o->oSubAction) {
        case 0:
            o->oBingoId = get_unique_id(BINGO_UPDATE_KILLED_SCUTTLEBUG, o->oPosX, o->oPosY, o->oPosZ);
            o->oSubAction = 1;
            // Fall immediately into original step 0 (no 'break')
        case 1:
            if (o->oMoveFlags & 1)
                PlaySound2(SOUND_OBJ_GOOMBA_ALERT);
            if (o->oMoveFlags & 3) {
                o->oHomeX = o->oPosX;
                o->oHomeY = o->oPosY;
                o->oHomeZ = o->oPosZ;
                o->oSubAction++;
            }
            break;
        case 2:
            o->oForwardVel = 5.0f;
            if (obj_lateral_dist_from_mario_to_home() > 1000.0f)
                o->oAngleToMario = obj_angle_to_home();
            else {
                if (o->oScuttlebugUnkF8 == 0) {
                    o->oScuttlebugUnkFC = 0;
                    o->oAngleToMario = angle_to_object(o, gMarioObject);
                    if (abs_angle_diff(o->oAngleToMario, o->oMoveAngleYaw) < 0x800) {
                        o->oScuttlebugUnkF8 = 1;
                        o->oVelY = 20.0f;
                        PlaySound2(SOUND_OBJ2_SCUTTLEBUG_ALERT);
                    }
                } else if (o->oScuttlebugUnkF8 == 1) {
                    o->oForwardVel = 15.0f;
                    o->oScuttlebugUnkFC++;
                    if (o->oScuttlebugUnkFC > 50)
                        o->oScuttlebugUnkF8 = 0;
                }
            }
            if (func_802C5A64(&o->oAngleToMario))
                o->oSubAction = 3;
            obj_rotate_yaw_toward(o->oAngleToMario, 0x200);
            break;
        case 3:
            o->oForwardVel = 5.0f;
            if ((s16) o->oMoveAngleYaw == (s16) o->oAngleToMario)
                o->oSubAction = 2;
            if (o->oPosY - o->oHomeY < -200.0f)
                mark_object_for_deletion(o);
            obj_rotate_yaw_toward(o->oAngleToMario, 0x400);
            break;
        case 4:
            o->oFlags &= ~8;
            o->oForwardVel = -10.0f;
            o->oVelY = 30.0f;
            PlaySound2(SOUND_OBJ2_SCUTTLEBUG_ALERT);
            o->oSubAction++;
            break;
        case 5:
            o->oForwardVel = -10.0f;
            if (o->oMoveFlags & 1) {
                o->oSubAction++;
                o->oVelY = 0.0f;
                o->oScuttlebugUnkFC = 0;
                o->oFlags |= 8;
                o->oInteractStatus = 0;
            }
            break;
        case 6:
            o->oForwardVel = 2.0f;
            o->oScuttlebugUnkFC++;
            if (o->oScuttlebugUnkFC > 30)
                o->oSubAction = 1;
            break;
    }
    if (o->oForwardVel < 10.0f)
        sp18 = 1.0f;
    else
        sp18 = 3.0f;
    func_8029ED98(0, sp18);
    if (o->oMoveFlags & 3)
        func_802BE2E8(1, 23, SOUND_OBJ2_SCUTTLEBUG_WALK);
    if (o->parentObj != o) {
        if (obj_is_hidden(o))
            mark_object_for_deletion(o);
        if (o->activeFlags == 0)
            o->parentObj->oScuttlebugSpawnerUnk88 = 1;
    }
    obj_move_standard(-50);
}

void bhv_scuttlebug_spawn_loop(void) {
    struct Object *scuttlebug;
    if (o->oAction == 0) {
        if (o->oTimer > 30 && 500.0f < o->oDistanceToMario && o->oDistanceToMario < 1500.0f) {
            PlaySound2(SOUND_OBJ2_SCUTTLEBUG_ALERT);
            scuttlebug = spawn_object(o, MODEL_SCUTTLEBUG, bhvScuttlebug);
            scuttlebug->oScuttlebugUnkF4 = o->oScuttlebugSpawnerUnkF4;
            scuttlebug->oForwardVel = 30.0f;
            scuttlebug->oVelY = 80.0f;
            o->oAction++;
            o->oScuttlebugUnkF4 = 1;
        }
    } else if (o->oScuttlebugSpawnerUnk88 != 0) {
        o->oScuttlebugSpawnerUnk88 = 0;
        o->oAction = 0;
    }
}
