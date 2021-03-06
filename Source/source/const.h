/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Globally used constants.
 */

#ifndef CONST_INCLUDED
#define CONST_INCLUDED

#define _USE_MATH_DEFINES
#include <math.h>
#include <string>
#include <vector>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>

#include "utils/math_utils.h"

using namespace std;

/*
 * Version stuff.
 * On every release, update these numbers, and
 * update the numbers on the resouce (.rc) file.
 */
const unsigned char VERSION_MAJOR = 0;
const unsigned char VERSION_MINOR = 17;
const unsigned char VERSION_REV   = 1;


const size_t INVALID = UINT32_MAX;
const float LARGE_FLOAT = 999999.0f;

const string ANIMATION_EDITOR_TUTORIAL_URL =
    "http://www.pikminfanon.com/Pikifen/Creating_animations";
const string AREA_EDITOR_TUTORIAL_URL =
    "http://www.pikminfanon.com/Pikifen/Creating_areas";

//How many entries of the animation editor history to store, at max.
const size_t ANIMATION_EDITOR_HISTORY_SIZE = 6;
//How long it takes for the area name to fade away, in-game.
const float AREA_TITLE_FADE_DURATION = 3.0f;
//How fast the "invalid cursor" effect goes, per second.
const float CURSOR_INVALID_EFFECT_SPEED = TAU * 2;
//Every X seconds, the cursor's position is saved, to create the trail effect.
const float CURSOR_SAVE_INTERVAL = 0.03f;
//Number of positions of the cursor to keep track of.
const unsigned char CURSOR_SAVE_N_SPOTS = 10;
//Default area editor auto-backup interval, in seconds.
const float DEF_AREA_EDITOR_BACKUP_INTERVAL = 120.0f;
//Default area editor grid interval.
const float DEF_AREA_EDITOR_GRID_INTERVAL = 32.0f;
//Default area editor undo history size limit.
const size_t DEF_AREA_EDITOR_UNDO_LIMIT = 20;
//Default editor mouse drag threshold range.
const size_t DEF_EDITOR_MOUSE_DRAG_THRESHOLD = 4;
//Default leader punch strength.
const float DEF_PUNCH_STRENGTH = 2.0f;
//The default rotation speed of a mob type.
const float DEF_ROTATION_SPEED = 630.0f;
//The whistle can't go past this radius, by default.
const float DEF_WHISTLE_RANGE = 80.0f;
//How long to suck a mob in for, when being delivered to an Onion/ship.
const float DELIVERY_SUCK_TIME = 1.5f;
//When getting a framerate average, use a sample of this size.
const size_t FRAMERATE_AVG_SAMPLE_SIZE = 30;
//Only save the latest N FPS samples.
const size_t FRAMERATE_HISTORY_SIZE = 300;
//If the Pikmin is within this distance of the mob, it can ground attack.
const float GROUNDED_ATTACK_DIST = 5.0f;
//"Move group" arrows move these many units per second.
const float GROUP_MOVE_ARROW_SPEED = 400.0f;
//Seconds that need to pass before another "move group" arrow appears.
const float GROUP_MOVE_ARROWS_INTERVAL = 0.1f;
//Put this space between the leader and the "main" member of the group,
//when using group moving.
const float GROUP_MOVE_MARGIN = 8.0f;
//When group moving, the group can scale this much vertically.
//Basically, the tube shape's girth can reach this scale.
const float GROUP_MOVE_VERTICAL_SCALE = 0.5f;
//If there's less than this much gap between the leader and group,
//then the group's Pikmin should shuffle a bit to keep up with the leader.
const float GROUP_SHUFFLE_DIST = 40.0f;
//Pikmin must be at least these many units away from one another;
//used when calculating group spots.
const float GROUP_SPOT_INTERVAL = 2.0f;
//The idle glow spins these many radians per second.
const float IDLE_GLOW_SPIN_SPEED = TAU / 4;
//Maximum number of players that can play the game.
const size_t MAX_PLAYERS = 4;
//How many pixels the mouse cursor moves, per second,
//when using an analog stick.
const float MOUSE_CURSOR_MOVE_SPEED = 500.0f;
//Multiply the stretch of the shadow by this much.
const float MOB_SHADOW_STRETCH_MULT = 0.5f;
//For every unit above the ground that the mob is on,
//the shadow goes these many units to the side.
const float MOB_SHADOW_Y_MULT = 0.2f;
//When using the height effect, scale the mob by this factor.
const float MOB_HEIGHT_EFFECT_FACTOR = 0.0017;
//Number of maturities.
const size_t N_MATURITIES = 3;
//When an opponent is hit, it takes this long to be possible to hit it again.
const float OPPONENT_HIT_REGISTER_TIMEOUT = 0.5f;
//How frequently should a replay state be saved.
const float REPLAY_SAVE_FREQUENCY = 1.0f;
//Red color's index moves these many units per second.
//(Green is fast and blue is faster still).
const unsigned int SHIP_BEAM_RING_COLOR_SPEED = 255;
//Duration of the "smack" particle.
const float SMACK_PARTICLE_DUR = 0.1f;
//The Sun Meter's sun spins these many radians per second.
const float SUN_METER_SUN_SPIN_SPEED = 0.5f;
//Tree shadows sway this much away from their neutral position.
const float TREE_SHADOW_SWAY_AMOUNT = 8.0f;
//Tree shadows sway this much per second (TAU = full back-and-forth cycle).
const float TREE_SHADOW_SWAY_SPEED = TAU / 8;
//A new "mob thrown" particle is spawned every X seconds.
const float THROW_PARTICLE_INTERVAL = 0.02f;
//Seconds that need to pass before another dot is added.
const float WHISTLE_DOT_INTERVAL = 0.03;
//A whistle dot spins these many radians a second.
const float WHISTLE_DOT_SPIN_SPEED = TAU / 4;
//Time the whistle animations take to fade out.
const float WHISTLE_FADE_TIME = 0.1f;
//Whistle rings move these many units per second.
const float WHISTLE_RING_SPEED = 600.0f;
//Seconds that need to pass before another whistle ring appears.
const float WHISTLE_RINGS_INTERVAL = 0.1f;

const unsigned int DEF_FPS   = 60;
const bool DEF_SCR_FULLSCREEN = false;
const unsigned int DEF_SCR_W = 1024;
const unsigned int DEF_SCR_H = 768;
const int SMALLEST_SCR_W = 640;
const int SMALLEST_SCR_H = 480;

const string GAME_DATA_FOLDER_PATH = "Game_data";
const string USER_DATA_FOLDER_PATH = "User_data";
const string TYPES_FOLDER_PATH =
    GAME_DATA_FOLDER_PATH + "/Types";
const string MISC_FOLDER_PATH =
    GAME_DATA_FOLDER_PATH + "/Misc";
const string TEXTURES_FOLDER_NAME =
    "Textures";
const string USER_AREA_DATA_FOLDER_PATH =
    USER_DATA_FOLDER_PATH + "/Areas";
const string ERROR_LOG_FILE_PATH =
    USER_DATA_FOLDER_PATH + "/Error_log.txt";
const string CREATOR_TOOLS_FILE_PATH =
    USER_DATA_FOLDER_PATH + "/Tools.txt";
const string OPTIONS_FILE_PATH =
    USER_DATA_FOLDER_PATH + "/Options.txt";

const string ANIMATIONS_FOLDER_PATH =
    GAME_DATA_FOLDER_PATH + "/Animations";
const string AREAS_FOLDER_PATH =
    GAME_DATA_FOLDER_PATH + "/Areas";
const string AUDIO_FOLDER_PATH =
    GAME_DATA_FOLDER_PATH + "/Audio";
const string GRAPHICS_FOLDER_PATH =
    GAME_DATA_FOLDER_PATH + "/Graphics";
const string TEXTURES_FOLDER_PATH =
    GRAPHICS_FOLDER_PATH + "/" + TEXTURES_FOLDER_NAME;

const string CONFIG_FILE =
    MISC_FOLDER_PATH + "/Config.txt";
const string PARTICLE_GENERATORS_FILE_PATH =
    MISC_FOLDER_PATH + "/Particle_generators.txt";
const string SPIKE_DAMAGE_TYPES_FILE_PATH =
    MISC_FOLDER_PATH + "/Spike_damage.txt";
const string SYSTEM_ANIMATIONS_FILE_PATH =
    MISC_FOLDER_PATH + "/System_animations.txt";
const string SYSTEM_ASSET_FILE_NAMES_FILE_PATH =
    MISC_FOLDER_PATH + "/System_asset_file_names.txt";
const string TITLE_SCREEN_FILE_PATH =
    MISC_FOLDER_PATH + "/Title_screen.txt";
const string WEATHER_FILE_PATH =
    MISC_FOLDER_PATH + "/Weather.txt";

enum CREATOR_TOOL_IDS {
    CREATOR_TOOL_NONE,
    CREATOR_TOOL_AREA_IMAGE,
    CREATOR_TOOL_CHANGE_SPEED,
    CREATOR_TOOL_GEOMETRY_INFO,
    CREATOR_TOOL_HITBOXES,
    CREATOR_TOOL_HURT_MOB,
    CREATOR_TOOL_MOB_INFO,
    CREATOR_TOOL_NEW_PIKMIN,
    CREATOR_TOOL_TELEPORT,
};

enum HUD_ELEMENTS {
    HUD_ITEM_TIME,
    HUD_ITEM_DAY_BUBBLE,
    HUD_ITEM_DAY_NUMBER,
    HUD_ITEM_LEADER_1_ICON,
    HUD_ITEM_LEADER_2_ICON,
    HUD_ITEM_LEADER_3_ICON,
    HUD_ITEM_LEADER_1_HEALTH,
    HUD_ITEM_LEADER_2_HEALTH,
    HUD_ITEM_LEADER_3_HEALTH,
    HUD_ITEM_PIKMIN_STANDBY_ICON,
    HUD_ITEM_PIKMIN_STANDBY_M_ICON,
    HUD_ITEM_PIKMIN_STANDBY_NR,
    HUD_ITEM_PIKMIN_STANDBY_X,
    HUD_ITEM_PIKMIN_GROUP_NR,
    HUD_ITEM_PIKMIN_FIELD_NR,
    HUD_ITEM_PIKMIN_TOTAL_NR,
    HUD_ITEM_PIKMIN_SLASH_1,
    HUD_ITEM_PIKMIN_SLASH_2,
    HUD_ITEM_PIKMIN_SLASH_3,
    HUD_ITEM_SPRAY_1_ICON,
    HUD_ITEM_SPRAY_1_AMOUNT,
    HUD_ITEM_SPRAY_1_BUTTON,
    HUD_ITEM_SPRAY_2_ICON,
    HUD_ITEM_SPRAY_2_AMOUNT,
    HUD_ITEM_SPRAY_2_BUTTON,
    HUD_ITEM_SPRAY_PREV_ICON,
    HUD_ITEM_SPRAY_PREV_BUTTON,
    HUD_ITEM_SPRAY_NEXT_ICON,
    HUD_ITEM_SPRAY_NEXT_BUTTON,
    
    N_HUD_ITEMS,
};

enum LIMB_DRAW_METHODS {
    LIMB_DRAW_BELOW_BOTH,
    LIMB_DRAW_BELOW_CHILD,
    LIMB_DRAW_BELOW_PARENT,
    LIMB_DRAW_ABOVE_PARENT,
    LIMB_DRAW_ABOVE_CHILD,
    LIMB_DRAW_ABOVE_BOTH,
};

const unsigned char N_WHISTLE_RING_COLORS = 8;
const unsigned char WHISTLE_RING_COLORS[N_WHISTLE_RING_COLORS][3] = {
    {255, 255, 0  },
    {255, 0,   0  },
    {255, 0,   255},
    {128, 0,   255},
    {0,   0,   255},
    {0,   255, 255},
    {0,   255, 0  },
    {128, 255, 0  }
};

#endif //ifndef CONST_INCLUDED
