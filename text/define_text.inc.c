// == debug table ==

#ifndef VERSION_EU

// (this wasn't translated for US, and was removed in EU)

static const u8 Debug0[] = {
    _("ＳＴＡＧＥ　ＳＥＬＥＣＴ\n"
      "　つづける？\n"
      "　１　マウンテン\n"
      "　２　ファイアーバブル\n"
      "　３　スノースライダー\n"
      "　４　ウォーターランド\n"
      "　　　クッパ１ごう\n"
      "　もどる")
};

static const u8 Debug1[] = {
    _("ＰＡＵＳＥ　　　　\n"
      "　つづける？\n"
      "　やめる　？")
};

static const struct DialogEntry debug_text_entry_0 = {
    1, 8, 30, 200, Debug0
};

static const struct DialogEntry debug_text_entry_1 = {
    1, 3, 100, 150, Debug1
};

const struct DialogEntry *const seg2_debug_text_table[] = {
    &debug_text_entry_0, &debug_text_entry_1, NULL,
};

#endif


// == dialog ==
// (defines en_dialog_table etc.)

#define DEFINE_DIALOG(id, _1, _2, _3, _4, str) \
    static const u8 dialog_text_ ## id[] = { str };

#include "dialogs.h"

#undef DEFINE_DIALOG
#define DEFINE_DIALOG(id, unused, linesPerBox, leftOffset, width, _) \
    static const struct DialogEntry dialog_entry_ ## id = { \
        unused, linesPerBox, leftOffset, width, dialog_text_ ## id \
    };

#include "dialogs.h"

#undef DEFINE_DIALOG
#define DEFINE_DIALOG(id, _1, _2, _3, _4, _5) &dialog_entry_ ## id,

const struct DialogEntry *const seg2_dialog_table[] = {
#include "dialogs.h"
    NULL
};


// == courses ==
// (defines en_course_name_table etc.)

#define COURSE_ACTS(id, name, a,b,c,d,e,f) \
    static const u8 course_name_ ## id[] = { name };

#define SECRET_STAR(id, name) \
    static const u8 course_name_ ## id[] = { name };

#define CASTLE_SECRET_STARS(str) \
    static const u8 course_name_castle_secret_stars[] = { str };

#define EXTRA_TEXT(id, str)

#include "courses.h"

#undef COURSE_ACTS
#undef SECRET_STAR
#undef CASTLE_SECRET_STARS

#define COURSE_ACTS(id, name, a,b,c,d,e,f) course_name_ ## id,
#define SECRET_STAR(id, name) course_name_ ## id,
#define CASTLE_SECRET_STARS(str) course_name_castle_secret_stars,

const u8 *const seg2_course_name_table[] = {
#include "courses.h"
    NULL
};

#undef COURSE_ACTS
#undef SECRET_STAR
#undef CASTLE_SECRET_STARS

#define COURSE_ACTS(id, name, a,b,c,d,e,f) \
    static const u8 lowercase_course_name_ ## id[] = { name };

#define SECRET_STAR(id, name) \
    static const u8 lowercase_course_name_ ## id[] = { name };

#define CASTLE_SECRET_STARS(str) \
    static const u8 lowercase_course_name_castle_secret_stars[] = { str };

#define EXTRA_TEXT(id, str)

#include "courses_lowercase.h"

#undef COURSE_ACTS
#undef SECRET_STAR
#undef CASTLE_SECRET_STARS

#define COURSE_ACTS(id, name, a,b,c,d,e,f) lowercase_course_name_ ## id,
#define SECRET_STAR(id, name) lowercase_course_name_ ## id,
#define CASTLE_SECRET_STARS(str) lowercase_course_name_castle_secret_stars,

const u8 *const seg2_course_name_table_lowercase[] = {
#include "courses_lowercase.h"
    NULL
};

#undef COURSE_ACTS
#undef SECRET_STAR
#undef CASTLE_SECRET_STARS


// == acts ==
// (defines en_act_name_table etc.)

#define COURSE_ACTS(id, name, a,b,c,d,e,f) \
    static const u8 act_name_ ## id ## _1[] = { a }; \
    static const u8 act_name_ ## id ## _2[] = { b }; \
    static const u8 act_name_ ## id ## _3[] = { c }; \
    static const u8 act_name_ ## id ## _4[] = { d }; \
    static const u8 act_name_ ## id ## _5[] = { e }; \
    static const u8 act_name_ ## id ## _6[] = { f };

#define SECRET_STAR(id, name)
#define CASTLE_SECRET_STARS(str)

#undef EXTRA_TEXT
#define EXTRA_TEXT(id, str) \
    static const u8 extra_text_ ## id[] = { str };

#include "courses.h"

#undef COURSE_ACTS
#undef EXTRA_TEXT

#define COURSE_ACTS(id, name, a,b,c,d,e,f) \
    act_name_ ## id ## _1, act_name_ ## id ## _2, act_name_ ## id ## _3, \
    act_name_ ## id ## _4, act_name_ ## id ## _5, act_name_ ## id ## _6,
#define EXTRA_TEXT(id, str) extra_text_ ## id,

const u8 *const seg2_act_name_table[] = {
#include "courses.h"
    NULL
};

#undef COURSE_ACTS

#define COURSE_ACTS(id, name, a,b,c,d,e,f) \
    static const u8 lowercase_act_name_ ## id ## _1[] = { a }; \
    static const u8 lowercase_act_name_ ## id ## _2[] = { b }; \
    static const u8 lowercase_act_name_ ## id ## _3[] = { c }; \
    static const u8 lowercase_act_name_ ## id ## _4[] = { d }; \
    static const u8 lowercase_act_name_ ## id ## _5[] = { e }; \
    static const u8 lowercase_act_name_ ## id ## _6[] = { f };

#define SECRET_STAR(id, name)
#define CASTLE_SECRET_STARS(str)

#undef EXTRA_TEXT
#define EXTRA_TEXT(id, str) \
    static const u8 lowercase_extra_text_ ## id[] = { str };

#include "courses_lowercase.h"

#undef COURSE_ACTS
#undef EXTRA_TEXT

#define COURSE_ACTS(id, name, a,b,c,d,e,f) \
    lowercase_act_name_ ## id ## _1, lowercase_act_name_ ## id ## _2, lowercase_act_name_ ## id ## _3, \
    lowercase_act_name_ ## id ## _4, lowercase_act_name_ ## id ## _5, lowercase_act_name_ ## id ## _6,
#define EXTRA_TEXT(id, str) extra_text_ ## id,

const u8 *const seg2_act_name_table_lowercase[] = {
#include "courses_lowercase.h"
    NULL
};
