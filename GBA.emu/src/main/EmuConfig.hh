#pragma once

#define CONFIG_FILE_NAME "GbaEmu.config"

static const char *touchConfigFaceBtnName = "A/B", *touchConfigCenterBtnName = "Select/Start";
static const uint systemFaceBtns = 4, systemCenterBtns = 2;
static const bool systemHasTriggerBtns = 1, systemHasRevBtnLayout = 0;

namespace EmuCheats
{

static const uint MAX = 100;
static const uint MAX_CODE_TYPES = 2;

}

namespace EmuControls
{

using namespace Input;
static const uint categories = 2;
static const uint gamepadKeys = 18;
static const uint systemTotalKeys = gameActionKeys + gamepadKeys;

}

#define EMU_FRAMEWORK_BUNDLED_GAMES
