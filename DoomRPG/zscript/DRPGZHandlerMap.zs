class DRPGZEHandlerMap : EventHandler
{
    // Structs
    struct LevelInfo_S
    {
        bool Init;
    }
    // Pointers
    LevelInfo_S CurrentLevel;

    override void WorldTick()
    {
        if (!CurrentLevel.Init)
            CurrentLevel.Init = players[consoleplayer].mo.ACS_ScriptCall("LevelInfoHelper", 1);
    }
}
