#include "Defs.h"

#include "Arena.h"
#include "Augs.h"
#include "HealthBars.h"
#include "HUD.h"
#include "ItemData.h"
#include "Map.h"
#include "Menu.h"
#include "Mission.h"
#include "Monsters.h"
#include "Outpost.h"
#include "Popoffs.h"
#include "RPG.h"
#include "Shield.h"
#include "Shop.h"
#include "Skills.h"
#include "Stats.h"
#include "Stims.h"
#include "Turret.h"
#include "Utils.h"

// Flags
bool Transported;
bool GlobalsInitialized;
int CompatMode;
int CompatMonMode;
bool WadSmoosh;

// Arrays
str PlayerWeapon[MAX_PLAYERS];

// Structs
PlayerData _PlayerData[MAX_PLAYERS];

// Items
bool RPGMap ItemTIDsInitialized;
int RPGMap ItemTIDs[MAX_ITEMS];

// --------------------------------------------------
// Multiplayer HUD
//

Address(8) AddressSpace MapArraySpace    RPGMap;
Address(8) AddressSpace WorldArraySpace  RPGWorld;
Address(8) AddressSpace GlobalArraySpace RPGGlobal;

Address(2)  AddressSpace GlobalArraySpace EPArray;

Address(50) AddressSpace GlobalArraySpace ShieldArray;
Address(51) AddressSpace GlobalArraySpace ShieldCapacityArray;
Address(52) AddressSpace GlobalArraySpace ShieldHealthArray;

// Energy Points (EP)
int EPArray Address(0) EP[MAX_PLAYERS];

// Shield
int ShieldArray Address(0) Shield[MAX_PLAYERS];
int ShieldCapacityArray Address(0) ShieldCapacity[MAX_PLAYERS];
int ShieldHealthArray Address(0) ShieldHealth[MAX_PLAYERS];

str const StatColors[STAT_MAX] =
{
    "Red",         // Strength
    "Green",       // Defense
    "Brick",       // Vitality
    "LightBlue",   // Energy
    "Purple",      // Regeneration
    "Orange",      // Agility
    "Blue",        // Capacity
    "Gold"         // Luck
};

NamedScript Type_OPEN void GlobalInit()
{
    if (!GlobalsInitialized)
    {
        // Version Info
        Log("\CnDoom RPG SE (GDCC) (Compiled on %S) loaded!", __DATE__);

        // Compatibility checking
        CheckCompatibility();

        // Get the XP Curve from the CVAR
        XPCurve = GetCVar("drpg_xp_curve");

        // Initial build of ItemData
        BuildItemData();

        // Initialize XP, Stat and Rank Tables
        InitXPTable();
        Delay(1);
        InitStatXPTable();
        Delay(1);
        InitNegativeStatXPTable();
        Delay(1);
        InitRankTable();

        // Arena Wave
        ArenaMaxWave = 1;

        GlobalsInitialized = true;

        Delay(1); // Allow CVARs to update
    }

    // Create Translations
    CreateTranslations();

    // Initial build of SkillData
    BuildSkillData();

    // Initial build of TurretUpgradeData
    BuildTurretData();

    // Initialize the Item handling script
    ItemHandler();
}

// Init Script
NamedScript Type_ENTER void Init()
{
    // Wait until globals are initialized
    while (!GlobalsInitialized) Delay(1);

    // Delay to allow for item data to initialize
    Delay(3);

    // [KS] This needs to be done on its' own, so death exits don't rob you of your TID and break EVERYTHING EVER FFS
    int ATID = ActivatorTID();
    if (ATID == 0 || ATID == 30000) // 30000 is DRLA's PID.
    {
        AssignTIDs();
        if (!InTitle) // Don't give equipment on the title screen
        {
            if (!Player.FirstRun)
                Player.Capacity = GetActivatorCVar("drpg_start_capacity");

            SortStartingItems();
            DefaultLoadout();
        }
    }

    if (!Player.FirstRun)
    {
        // [KS] Portable always-use items, because having too many mandatory use-keys is bad, yo.
        if (GetActivatorCVar("drpg_give_quickinventory"))
        {
            GiveInventory("DRPGPortableStimInjectorItem", 1); // Use Stim
            GiveInventory("DRPGPortableStimDisposalItem", 1); // Toss Stim
            GiveInventory("DRPGPortableShieldItem", 1); // Toggle Shield
            GiveInventory("DRPGPortableMedkitItem", 1); // Quick Heal
            GiveInventory("DRPGPortableFocusItem", 1); // Focus
            GiveInventory("DRPGPortableAugItem", 1); // Reactivate Augs
        }

        // Funds
        GiveInventory("DRPGCredits", GetActivatorCVar("drpg_start_credits"));
        Player.PrevCredits = CheckInventory("DRPGCredits");
        GiveInventory("DRPGModule", GetActivatorCVar("drpg_start_modules"));
        GiveInventory("DRPGTurretPart", GetActivatorCVar("drpg_start_turretparts"));

        // Level/Rank
        if (GetCVar("drpg_start_level") > 0)
        {
            Player.Level = GetActivatorCVar("drpg_start_level");
            Player.XP = XPTable[Player.Level - 1];
        }
        if (GetCVar("drpg_start_rank") > 0)
        {
            Player.RankLevel = GetActivatorCVar("drpg_start_rank");
            Player.Rank = RankTable[Player.RankLevel - 1];
        }

        // Stats
        Player.Strength = GetActivatorCVar("drpg_start_strength");
        Player.Defense = GetActivatorCVar("drpg_start_defense");
        Player.Vitality = GetActivatorCVar("drpg_start_vitality");
        Player.PrevVitality = Player.Vitality;
        Player.Energy = GetActivatorCVar("drpg_start_energy");
        Player.PrevEnergy = Player.Energy;
        Player.Regeneration = GetActivatorCVar("drpg_start_regeneration");
        Player.Agility = GetActivatorCVar("drpg_start_agility");
        Player.Capacity = GetActivatorCVar("drpg_start_capacity");
        Player.Luck = GetActivatorCVar("drpg_start_luck");

        // Total Stat Values
        Player.StrengthTotal = Player.Strength;
        Player.DefenseTotal = Player.Defense;
        Player.VitalityTotal = Player.Vitality;
        Player.EnergyTotal = Player.Energy;
        Player.RegenerationTotal = Player.Regeneration;
        Player.AgilityTotal = Player.Agility;
        Player.CapacityTotal = Player.Capacity;
        Player.LuckTotal = Player.Luck;

        // Natural Bonuses
        Player.StrengthNat = 0;
        Player.DefenseNat = 0;
        Player.VitalityNat = 0;
        Player.EnergyNat = 0;
        Player.RegenerationNat = 0;
        Player.AgilityNat = 0;
        Player.CapacityNat = 0;
        Player.LuckNat = 0;

        // Stat XP
        Player.StrengthXP = 0;
        Player.DefenseXP = 0;
        Player.VitalityXP = 0;
        Player.EnergyXP = 0;
        Player.RegenerationXP = 0;
        Player.AgilityXP = 0;
        Player.CapacityXP = 0;
        Player.LuckXP = 0;

        // Default Health/EP
        Player.EP = Player.EnergyTotal * 10;
        Player.ActualHealth = Player.VitalityTotal * 10;
        Player.PrevHealth = Player.ActualHealth;
        SetActorProperty(0, APROP_Health, Player.ActualHealth);

        // Setup HUD stuff
        Player.StatusTypeHUD = SE_NONE;

        for (int i = 0; i < MAPEVENT_MAX; i++)
            Player.SeenEventTip[i] = false;

        // Setup the Skill Wheel from the persistence CVARs
        for (int i = 0; i < MAX_SKILLKEYS; i++)
        {
            Player.SkillCategory[i] = GetActivatorCVar(StrParam("drpg_skillwheel_category_%d", i + 1));
            Player.SkillIndex[i] = GetActivatorCVar(StrParam("drpg_skillwheel_index_%d", i + 1));
        }

        // Transport should always be known
        Player.SkillLevel[MAX_CATEGORIES - 1][SkillCategoryMax[MAX_CATEGORIES - 1] - 1].Level = 1;
        Player.SkillLevel[MAX_CATEGORIES - 1][SkillCategoryMax[MAX_CATEGORIES - 1] - 1].CurrentLevel = 1;

        // Set default selected skill to nothing
        Player.SkillSelected = -1;

        // Unset current reviver
        Player.Reviver = -1;

        // Fill Augmentation Battery
        Player.Augs.Battery = Player.CapacityTotal * 10;

        // Setup the New! shield parts arrays
        for (int i = 0; i < SHIELDPAGE_MAX; i++)
            for (int j = 0; j < MAX_PARTS; j++)
                Player.NewShieldParts[i][j] = true;
        for (int i = 0; i < MAX_ACCESSORIES; i++)
            Player.NewShieldAccessoryParts[i] = true;

        // Done first run
        Player.FirstRun = true;
    }

    // Death exit handling
    if (Player.ActualHealth <= 0)
        Player.ActualHealth = Player.HealthMax;

    // Apply camera textures and vars
    SetCameraToTexture(Player.TID, StrParam("P%iVIEW", PlayerNumber() + 1), 110);
    SetCameraToTexture(Player.TID, StrParam("P%iSVIEW", PlayerNumber() + 1), 90);
    Player.PlayerView = PlayerNumber();

    // Remove Aura if the keep Aura CVAR is off
    if (!GetCVar("drpg_skill_keepauras"))
        RemoveAura();

    // Transport FX
    if (Transported)
    {
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            // Player is not in-game
            if (!PlayerInGame(i)) continue;

            TransportInFX(Players(i).TID);
        }

        SetPlayerProperty(0, 0, PROP_TOTALLYFROZEN); // In case we were in the menu
        Transported = false;
    }

    // Clear the Player's summons
    for (int i = 0; i < MAX_SUMMONS; i++)
        Player.SummonTID[i] = 0;
    Player.Summons = 0;

    // Initialize DropTID Array
    ArrayCreate(&Player.DropTID, "Drops", 64, sizeof(int));

    // Mission Initialization
    InitMission();

    // Small delay before initializing the scripts for mod compatibility
    Delay(4);

    // Reassignment is necessary for some mods
    if (CompatMode == COMPAT_DRLA)
        AssignTIDs();

    // Execute Game Loops
    Loop();
    PlayerHealth();
    MoneyChecker();
    ShieldTimer();
    WeaponSpeed();
    StatRandomizer();
    DamageNumbers();
    InfoPopoffs();
    HealthBars();
    AutosaveHandler();
    ShopItemAutoHandler();
}

// Loop Script
NamedScript void Loop()
{
Start:

    // If we're on the title map, terminate
    if (InTitle)
    {
        SetActorProperty(0, APROP_Invulnerable, true);
        TeleportOther(Player.TID, 1003, false);
        return;
    }

    // If the player's dead, terminate
    if (GetActorProperty(0, APROP_Health) <= 0) return;

    // Update Functions
    CheckCombo();
    CheckStats();
    CheckHardStatCaps();
    CheckRegen();
    CheckLuck();
    CheckHealth();
    CheckAugSlots();
    CheckAugs();
    CheckLevel();
    CheckRank();
    CheckSkills();
    CheckAuras();
    CheckBurnout();
    CheckStim();
    CheckPerks();
    CheckShields();
    CheckShieldAccessory();
    CheckStatBounds();
    CheckToxicity();
    CheckStimImmunity();
    CheckStatusEffects();
    CheckCapacity();
    CheckShopCard();
    CheckMission();
    CheckHUD();
    CheckKeys();

    // Handle the pulsating menu colors
    MenuCursorColor = CursorColors[(Timer() / 3) % 6];

    // Calculate Shop Discount
    Player.ShopDiscount = (int)((Player.RankLevel * 2.1) + (CurrentLevel->UACBase ? (Player.ShopCard * 5) : 0));

    // Main Menu
    if (Player.InMenu)
        MenuLoop();

    // Shop Menu
    if (Player.InShop)
        ShopLoop();

    // Menu dimming
    if (Player.InMenu || Player.InShop || Player.OutpostMenu > 0)
    {
        if (GetCVar("drpg_menudim") && !Player.StatusType[SE_BLIND])
            FadeRange(0, 0, 0, 0.65, 0, 0, 0, 0.0, 0.25);
    }

    // Menu-specific Help
    if (Player.InMenu || Player.InShop || Player.OutpostMenu > 0 || Player.CrateOpen || Player.CrateHacking)
        MenuHelp();

    // Menu icon in multiplayer
    if (InMultiplayer && (Player.InMenu || Player.InShop || Player.OutpostMenu > 0))
    {
        fixed X = GetActorX(Player.TID);
        fixed Y = GetActorY(Player.TID);
        fixed Z = GetActorZ(Player.TID);
        fixed Height = GetActorPropertyFixed(Player.TID, APROP_Height);

        SpawnForced("DRPGMenuIcon", X, Y, Z + Height + 8.0, 0, 0);
    }

    // Regeneration
    DoRegen();

    // Always Quick Heal if CVAR is set
    if (GetActivatorCVar("drpg_auto_heal"))
        if (Player.ActualHealth < Player.HealthMax / GetActivatorCVar("drpg_auto_heal_percent"))
            UseMedkit();

    // Set mass stupid high when Invulnerable or transporting to prevent knockback
    if (CheckInventory("PowerInvulnerable") || Transported)
        Player.Mass *= 128;

    // Apply Stats
    SetActorPropertyFixed(Player.TID, APROP_DamageMultiplier, (1.0 + ((fixed)Player.TotalDamage / 100.0)) * Player.DamageMult);
    SetActorPropertyFixed(Player.TID, APROP_DamageFactor, Player.DamageFactor);
    SetActorProperty(Player.TID, APROP_Mass, Player.Mass);
    SetActorProperty(Player.TID, APROP_SpawnHealth, Player.HealthMax);
    SetActorPropertyFixed(Player.TID, APROP_Speed, Player.Speed);
    SetActorPropertyFixed(Player.TID, APROP_JumpZ, Player.JumpHeight);

    // Store your current weapon in the Player Weapon array
    PlayerWeapon[PlayerNumber()] = GetWeapon();

    // DRLA Debug Mode
    if (GetCVar("drpg_debug_drla") & DDM_NOLIMITS)
    {
        SetInventory("RLWeaponLimit", 0);
        SetInventory("RLArmorInInventory", 0);
        SetInventory("RLModLimit", 0);
        SetInventory("RLScavengerModLimit", 0);
        SetInventory("RLArmorModItemInInventory", 0);
        SetInventory("RLSkullLimit", 0);
        SetInventory("RLPhaseDeviceLimit", 0);
    }

    if (GetCVar("drpg_multi_revives"))
        ReviveHandler();

    // Loop
    Delay(1);

    goto Start;
}

// Health Handler Entry Point
NamedScript void PlayerHealth()
{
    int BeforeHealth;
    int AfterHealth;

    while (true)
    {
        BeforeHealth = GetActorProperty(0, APROP_Health);

        // If the player's dead, terminate
        if (BeforeHealth <= 0) break;

        Delay(1);

        AfterHealth = GetActorProperty(0, APROP_Health);

        if (AfterHealth > BeforeHealth)
            Player.ActualHealth += AfterHealth - BeforeHealth;

        // Update health
        SetActorProperty(0, APROP_Health, Player.ActualHealth);
    }
}

// Damage Handler Entry Point
NamedScript DECORATE int PlayerDamage(int Inflictor, int DamageTaken)
{
    bool CanSurvive;
    bool Critical;
    int MonsterID;
    fixed LuckChance;
    fixed EnergyLevel;

    CanSurvive = Player.SurvivalBonus > 0 && RandomFixed(0.0, 100.0) <= Player.SurvivalBonus && !Player.LastLegs;

    if (Player.ActualHealth > 2)
        Player.LastLegs = false;

    if (CanSurvive || CheckInventory("DRPGLife"))
        SetPlayerProperty(0, 1, PROP_BUDDHA);
    else
        SetPlayerProperty(0, 0, PROP_BUDDHA);

    // Assign attacker's ID
    Player.DamageTID = Inflictor;

    Player.AutosaveTimerReset = true;

    // Find Monster ID
    MonsterID = FindMonster(Player.DamageTID);

    // Calculate monster crit/status chance
    LuckChance = (fixed)Monsters[MonsterID].Luck / 20.0;
    EnergyLevel = (fixed)Monsters[MonsterID].Energy / 12.5;

    // Calculate a critical hit
    if (Player.DamageTID > 0 && MonsterID > 0)
        if (RandomFixed(0.0, 100.0) <= EnergyLevel)
        {
            DamageTaken *= 2;
            Critical = true;
        }

    AugDamage(DamageTaken);
    ToxicityDamage();

    if (MonsterID > 0)
        StatusDamage(DamageTaken, LuckChance, Critical);
    else
        StatusDamage(DamageTaken, RandomFixed(0.0, 100.0), Critical);

    ResetRegen();
    DamageHUD(DamageTaken, Critical);

    Player.DamageType = DT_NONE;

    Player.ActualHealth -= DamageTaken;
    // Receiving damage to health interrupts focusing
    Player.Focusing = false;

    if (Player.ActualHealth <= 1) // Near-Death stuff
    {
        // Extra Life check
        if (CheckInventory("DRPGLife"))
        {
            Player.ActualHealth = Player.HealthMax;
            ActivatorSound("health/resurrect", 127);
            if (!CurrentLevel->UACBase)
            {
                SetInventory("ArtiTeleport", 1);
                UseInventory("ArtiTeleport");
            }
            TakeInventory("DRPGLife", 1);

            SetHudSize(320, 200, false);
            SetFont("BIGFONT");
            HudMessage("Used an Extra Life!");
            EndHudMessage(HUDMSG_FADEOUT, 0, "Gold", 160.0, 140.0, 0.5, 1.5);
            PrintSpriteFade("P1UPA0", 0, 172.0, 210.0, 0.5, 1.5);
        }

        // Survival Bonus
        if (CanSurvive)
        {
            Player.ActualHealth = 2;
            Player.LastLegs = true;

            if (Player.Shield.Accessory && Player.Shield.Accessory->PassiveEffect == SHIELD_PASS_SURVIVECHARGE)
                Player.Shield.Charge = Player.Shield.Capacity;

            ActivatorSound("health/survive", 127);
            SetHudSize(320, 200, false);
            SetFont("BIGFONT");
            HudMessage("Agility Save!");
            EndHudMessage(HUDMSG_FADEOUT, 0, "Orange", 160.0, 140.0, 0.5, 0.5);
            PrintSpriteFade("AGISAVE", 0, 160.0, 140.0, 0.5, 0.5);
        }
    }

    SetActorProperty(0, APROP_Health, Player.ActualHealth);

    return 0;
}

// Shield Damage Handler Entry Point
NamedScript DECORATE int ShieldDamage(int DamageTaken)
{
    int ShieldDamageAmount;

    ShieldTimerReset();

    if (Player.Shield.Charge > 0)
    {
        Player.AutosaveTimerReset = true;
        AugDamage(DamageTaken);
        ToxicityDamage();
        StatusDamage(DamageTaken, RandomFixed(0.0, 100.0), false);
        DamageHUD(DamageTaken, false);

        ShieldDamageAmount = DamageTaken; // For callback
        if (ShieldDamageAmount > Player.Shield.Charge)
            ShieldDamageAmount = Player.Shield.Charge;

        Player.Shield.Charge -= DamageTaken;
        Player.Shield.Full = false;

        FadeRange(0, 100, 255, 0.25, 0, 100, 255, 0, 0.25);
        PlaySound(0, "shield/hit", 5, 1.0, false, 1.0);
        if (Player.Shield.Accessory && Player.Shield.Accessory->Damage)
            Player.Shield.Accessory->Damage(ShieldDamageAmount);

        if (Player.Shield.Charge <= 0)
        {
            if (Player.Shield.Charge < 0)
            {
                DamageTaken = -Player.Shield.Charge;
                Player.Shield.Charge = 0;
            }
            else
                DamageTaken = 0;

            PlaySound(0, "shield/empty", 5, 1.0, false, 1.0);
            if (Player.Shield.Accessory && Player.Shield.Accessory->Break)
                Player.Shield.Accessory->Break();
        }
        else
            DamageTaken = 0;
    }

    return DamageTaken;
}

NamedScript void MoneyChecker()
{
    int BeforeCredits;
    int AfterCredits;

Start:

    BeforeCredits = CheckInventory("DRPGCredits");

    Delay(1);

    AfterCredits = CheckInventory("DRPGCredits");

    goto Start;
}

NamedScript DECORATE int AddHealth(int HealthPercent, int MaxPercent)
{
    int RealMax = Player.HealthMax * MaxPercent / 100;
    int HealthAmount = Player.HealthMax * HealthPercent / 100;

    if (Player.OverHeal)
        if (MaxPercent >= 200 && Player.ActualHealth + HealthAmount >= RealMax)
            Player.OverHeal = false;

    if (HealthAmount > 0 && Player.ActualHealth >= RealMax)
        return 0;

    if (Player.ActualHealth + HealthAmount > RealMax)
        HealthAmount = RealMax - Player.ActualHealth;

    // Add Vitality XP for using healing items
    if (GetCVar("drpg_levelup_natural"))
    {
        fixed Scale = GetCVarFixed("drpg_vitality_scalexp");
        if (GetCVar("drpg_allow_spec"))
        {
            if (GetActivatorCVar("drpg_character_spec") == 3)
                Scale *= 2;
        }

        int Factor = CalcPercent(HealthAmount, Player.HealthMax);
        Player.VitalityXP += (int)(Factor * Scale * 10);
    }

    Player.ActualHealth += HealthAmount;
    return 1;
}

NamedScript DECORATE int AddHealthDirect(int HealthAmount, int MaxPercent)
{
    int RealMax = Player.HealthMax * MaxPercent / 100;

    if (Player.OverHeal)
        if (MaxPercent >= 200 && Player.ActualHealth + HealthAmount >= RealMax)
            Player.OverHeal = false;

    if (HealthAmount > 0 && Player.ActualHealth >= RealMax)
        return 0;

    if (Player.ActualHealth + HealthAmount > RealMax)
        HealthAmount = RealMax - Player.ActualHealth;

    // Add Vitality XP for using healing items
    if (GetCVar("drpg_levelup_natural"))
    {
        fixed Scale = GetCVarFixed("drpg_vitality_scalexp");
        if (GetCVar("drpg_allow_spec"))
        {
            if (GetActivatorCVar("drpg_character_spec") == 3)
                Scale *= 2;
        }

        int Factor = CalcPercent(HealthAmount, Player.HealthMax);
        Player.VitalityXP += (int)(Factor * Scale * 10);
    }

    Player.ActualHealth += HealthAmount;
    return 1;
}

// Handles Weapon Firing Speed
NamedScript void WeaponSpeed()
{
    int Time;

Start:

    if (Player.Stim.PowerupTimer[STIM_RAGE] > 0 || Player.WeaponSpeed >= 100 || Player.WeaponSpeed <= 0)
        Time = 4;
    else
        Time = (int)(4 * (100.0 / Player.WeaponSpeed - 0.05)) + 1;

    if (Player.Stim.PowerupTimer[STIM_RAGE] > 0 || (Player.WeaponSpeed > 0 && GetActivatorCVar("drpg_stat_weaponspeed")))
        SetInventory("DRPGSpeed", 1);

    Delay(Time);
    goto Start;
}

// Stat Randomizer Script
NamedScript void StatRandomizer()
{
Start:

    if (GetActivatorCVar("drpg_auto_spend"))
    {
        while (CheckInventory("DRPGModule") > 0 && !StatsCapped())
        {
            // Select Preferred Stat
            if (Random(1, 2) == 1 && GetActivatorCVar("drpg_auto_spend_pref") >= 0)
                IncreaseStat(GetActivatorCVar("drpg_auto_spend_pref"));
            else // Select Random Stat
                IncreaseStat(Random(0, STAT_MAX - 1));

            Delay(1);
        }
    }

    Delay(1);
    goto Start;
}

// Handles autosaving
NamedScript void AutosaveHandler()
{
    // Terminate if the autosave CVar is disabled
    if (GetCVar("drpg_autosave") == 0 || (PlayerCount() > 1 && !Arbitrator)) return;

    int SaveTimer = GetCVar("drpg_autosave") * (35 * 60);
    bool Safe;
    int DamageTimer;

    while (true)
    {
        if (Player.AutosaveTimerReset)
            DamageTimer = 0;
        else
            ++DamageTimer;
        Player.AutosaveTimerReset = false;

        if (Player.InMenu || Player.InShop || Player.OutpostMenu > 0)
            DamageTimer = ASAVE_SAFETIME;

        if (!SaveTimer)
        {
            Safe = true;

            if (DamageTimer < ASAVE_SAFETIME ||
                    GetActorProperty(Player.TID, APROP_Health) <= GetActorProperty(Player.TID, APROP_SpawnHealth) / 10)
                Safe = false;

            if (Safe)
            {
                Autosave();
                SaveTimer = GetCVar("drpg_autosave") * (35 * 60);
            }
            else
                SaveTimer = ASAVE_RETRYTIME;
        }
        else
            --SaveTimer;

        Delay(1);
    }
}

NamedScript Type_OPEN void ShopSpecialHandler()
{
    bool ValidItem;
    int Tries, MinValue, MaxValue, Category, Index;
Start:

    // Reset the item
    if (ShopSpecialTimer <= 0)
    {
        ValidItem = false;
        Tries = 0;
        MinValue = 0;
        MaxValue = 0;
        Category = 0;
        Index = 0;

        // Calculate min and max based on settings
        switch (GetCVar("drpg_shopspecial_type"))
        {
        case SHOPSPECIAL_MINMAX:
            MinValue = GetCVar("drpg_shopspecial_min");
            MaxValue = GetCVar("drpg_shopspecial_max");
            break;
        case SHOPSPECIAL_LEVEL:
            MinValue = AveragePlayerLevel() * MAX_LEVEL;
            MaxValue = AveragePlayerLevel() * (100 * MAX_LEVEL);
            break;
        case SHOPSPECIAL_RANK:
            MinValue = AveragePlayerRank() * (25 * MAX_RANK);
            MaxValue = AveragePlayerRank() * (2500 * MAX_RANK);
            break;
        case SHOPSPECIAL_CREDITS:
            MinValue = AveragePlayerCredits() / 10;
            MaxValue = AveragePlayerCredits() * 10;
            break;
        case SHOPSPECIAL_LUCK:
            MinValue = AveragePlayerLuck() * 500;
            MaxValue = AveragePlayerLuck() * 5000;
            break;
        }
        if (DebugLog)
            Log("\CdDEBUG: Shop Special \C-Min/Max Calculated: \Ca%d \C-/ \Cd%d", MinValue, MaxValue);

        // Blank out the item until a new one is found
        ShopSpecialItem = GetBlankItem();

        while (!ValidItem)
        {
            ValidItem = true;

            Delay(1);

            // Pick an item
            Category = (GetCVar("drpg_shopspecial_category") >= 0 ? GetCVar("drpg_shopspecial_category") : Random(0, (ItemCategories - 1)));
            Index = Random(0, ItemMax[Category] - 1);

            // We're not having any luck meeting the criteria, or the min/max value are impossible values to meet (0 / 0)
            // Break and give up on generating a shop special this time
            if (Tries > 100 || (MinValue == 0 && MaxValue == 0))
            {
                ValidItem = false;
                break;
            }

            // Retry if you land on an item with no cost
            if (ItemData[Category][Index].Price == 0)
            {
                ValidItem = false;
                Tries++;
                continue;
            }

            // Skip Loot category entirely
            if (Category == 7)
            {
                ValidItem = false;
                Tries++;
                continue;
            }

            // Make sure the item is within the valid price range
            if (ItemData[Category][Index].Price < MinValue || ItemData[Category][Index].Price > MaxValue)
            {
                ValidItem = false;
                Tries++;
                continue;
            }

            // Item is valid, continue
            if (ValidItem)
                break;
        }

        if (ValidItem)
        {
            // Assign the item to the Shop Special item
            ShopSpecialItem = &ItemData[Category][Index];

            // Spawn the item if you're in the Outpost
            SpawnShopSpecialItem();
        }

        // Reset the timer and bought status
        ShopSpecialTimer = 35 * 60 * GetCVar("drpg_shopspecial_time");
        ShopSpecialBought = false;

        if (DebugLog)
        {
            if (ShopSpecialItem == GetBlankItem())
                Log("\CdDEBUG: Shop Special expired! \CaNo new item generated");
            else
                Log("\CdDEBUG: Shop Special expired! Now \Cj%S", ShopSpecialItem->Name);
        }
    }

    // Remove the item if it was bought
    if (CurrentLevel->UACBase && ShopSpecialBought)
        Thing_Remove(ShopSpecialTID + 1);

    // Decrease the timer
    ShopSpecialTimer--;

    Delay(1);
    goto Start;
}

// Handles the Items dynamic array and associated behavior
NamedScript void ItemHandler()
{
    fixed Dist, Height, Divisor, Angle, Pitch, Amount, X, Y, Z;
    int ItemTID;
    bool NoClip;

    // Create the Items Array
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        ItemTIDs[i] = -1;

        if ((i % 15000) == 0)
            Delay(1);
    }

    // Items array initialization complete
    ItemTIDsInitialized = true;

    while (ItemTIDsInitialized && !CurrentLevel->UACBase)
    {
        for (int i = 0; i < MAX_ITEMS; i++)
        {
            // We've hit the end of the array
            if (ItemTIDs[i] == -1) break;

            // Invalid item, continue
            if (ItemTIDs[i] == 0) continue;

            // This item was picked up or otherwise removed
            if (ClassifyActor(ItemTIDs[i]) == ACTOR_NONE || ClassifyActor(ItemTIDs[i]) == ACTOR_WORLD) continue;

            NoClip = false;
            // Iterate players and check distances
            for (int j = 0; j < MAX_PLAYERS; j++)
            {
                // Skip this player if they aren't in the game
                if (!PlayerInGame(j)) continue;

                // Skip this player if they aren't Magnetic
                if (Players(j).Stim.PowerupTimer[STIM_MAGNETIC] <= 0) continue;

                // Skip this player if they are dead
                if (GetActorProperty(Players(j).TID, APROP_Health) <= 0) continue;

                Dist = Distance(ItemTIDs[i], Players(j).TID);
                Height = GetActorPropertyFixed(Players(j).TID, APROP_Height);
                Divisor = (Dist - 16.0) + Dist;
                Angle = VectorAngle (GetActorX(Players(j).TID) - GetActorX(ItemTIDs[i]), GetActorY(Players(j).TID) - GetActorY(ItemTIDs[i]));
                Pitch = VectorAngle (Dist, GetActorZ(Players(j).TID) - GetActorZ(ItemTIDs[i]));

                // Calculate the amount based on close versus far distance
                if (Dist < 16.0)
                    Amount = 1.0;
                else
                    Amount = 16.0 / Divisor;

                // Calculate the lerped positions
                X = (Amount * 16.0) * Cos(Angle) * Cos(Pitch);
                Y = (Amount * 16.0) * Sin(Angle) * Cos(Pitch);
                Z = (Amount * 16.0) * Sin(Pitch);

                SetActorVelocity(ItemTIDs[i], X, Y, Z, true, false);
                NoClip = true;
            }

            // Enable/Disable clipping on this item
            if (NoClip)
                SetActorInventory(ItemTIDs[i], "DRPGItemNoClip", 1);
            else
                SetActorInventory(ItemTIDs[i], "DRPGItemNoClipOff", 1);
        }

        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            // Skip this player if they aren't in the game
            if (!PlayerInGame(i)) continue;

            // Skip this player if they aren't Magnetic
            if (Players(i).Stim.PowerupTimer[STIM_MAGNETIC] <= 0) continue;

            // Skip this player if they are dead
            if (GetActorProperty(Players(i).TID, APROP_Health) <= 0) continue;


            int *ItemTID = (int *)Player.DropTID.Data;

            for (int j = 0; j < Players(i).DropTID.Position; j++)
            {
                NoClip = false;
                if (ClassifyActor(ItemTID[j]) == ACTOR_NONE || ClassifyActor(ItemTID[j]) == ACTOR_WORLD) continue;

                Dist = Distance(ItemTID[j], Players(i).TID);
                Height = GetActorPropertyFixed(Players(i).TID, APROP_Height);
                Divisor = (Dist - 16.0) + Dist;
                Angle = VectorAngle (GetActorX(Players(i).TID) - GetActorX(ItemTID[j]), GetActorY(Players(i).TID) - GetActorY(ItemTID[j]));
                Pitch = VectorAngle (Dist, GetActorZ(Players(i).TID) - GetActorZ(ItemTID[j]));

                // Calculate the amount based on close versus far distance
                if (Dist < 16.0)
                    Amount = 1.0;
                else
                    Amount = 16.0 / Divisor;

                // Calculate the lerped positions
                X = (Amount * 16.0) * Cos(Angle) * Cos(Pitch);
                Y = (Amount * 16.0) * Sin(Angle) * Cos(Pitch);
                Z = (Amount * 16.0) * Sin(Pitch);

                SetActorVelocity(ItemTID[j], X, Y, Z, true, false);
                NoClip = true;

                if (NoClip)
                    SetActorInventory(ItemTID[j], "DRPGItemNoClip", 1);
                else
                    SetActorInventory(ItemTID[j], "DRPGItemNoClipOff", 1);
            }
        }

        Delay(4);
    }
}

// Initializes an item and adds it to the Items array
NamedScript DECORATE void ItemInit()
{
    int TID;

    // Delay while the Items array is being initialized
    while (!ItemTIDsInitialized) Delay(1);

    // [KS] Some items cease existing once they're added to inventory
    // If that's the case, return early so we don't add empty item info
    if (ClassifyActor(0) == ACTOR_WORLD)
        return;

    // Add to Items array
    for (int i = 0; i < MAX_ITEMS; i++)
        if (ItemTIDs[i] == -1)
        {
            //if (DebugLog)
            //    Log("\CdDEBUG: \C-Item \Cd%S\C- added (Index \Cd%d\C-)", GetActorClass(0), i);

            // Doesn't have a TID, so assign it one
            if (ActivatorTID() == 0)
            {
                TID = UniqueTID();
                ItemTIDs[i] = TID;
                Thing_ChangeTID(0, TID);
            }
            else
                ItemTIDs[i] = ActivatorTID();

            break;
        }
}

NamedScript Console void ItemDump()
{
    Log("\Cd  ===== ITEM ARRAY DATA =====");

    for (int i = 0; i < MAX_ITEMS; i++)
    {
        if (ItemTIDs[i] == -1) break;
        Log("\Cd%d\C-: \Cd%S", i, GetActorClass(ItemTIDs[i]));
    }

    Log("\Cd  ===== ITEM ARRAY DATA END =====");
}

// Dynamic Loot Generation System
NamedScript bool DynamicLootGeneratorCheckRemoval(int TID, fixed Z)
{
    bool Remove = true;

    for (int i = 0; ItemTIDs[i] != -1; i++)
    {
        // If this spot is blank, skip it
        if (ClassifyActor(ItemTIDs[i]) == ACTOR_WORLD) continue;

        // Randomly continue for variance
        if (Random(1, 4) == 1) continue;

        bool CanSee = CheckSight(ItemTIDs[i], TID, 0);

        // [KS] Don't spawn stuff on special-effects flats like skyfloor or blackness, or at extreme height differences, because more often than not those are used as void space filler or instakill floors.
        // Examples: Epic2 MAP14, SF2012 MAP02, some CC4 maps.
        bool BadFloor = (CheckActorFloorTexture(TID, "F_SKY1") || CheckActorFloorTexture(TID, "F_SKY2") || CheckActorFloorTexture(TID, "BLACK") || CheckActorFloorTexture(TID, "FBLACK") || CheckActorFloorTexture(TID, "ALLBLAKF"));

        if (CanSee && !BadFloor && Abs(Z - GetActorZ(ItemTIDs[i])) <= 24)
        {
            Remove = false;
            break;
        }
    }

    return Remove;
}

NamedScript OptionalArgs(1) void DynamicLootGenerator(str Actor, int MaxItems)
{
    LogMessage(StrParam("Running DynamicLootGenerator to create %d items of %S", MaxItems, Actor), LOG_DEBUG);
    fixed LowerX = GetActorX(0);
    fixed UpperX = GetActorX(0);
    fixed LowerY = GetActorY(0);
    fixed UpperY = GetActorY(0);
    int LevelNum = CurrentLevel->LevelNum;
    int Items = 0;
    int Iterations = 0;
    int NumItems = 0;

    // Initial delay to make sure all items have been added/initialized in the level
    while (!ItemTIDsInitialized) Delay(1);
    Delay(4);

    // Determine the max amount of items to create if it's not specifically specified
    if (MaxItems == 0)
    {
        MaxItems = Random(LevelNum / GameSkill(), LevelNum) + ((AveragePlayerLuck() + AveragePlayerLevel()) / (GameSkill() * 2));
        MaxItems *= GetCVarFixed("drpg_lootgen_factor");
    }

    if (MaxItems == 0)
    {
        if (DebugLog)
            Log("\CdDebug: \C-Skipped item generation.");
        return;
    }
    else if (MaxItems < 0)
    {
        // [KS] Negative luck? Na na nana na! *raspberry*
        if (CurrentLevel->Event == MAPEVENT_MEGABOSS)
            return;

        MaxItems = -MaxItems;
        Actor = "DRPGGenericMonsterDropper";
    }

    fixed ItemX, ItemY;

    // Find the furthest item from the player to determine some approximate boundaries to work with
    for (int i = 0; ItemTIDs[i] != -1; i++)
    {
        if (ClassifyActor(ItemTIDs[i]) == ACTOR_NONE) continue;

        NumItems = i + 1;

        ItemX = GetActorX(ItemTIDs[i]);
        ItemY = GetActorY(ItemTIDs[i]);

        if (LowerX > ItemX) LowerX = ItemX;
        if (UpperX < ItemX) UpperX = ItemX;
        if (LowerY > ItemY) LowerY = ItemY;
        if (UpperY < ItemY) UpperY = ItemY;

        if (i % 1000 == 0) Delay(1);
    }

    if (NumItems < 1) // [KS] If we can't find any possible positions to spawn anything, fuck it.
        return;

    int TID, A;
    fixed X, Y, Z;

    while (Items < MaxItems)
    {
        TID = UniqueTID();
        X = RandomFixed(LowerX, UpperX);
        Y = RandomFixed(LowerY, UpperY);
        Z = 0;
        A = 32 * Random(0, 7);

        // Try to keep Z near the floor
        SpawnForced("MapSpot", X, Y, 0, TID, 0);
        Z = GetActorFloorZ(TID);
        Thing_Remove(TID);

        bool Spawned = Spawn("MapSpotTall", X, Y, Z, TID, A);
        if (Spawned)
        {
            bool Remove = ScriptCall("DRPGZUtilities", "CheckActorInMap", TID) ? DynamicLootGeneratorCheckRemoval(TID, Z) : true;
            bool Visible = false;

            if (!Remove)
            {
                SetActorFlag(TID, "LOOKALLAROUND", true);
                for (int i = 0; i < MAX_PLAYERS; i++)
                {
                    if (CheckSight(TID, Players(i).TID, CSF_NOFAKEFLOORS))
                    {
                        Visible = true;
                        break;
                    }
                    else if (CheckSight(Players(i).TID, TID, CSF_NOFAKEFLOORS))
                    {
                        Visible = true;
                        break;
                    }
                }
            }
            Thing_Remove(TID);
            if (!Remove)
            {
                if (Actor == "DRPGGenericMonsterDropper")
                {
                    str SpawnMonster;
                    if (CurrentLevel != NULL && CurrentLevel->Event == MAPEVENT_ONEMONSTER)
                        SpawnMonster = GetMissionMonsterActor(CurrentLevel->SelectedMonster->Actor);
                    else if (GetCVar("drpg_monster_adaptive_spawns"))
                        SpawnMonster = Monsters[Random(1, CurrentLevel->MaxTotalMonsters)].Actor;
                    else
                        SpawnMonster = Actor;
                    if (!Visible && Spawn(SpawnMonster, X, Y, Z, TID, A))
                        Items++;
                }
                else if (Spawn(Actor, X, Y, Z, TID, A))
                    Items++;
            }
        }

        if (DebugLog)
        {
            HudMessage("\CfGenerating Loot\n\Cd%d \Cj/ \Cd%d\n\n\CdActor: \C-%S\n\CdIteration: %d\n\CiBoundaries: %.2k-%.2k, %.2k-%.2k\n\nX: %.2k\nY: %.2k\nZ: %.2k", Items, MaxItems, Actor, Iterations, LowerX, UpperX, LowerY, UpperY, X, Y, Z);
            EndHudMessage(HUDMSG_FADEOUT, MAKE_ID('L', 'O', 'O', 'T'), "White", 1.5, 0.8, 1.5, 0.5);
        }

        Iterations++;

        if (Iterations % 50 == 0) Delay(1);

        /*if (Iterations == 2000) // Trick to restart ourselves if we ran out of iterations
        {
            if (DebugLog)
                Log("\CdDebug: \C-Dynamic Loot Generation created \Cd%d\C- items of type \Cd%S\C- and is restarting to place more", Items, Actor);
            DynamicLootGenerator(Actor, MaxItems - Items);
            return;
        }*/
    }

    if (DebugLog)
        Log("\CdDebug: \C-Dynamic Loot Generation created \Cd%d\C- items of type \Cd%S", Items, Actor);
}

// Activate Focus Mode
NamedScript KeyBind void ToggleFocusMode()
{
    Player.Focusing = !Player.Focusing;

    if (Player.Focusing)
        FocusMode();
}

// Focus Mode Script
NamedScript void FocusMode()
{
    int RegenWindupSpeed = ((35 * 40) - ((35 * 35) * Player.RegenerationTotal / 200));
    if (RegenWindupSpeed < 35) // Enforce a 1-second wind-up at least
        RegenWindupSpeed = 35;
    int StartWindupSpeed = RegenWindupSpeed;
    int RegenDelay = (RegenWindupSpeed * (Player.EPTime / 4)) / StartWindupSpeed;
    int Percent;

    // [KS] Someone did this!
    if (GetActorProperty(0, APROP_Health) <= 0) return;

    // Return if you're already at max EP
    if (Player.EP >= Player.EPMax) return;

    SetPlayerProperty(0, 1, PROP_FROZEN);

    while (Player.Focusing)
    {
        Percent = 100 - (RegenWindupSpeed * 100 / StartWindupSpeed);
        fixed X = GetActorX(0) + Cos((fixed)Timer() / 64.0) * 32.0;
        fixed Y = GetActorY(0) + Sin((fixed)Timer() / 64.0) * 32.0;
        fixed Z = GetActorZ(0);

        if (GetActorProperty(0, APROP_Health) <= 0)
        {
            Player.Focusing = false;
            return;
        }

        Player.EPRate = 0;

        PlaySound(0, "misc/epfocus", CHAN_BODY, 0.5, true, ATTN_NORM);
        SpawnForced("DRPGEPFocusAura", X, Y, Z, 0, 0);
        DrawProgressBar("\CnFocusing", Percent);

        if (RegenWindupSpeed > 1)
            RegenWindupSpeed--;

        if (RegenDelay > 0)
            RegenDelay--;
        else
        {
            Player.EP += Player.EPAmount;
            if (Player.EP > Player.EPMax)
                Player.EP = Player.EPMax;
            RegenDelay = (RegenWindupSpeed * (Player.EPTime / 4)) / StartWindupSpeed;
        }

        // Moving and attacking interrupts focusing
        if (IsPlayerMoving() || Player.EP >= Player.EPMax || CheckInput(BT_ATTACK, KEY_HELD, true, -1) || CheckInput(BT_ALTATTACK, KEY_HELD, true, -1))
            Player.Focusing = false;

        Delay(1);
    }

    Player.Focusing = false; // So we can't gain Regen XP out of thin air
    PlaySound(0, "misc/epfocusdone", CHAN_BODY, 0.5, false, ATTN_NORM);
    SetPlayerProperty(0, 0, PROP_FROZEN);
}

// Death Script
NamedScript Type_DEATH void Dead()
{
    // Reset menu vars
    Player.InMenu = false;
    Player.InShop = false;
    Player.OutpostMenu = 0;

    // Remove Aura
    RemoveAura();

    // Remove Shield
    SetInventory("DRPGShield", 0);

    // Remove Status Effect
    for (int i = 0; i < SE_MAX; i++)
        Player.StatusTimer[i] = 0;

    // Drop Credits
    if (GetCVar("drpg_multi_dropcredits") && CheckInventory("DRPGCredits") > 0)
    {
        int DropAmount = CheckInventory("DRPGCredits") / 100 * GetCVar("drpg_multi_dropcredits_percent");

        // Cap out at a million so if you have stupid amounts of Credits you don't freeze/nuke the game
        if (DropAmount > 1000000) DropAmount = 1000000;

        TakeInventory("DRPGCredits", DropAmount);
        DropMoney(PlayerNumber(), 0, DropAmount);
    }

    // Drop Inventory
    if (GetCVar("drpg_multi_dropinv"))
        DropEntireInventory();

    // VENG-R Accessory
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        // Skip yourself
        if (Player.TID == Players(i).TID) continue;

        if (Players(i).Shield.Accessory && Players(i).Shield.Accessory->PassiveEffect == SHIELD_PASS_AVENGER)
        {
            GiveActorInventory(Players(i).TID, "DRPGShieldAvengerDamage", 1);
            GiveActorInventory(Players(i).TID, "DRPGShieldAvengerDefense", 1);
        }
    }

    if (GetCVar("drpg_multi_revives"))
    {
        // Incapacitation announcement
        SetHudSize(320, 200, false);
        SetFont("SMALLFONT");
        if (AlivePlayers() >= 1)
            HudMessage("%tS was incapacitated", PlayerNumber() + 1);
        else
            HudMessage("%tS has died", PlayerNumber() + 1);
        EndHudMessageBold(HUDMSG_FADEOUT, 0, "Brick", 160.0, 140.0, 1.5, 1.0);

        // Remember body location
        Player.BodyTID = GetUniqueTID();

        // Assign TID to player's body
        Thing_ChangeTID(Player.TID, Player.BodyTID);
        Delay(1);

        // Actualize the actual health
        Player.ActualHealth = GetActorProperty(0, APROP_Health);
    }
    else
    {
        // Remove TID
        Thing_ChangeTID(Player.TID, 0);
    }
}

// Respawn
NamedScript Type_RESPAWN void Respawn()
{
    // Reassign TID
    if (CompatMode == COMPAT_DRLA)
        Delay(4);
    AssignTIDs();

    // Heal to max health if revives are disabled or revive at the body's location
    if (!GetCVar("drpg_multi_revives"))
        Player.ActualHealth = Player.HealthMax;
    else if (Player.BodyTID != 0)
    {
        SetActorPosition(Player.TID, GetActorX(Player.BodyTID), GetActorY(Player.BodyTID), GetActorZ(Player.BodyTID), 0);
        SetActorAngle(Player.TID, GetActorAngle(Player.BodyTID));
        Thing_Remove(Player.BodyTID);
        Player.BodyTID = 0;
    }
    SetActorProperty(0, APROP_Health, Player.ActualHealth);

    // XP/Rank Penalty
    if (GetCVar("drpg_multi_takexp"))
    {
        long int XPPenalty = (long int)(XPTable[Player.Level] * GetCVar("drpg_multi_takexp_percent") / 100);
        long int RankPenalty = (long int)(RankTable[Player.RankLevel] * GetCVar("drpg_multi_takexp_percent") / 100);

        if (XPPenalty > 0 || RankPenalty > 0)
        {
            Player.XP -= XPPenalty;
            Player.Rank -= RankPenalty;
            SetFont("BIGFONT");
            HudMessage("\CjXP -%d\n\CkRank -%d", XPPenalty, RankPenalty);
            EndHudMessage(HUDMSG_FADEOUT | HUDMSG_LOG, 0, "White", 1.5, 0.75, 2.0, 2.0);
        }
    }

    // Restore EP if CVAR is set
    if (GetCVar("drpg_multi_restoreep"))
        Player.EP = Player.EPMax;

    // Give a box of ammo if a specific ammo type is empty if the CVAR is set
    if (GetCVar("drpg_multi_restoreammo"))
    {
        if (CheckInventory("Clip") < GetAmmoAmount("Clip") * (Player.CapacityTotal / 10))
            SetInventory("Clip", GetAmmoAmount("Clip") * (Player.CapacityTotal / 10));
        if (CheckInventory("Shell") < GetAmmoAmount("Shell") * (Player.CapacityTotal / 10))
            SetInventory("Shell", GetAmmoAmount("Shell") * (Player.CapacityTotal / 10));
        if (CheckInventory("RocketAmmo") < GetAmmoAmount("RocketAmmo") * (Player.CapacityTotal / 10))
            SetInventory("RocketAmmo", GetAmmoAmount("RocketAmmo") * (Player.CapacityTotal / 10));
        if (CheckInventory("Cell") < GetAmmoAmount("Cell") * (Player.CapacityTotal / 10))
            SetInventory("Cell", GetAmmoAmount("Cell") * (Player.CapacityTotal / 10));
    }

    // Apply camera textures and vars
    SetCameraToTexture(Player.TID, StrParam("P%iVIEW", PlayerNumber() + 1), 110);
    SetCameraToTexture(Player.TID, StrParam("P%iSVIEW", PlayerNumber() + 1), 90);
    Player.PlayerView = PlayerNumber();

    // Run Scripts
    Loop();
    PlayerHealth();
    MoneyChecker();
    DamageNumbers();
    InfoPopoffs();
    HealthBars();
    ShieldTimer();

    // NU-YU Accessory
    if (Player.Shield.Accessory && Player.Shield.Accessory->PassiveEffect == SHIELD_PASS_HYPERION)
    {
        ActivatorSound("shield/newu", 127);
        Player.Shield.Active = true;
        Player.Shield.Charge = Player.Shield.Capacity;
        GiveInventory("DRPGShieldNewUProtection", 1);
    }

    // Quick Teleport
    if (CheckInput(BT_SPEED, KEY_HELD, false, PlayerNumber()))
    {
        int PlayerNum = -1;
        while (PlayerNum == -1 || PlayerNum == PlayerNumber() || !PlayerInGame(PlayerNum))
            PlayerNum = Random(0, PlayerCount());
        PlayerTeleport(PlayerNum);
        Player.EP -= ScaleEPCost(50);
    }
}

str const Loadout_WeaponInfo[LOADOUT_WEAPONS][3] =
{
    // CVAR, Vanilla Actor, Extras Actor

    { "drpg_start_weapon_chainsaw",         "DRPGChainsaw",         "DRPGChainsawExtras",       },
    { "drpg_start_weapon_pistol",           "DRPGPistol",           "DRPGPistolExtras",         },
    { "drpg_start_weapon_shotgun",          "DRPGShotgun",          "DRPGShotgunExtras",        },
    { "drpg_start_weapon_ssg",              "DRPGSuperShotgun",     "DRPGSuperShotgunExtras",   },
    { "drpg_start_weapon_chaingun",         "DRPGChaingun",         "DRPGChaingunExtras",       },
    { "drpg_start_weapon_rocketlauncher",   "DRPGRocketLauncher",   "DRPGRocketLauncherExtras", },
    { "drpg_start_weapon_plasmarifle",      "DRPGPlasmaRifle",      "DRPGPlasmaRifleExtras",    },
    { "drpg_start_weapon_bfg",              "DRPGBFG9000",          "DRPGBFG9000Extras",        },
};
str const Loadout_ArmorInfo[LOADOUT_ARMORS] =
{
    "DRPGGreenArmorEffect",
    "DRPGBlueArmorEffect",
    "DRPGYellowArmorEffect",
    "DRPGRedArmorEffect",
    "DRPGWhiteArmorEffect",
    "DRPGReinforcedGreenArmorEffect",
    "DRPGReinforcedBlueArmorEffect",
    "DRPGReinforcedYellowArmorEffect",
    "DRPGReinforcedRedArmorEffect",
    "DRPGReinforcedWhiteArmorEffect",
};
str const Loadout_ShieldPartColorCodes[LOADOUT_SHIELDPARTS] =
{
    "\Cv",  // Capacity +
    "\Ch",  // Capacity -
    "\Cd",  // Charge Rate +
    "\Cq",  // Charge Rate -
    "\Cr",  // Delay Rate +
    "\Cg",  // Delay Rate -
};
str const Loadout_ShieldAccessoryColorCodes[LOADOUT_SHIELDACCS] =
{
    "\Cv",  // Shield Capacity
    "\Cg",  // Strength / Damage
    "\Ca",  // Vitality / Health
    "\Ci",  // Agility / Speed
    "\Cd",  // Defense / Armor / Shield Charge
    "\Cf",  // Luck / Money
    "\Cn",  // Energy
    "\Ch",  // Capacity / Ammo
    "\Ck",  // Rank
    "\Cc",  // Augmentation
    "\Cq",  // Powerup
    "\Cj",  // Misc / Multi-Category
};
str const Loadout_DRLAWeaponColorCodes[LOADOUT_DRLAWEAPONS] =
{
    "",     // Common
    "\Cv",  // Assembled
    "\Ct",  // Exotic
    "\Ci",  // Superior
    "\Cd",  // Unique
    "\Cg",  // Demonic
    "\Cf",  // Legendary
};
str const Loadout_DRLAArmorColorCodes[LOADOUT_DRLAARMORS] =
{
    "",     // Common
    "\Cv",  // Assembled
    "\Ct",  // Exotic
    "\Cc",  // Onyx
    "\Cd",  // Unique
    "\Cf",  // Legendary
};
str const Loadout_DRLAModPacks[LOADOUT_DRLAMODPACKS] =
{
    "RLPowerModItem",
    "RLBulkModItem",
    "RLAgilityModItem",
    "RLTechnicalModItem",
    "RLSniperModItem",
    "RLFirestormModItem",
    "RLNanoModItem",
    "RLOnyxModItem",
    // RLArtiModItem is unnecessary since it provides random modpacks from above
};

// Shield Databases
static ItemInfoPtr Loadout_ShieldBodyTypes[LOADOUT_SHIELDPARTS][MAX_BODIES];
static ItemInfoPtr Loadout_ShieldBatteryTypes[LOADOUT_SHIELDPARTS][MAX_BATTERIES];
static ItemInfoPtr Loadout_ShieldCapacitorTypes[LOADOUT_SHIELDPARTS][MAX_CAPACITORS];
static ItemInfoPtr Loadout_ShieldAccessoryTypes[LOADOUT_SHIELDACCS][MAX_ACCESSORIES];
static int Loadout_BodyCount[LOADOUT_SHIELDPARTS];
static int Loadout_BatteryCount[LOADOUT_SHIELDPARTS];
static int Loadout_CapacitorCount[LOADOUT_SHIELDPARTS];
static int Loadout_AccessoryCount[LOADOUT_SHIELDACCS];

// DRLA Databases
static ItemInfoPtr Loadout_DRLAWeaponTypes[LOADOUT_DRLAWEAPONS][ITEM_MAX];
static ItemInfoPtr Loadout_DRLAArmorTypes[LOADOUT_DRLAARMORS][ITEM_MAX];
static ItemInfoPtr Loadout_DRLABootsTypes[LOADOUT_DRLAARMORS][ITEM_MAX];
static int Loadout_DRLAWeaponCount[LOADOUT_DRLAWEAPONS];
static int Loadout_DRLAArmorCount[LOADOUT_DRLAARMORS];
static int Loadout_DRLABootsCount[LOADOUT_DRLAARMORS];

NamedScript void SortStartingItems()
{
    // Build Shield and DRLA item databases based on their respective items' color codes
    for (int i = 0; i < MAX_BODIES; i++)
        for (int j = 0; j < LOADOUT_SHIELDPARTS; j++)
            if (Contains(ShieldParts[0][i].Name, Loadout_ShieldPartColorCodes[j]))
            {
                Loadout_ShieldBodyTypes[j][Loadout_BodyCount[j]++] = FindItemInCategory(ShieldParts[0][i].Actor, 5);
                break;
            }
    for (int i = 0; i < MAX_BATTERIES; i++)
        for (int j = 0; j < LOADOUT_SHIELDPARTS; j++)
            if (Contains(ShieldParts[1][i].Name, Loadout_ShieldPartColorCodes[j]))
            {
                Loadout_ShieldBatteryTypes[j][Loadout_BatteryCount[j]++] = FindItemInCategory(ShieldParts[1][i].Actor, 5);
                break;
            }
    for (int i = 0; i < MAX_CAPACITORS; i++)
        for (int j = 0; j < LOADOUT_SHIELDPARTS; j++)
            if (Contains(ShieldParts[2][i].Name, Loadout_ShieldPartColorCodes[j]))
            {
                Loadout_ShieldCapacitorTypes[j][Loadout_CapacitorCount[j]++] = FindItemInCategory(ShieldParts[2][i].Actor, 5);
                break;
            }
    for (int i = 0; i < MAX_ACCESSORIES; i++)
        for (int j = 0; j < LOADOUT_SHIELDACCS; j++)
            if (Contains(ShieldAccessories[i].Name, Loadout_ShieldAccessoryColorCodes[j]))
            {
                Loadout_ShieldAccessoryTypes[j][Loadout_AccessoryCount[j]++] = FindItemInCategory(ShieldAccessories[i].Actor, 5);
                break;
            }
    for (int i = 0; i < ItemMax[0]; i++)
        for (int j = 1; j < LOADOUT_DRLAWEAPONS; j++)
        {
            if (i < 10) // Special catch for Common
            {
                Loadout_DRLAWeaponTypes[0][Loadout_DRLAWeaponCount[0]++] = &ItemData[0][i];
                break;
            }
            else if (Contains(ItemData[0][i].Name, Loadout_DRLAWeaponColorCodes[j]))
            {
                Loadout_DRLAWeaponTypes[j][Loadout_DRLAWeaponCount[j]++] = &ItemData[0][i];
                break;
            }
        };
    for (int i = 1; i < ItemMax[3]; i++)
        for (int j = 1; j < LOADOUT_DRLAARMORS; j++)
        {
            // Onyx is handled below
            if (j == 3) continue;

            if (i < 4) // Special catch for Common
            {
                Loadout_DRLAArmorTypes[0][Loadout_DRLAArmorCount[0]++] = &ItemData[3][i];
                break;
            }
            else if (Contains(ItemData[3][i].Name, "Onyx")) // Special catch for Onyx
            {
                Loadout_DRLAArmorTypes[3][Loadout_DRLAArmorCount[3]++] = &ItemData[3][i];
                break;
            }
            else if (Contains(ItemData[3][i].Name, Loadout_DRLAArmorColorCodes[j]))
            {
                Loadout_DRLAArmorTypes[j][Loadout_DRLAArmorCount[j]++] = &ItemData[3][i];
                break;
            }
        }
    for (int i = 0; i < ItemMax[9]; i++)
        for (int j = 1; j < LOADOUT_DRLAARMORS; j++)
        {
            if (i < 3) // Special catch for Common
            {
                Loadout_DRLABootsTypes[0][Loadout_DRLABootsCount[0]++] = &ItemData[9][i];
                break;
            }
            else if (Contains(ItemData[9][i].Name, Loadout_DRLAArmorColorCodes[j]))
            {
                Loadout_DRLABootsTypes[j][Loadout_DRLABootsCount[j]++] = &ItemData[9][i];
                break;
            }
        }
}

// Gives the player their default loadout based on their starting options
NamedScript void Loadout_GiveAugs()
{
    // Active Augs
    int ActiveAugs = 0;
    int Type;

    while (ActiveAugs < GetActivatorCVar("drpg_start_aug_amount"))
    {
        Type = Random(0, AUG_MAX - 1);

        // Skip this aug if it's already active
        if (Player.Augs.Level[Type] > 0) continue;

        Player.Augs.Level[Type]++;
        ActiveAugs++;
    }

    // Aug Canisters/Upgrades
    GiveInventory("DRPGAugCanister", GetActivatorCVar("drpg_start_aug_canisters"));
    GiveInventory("DRPGAugUpgradeCanister", GetActivatorCVar("drpg_start_aug_upgrades"));

    // Aug Slots
    Player.Augs.BaseSlots = GetActivatorCVar("drpg_start_aug_slots");
}

NamedScript void Loadout_GiveShieldParts()
{
    // Shield Parts
    for (int i = 0; i < 3; i++)
    {
        int *Count[3] =
        {
            &Loadout_BodyCount[0],
            &Loadout_BatteryCount[0],
            &Loadout_CapacitorCount[0]
        };
        int Total[3] =
        {
            GetActivatorCVar("drpg_start_shield_amount_body"),
            GetActivatorCVar("drpg_start_shield_amount_battery"),
            GetActivatorCVar("drpg_start_shield_amount_capacitor")
        };
        int Current[3];

        // Calculate total if we specify a type so that overflows or explosions don't happen
        if (GetActivatorCVar("drpg_start_shield_type") >= 0)
            if (Total[i] > Count[i][GetActivatorCVar("drpg_start_shield_type")])
                Total[i] = Count[i][GetActivatorCVar("drpg_start_shield_type")];

        while (Current[i] < Total[i])
        {
            int Category = Random(0, LOADOUT_SHIELDPARTS - 1);

            // Force the category type if the CVAR is set
            if (GetActivatorCVar("drpg_start_shield_type") >= 0)
                Category = GetActivatorCVar("drpg_start_shield_type");

            for (int j = 0; j < Count[i][Category]; j++)
            {
                ItemInfoPtr *Type[3];

                Type[0] = &Loadout_ShieldBodyTypes[Category][j];
                Type[1] = &Loadout_ShieldBatteryTypes[Category][j];
                Type[2] = &Loadout_ShieldCapacitorTypes[Category][j];

                if (!CheckInventory((*(Type[i]))->Actor))
                {
                    GiveInventory((*(Type[i]))->Actor, 1);
                    Current[i]++;
                    break;
                }
            }
        }
    }
}

NamedScript void Loadout_GiveShieldAccessories()
{
    // Shield Accessories
    int TotalAccessories = GetActivatorCVar("drpg_start_shield_amount_acc");
    int Accessories = 0;
    int Category;
    if (GetActivatorCVar("drpg_start_shield_type_acc") >= 0)
        if (TotalAccessories > Loadout_AccessoryCount[GetActivatorCVar("drpg_start_shield_type_acc")])
            TotalAccessories = Loadout_AccessoryCount[GetActivatorCVar("drpg_start_shield_type_acc")];

    while (Accessories < TotalAccessories)
    {
        Category = Random(0, LOADOUT_SHIELDACCS - 1);

        // Force the category type if the CVAR is set
        if (GetActivatorCVar("drpg_start_shield_type_acc") >= 0)
            Category = GetActivatorCVar("drpg_start_shield_type_acc");

        for (int i = 0; i < Loadout_AccessoryCount[Category]; i++)
        {
            ItemInfoPtr Type = Loadout_ShieldAccessoryTypes[Category][i];

            if (!CheckInventory(Type->Actor))
            {
                GiveInventory(Type->Actor, 1);
                Accessories++;
                break;
            }
        }
    }
}

NamedScript void Loadout_GiveStims()
{
    // Stims
    int Injectors = 0;

    while (Injectors < GetActivatorCVar("drpg_start_stim_injectors"))
    {
        switch (Random(1, 4))
        {
        case 1: // Small
            if (CheckInventory("DRPGStimSmall") >= 1000) continue;
            GiveInventory("DRPGStimSmall", 1);
            break;
        case 2: // Medium
            if (CheckInventory("DRPGStimMedium") >= 1000) continue;
            GiveInventory("DRPGStimMedium", 1);
            break;
        case 3: // Large
            if (CheckInventory("DRPGStimLarge") >= 1000) continue;
            GiveInventory("DRPGStimLarge", 1);
            break;
        case 4: // XL
            if (CheckInventory("DRPGStimXL") >= 1000) continue;
            GiveInventory("DRPGStimXL", 1);
            break;
        }

        Injectors++;
    }
}

NamedScript void Loadout_GiveVials()
{
    int Vials = 0;
    int MaxVials = GetActivatorCVar("drpg_start_stim_vials");
    if (MaxVials > 3000) MaxVials = 3000;
    int Capacity = Player.Capacity * 10;
    int Types = 8;
    if (GetActivatorCVar("drpg_start_stim_boosters")) Types += 2;
    if (GetActivatorCVar("drpg_start_stim_powerups")) Types += 9;
    if (MaxVials > Types * Capacity) MaxVials = Types * Capacity;
    int Type;

    while (Vials < MaxVials)
    {
        Type = Random(0, STIM_MAX - 1);

        // Don't include Boosters
        if (!GetActivatorCVar("drpg_start_stim_boosters") && Type >= StimStatsEnd && Type <= StimPowerupStart) continue;

        // Don't include Powerups
        if (!GetActivatorCVar("drpg_start_stim_powerups") && Type >= StimPowerupStart && Type <= StimPowerupEnd) continue;

        // Skip this one if this vial type is full
        if (Player.Stim.Vials[Type] >= Capacity) continue;

        Player.Stim.Vials[Type]++;
        Vials++;
    }
}

// DRLA
NamedScript void Loadout_GiveDRLAEquipment()
{
    bool FoundWeapon = false;
    bool FoundArmor = false;
    bool FoundBoots = false;
    int WeaponCount = 0;
    int MaxWeaponCount = GetActivatorCVar("drpg_start_drla_weapon_amount");
    int ModPackCount = 0;
    int ModPackMax = GetActivatorCVar("drpg_start_drla_modpacks");

    // Weapons
    if (GetActivatorCVar("drpg_start_drla_weapon_type") > 0)
        if (MaxWeaponCount > Loadout_DRLAWeaponCount[GetActivatorCVar("drpg_start_drla_weapon_type")])
            MaxWeaponCount = Loadout_DRLAWeaponCount[GetActivatorCVar("drpg_start_drla_weapon_type")];
    while (WeaponCount < MaxWeaponCount)
    {
        for (int i = 0; i < LOADOUT_DRLAWEAPONS; i++)
        {
            FoundWeapon = false;

            // Force the weapon type if the CVAR is set
            if (GetActivatorCVar("drpg_start_drla_weapon_type") >= 0)
                i = GetActivatorCVar("drpg_start_drla_weapon_type");

            for (int j = 0; j < Loadout_DRLAWeaponCount[i]; j++)
                if (Random(1, 10) == 1 && !CheckInventory(Loadout_DRLAWeaponTypes[i][j]->Actor))
                {
                    GiveInventory(Loadout_DRLAWeaponTypes[i][j]->Actor, 1);
                    WeaponCount++;
                    FoundWeapon = true;
                    break;
                }

            if (FoundWeapon)
                break;
        }
    }
    GiveInventory("RLWeaponLimit", WeaponCount);

    // Armor
    if (GetActivatorCVar("drpg_start_drla_armor") >= -1)
        while (!FoundArmor)
        {
            for (int i = 0; i < LOADOUT_DRLAARMORS; i++)
            {
                // If there is nothing in this category, skip it
                if (GetActivatorCVar("drpg_start_drla_armor") >= 0 && Loadout_DRLAArmorCount[GetActivatorCVar("drpg_start_drla_armor")] == 0)
                {
                    FoundArmor = true;
                    continue;
                }
                else if (GetActivatorCVar("drpg_start_drla_armor") >= 0) // Force the armor type if the CVAR is setup
                    i = GetActivatorCVar("drpg_start_drla_armor");

                for (int j = 0; j < Loadout_DRLAArmorCount[i]; j++)
                    if (Random(1, 10) == 1)
                    {
                        GiveInventory(Loadout_DRLAArmorTypes[i][j]->Actor, 1);
                        FoundArmor = true;
                        break;
                    }

                if (FoundArmor)
                    break;
            }
        }

    // Boots
    if (GetActivatorCVar("drpg_start_drla_boots") >= -1)
        while (!FoundBoots)
        {
            for (int i = 0; i < LOADOUT_DRLAARMORS; i++)
            {
                // If there is nothing in this category, skip it
                if (GetActivatorCVar("drpg_start_drla_boots") >= 0 && Loadout_DRLABootsCount[GetActivatorCVar("drpg_start_drla_boots")] == 0)
                {
                    FoundBoots = true;
                    continue;
                }
                else if (GetActivatorCVar("drpg_start_drla_boots") >= 0) // Force the boots type if the CVAR is setup
                    i = GetActivatorCVar("drpg_start_drla_boots");

                for (int j = 0; j < Loadout_DRLABootsCount[i]; j++)
                    if (Random(1, 10) == 1)
                    {
                        GiveInventory(Loadout_DRLABootsTypes[i][j]->Actor, 1);
                        FoundBoots = true;
                        break;
                    }

                if (FoundBoots)
                    break;
            }
        }

    // Mod Packs
    int Type;
    if (PlayerClass(0) == 2)
        ModPackMax--;
    while (ModPackCount < ModPackMax)
    {
        Type = Random(0, LOADOUT_DRLAMODPACKS - 1);

        // Stop if we're not the technician and we try and give over 4 mods
        if (PlayerClass(0) != 2 && ModPackCount >= 4) break;

        // Skip exotic modpacks if the selection is disabled
        if (Type > 3 && !GetActivatorCVar("drpg_start_drla_modpacks_amount")) continue;

        GiveInventory(Loadout_DRLAModPacks[Type], 1);
        ModPackCount++;
    }
}

NamedScript void DefaultLoadout()
{
    // Weapons
    if (CompatMode != COMPAT_DRLA && CompatMode != COMPAT_LEGENDOOM)
    {
        for (int i = 0; i < 8; i++)
        {
            if (GetActivatorCVar(Loadout_WeaponInfo[i][0]))
            {
                if (CompatMode == COMPAT_EXTRAS)
                {
                    GiveInventory(Loadout_WeaponInfo[i][2], 1);
                    SetWeapon(Loadout_WeaponInfo[i][2]);
                }
                else
                {
                    GiveInventory(Loadout_WeaponInfo[i][1], 1);
                    SetWeapon(Loadout_WeaponInfo[i][1]);
                }
            }
            else
            {
                if (CompatMode == COMPAT_EXTRAS)
                    TakeInventory(Loadout_WeaponInfo[i][2], 1);
                else
                    TakeInventory(Loadout_WeaponInfo[i][1], 1);
            }
        }
    }

    // Ammo
    SetInventory("Clip", GetActivatorCVar("drpg_start_ammo_bullet"));
    SetInventory("Shell", GetActivatorCVar("drpg_start_ammo_shell"));
    SetInventory("RocketAmmo", GetActivatorCVar("drpg_start_ammo_rocket"));
    SetInventory("Cell", GetActivatorCVar("drpg_start_ammo_cell"));

    // Armor
    if (CompatMode != COMPAT_DRLA && GetActivatorCVar("drpg_start_armor") >= 0)
        GiveInventory(Loadout_ArmorInfo[GetActivatorCVar("drpg_start_armor")], 1);

    // Medkit
    Player.Medkit = GetActivatorCVar("drpg_start_medkit");

    Loadout_GiveAugs();
    Loadout_GiveShieldParts();
    Loadout_GiveShieldAccessories();
    Loadout_GiveStims();
    Loadout_GiveVials();

    // Minigame Chips
    GiveInventory("DRPGChipGold", GetActivatorCVar("drpg_start_bonus_goldchips"));
    GiveInventory("DRPGChipPlatinum", GetActivatorCVar("drpg_start_bonus_platchips"));

    // UAC Shop Card
    if (GetActivatorCVar("drpg_start_bonus_shopcard") > 0)
    {
        // Diamond Card
        if (GetActivatorCVar("drpg_start_bonus_shopcard") >= 5)
            GiveInventory("DRPGDiamondUACCard", 1);
        else
            GiveInventory("DRPGUACCard", GetActivatorCVar("drpg_start_bonus_shopcard"));
    }

    if (CompatMode == COMPAT_DRLA)
        Loadout_GiveDRLAEquipment();
}

// Apply values to global vars visible on the HUD
void CheckHUD()
{
    int PlayerNum = PlayerNumber();

    // EP
    EP[PlayerNum] = Player.EP;

    // EP Bar on HUD
    if (Player.EPMax <= 0)
        SetInventory("DRPGEP", 0);
    else
        SetInventory("DRPGEP", Player.EP * 100 / Player.EPMax);

    // Shield
    if (Player.Shield.Capacity > 0)
    {
        Shield[PlayerNum] = Player.Shield.Charge;
        ShieldCapacity[PlayerNum] = Player.Shield.Capacity;
        ShieldHealth[PlayerNum] = Player.ActualHealth;
        SetInventory("DRPGShieldCapacity", (int)(((fixed)Player.Shield.Charge / (fixed)Player.Shield.Capacity) * 100.0));
        SetInventory("DRPGShield", Player.Shield.Active);
    }
}

void CheckKeys()
{
    str const Keycards[6] =
    {
        "DRPGRedCard",
        "DRPGYellowCard",
        "DRPGBlueCard",
        "DRPGRedSkull",
        "DRPGYellowSkull",
        "DRPGBlueSkull"
    };

    // Don't bother if we're not in multiplayer
    if (!InMultiplayer) return;

    for (int i = 0; i < 6; i++)
    {
        if (CheckInventory(Keycards[i]))
        {
            for (int j = 0; j < MAX_PLAYERS; j++)
            {
                if (!PlayerInGame(j)) continue;
                if (CheckActorInventory(Players(j).TID, Keycards[i])) continue;

                SetActorInventory(Players(j).TID, Keycards[i], 1);
            }
        }
    }
}

void CheckCompatibility()
{
    bool Success = false;
    int TID = UniqueTID();

    if (DebugLog)
        Log("\CdDEBUG: \C-Checking Compatibility...");

    SkillLevelsMax = MAX_SKILLLEVELS_DF;

    CompatMode = COMPAT_NONE;
    CompatMonMode = COMPAT_NONE;

    MonsterData = MonsterDataDF;
    MonsterDataAmount = MAX_DEF_MONSTERS_DF;

    MegaBosses = MegaBossesDF;
    MegaBossesAmount = MAX_MEGABOSSES_DF;

    WadSmoosh = false;

    // WadSmoosh
    Success = SpawnForced("DRPGWadSmooshActive", 0, 0, 0, TID, 0);
    if (Success)
    {
        if (DebugLog)
            Log("\CdDEBUG: \CaWadSmoosh\C- detected");
        WadSmoosh = true;
        Thing_Remove(TID);
    }

    // Extras
    Success = SpawnForced("DRPGExtrasIsLoaded", 0, 0, 0, TID, 0);
    if (Success)
    {
        if (DebugLog)
            Log("\CdDEBUG: \CaExtras\C- detected");
        CompatMode = COMPAT_EXTRAS;
        Thing_Remove(TID);
    }

    // LegenDoom
    Success = SpawnForced("LDLegendaryZombie", 0, 0, 0, TID, 0);
    if (Success)
    {
        if (DebugLog)
            Log("\CdDEBUG: \CdLegenDoom\C- detected");

        CompatMode = COMPAT_LEGENDOOM;
        CompatMonMode = COMPAT_LEGENDOOM;
        MonsterData = MonsterDataLD;
        Thing_Remove(TID);
    }

    // DoomRL Arsenal
    Success = SpawnForced("RLPistolPickup", 0, 0, 0, TID, 0);
    if (Success)
    {
        if (DebugLog)
            Log("\CdDEBUG: \CdDoomRL \C-detected");

        CompatMode = COMPAT_DRLA;
        SetInventory("DRPGDRLAActive", 1);
        Thing_Remove(TID);
    }

    // DoomRL Monsters
    Success = SpawnForced("RLBaronOfHell", 0, 0, 0, TID, 0);
    if (Success)
    {
        if (DebugLog)
            Log("\CdDEBUG: \CdDoomRL Monsters \C-detected");

        SkillLevelsMax = MAX_SKILLLEVELS_DRLA;
        CompatMonMode = COMPAT_DRLA;
        MonsterData = MonsterDataDRLA;
        MonsterDataAmount = MAX_DEF_MONSTERS_DRLA;
        Thing_Remove(TID);
    }

    // Colourful Hell
    Success = SpawnForced("RedZombie", 0, 0, 0, TID, 0);
    if (Success)
    {
        if (DebugLog)
            Log("\CdDEBUG: \CdColourful Hell \C-detected");

        CompatMonMode = COMPAT_CH;
        MonsterData = MonsterDataCH;
        MonsterDataAmount = MAX_DEF_MONSTERS_CH;
        MegaBosses = MegaBossesCH;
        MegaBossesAmount = MAX_MEGABOSSES_CH;
        Thing_Remove(TID);
    }

    if (DebugLog && CompatMode == COMPAT_NONE && CompatMonMode == COMPAT_NONE && !WadSmoosh)
        Log("\CdDEBUG: \C-No compatible mods found");
}

void AssignTIDs()
{
    Player.TID = PLAYER_TID + PlayerNumber();
    Thing_ChangeTID(0, Player.TID);

    if (DebugLog)
        Log("\CdDEBUG: Player TID: %d", Player.TID);
}

NamedScript void ReviveHandler()
{
    for (int i = 0; i < PlayerCount(); i++)
    {
        if (i != PlayerNumber() && Players(i).ActualHealth <= 0 && Players(i).BodyTID > 0 && Distance(Player.TID, Players(i).BodyTID) < 48)
        {
            if ((!Player.InMenu && !Player.InShop && !Player.OutpostMenu && !Player.CrateOpen) && !Player.MenuBlock)
            {
                SetHudSize(0, 0, false);
                SetFont("BIGFONT");
                if (Players(i).Reviver == -1 || Players(i).Reviver == PlayerNumber())
                {
                    if (Player.Medkit > 0)
                    {
                        int HealAmount = Players(i).HealthMax - Players(i).ActualHealth;
                        int Expense = Player.Medkit > HealAmount ? HealAmount : Player.Medkit;
                        bool Stabilize = Players(i).ActualHealth + Expense < 0;
                        if (CheckInput(BT_USE, KEY_HELD, false, PlayerNumber()))
                        {
                            SetPlayerProperty(PlayerNumber(), true, PROP_TOTALLYFROZEN);
                            Players(i).ReviveKeyTimer++;
                            if (Players(i).ReviveKeyTimer >= 105)
                            {
                                if (!Stabilize)
                                {
                                    ScriptCall("DRPGZUtilities", "ForceRespawn", i);
                                    HudMessage("%tS was revived", i + 1);
                                    EndHudMessageBold(HUDMSG_FADEOUT, 0, "Green", 1.5, 0.8, 1.0, 1.0);
                                }
                                else
                                {
                                    HudMessage("%tS was stabilized", i + 1);
                                    EndHudMessage(HUDMSG_FADEOUT, 0, "Green", 1.5, 0.7, 1.0, 1.0);
                                }
                                SetPlayerProperty(PlayerNumber(), false, PROP_TOTALLYFROZEN);
                                Players(i).ActualHealth += Expense;
                                Player.Medkit -= Expense;
                                Players(i).ReviveKeyTimer = 0;
                                Players(i).Reviver = -1;
                            }
                            else if (Players(i).ReviveKeyTimer > 0)
                            {
                                int Percent = CalcPercent(Players(i).ReviveKeyTimer, 105);
                                if (!Stabilize)
                                    DrawProgressBar("Reviving", Percent);
                                else
                                    DrawProgressBar("Stabilizing", Percent);
                            }
                        }
                        else
                        {
                            SetPlayerProperty(PlayerNumber(), false, PROP_TOTALLYFROZEN);
                            Players(i).ReviveKeyTimer = 0;
                            Players(i).Reviver = -1;
                            if (!Stabilize)
                                HudMessage("Hold \Cd%jS\C- to revive", "+use");
                            else
                                HudMessage("Hold \Cd%jS\C- to stabilize", "+use");
                            EndHudMessage(HUDMSG_PLAIN, 0, "Green", 1.5, 0.75, 0.05);
                        }
                    }
                    else
                    {
                        HudMessage("Find some medkit to revive");
                        EndHudMessage(HUDMSG_PLAIN, 0, "Green", 1.5, 0.75, 0.05);
                    }
                }
                else
                {
                    HudMessage("%tS is receiving treatment from %tS", i + 1, PlayerNumber());
                    EndHudMessage(HUDMSG_PLAIN, 0, "Green", 1.5, 0.75, 0.05);
                }
            }
        }
    }
}

NamedScript int AlivePlayers()
{
    int AlivePlayers = 0;
    for (int i = 0; i < PlayerCount(); i++)
        if (Players(i).ActualHealth > 0)
            AlivePlayers++;
    return AlivePlayers;
}
