/* gameconst.h -- engine-wide constants: table sizes, AI states, UI metrics. */
#ifndef GAMECONST_H
#define GAMECONST_H

#define kMaxShips               64      /* shipData slots; slot 0 is the player */

/* ShipObj.aiState: the high-level goal, named as the game's own debug
   readout (DrawHighGoalAbbreviation) prints it */
#define kAIStateHiJack          0
#define kAIStateGoStel          1
#define kAIStateHypOut          2
#define kAIStateDefRet          3
#define kAIStateFightSh         4
#define kAIStateGoHome          5
#define kAIStateWait            6
#define kAIStateInspect         7
#define kAIStateJumpIn          8
#define kAIStateRefuel          9
#define kAIStateFlyPr           0xa
#define kAIStateHypPr           0xb
#define kAIStateProtect         0xc
#define kAIStatePlunder         0xd
#define kAIStateUnPlunder       0xe
#define kAIStateRepair          0xf
#define kAIStateAtkAster        0x10
#define kAIStateScoop           0x11
#define kAIStateDestStel        0x12
#define kAIStateConfused        0x13
#define kAIStateGoHGate         0x14
#define kAIStateEmerge          0x15
#define kAIStateCountWait       0x16

#define kShipTypeEscapePod      0x2ff   /* the last shïp slot is the escape pod */

/* GovtRecord.flags */
#define kGovtAttacksAnyone      0x0001
#define kGovtFlag2              0x0002
#define kGovtHuntsOutlawPlayer  0x0040

/* PersRecord.flags */
#define kPersFlagHunts          0x0001

#define kScanRangeStep          0x258   /* ShipObj.aggression units -> pixels */

#define kMaxBoxes               64      /* boxData slots */

/* ShipObj.aiSubstate: the low-level goal, named as DrawLowGoalAbbreviation
   prints it */
#define kSubLoJack              0
#define kSubKillSpd             1
#define kSubFlyStel             2
#define kSubFlyHypD             3
#define kSubHypJmp              4
#define kSubRunAwy              5
#define kSubTF                  6       /* "T+F" */
#define kSubMissile             7
#define kSubDock                8
#define kSubChase               9
#define kSubZoom                0xa
#define kSubChasSlw             0xb
#define kSubFormFly             0xc
#define kSubJmpPar              0xd
#define kSubWaitTarg            0xe
#define kSubBoard               0xf
#define kSubVeerOff             0x10
#define kSubAfterBurn           0x11
#define kSubChaseSwarm          0x12
#define kSubChaseAst            0x13
#define kSubKillAst             0x14
#define kSubScoop               0x15
#define kSubAttkStel            0x16
#define kSubEnterHG             0x17

/* ShipObj.escortOrder and playerEscortOrders[]: an escort's standing order */
#define kOrderFormation         0
#define kOrderDefend            1
#define kOrderAttack            2
#define kOrderRegroup           3       /* fighters return to the bay */
#define kOrderHold              4       /* hold position */


#define kMaxActiveMissions      16      /* the player's active-mission slots */
#define kAnimDone        0x7d00
#define kMaxMissions    1000    /* mïsn resources; also bounds every mission list */
#define kBetaPilotVersion 0x6b
#define kBlacklistCount 38
#define kBoxSlots               0x40
#define kBracketInset       0x10
#define kCargoRect  7
#define kCargoTypes         6       /* the standard commodities */
#define kMaxCargo           256     /* cargo name slots, junk included */
#define kCommHold           0xc
#define kCommWidth          0xf0
#define kDailyRollRange   100       /* every daily 1..100 reroll in the game */
#define kDepartureRoutePicked 3
#define kDudeShipSlots   16
#define kEscortAI             6
#define kExpandRate         60.0     /* units per second, a pooled double */
#define kGameYearOffset 250     /* the game opens this far after the host clock */
#define kMaxGovts               256     /* govtData entries */
#define kGridCellHigh 55
#define kGridCellWide 84
#define kGridCells    20
#define kGridCols     4
#define kGridItem     5
#define kHalfCircle             180
#define kHalfTurn               180.0f
#define kHiliteFull         0x20
#define ROTL(x, n)          ((((x) >> (8 - (n))) | ((x) << (n))))
#define kHyperFuelUnit      100.0   /* one jump costs this much fuel */
#define kItemAccept                    1
#define kMaxJunks           128
#define kKeyMask            0x0fffffffffffffffLL   /* the licence code's low 60 bits */
#define kLicenseCodePair    0x4b7a0
#define kLicenseCopies      0x1d1760
#define kLicenseName        0xe26f8
#define kMainButtons     6
#define kMapPictItem   3
#define kMasterStellarSprites 256
#define kMaxAsteroids           16
#define kMaxBeams               64
#define kMaxExplods       0x20
#define kMaxCrons            512
#define kMaxDebris        0x40
#define kMaxDudes         512
#define kMaxDisasters   0x100
#define kMaxFleets      0x100
#define kMaxHyperRoute        32
#define kMaxMissionBits    10000
#define kNavSlots               16      /* gSystems[].navDefault entries */
#define kMaxSystemLinks         16      /* gSystems[].links entries */
#define kSpobHyperLinks         8       /* stellarData[].hyperLinks: a gate's destinations */
#define kMaxNavTargets    4       /* navTargetSprite[] */
#define kMaxNebulae       0x20
#define kMaxOutfits         0x200
#define kMaxPersons       0x400
#define kMaxPods         32
#define kMaxRanks                   0x80
#define kMaxShipTypes    0x300 /* gShipTypes entries */
#define kMaxShots               0x80  /* shotData stride 0x48, 0x80 slots */
#define kMaxStars        20
#define kMaxSystems           0x800
#define kMaxWeapTypes           256     /* gWeapons entries, and ship->weap[] is indexed the same way */
#define kMinPilotVersion 0x12c
#define kMissionRecStride 0x473   /* in shorts */
#define kMissionVenues           2
#define kMurkFull               32      /* fully lit hilite value */
#define kNavHyperLabel      0x159   /* "jumping to:" */
#define kNavNoDestination   0x156   /* "no destination" */
#define kNavRect  4
#define kNavStellarLabel    0x157   /* "landing on:" */
#define kNavUnexplored      0x15a   /* "unexplored" */
#define kNavUnknown         0x158   /* "unknown" */
#define kParticleColor          0x20
#define kParticleLifeScale      1.25
#define kParticleSpread         0x14
#define kPilotBetaFile     (-45)
#define kPilotCryptKey 0xb36a210fUL
#define kPilotDirList 0x82
#define kPilotHasJunk      (-46)
#define kPilotOK             0
#define kPilotUnreadable   (-43)
#define kPilotWrongVersion (-42)
#define kPortButtons        7
#define kPortPriceScale 0.01
#define kPrefsSig       0x69
#define kQuarterCircle          0x5a
#define kRankRecordSize             0x120
#define kScrollToEnd  9999
#define kShipRecBanks       0x0002
#define kStatusDestroyed    0x18c
#define kStatusDisabled     0x15b
#define kStatusHostile      0xa9
#define kStatusNeutral      0xa8
#define kStatusNoTarget     0x15d /* "no target" */
#define kStatusNoWeapon     0x15e    /* "no weapon selected" */
#define kStatusVulnerable   0x15c
#define kStellarSprites     0x10
#define kTargPanelInset     194   /* the panel sits this far in from the window's right */
#define kTargRect  6
#define kVoiceMessages           3
#define kWeapRect   5


#define kVoiceSets               8


#define kMaxStellars        0x800
#define kMissionStateStride 5       /* in longs */
#define kFullCircle         360     /* degrees; the float form is kFullCircleF */
#define kFullCircleF        360.0f
#define kSpawnReserve       8       /* ship slots a spawner leaves free */

#endif /* GAMECONST_H */
