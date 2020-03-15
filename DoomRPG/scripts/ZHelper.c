// Stuff here is designed to help out other functions that have been moved to ZScript

#include "Defs.h"

#include "Map.h"
#include "RPG.h"

NamedScript DECORATE int GiveTipHelper()
{
    if ((CurrentLevel->Event == MAPEVENT_SKILL_HELL && GameSkill() != 5) || (CurrentLevel->Event == MAPEVENT_SKILL_ARMAGEDDON && GameSkill() != 6))
        return 0;

    if (CurrentLevel->Event != MAPEVENT_NONE && !Player.SeenEventTip[CurrentLevel->Event])
    {
        Player.SeenEventTip[CurrentLevel->Event] = true;
        return CurrentLevel->Event;
    }

    return 0;
}

NamedScript DECORATE int LevelInfoHelper(int Value)
{
    // Value valid args.
    // 1: Return CurrentLevel->Init.
    // Returns 0 if no valid arg is found

    if (Value == 1)
        return CurrentLevel->Init;

    return 0;
}
