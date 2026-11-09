/*
 * space.h -- the play field: physics, shots, beams, explosions, asteroids, stars.
 *
 * One declaration per name for the whole tree.
 * Addresses are the win 1.0.10 exe's.
 */
#ifndef SPACE_H
#define SPACE_H
#include "gametypes.h"
#include "shipobj.h"

#ifdef __cplusplus
extern "C" {
#endif


/* --- one declaration per name --- */
extern void AdjustInertialessShipVelocity(ShipObj *ship);                    /* 0x0043b020 */
extern void AdjustedAccel(short angle, float accel, FloatPoint *vel,
                          float maxSpeed);                                  /* 0x0043b4e0 */
extern void ApplyGravity(FloatPoint *a, FloatPoint *b, float force,
                         FloatPoint *out);           /* 0x0046e2f0 (engineutils.c) */
extern short CalcSpobLeadAngle(SpobRecord *spob, ShipObj *target); /* 0x0043ba30 */
extern void CheckShotProximities(void);          /* 0x00437e20 */
#ifdef MACBUILD /* unprototyped: the mac hyper arrival passes an argument (handleplayer.c) */
extern void CreateAsteroids();
#else
extern void CreateAsteroids(void);                      /* 0x004216b0 initevnova.c */
#endif
extern void DestroyPartialFleetCargo(short shipIndex);    /* 0x00469810 mac _DestroyPartialFleetCargo (engineutils.c) */
extern void WrapObject(FloatPoint *pos);           /* 0x0045c6d0 (handleplayer.c) */
extern void BuildTrigTable(void);                          /* 0x00461830 trig tables */
extern void DestroyAsteroid(AsteroidObj *roid);                                /* 0x00462550 (mac _DestroyAsteroid) */
extern void ClearStarfield(void);                                 /* 0x0046ede0 */
extern void HandleAsteroids(void);               /* 0x00436910 */
extern void HandleBoxes(void);                   /* 0x0042c1b0 */
extern void HandleDeadlyStellars(void);          /* 0x0043aed0 */
extern void HandleEscapePods(void);              /* 0x0043b170 */
extern void HandleExplods(void);                 /* 0x0042e160 */
extern void HandleGravity(void);                 /* 0x0043adb0 */
extern void HandleShipPointDefense(ShipObj *ship);                    /* 0x0043a310 */
extern void HandleSmoke(void);                   /* 0x0042c660 */
extern void HandleStars(void);                   /* 0x0042e590 */
extern void HandleStellarWeapons(void);          /* 0x0042d890 */
extern short PickEmergeStellar(ShipObj *ship);   /* 0x0046e9e0 (engineutils.c) */
extern void ModifyShotStartPosition2(ShipObj *ship, FloatPoint *pos, short weapType,
                                      short mount, short lane); /* 0x0046c5c0 */
extern void NewParticle(long color, long horizLoc, long vertLoc,
                        long horizSpeed, long vertSpeed,
                        short shade, short lifeCount);      /* 0x0047b960 */
extern void RandomizeStars(void);                           /* 0x0046ebf0 */
extern void ScrollStarfield(float dx, float dy);         /* 0x0046ee50 (engineutils.c FUN_) */
extern void SpawnBeam(short ship, short target, short weapType, short angle, short mount,
                      short flag);                           /* 0x00427a90 */
extern short TableArcTan(float dx, float dy);    /* 0x004619b0 */
extern void WrapPlayerPosition(void);                       /* 0x0045baa0 */
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern short tableArcTan[0x400];    /* 0x007cbc8c */
extern float tableTan[360];         /* 0x007cb6e8 */
extern Boolean killShots;               /* the five kill flags, order within the five */
extern void *shotSnd[];                             /* 0x00591574 (mac name) */
extern Boolean updateSmokeThisFrame;                /* 0x00596d3d (mac name) */
extern FloatPoint wrapDelta;                    /* 0x007cab2c (mac name) */
EXTERN_CPP_END
#pragma cplusplus reset


extern void Accel(short angle, float mag, FloatPoint *out); /* 0x0043b4a0 */
extern float XYFloatSquaredDist(FloatPoint *a, FloatPoint *b); /* 0x0043b710 */
extern short CalcLeadAngle(ShipObj *self, ShipObj *target, short weaponType,
                           FloatPoint *overridePos);                    /* 0x0043b740 mac _CalcLeadAngle */
extern short CalcObjectLeadAngle(ShipObj *self, FloatPoint *targetPos, FloatPoint *targetVel,
                          short weaponType, FloatPoint *overridePos);   /* 0x0043b8c0 mac _CalcObjectLeadAngle */
#pragma cplusplus on
EXTERN_CPP_BEGIN
extern BeamRecord *beamData;                         /* 0x005912c4 */
extern AsteroidObj *asteroidData;                    /* 0x005912d8 */
extern ShotObj *shotData;                           /* 0x005914b0 */
extern StarSlot *starData;                           /* 0x005914c8 */
EXTERN_CPP_END
#pragma cplusplus reset


/* --- one declaration per name --- */
extern void HandleAsteroidHit(ShotObj *shot, AsteroidObj *roid);  /* 0x00436ff0 HandleAsteroidHit */
extern void HandleShipHit(ShotObj *shot, ShipObj *ship, Boolean spawnSubs); /* 0x00437780 HandleShipHit */
extern void HandleBeams(unsigned char advance);  /* 0x0042f270 */
extern void HandleShot(ShotObj *shot);           /* 0x00435830 */
extern void HandleShotGuidance(ShotObj *shot);                   /* 0x00431530 */
extern Boolean ShotCanHitShip(ShotObj *shot, ShipObj *ship);    /* 0x00426ef0 */
extern short ModifyShotStartPosition(ShipObj *ship, FloatPoint *shotPos,
                                    short weapType, FloatPoint *targetPos);
                                                                      /* 0x0046c320 */
extern short SelectClosestShotStartPosition(ShipObj *ship, FloatPoint *base, short unused,
                                            short weapType, short mount,
                                            FloatPoint *target);      /* 0x0046c4e0 */
extern short SpawnShot(short ship, short target, short weapType, Boolean flag);
                                                                      /* 0x0041fd30 */
extern void SpawnParticles(short x, short y, float speed, short spreadPct,
                         short minLife, short maxLife, long color, short shade,
                         short count, short jitter);                  /* 0x004274d0 */
extern void SpawnShotSubmunitions(ShotObj *shot, short ship);         /* 0x00420d30 mac _SpawnShotSubmunitions */
extern void SpawnAsteroid(unsigned char type);                        /* 0x00421830 */
extern void SpawnSubAsteroid(FloatPoint *pos, short type);            /* 0x00421e60 */

#ifdef __cplusplus
}
#endif

#endif /* SPACE_H */
