#include <ultra64.h>
#include <stdio.h>

#include "sm64.h"
#include "display.h"
#include "game.h"
#include "level_update.h"
#include "camera.h"
#include "print.h"
#include "ingame_menu.h"
#include "hud.h"
#include "segment2.h"
#include "area.h"
#include "bingo_ui.h"
#include "bingo.h"
#include "save_file.h"
#include "print.h"
#include "bingo_ui.h"
#include "strcpy.h"

/* @file hud.c
 * This file implements HUD rendering and power meter animations.
 * That includes stars, lives, coins, camera status, power meter, timer
 * cannon reticle, and the unused keys.
 **/

struct PowerMeterHUD {
    s8 animation;
    s16 x;
    s16 y;
    f32 unused;
};

struct UnusedStruct803314F0 {
    u32 unused1;
    u16 unused2;
    u16 unused3;
};

struct CameraHUD {
    s16 status;
};

// Stores health segmented value defined by numHealthWedges
// When the HUD is rendered this value is 8, full health.
static s16 sPowerMeterStoredHealth;

static struct PowerMeterHUD sPowerMeterHUD = {
    POWER_METER_HIDDEN,
    140,
    166,
    1.0,
};

// Power Meter timer that keeps counting when it's visible.
// Gets reset when the health is filled and stops counting
// when the power meter is hidden.
s32 sPowerMeterVisibleTimer = 0;

static struct UnusedStruct803314F0 unused803314F0 = { 0x00000000, 0x000A, 0x0000 };

static struct CameraHUD sCameraHUD = { CAM_STATUS_NONE };

/**
 * Renders a rgba16 16x16 glyph texture from a table list.
 */
void render_hud_tex_lut(s32 x, s32 y, u8 *texture) {
    gDPPipeSync(gDisplayListHead++);
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
    gDPSetTextureImage(gDisplayListHead++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture);
    gSPDisplayList(gDisplayListHead++, &dl_hud_img_load_tex_block);
    gSPTextureRectangle(gDisplayListHead++, x << 2, y << 2, (x + 16) << 2, (y + 16) << 2,
                        G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
}

/**
 * Renders a rgba16 8x8 glyph texture from a table list.
 */
void render_hud_small_tex_lut(s32 x, s32 y, u8 *texture) {
    gDPSetTile(gDisplayListHead++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0,
                G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD);
    gDPTileSync(gDisplayListHead++);
    gDPSetTile(gDisplayListHead++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 2, 0, G_TX_RENDERTILE, 0,
                G_TX_CLAMP, 3, G_TX_NOLOD, G_TX_CLAMP, 3, G_TX_NOLOD);
    gDPSetTileSize(gDisplayListHead++, G_TX_RENDERTILE, 0, 0, (8 - 1) << G_TEXTURE_IMAGE_FRAC, (8 - 1) << G_TEXTURE_IMAGE_FRAC);
    gDPPipeSync(gDisplayListHead++);
    gDPSetTextureImage(gDisplayListHead++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, texture);
    gDPLoadSync(gDisplayListHead++);
    gDPLoadBlock(gDisplayListHead++, G_TX_LOADTILE, 0, 0, 8 * 8 - 1, CALC_DXT(8, G_IM_SIZ_16b_BYTES));
    gSPTextureRectangle(gDisplayListHead++, x << 2, y << 2, (x + 8) << 2, (y + 8) << 2, G_TX_RENDERTILE,
                        0, 0, 1 << 10, 1 << 10);
}

/**
 * Renders power meter health segment texture using a table list.
 */
void render_power_meter_health_segment(s16 numHealthWedges) {
    u8 *(*healthLUT)[];

    healthLUT = segmented_to_virtual(&power_meter_health_segments_lut);

    gDPPipeSync(gDisplayListHead++);
    gDPSetTextureImage(gDisplayListHead++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1,
                       (*healthLUT)[numHealthWedges - 1]);
    gDPLoadSync(gDisplayListHead++);
    gDPLoadBlock(gDisplayListHead++, G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES));
    gSP1Triangle(gDisplayListHead++, 0, 1, 2, 0);
    gSP1Triangle(gDisplayListHead++, 0, 2, 3, 0);
}

/**
 * Renders power meter display lists.
 * That includes the "POWER" base and the colored health segment textures.
 */
void render_dl_power_meter(s16 numHealthWedges) {
    Mtx *mtx;

    mtx = alloc_display_list(sizeof(Mtx));

    if (mtx == NULL) {
        return;
    }

    guTranslate(mtx, (f32) sPowerMeterHUD.x, (f32) sPowerMeterHUD.y, 0);

    gSPMatrix(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(mtx++),
              G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPDisplayList(gDisplayListHead++, &dl_power_meter_base);

    if (numHealthWedges != 0) {
        gSPDisplayList(gDisplayListHead++, &dl_power_meter_health_segments_begin);
        render_power_meter_health_segment(numHealthWedges);
        gSPDisplayList(gDisplayListHead++, &dl_power_meter_health_segments_end);
    }

    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}

/**
 * Power meter animation called when there's less than 8 health segments
 * Checks its timer to later change into deemphasizing mode.
 */
void animate_power_meter_emphasized(void) {
    s16 hudDisplayFlags;
    hudDisplayFlags = gHudDisplay.flags;

    if (!(hudDisplayFlags & HUD_DISPLAY_FLAG_EMPHASIZE_POWER)) {
        if (sPowerMeterVisibleTimer == 45.0) {
            sPowerMeterHUD.animation = POWER_METER_DEEMPHASIZING;
        }
    } else {
        sPowerMeterVisibleTimer = 0;
    }
}

/**
 * Power meter animation called after emphasized mode.
 * Moves power meter y pos speed until it's at 200 to be visible.
 */
static void animate_power_meter_deemphasizing(void) {
    s16 speed = 5;

    if (sPowerMeterHUD.y >= 181) {
        speed = 3;
    }

    if (sPowerMeterHUD.y >= 191) {
        speed = 2;
    }

    if (sPowerMeterHUD.y >= 196) {
        speed = 1;
    }

    sPowerMeterHUD.y += speed;

    if (sPowerMeterHUD.y >= 201) {
        sPowerMeterHUD.y = 200;
        sPowerMeterHUD.animation = POWER_METER_VISIBLE;
    }
}

/**
 * Power meter animation called when there's 8 health segments.
 * Moves power meter y pos quickly until it's at 301 to be hidden.
 */
static void animate_power_meter_hiding(void) {
    sPowerMeterHUD.y += 20;
    if (sPowerMeterHUD.y >= 301) {
        sPowerMeterHUD.animation = POWER_METER_HIDDEN;
        sPowerMeterVisibleTimer = 0;
    }
}

/**
 * Handles power meter actions depending of the health segments values.
 */
void handle_power_meter_actions(s16 numHealthWedges) {
    // Show power meter if health is not full, less than 8
    if (numHealthWedges < 8 && sPowerMeterStoredHealth == 8 && sPowerMeterHUD.animation == POWER_METER_HIDDEN) {
        sPowerMeterHUD.animation = POWER_METER_EMPHASIZED;
        sPowerMeterHUD.y = 166;
    }

    // Show power meter if health is full, has 8
    if (numHealthWedges == 8 && sPowerMeterStoredHealth == 7) {
        sPowerMeterVisibleTimer = 0;
    }

    // After health is full, hide power meter
    if (numHealthWedges == 8 && sPowerMeterVisibleTimer > 45.0) {
        sPowerMeterHUD.animation = POWER_METER_HIDING;
    }

    // Update to match health value
    sPowerMeterStoredHealth = numHealthWedges;

    // If mario is swimming, keep showing power meter
    if (gPlayerCameraState->action & ACT_FLAG_SWIMMING) {
        if (sPowerMeterHUD.animation == POWER_METER_HIDDEN
            || sPowerMeterHUD.animation == POWER_METER_EMPHASIZED) {
            sPowerMeterHUD.animation = POWER_METER_DEEMPHASIZING;
            sPowerMeterHUD.y = 166;
        }
        sPowerMeterVisibleTimer = 0;
    }
}

/**
 * Renders the power meter that shows when Mario is in underwater
 * or has taken damage and has less than 8 health segments.
 * And calls a power meter animation function depending of the value defined.
 */
void render_hud_power_meter(void) {
    s16 shownHealthWedges = gHudDisplay.wedges;

    if (sPowerMeterHUD.animation != POWER_METER_HIDING) {
        handle_power_meter_actions(shownHealthWedges);
    }

    if (sPowerMeterHUD.animation == POWER_METER_HIDDEN) {
        return;
    }

    switch (sPowerMeterHUD.animation) {
        case POWER_METER_EMPHASIZED:
            animate_power_meter_emphasized();
            break;
        case POWER_METER_DEEMPHASIZING:
            animate_power_meter_deemphasizing();
            break;
        case POWER_METER_HIDING:
            animate_power_meter_hiding();
            break;
        default:
            break;
    }

    render_dl_power_meter(shownHealthWedges);

    sPowerMeterVisibleTimer += 1;
}

/**
 * Renders the amount of lives Mario has.
 */
void render_hud_mario_lives(void) {
    print_text(22, HUD_TOP_Y, ","); // 'Mario Head' glyph
    print_text(38, HUD_TOP_Y, "*"); // 'X' glyph
    print_text_fmt_int(54, HUD_TOP_Y, "%d", gHudDisplay.lives);
}

void render_hud_click_game(void) {
    gSPDisplayList(gDisplayListHead++, dl_hud_img_begin);
    print_bingo_icon(20, 20, BINGO_ICON_STAR_CLICK_GAME);
    gSPDisplayList(gDisplayListHead++, dl_hud_img_end);
    print_text(38, 20, "*"); // 'X' glyph
    print_text_fmt_int(54, 20, "%d", gBingoClickCounter);
}

void render_hud_reverse_joystick(void) {
    gSPDisplayList(gDisplayListHead++, dl_hud_img_begin);
    print_bingo_icon(20, 20, BINGO_ICON_STAR_REVERSE_JOYSTICK);
    gSPDisplayList(gDisplayListHead++, dl_hud_img_end);
    print_text(38, 20, "REV");
}

/**
 * Renders the amount of coins collected.
 */
void render_hud_coins(void) {
    print_text(168, HUD_TOP_Y, "+"); // 'Coin' glyph
    print_text(184, HUD_TOP_Y, "*"); // 'X' glyph
    print_text_fmt_int(198, HUD_TOP_Y, "%d", gHudDisplay.coins);
}

#ifdef VERSION_JP
#define HUD_STARS_X 247
#else
#define HUD_STARS_X 242
#endif

/**
 * Renders the amount of stars collected.
 * Disables "X" glyph when Mario has 100 stars or more.
 */
void render_hud_stars(void) {
    s8 showX = 0;

    if (gHudFlash == 1 && gGlobalTimer & 0x08) {
        return;
    }

    if (gHudDisplay.stars < 100) {
        showX = 1;
    }

    print_text(HUD_STARS_X, HUD_TOP_Y, "-"); // 'Star' glyph
    if (showX == 1) {
        print_text((HUD_STARS_X + 16), HUD_TOP_Y, "*"); // 'X' glyph
    }
    print_text_fmt_int(((showX * 14) + (HUD_STARS_X + 16)), HUD_TOP_Y, "%d", gHudDisplay.stars);
}

/**
 * Unused function that renders the amount of keys collected.
 * Leftover function from the beta version of the game.
 */
void render_hud_keys(void) {
    s16 i;

    for (i = 0; i < gHudDisplay.keys; i++) {
        print_text((i * 16) + 220, 142, "/"); // unused glyph - beta key
    }
}

/**
 * Renders the timer when Mario start sliding in PSS.
 */
void render_hud_timer(void) {
    u8 *(*hudLUT)[58];
    u16 timerValFrames;
    u16 timerMins;
    u16 timerSecs;
    u16 timerFracSecs;

    hudLUT = segmented_to_virtual(&main_hud_lut);
    timerValFrames = gHudDisplay.timer;
#ifdef VERSION_EU
    switch (eu_get_language()) {
        case LANGUAGE_ENGLISH:
            print_text(170, 185, "TIME");
            break;
        case LANGUAGE_FRENCH:
            print_text(165, 185, "TEMPS");
            break;
        case LANGUAGE_GERMAN:
            print_text(170, 185, "ZEIT");
            break;
    }
#endif
    timerMins = timerValFrames / (30 * 60);
    timerSecs = (timerValFrames - (timerMins * 1800)) / 30;

    timerFracSecs = ((timerValFrames - (timerMins * 1800) - (timerSecs * 30)) & 0xFFFF) / 3;
#ifndef VERSION_EU
    print_text(170, 185, "TIME");
#endif
    print_text_fmt_int(229, 185, "%0d", timerMins);
    print_text_fmt_int(249, 185, "%02d", timerSecs);
    print_text_fmt_int(283, 185, "%d", timerFracSecs);
    gSPDisplayList(gDisplayListHead++, dl_hud_img_begin);
    render_hud_tex_lut(239, 32, (*hudLUT)[GLYPH_APOSTROPHE]);
    render_hud_tex_lut(274, 32, (*hudLUT)[GLYPH_DOUBLE_QUOTE]);
    gSPDisplayList(gDisplayListHead++, dl_hud_img_end);
}

/**
 * Sets HUD status camera value depending of the actions
 * defined in update_camera_status.
 */
void set_hud_camera_status(s16 status) {
    sCameraHUD.status = status;
}

/**
 * Renders camera HUD glyphs using a table list, depending of
 * the camera status called, a defined glyph is rendered.
 */
void render_hud_camera_status(void) {
    u8 *(*cameraLUT)[6];
    s32 x;
    s32 y;

    cameraLUT = segmented_to_virtual(&main_hud_camera_lut);
    x = 266;
    y = 205;

    if (sCameraHUD.status == CAM_STATUS_NONE) {
        return;
    }

    gSPDisplayList(gDisplayListHead++, dl_hud_img_begin);
    render_hud_tex_lut(x, y, (*cameraLUT)[GLYPH_CAM_CAMERA]);

    switch (sCameraHUD.status & CAM_STATUS_MODE_GROUP) {
        case CAM_STATUS_MARIO:
            render_hud_tex_lut(x + 16, y, (*cameraLUT)[GLYPH_CAM_MARIO_HEAD]);
            break;
        case CAM_STATUS_LAKITU:
            render_hud_tex_lut(x + 16, y, (*cameraLUT)[GLYPH_CAM_LAKITU_HEAD]);
            break;
        case CAM_STATUS_FIXED:
            render_hud_tex_lut(x + 16, y, (*cameraLUT)[GLYPH_CAM_FIXED]);
            break;
    }

    switch (sCameraHUD.status & CAM_STATUS_C_MODE_GROUP) {
        case CAM_STATUS_C_DOWN:
            render_hud_small_tex_lut(x + 4, y + 16, (*cameraLUT)[GLYPH_CAM_ARROW_DOWN]);
            break;
        case CAM_STATUS_C_UP:
            render_hud_small_tex_lut(x + 4, y - 8, (*cameraLUT)[GLYPH_CAM_ARROW_UP]);
            break;
    }

    gSPDisplayList(gDisplayListHead++, dl_hud_img_end);
}

s32 sLowestFreeSlotIndex = 0;

struct Slot {
    enum BingoObjectiveIcon icon;
    s8 printTimes;
    char message[10];
    enum BingoObjectiveIcon iconMessage;
    s32 fadeTimer;
    s8 horribleHackForWallkickDedupe;
};

#define MAX_SLOTS 5
#define MAX_FADE_TIMER (30 * 6)
struct Slot sSlots[MAX_SLOTS] = { 0 };

void delete_slot(s32 delete) {
    s32 i;
    for (i = 0; i < sLowestFreeSlotIndex; i++) {
        if (i < delete) {
            continue;
        } else {
            if (i < (sLowestFreeSlotIndex - 1)) {
                sSlots[i].icon = sSlots[i + 1].icon;
                sSlots[i].printTimes = sSlots[i + 1].printTimes;
                strcpy(sSlots[i].message, sSlots[i + 1].message);
                sSlots[i].iconMessage = sSlots[i + 1].iconMessage;
                sSlots[i].fadeTimer = sSlots[i + 1].fadeTimer;
                sSlots[i].horribleHackForWallkickDedupe = sSlots[i + 1].horribleHackForWallkickDedupe;
            } else {
                sSlots[i].icon = 0;
                sSlots[i].printTimes = 0;
                strcpy(sSlots[i].message, "");
                sSlots[i].iconMessage = 0;
                sSlots[i].fadeTimer = 0;
                sSlots[i].horribleHackForWallkickDedupe = 0;
            }
        }
    }
    sLowestFreeSlotIndex--;
}

void bingo_hud_update_message(enum BingoObjectiveIcon icon, char message[10], s8 horribleHackWallkick) {
    s32 i;
    struct Slot *slot;

    // Look through existing slots for matching icons
    // If any exist, delete them and shift everything down,
    // then add a new one.
    // Otherwise, check for extra room, and add it if there is.

    if (!horribleHackWallkick) {
        for (i = 0; i < sLowestFreeSlotIndex; i++) {
            slot = &sSlots[i];
            // TODO: Make deduplication not depend on the "printTimes" hack
            if (slot->icon == icon && slot->printTimes == 1 && slot->horribleHackForWallkickDedupe == 0) {
                delete_slot(i);
                break;
            }
        }
    }

    if (sLowestFreeSlotIndex >= MAX_SLOTS) {
        return;  // refuse to print too much
        // (Note, this won't happen if we just deleted a slot.)
    }

    slot = &sSlots[sLowestFreeSlotIndex];
    slot->icon = icon;
    slot->printTimes = 1;
    slot->horribleHackForWallkickDedupe = horribleHackWallkick;
    strcpy(slot->message, message);
    slot->fadeTimer = 0;

    sLowestFreeSlotIndex++;
}

void bingo_hud_update_number(enum BingoObjectiveIcon icon, s32 number) {
    char message[10];
    sprintf(message, "%d", number);
    bingo_hud_update_message(icon, message, 0);
}

void bingo_hud_update_state(enum BingoObjectiveIcon icon, enum BingoObjectiveIcon stateIcon) {
    struct Slot *slot;

    if (sLowestFreeSlotIndex >= MAX_SLOTS) {
        return;
    }
    slot = &sSlots[sLowestFreeSlotIndex];
    slot->icon = stateIcon;
    slot->printTimes = 0;
    slot->iconMessage = icon;
    slot->fadeTimer = 0;

    sLowestFreeSlotIndex++;

}

u8 frame_to_opacity(s32 fadeTimer) {
    if (fadeTimer < (30 * 4)) {
        return 255;
    } else {
        return 255 - ((s32) (255 / (30 * 2)) * (fadeTimer - (30 * 4)));
    }
}

void bingo_hud_render(void) {
    s32 i;
    s32 slotsToRemove = 0;
    struct Slot *slot;
    u8 opacity;

    for (i = 0; i < sLowestFreeSlotIndex; i++) {
        slot = &sSlots[i];
        opacity = frame_to_opacity(slot->fadeTimer);
        gSPDisplayList(gDisplayListHead++, dl_hud_img_begin);
        print_bingo_icon_alpha(20, 40 + 20 * i, slot->icon, opacity);
        gSPDisplayList(gDisplayListHead++, dl_hud_img_end);
        if (slot->printTimes) {
            print_text_alpha(38, 40 + 20 * i, "*", opacity);
            print_text_alpha(53, 40 + 20 * i, slot->message, opacity);
        } else {
            gSPDisplayList(gDisplayListHead++, dl_hud_img_begin);
            print_bingo_icon_alpha(38, 40 + 20 * i, slot->iconMessage, opacity);
            gSPDisplayList(gDisplayListHead++, dl_hud_img_end);
        }

        slot->fadeTimer++;
        if (slot->fadeTimer > MAX_FADE_TIMER) {
            slotsToRemove++;
        }
    }
    for (i = 0; i < slotsToRemove; i++) {
        delete_slot(0);
    }
}

/**
 * Render HUD strings using hudDisplayFlags with it's render functions,
 * excluding the cannon reticle which detects a camera preset for it.
 */
void render_hud(void) {
    s16 hudDisplayFlags;
#ifdef VERSION_EU
    Mtx *mtx;
#endif

    hudDisplayFlags = gHudDisplay.flags;

    if (hudDisplayFlags == HUD_DISPLAY_NONE) {
        sPowerMeterHUD.animation = POWER_METER_HIDDEN;
        sPowerMeterStoredHealth = 8;
        sPowerMeterVisibleTimer = 0;
        if (gPlayer1Controller->buttonDown & L_TRIG /* need something else here for file select */) {
            create_dl_ortho_matrix();
            draw_bingo_screen();
        }
    } else {
#ifdef VERSION_EU
        // basically create_dl_ortho_matrix but guOrtho screen width is different
        mtx = alloc_display_list(sizeof(*mtx));
        if (mtx == NULL) {
            return;
        }
        create_dl_identity_matrix();
        guOrtho(mtx, -16.0f, 336.0f, 0, 240.0f, -10.0f, 10.0f, 1.0f);
        gMoveWd(gDisplayListHead++, 0xE, 0, 0xFFFF);
        gSPMatrix(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(mtx),
                G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);

#else
        create_dl_ortho_matrix();
#endif

        if (gPlayer1Controller->buttonPressed & L_TRIG
            && gbBingosCompleted >= gbBingoTarget
            && gbBingoShowCongratsCounter < gbBingoShowCongratsLimit
        ) {
            // If you press L twice, the congrats message goes away.
            gbBingoShowCongratsCounter++;
        }
        if (gPlayer1Controller->buttonDown & L_TRIG || gForceDrawBingoScreen == 1) {
            draw_bingo_screen();
        } else {
            if ((gbBingosCompleted >= gbBingoTarget) && gbBingoShowCongratsCounter < gbBingoShowCongratsLimit) {
                draw_bingo_win_screen();
            } else {
                bingo_hud_render();
            }

            if (gbBingoShowTimer) {
                draw_bingo_hud_timer();
            }

            if (gCurrentArea != NULL && gCurrentArea->camera->mode == CAMERA_MODE_INSIDE_CANNON) {
                render_hud_cannon_reticle();
            }

            if (hudDisplayFlags & HUD_DISPLAY_FLAG_LIVES) {
                gOptionSelectIconOpacity = 255;
                render_hud_mario_lives();
            }

            if (gBingoClickGameActive) {
                render_hud_click_game();
            }

            if (gBingoReverseJoystickActive) {
                render_hud_reverse_joystick();
            }

            if (hudDisplayFlags & HUD_DISPLAY_FLAG_COIN_COUNT) {
                render_hud_coins();
            }

            if (hudDisplayFlags & HUD_DISPLAY_FLAG_STAR_COUNT) {
                render_hud_stars();
            }

            if (hudDisplayFlags & HUD_DISPLAY_FLAG_KEYS) {
                render_hud_keys();
            }

            if (hudDisplayFlags & HUD_DISPLAY_FLAG_CAMERA_AND_POWER) {
                render_hud_power_meter();
                render_hud_camera_status();
            }

            if (hudDisplayFlags & HUD_DISPLAY_FLAG_TIMER) {
                render_hud_timer();
            }
        }
    }
}
