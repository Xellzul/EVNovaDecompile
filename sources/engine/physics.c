// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* physics.c -- ship motion helpers (win 0x0043b4a0..0x0043ba30: Accel, AdjustedAccel, CalcInterceptAngle, SpeedLimit ...). Notes: Research/notes/physics.md */
#include <math.h>

#include "shipobj.h"
#include "resources.h"
#include "game.h"
#include "render.h"
#include "space.h"
#include "player.h"
#include "ships.h"
#include "gametypes.h"

/* TableSin/TableCos look up the 360-entry tables BuildTrigTable fills and leave the result in trigResult. */

/* CalcInterceptAngle returns an int; its callers narrow it to short for Accel. */

/* Accel (mac _Accel). STATE: MATCH, byte-identical; mac MATCH i386 and ppc. OPEN: none */
// FUNCTION: EVNOVA 0x0043b4a0
// MAC: _Accel i386 0x0002ade6 ppc 0x0002f31c
void Accel(short angle, float mag, FloatPoint *out)
{
    TableSin(angle);
    out->x += trigResult * mag;
    TableCos(angle);
    out->y -= trigResult * mag;
}

/* AdjustedAccel (mac _AdjustedAccel). STATE: MATCH, byte-identical; mac MATCH i386 and ppc. OPEN: none */
// FUNCTION: EVNOVA 0x0043b4e0
// MAC: _AdjustedAccel i386 0x0002ae4b ppc 0x0002f390
void AdjustedAccel(short angle, float accel, FloatPoint *vel, float maxSpeed)
{
    FloatPoint max;
    FloatPoint delta;

    max.x = 0.0f;
    max.y = 0.0f;
    delta.x = 0.0f;
    delta.y = 0.0f;
    Accel(angle, maxSpeed, &max);
    Accel(angle, accel, &delta);
    if (delta.x > 0.0 && max.x > 0.0) {
        if (vel->x < max.x)
            vel->x += delta.x;
    } else if (delta.x < 0.0 && max.x < 0.0) {
        if (vel->x > max.x)
            vel->x += delta.x;
    } else {
        vel->x += delta.x;
    }
    if (delta.y > 0.0 && max.y > 0.0) {
        if (vel->y < max.y)
            vel->y += delta.y;
    } else if (delta.y < 0.0 && max.y < 0.0) {
        if (vel->y > max.y)
            vel->y += delta.y;
    } else {
        vel->y += delta.y;
    }
}

#ifdef MACBUILD
/* FloatAbs (mac _FloatAbs, mac-only: the win port takes the fabs intrinsic). STATE: mac MATCH i386
   and ppc. OPEN: none */
// MAC: _FloatAbs i386 0x0002af44 ppc 0x0002f4cc
float FloatAbs(float x)
{
    float r;

    if (x < 0.0)
        r = x * -1.0;
    else
        r = x;
    return r;
}
#else
#define FloatAbs(x) fabs(x)
#endif

/* the heading from b to a, in degrees */
/* CalcInterceptAngle (mac _CalcInterceptAngle). STATE: MATCH, byte-identical; mac MATCH i386 and ppc.
   OPEN: none */
// FUNCTION: EVNOVA 0x0043b670
// MAC: _CalcInterceptAngle i386 0x0002af60 ppc 0x0002f4e8
short CalcInterceptAngle(FloatPoint *a, FloatPoint *b)
{
    return TableArcTan(a->x - b->x, a->y - b->y);
}

/* clamp both components into [-mag, mag] */
/* SpeedLimit (mac _SpeedLimit). STATE: MATCH, byte-identical; mac MATCH i386 and ppc. OPEN: none */
// FUNCTION: EVNOVA 0x0043b6a0
// MAC: _SpeedLimit i386 0x0002af91 ppc 0x0002f528
void SpeedLimit(FloatPoint *p, float mag)
{
    if (p->x > mag)
        p->x = mag;
    if (p->x < -mag)
        p->x = -mag;
    if (p->y > mag)
        p->y = mag;
    if (p->y < -mag)
        p->y = -mag;
}

/* XYFloatSquaredDist (mac _XYFloatSquaredDist). STATE: MATCH, byte-identical; mac MATCH i386 and ppc.
   OPEN: none */
// FUNCTION: EVNOVA 0x0043b710
// MAC: _XYFloatSquaredDist i386 0x0002afdb ppc 0x0002f570
float XYFloatSquaredDist(FloatPoint *a, FloatPoint *b)
{
    float dx;
    float dy;

    dx = FloatAbs(a->x - b->x);
    dy = FloatAbs(a->y - b->y);
    return dx * dx + dy * dy;
}

#ifdef V111
static float LeadTime(float dist, float speed, short guidance)
{
    if (guidance == kGuidanceRocket) {
        if (dist <= speed * 19.59)
            return dist / (speed * 0.316);
        else
            return (dist - speed * 19.59) / speed + 2.066666666666667;
    }
    return dist / speed;
}
#endif

/* CalcLeadAngle (mac _CalcLeadAngle). STATE: MATCH, byte-identical; mac MATCH i386 and ppc. OPEN: none */
// FUNCTION: EVNOVA 0x0043b740
// MAC: _CalcLeadAngle i386 0x0002b025 ppc 0x0002f5b4
short CalcLeadAngle(ShipObj *self, ShipObj *target, short weaponType,
                   FloatPoint *overridePos)
{
    short angle;

    angle = CalcInterceptAngle(overridePos, &target->pos);
    if (weaponType != -1) {
#ifndef MAC1010
        short guidance = weaponStats[weaponType].guidance;
#endif
        if (weaponStats[weaponType].guidance == kGuidanceUnguided ||
            weaponStats[weaponType].guidance == kGuidanceTurret ||
            weaponStats[weaponType].guidance == kGuidanceRocket ||
            weaponStats[weaponType].guidance == kGuidanceFrontQuad ||
            weaponStats[weaponType].guidance == kGuidanceRearQuad ||
            weaponStats[weaponType].guidance == kGuidancePointDef) {
            float targetX;
            float dx;
            float dy;
            float speed;
            float dist;
            FloatPoint lead;

            targetX = target->pos.x;
            dx = targetX;
            dy = target->pos.y;
#ifdef V111
            dx -= overridePos->x;
            dy -= overridePos->y;
            speed = LeadTime(FUN_004eed20(dx * dx + dy * dy), weaponStats[weaponType].speed100, guidance);
#else
#ifdef MAC1010
            if (overridePos != 0) {
                dx -= overridePos->x;
                dy -= overridePos->y;
            } else {
                dx -= self->pos.x;
                dy -= self->pos.y;
            }
            dist = sqrt(dx * dx + dy * dy);
#else
            if (overridePos == 0) {
                dx -= self->pos.x;
                dy -= self->pos.y;
            } else {
                dx -= overridePos->x;
                dy -= overridePos->y;
            }
            dist = FUN_004eed20(dx * dx + dy * dy);
#endif
            speed = weaponStats[weaponType].speed100;
#ifdef MAC1010
            if (weaponStats[weaponType].guidance == kGuidanceRocket) {
#else
            if (guidance == kGuidanceRocket) {
#endif
                if (dist <= speed * 19.59)
                    speed = dist / (speed * 0.316);
                else
                    speed = (dist - speed * 19.59) / speed + 2.066666666666667;
            } else {
                speed = dist / speed;
            }
#endif
#ifdef MAC1010
            lead.x = (target->vel.x - self->vel.x) * speed + target->pos.x;
#else
            lead.x = (target->vel.x - self->vel.x) * speed + targetX;
#endif
            lead.y = (target->vel.y - self->vel.y) * speed + target->pos.y;
            angle = CalcInterceptAngle(overridePos, &lead);
        }
    }
    return angle;
}

/* CalcObjectLeadAngle (mac _CalcObjectLeadAngle). STATE: MATCH, byte-identical; mac MATCH i386 and
   ppc. OPEN: none */
// FUNCTION: EVNOVA 0x0043b8c0
// MAC: _CalcObjectLeadAngle i386 0x0002b18e ppc 0x0002f724
short CalcObjectLeadAngle(ShipObj *self, FloatPoint *targetPos, FloatPoint *targetVel,
                   short weaponType, FloatPoint *overridePos)
{
    short angle;

    angle = CalcInterceptAngle(overridePos, targetPos);
    if (weaponType != -1) {
#ifndef MAC1010
        short guidance = weaponStats[weaponType].guidance;
#endif
        if (weaponStats[weaponType].guidance == kGuidanceUnguided ||
            weaponStats[weaponType].guidance == kGuidanceTurret ||
            weaponStats[weaponType].guidance == kGuidanceFrontQuad ||
            weaponStats[weaponType].guidance == kGuidanceRearQuad ||
            weaponStats[weaponType].guidance == kGuidancePointDef) {
            float targetX;
            float dx;
            float dy;
            float speed;
            float dist;
            FloatPoint lead;

            targetX = targetPos->x;
            dx = targetX;
            dy = targetPos->y;
            if (overridePos != 0) {
                dx -= overridePos->x;
                dy -= overridePos->y;
            } else {
                dx -= self->pos.x;
                dy -= self->pos.y;
            }
            speed = weaponStats[weaponType].speed100;
#ifdef MAC1010
            dist = sqrt(dx * dx + dy * dy);
            if (weaponStats[weaponType].guidance == kGuidanceRocket) {
                if (dist <= speed * 19.59)
                    dist = dist / (speed * 0.316);
                else
                    dist = (dist - speed * 19.59) / speed + 2.066666666666667;
            } else {
                dist = dist / speed;
            }
            lead.x = (targetVel->x - self->vel.x) * dist + targetPos->x;
            lead.y = (targetVel->y - self->vel.y) * dist + targetPos->y;
#else
            dist = FUN_004eed20(dx * dx + dy * dy);
            if (guidance == kGuidanceRocket) {
                if (dist <= speed * 19.59)
                    speed = dist / (speed * 0.316);
                else
                    speed = (dist - speed * 19.59) / speed + 2.066666666666667;
            } else {
                speed = dist / speed;
            }
            lead.x = (targetVel->x - self->vel.x) * speed + targetX;
            lead.y = (targetVel->y - self->vel.y) * speed + targetPos->y;
#endif
            angle = CalcInterceptAngle(overridePos, &lead);
        }
    }
    return angle;
}

/* CalcSpobLeadAngle (mac _CalcSpobLeadAngle). STATE: MATCH, byte-identical; mac MATCH i386 and ppc.
   OPEN: none */
// FUNCTION: EVNOVA 0x0043ba30
// MAC: _CalcSpobLeadAngle i386 0x0002b2f0 ppc 0x0002f878
short CalcSpobLeadAngle(SpobRecord *spob, ShipObj *target)
{
    FloatPoint spobPos;
    short weaponType;
    short angle;

    spobPos.x = spob->pos.h;
    spobPos.y = spob->pos.v;
    angle = CalcInterceptAngle(&spobPos, &target->pos);
#ifdef MAC1010
    if (spob->weapon != -1) {
        float dx;
        float dy;
        float speed;
        float dist;
        float t;
        FloatPoint lead;

        dx = target->pos.x - spob->pos.h;
        dy = target->pos.y - spob->pos.v;
        speed = weaponStats[spob->weapon].speed100;
        dist = sqrt(dx * dx + dy * dy);
        if (weaponStats[spob->weapon].guidance == kGuidanceRocket) {
            if (dist <= speed * 19.59)
                t = dist / (speed * 0.316);
            else
                t = (dist - speed * 19.59) / speed + 2.066666666666667;
        } else {
            t = dist / speed;
        }
        lead.x = target->vel.x * t + target->pos.x;
        lead.y = target->vel.y * t + target->pos.y;
        angle = CalcInterceptAngle(&spobPos, &lead);
    }
#else
    weaponType = spob->weapon;
    if (weaponType != -1) {
        float targetX;
        float dx;
        float dy;
        float speed;
        float dist;
        FloatPoint lead;

        targetX = target->pos.x;
        dx = targetX - spob->pos.h;
        dy = target->pos.y - spob->pos.v;
        speed = weaponStats[weaponType].speed100;
        dist = FUN_004eed20(dx * dx + dy * dy);
        if (weaponStats[weaponType].guidance == kGuidanceRocket) {
            if (dist <= speed * 19.59)
                speed = dist / (speed * 0.316);
            else
                speed = (dist - speed * 19.59) / speed + 2.066666666666667;
        } else {
            speed = dist / speed;
        }
        lead.x = target->vel.x * speed + targetX;
        lead.y = target->vel.y * speed + target->pos.y;
        angle = CalcInterceptAngle(&spobPos, &lead);
    }
#endif
    return angle;
}
