class DRPGZEHandlerRPG : EventHandler
{
    // Externals
    DRPGZEHandlerMap RPGMap;
    DRPGZUtilities RPGUtils;

    override void OnRegister()
    {
        RPGMap = DRPGZEHandlerMap(EventHandler.Find("DRPGZEHandlerMap"));
    }

    // --------------------------
    // RenderOverlay - Tips Stuff
    // --------------------------
    int Tips; // 0: Off; 1: Default; 2: Map Event
    bool TipsSetup;
    int TipID;
    int TipTimer;
    int TipX;
    int TipY;

    // ----- Default Tips -----
    // Titles
    static const string dTipsT[] =
    {
        // Level/Rank
        "Level",
        "Rank",
        "Combo",

        // Currencies
        "Credits",
        "Modules",
        "Turret Parts",

        // Stats
        "Strength",
        "Defense",
        "Vitality",
        "Energy",
        "Regeneration",
        "Agility",
        "Capacity",
        "Luck",

        // Stats Extended
        "Survival Bonus",
        "Perks",
        "Stat Cap",

        // Skills
        "EP",
        //"Overdrive",
        "Auras",

        // Augmentations
        "Augmentations",
        "Augmentation Battery",
        "Augmentation Slots",

        // Shield
        "Shields",
        "Shield Components",
        "Shield Stats",
        "Shield Charge Cycle",
        //"Quick Shield Recharge",

        // Stims
        "Stims",
        "Stim Injectors",
        "Stim Vials",
        "Stim Stat Vials",
        "Stim Booster Vials",
        "Stim Powerup Vials",
        "Toxicity",

        // Turrets
        "Turrets",
        "Destroyed Turrets",
        //"Turret Maintenance",
        //"Turret Deployment",

        // Monsters
        "Monster Stats",
        "Monster Auras",
        "Monster Shadow Auras",
        "Monster Regeneration Cycle",
        "Monster Threat Level",
        "Megabosses",

        // Status Effects
        "Status Effects",
        "Blindness",
        "Confusion",
        "Poison",
        "Corrosion",
        "Fatigue",
        "Virus",
        "Silence",
        "Curse",
        "EMP",
        "Radiation",

        // Outpost/Arena
        "Outpost",
        "Arena",
        "Level Transporter",
        "Skill Computer",
        "Minigames",
        "Practice Target",

        // Missions
        "Missions",
        "Collection Missions",
        "Kill Missions",
        "Kill Auras Missions",
        "Kill Reinforcements Missions",
        "Assassination Missions",
        "Find Secrets Missions",
        "Find Items Missions",
        "Achieve Combo Missions",

        // Shop/Locker
        "Shop",
        //"Locker",
        "Shop Cards",

        // Level Events
        "Level Events",
        "Rolling Level Events"
    };
    // Title Colours
    static const int dTipsTC[] =
    {
        // Level/Rank
        Font.CR_WHITE,
        Font.CR_YELLOW,
        Font.CR_PURPLE,

        // Currencies
        Font.CR_GOLD,
        Font.CR_GREEN,
        Font.CR_GREY,

        // Stats
        Font.CR_RED,
        Font.CR_GREEN,
        Font.CR_BRICK,
        Font.CR_LIGHTBLUE,
        Font.CR_PURPLE,
        Font.CR_ORANGE,
        Font.CR_BLUE,
        Font.CR_GOLD,

        // Stats Extended
        Font.CR_ORANGE,
        Font.CR_GREEN,
        Font.CR_RED,

        // Skills
        Font.CR_LIGHTBLUE,
        //Font.CR_LIGHTBLUE,
        Font.CR_LIGHTBLUE,

        // Augmentations
        Font.CR_YELLOW,
        Font.CR_YELLOW,
        Font.CR_YELLOW,

        // Shield
        Font.CR_CYAN,
        Font.CR_CYAN,
        Font.CR_CYAN,
        Font.CR_CYAN,
        //Font.CR_CYAN,

        // Stims
        Font.CR_GREY,
        Font.CR_GREY,
        Font.CR_GREY,
        Font.CR_GREY,
        Font.CR_GREY,
        Font.CR_GREY,
        Font.CR_GREEN,

        // Turrets
        Font.CR_GREEN,
        Font.CR_GREEN,
        //Font.CR_GREEN,
        //Font.CR_GREEN,

        // Monsters
        Font.CR_RED,
        Font.CR_RED,
        Font.CR_RED,
        Font.CR_RED,
        Font.CR_RED,
        Font.CR_RED,

        // Status Effects
        Font.CR_BRICK,
        Font.CR_GREY,
        Font.CR_GOLD,
        Font.CR_DARKGREEN,
        Font.CR_GREEN,
        Font.CR_ORANGE,
        Font.CR_PURPLE,
        Font.CR_LIGHTBLUE,
        Font.CR_RED,
        Font.CR_CYAN,
        Font.CR_GREEN,

        // Outpost/Arena
        Font.CR_DARKGREEN,
        Font.CR_RED,
        Font.CR_GREEN,
        Font.CR_GREEN,
        Font.CR_GREEN,
        Font.CR_RED,

        // Missions
        Font.CR_GREEN,
        Font.CR_GREEN,
        Font.CR_BRICK,
        Font.CR_BRICK,
        Font.CR_BRICK,
        Font.CR_RED,
        Font.CR_YELLOW,
        Font.CR_LIGHTBLUE,
        Font.CR_PURPLE,

        // Shop/Locker
        Font.CR_GOLD,
        //Font.CR_BLUE,
        Font.CR_GOLD,

        // Level Events
        Font.CR_GREEN,
        Font.CR_GREEN
    };
    // Descriptions
    static const string dTipsD[] =
    {
        // Level/Rank
        "Your level determines your basic strength and progress through the game. Your level is increased by killing monsters, completing missions and achieving combos.",
        "Your rank determines your standing within the UAC. Rank will determine how much money you are paid by the UAC as well as the discount you will receive when purchasing items from the shop.",
        "The combo system allows you to kill enemies in rapid succession in order to gain bonus XP and Rank. When an enemy is killed, you will gain +1 to your combo counter and your combo timer will reset. When your timer reaches its cooldown point, indicated by the split between the red and green in the bar, it will add the indicated XP and Rank to your totals and calculate a bonus (the green number). If you keep incrementing your combo during the cooldown period, you can keep stacking XP and Rank into the bonus. When the combo timer ends, your combo will be completely reset and your bonus will be added to your totals.",

        // Currencies
        "Credits are the universal currency used by the UAC and are used for purchasing goods in the shop.",
        "Modules are used to upgrade your stats and skills. They are typically given by level ups, missions and completing level events as well as occasionally being dropped by monsters and found in the world.",
        "Turret parts are used to build and upgrade your turret. They can be found by disassembling broken-down turrets you find in the world.",

        // Stats
        "Your Strength stat determines the extra damage you deal with all types of attacks.",
        "Your Defense stat determines your resistance to all forms of damage.",
        "Your Vitality stat determines your max health and health regeneration rate.",
        "Your Energy stat determines your max EP and EP regeneration rate.",
        "Your Regeneration stat determines your regeneration rate and the time that Regeneration Spheres last.",
        "Your Agility stat governs your movement, jumping and fire speeds as well as your survival bonus.",
        "Your Capacity stat determines your ammo carrying capacity, how many compounds of each type you can carry for Stims as well as how many inventory items you can hold",
        "Your Luck stat determines which items can drop as well as the chance which they will drop. Luck also affects several other hidden factors as well.",

        // Stats Extended
        "Survival Bonus is a special attribute which works as a last stand mechanic. When you take a hit that would kill you and your survival bonus is triggered, you will not be killed by the attack.",
        "Perks are special enhancements that you receive when you upgrade a stat to 100 points or more.",
        "You can only increase a stat past 100 up to the maximum of your current level + 100.",

        // Skills
        "Energy Points are needed in order to use skills. They can also be used to charge your Shield and to deposit and withdraw items from your personal locker while not in the Outpost.",
        //StrParam("You can Overdrive a skill by holding the \Cd%jS\C- button. Doing this will always use the skill, regardless of it's EP cost. However, this will bring your current EP into the negatives. When negative, you will suffer stat penalties and not be able to use any skills until your EP is restored past 0.", "+speed"),
        "Auras are special passive abilities you can activate to give you temporary boosts to your stats. Auras will also affect teammates within it's radius, determined by your Energy stat",

        // Augmentations
        "Augmentations give you permanent bonuses to your stats while they are active.",
        "Augmentations can only work while your augmentation battery has a charge. Your battery can be recharged by using augmentation batteries from your inventory or by receiving certain types of damage. Battery drain is determined by how many augmentations you have active and how high their levels are. The more augmentation active and the higher their levels, the quicker the battery will drain.",
        "You can only activate as many augmentations as you have available slots for. To increase the number of slots you have, you must find an augmentation slot upgrade. Augmentation slot upgrades can be bought from the shop, found as rare drops from enemies or found in the world.",

        // Shield
        "Shields will protect you from taking direct damage by absorbing the damage into their own charge pool. When a shield takes damage, it's charge is directly affected and will be reduced by the damage's amount. When a shield's charge reaches 0, you will begin taking normal damage again.",
        "Shields are built from 4 basic components which you can collect and customize during the game. The body is named after the manufacturer and is generally responsible for modifying the capacity of the Shield, but also evenly modifies the other stats as well. The battery is generally responsible for modifying the charge rate of the shield. The capacitor is generally responsible for modifying the charge rate and delay rate of the shield. Accessories give the shield unique effects and abilities which can be triggered in various ways.",
        "Each shield has 4 stats which govern it's behavior and ability. Capacity determines the total amount of charge the shield can hold. Charge Rate determines the amount the charge pool will regenerate when a charge cycle is completed. Delay Rate determines the wait time that occurs when a charge cycle is interrupted before charging will resume again. Charge interval determines the length between charge cycles.",
        "A charging cycle occurs based on the Charge Interval (default of 1 second) and increases the Shield's charge pool, determined by the Charge Rate. When the shield is struck, the charging cycle is interrupted and a waiting period, determined by the Delay Rate, must complete before charging cycles will resume again.",
        //StrParam("You can quickly recharge your shield at the cost of EP by holding \Cd%jS\C- + \Cd%jS\C-.", "+speed", "+use"),

        // Stims
        "Stims are portable injected temporary stat increases. There are three forms of Stim vials that can be encountered: Stat vials, Booster vials and Powerup vials.",
        "Injectors are what are used to actually store the compounds for usage. Injectors can either be bought in the shop or found in the world. There are 4 different injector sizes, which hold varying amounts of compounds.",
        "Vials are the items you pickup which contain the different kinds of compounds. Vials can be bought in the shop or found by killing enemies which have auras or elsewhere in the world.",
        "Stat vials increase their various corresponding stats when used.",
        "Booster vials are special vials which have unique stat-related effects. Purifier vials increase the duration that stat vials will last. Potency vials increase all stats at once.",
        "Powerup vials give various special powers for a limited duration. Unlike the Stat stims, they are unaffected by Purifier vials, and their time is affected solely by the amount of vials put into the Stim.",
        "Toxicity is a measure of the radiation and toxins in your body. Toxicity is increased by taking radiation or toxin related damage, such as standing on damage floors. The higher your toxicity levels get, the more penalties you will suffer. If your toxicity levels reach 100%, you will die. Toxicity will naturally dissipate over time, the speed of which is determined by your Regeneration stat.",

        // Turrets
        "Turrets are portable sentry drones which can be built and equipped with multiple offensive and defensive upgrades. Turrets must initially be built from turret parts, at which point the turret may be used and upgraded further as more parts become available.",
        "In the world, you will find destroyed turrets. These turrets can be scavenged for spare turret parts. Some turrets will be in better condition than others, allowing for more parts to be scavenged.",
        //StrParam("Your turret will need routine maintenance performed on it in order to keep it in working condition. There are three components to maintenance: charging, repairing and refitting. Charging will charge the turret's internal battery, allowing it to stay active on the field. Repairing will patch up the turret, restoring it's health. Refitting occurs when upgrades are performed on the turret, and must be allowed to finish before the turret can be used again. In order for maintenance to begin and to continue, a steady supply of Credits will be deducted from your account. To send the turret in for maintenance, in either the turret menu or with the turret command wheel open, press \Cd%jS\C-.", "+speed"),
        //StrParam("To quickly deploy or deactivate your turret, use \Cd%jS\C- + \Cd%jS\C-.", "+speed", "+user2"),

        // Monsters
        "Like you, monsters also have a level and stats which will affect their abilities in various ways.",
        "Like you, monsters can also have one or multiple auras which will give them special abilities. Each aura a monster has will also double its respective stat.",
        "Monsters which have all auras active at once will become a shadow enemy with extremely high stats and abilities for their level. These enemies will receive names and should be priority targets to take down, as their combined abilities can quickly overwhelm a player if they are not prepared.",
        "Like you, monsters will regenerate a portion of their health every so often. The amount of health they will regenerate is determined by their regeneration stat.",
        "Each monster has a threat level. Their threat level is determined by their level, stats and active auras and ranges from 0 to 10. Threat level can be used as a general way to gauge a monster's abilities and which ones should be prioritized when attacking. The higher a monster's threat level, the higher XP and Rank you will receive for defeating them. Threat level is represented on a monster's health bar by the number of emblems present.",
        "Under certain circumstances, you will encounter monsters known as megabosses. These bosses are extremely difficult, having special and unique behaviors which can quickly and effectively destroy you if not prepared. When killed, megabosses will provide a tremendous amount of loot as well as XP and Rank.",

        // Status Effects
        "Status effects are debuffs which will affect you in different ways. You can get them from taking different types of damage or by exposing yourself to specific types of hazards. When inflicted with a status effect, it will last for a certain amount of time. Each status effect has 5 levels of intensity which are increasingly more detrimental to you. If you are hit with a status effect you already have, it will add to your current timer and potentially increase it's intensity.",
        "Blindness is a status effect which will darken your view and make it harder to see.",
        "Confusion is a status effect which will make your vision blurry and make it harder for you to aim and move.",
        "Poison is a status effect which will deal direct health damage to you every second. It will not actually kill you if your health reaches a critical level.",
        "Corrosion is a status effect which will deal damage to your armor every second.",
        "Fatigue is a status effect which will decrease your movement speed and jumping height.",
        "Virus is a status effect which will stop you from regenerating.",
        "Silence is a status effect which prevents you from using skills.",
        "Curse is a status effect which increases the amount of damage you take.",
        "EMP is a status effect which will completely disable your shields and augmentations and not allow you to re-enable them. It will also drain your augmentation battery.",
        "Radiation is a status effect which slowly increases your toxicity levels. It will not increase your toxicity past 85%.",

        // Outpost/Arena
        "The UAC Outpost is the main hub of operations where you can perform various activities, access various facilities and stock up between levels. You can transport to the Outpost at any time using the Transport skill.",
        "In the Arena, you can fight waves of enemies for XP, Rank and loot.",
        "In the Outpost, you can use the level transporter to view stats on all the levels you have visited and to transport back to them at will.",
        "In the Outpost, you can use the skill computer to change the current skill level without the need to restart the game.",
        "In the Outpost, there are minigames which you can play in order to win items using your gold and platinum chips.",
        "In the Outpost, you can shoot the target in order to gauge the amount of damage your weapons are doing.",

        // Missions
        "You can accept different missions in the Outpost in order to gain XP, Rank, Credits, Modules and items. There are several different types of missions at varying difficulty levels you can undertake. The available missions will refresh each time you visit the Outpost.",
        "You must collect a specific number of a specified item to complete the mission. These items are found scattered around the world.",
        "You must kill a given amount of enemies of the same type to complete the mission.",
        "You must Kill a given amount of enemies which have at least one aura to complete the mission.",
        "While in the field, monsters will spawn in around you. You must kill a given amount of them to complete the mission.",
        "You must eliminate the specified enemy, which will be located somewhere in the level and will always have a shadow aura to complete the mission.",
        "You must find the given amount of unique secrets to complete the mission.",
        "You must find the given amount of items to complete the mission.",
        "You must get your combo to the given amount to complete the mission.",

        // Shop/Locker
        "The shop is where you can go to purchase and sell the various equipment and items you can find during the game. The shop also contains a locker system, where you may withdraw and deposit items for later usage.",
        //StrParam("You can access the locker system via the shop and deposit/withdraw your items there. Withdrawing and depositing items has a 1%% cost to your EP per use outside the Outpost. You can switch between the shop and locker using the \Cd%jS\C- key.", "+jump"),
        "While playing, you may find UAC cards which provide you with a discount as well as other benefits. Finding a new card will replace your old card with the upgraded version. Discount benefits from a card will only apply when shopping at the Outpost.",

        // Level Events
        "When entering a level, sometimes unique level-wide events will happen. Each event has it's own unique characteristics and conditions for ending the event.",
        "Level events will not only occur when entering a level, but previous levels which you have visited will also sometimes contain events and require revisiting in order to participate in the event."
    };
    // ----- Map Event Tips -----
    // Titles
    static const string eTipsT[] =
    {
        "Megaboss Event",
        "Environmental Hazard Event",
        "Thermonuclear Bomb Event",
        "Low Power Event",
        "All Auras Event",
        "One-Monster Event",
        "Hell Unleashed Event",
        "Harmonized Destruction Event",
        "Cracks in the Veil Event",
        "12 Hours 'til Doomsday Event",
        "Vicious Downpour",
        "The Dark Zone",
        "C       O     N   S    U    M    E         Y   O     U",
        "Whispers of Darkness",
        "R\CiA\CkI\CdN\ChB\CtO\CaW\CjS",
        "Hell Event",
        "Armageddon Event",
        "Sinstorm"
    };
    // Title Colours
    static const int eTipsTC[] =
    {
        Font.CR_RED,
        Font.CR_DARKGREEN,
        Font.CR_ORANGE,
        Font.CR_DARKGRAY,
        Font.CR_GREEN,
        Font.CR_LIGHTBLUE,
        Font.CR_RED,
        Font.CR_GREEN,
        Font.CR_GREEN,
        Font.CR_RED,
        Font.CR_DARKGREEN,
        Font.CR_PURPLE,
        Font.CR_DARKRED,
        Font.CR_BLACK,
        Font.CR_RED,
        Font.CR_BRICK,
        Font.CR_BLACK,
        Font.CR_RED
    };
    // Descriptions
    static const string eTipsD[] =
    {
        "All of the hostiles have vacated the area, and a very powerful creature is lurking about. Return to the Outpost if needed, and make sure you have the power and munitions to take down the death machine.",
        "The area's covered in deadly radiation, but there is also a Radiation Neutralizer to fix the problem. Gather fuel and refill the Neutralizer whenever it runs low, until the radiation is brought down to safe levels.",
        "Find as many keys as you can to disable the countdown. Worse comes to worst, you can hack through the key slots by using the bomb without keys, but it's going to take time to do so for each key missing, as indicated by the HUD. And make sure the bomb doesn't get shot!",
        "The UAC had a major battle in this area, and the discards have been left behind when the area was damaged. Pluck some extra items out of the wreckage before they come back to clean up. You can even get some extra credits and modules if you restore the backup power before you leave.",
        "All of the monsters have rather deadly magic auras. Understand their weaknesses, and attempt to stay away from some of them, as they are dangerous.",
        "There is one species of demons currently occupying the area. Clearing the area of the hostiles will net you a bonus.",
        "Open the box, and partake in the rare contents within, but be warned: You will anger the demonic army, and they will pursue you endlessly until you are killed or leave the area. The longer you stay, the more likely they are to drop items, but the higher level they will become.",
        "All of the enemies in the area have one aura type. Get accustomed to the aura's strength and weakness and use it to your advantage. Don't get caught off-guard, as some monsters can and will have other auras as well.",
        "The area is spatially unstable. Avoid stepping into the portals, as they will teleport you to other points in the area at random. Enemies will be teleported in as well.",
        "Hell has claimed the area! Run while you still can.",
        "It's raining acidic substances. Best to wear a radiation suit if you plan to head outdoors.",
        "This is no ordinary mist. The evil fog will slowly sap away the light from its' surroundings. Once that happens, enemies will begin to resurrect and become infused with Shadow Auras. Sticking around here for long is not recommended without preparation.",
        "Tip: Run.",
        "'He who fights with monsters should look to it that he himself does not become a monster. And when you gaze long into an abyss, the abyss also gazes into you.'",
        "SHE BROKE THE REALITY GENERATOR AGAIN! WE'RE DOOMED!",
        "The difficulty level has increased to Hell for this area.",
        "The difficulty level has increased to Armageddon for this area.",
        "You're getting close to the source of the invasion. The demonic legion is pulling all of its' last stops. Expect frequent battles with shadow enemies, and reinforcements to pour in relentlessly. Be careful when traversing outdoors, as the fire rain will harm you as well."
    };

    override void RenderOverlay(RenderEvent e)
    {
        // --------------------------
        // Tips
        // --------------------------
        if (Tips > 0 && RPGMap.CurrentLevel.Init && level.maptime < TipTimer)
        {
            // Spicy
            double TipAlpha = (double)(TipTimer - level.maptime) / 100;

            // Map Event
            if (Tips == 2)
            {
                // Title
                RPGUtils.drawTextWrap(bigfont, eTipsTC[TipID], TipX, TipY, eTipsT[TipID], TipAlpha);
                // Description
                RPGUtils.drawTextWrap(smallfont, smallfont.CR_WHITE, TipX, TipY+20, eTipsD[TipID], TipAlpha);
            }
            // Default
            else
            {
                // Title
                RPGUtils.drawTextWrap(bigfont, dTipsTC[TipID], TipX, TipY, dTipsT[TipID], TipAlpha);
                // Description
                RPGUtils.drawTextWrap(smallfont, smallfont.CR_WHITE, TipX, TipY+20, dTipsD[TipID], TipAlpha);
            }
        }
    }

    override void WorldTick()
    {
        // ---------------------------
        // RenderOverlay Setup - Tips
        // ---------------------------
        if (Tips > 0 && !TipsSetup && RPGMap.CurrentLevel.Init)
        {
            // Get TipID for potential Event
            TipID = (players[consoleplayer].mo.ACS_ScriptCall("GiveTipHelper") - 1);

            // Map Event
            if (TipID > -1)
            {
                TipTimer = eTipsD[TipID].Length() * 0.1 * 35;
                Tips = 2;
            }
            // Default
            else
            {
                TipID = Random(0, dTipsT.size() - 1);
                TipTimer = dTipsD[TipID].Length() * 0.1 * 35;
            }

            TipX = screen.GetWidth();
            TipY = 500;
            TipsSetup = true;
        }
    }

    override void PlayerEntered(PlayerEvent e)
    {
        // -----
        // CVars
        // -----
        CVar i;

        i = CVar.FindCVar('drpg_tips');
        Tips = i.GetInt();
    }
}
