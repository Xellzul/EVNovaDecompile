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

/*
 * The win port replaces mac's libm sin/cos with 360-entry tables built at
 * startup by 0x00461830; each accessor leaves its result in one shared
 * global rather than returning it. SinLookup/CosLookup are positional;
 * trigResult is the mac symbol.
 */

/* CalcInterceptAngle returns an int; its callers narrow it to short for Accel. */

/* Accel (mac _Accel). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043b4a0
void Accel(short angle, float mag, FloatPoint *out)
{
    SinLookup(angle);
    out->x += trigResult * mag;
    CosLookup(angle);
    out->y -= trigResult * mag;
}

/* AdjustedAccel (mac _AdjustedAccel). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043b4e0
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

/* the heading from b to a, in degrees */
/* CalcInterceptAngle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043b670
short CalcInterceptAngle(FloatPoint *a, FloatPoint *b)
{
    return TableArcTan(a->x - b->x, a->y - b->y);
}

/* clamp both components into [-mag, mag] */
/* SpeedLimit. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043b6a0
void SpeedLimit(FloatPoint *p, float mag)
{
    float low = 0;

    if (p->x > mag)
        p->x = mag;
    low = -mag;
    if (p->x < low)
        p->x = low;
    if (p->y > mag)
        p->y = mag;
    if (p->y < low)
        p->y = low;
}

/* XYFloatSquaredDist (mac _XYFloatSquaredDist). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043b710
float XYFloatSquaredDist(FloatPoint *a, FloatPoint *b)
{
    float dx;
    float dy;

    dx = fabs(a->x - b->x);
    dy = fabs(a->y - b->y);
    return dx * dx + dy * dy;
}

/* CalcLeadAngle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043b740
short CalcLeadAngle(ShipObj *self, ShipObj *target, short weaponType,
                   FloatPoint *overridePos)
{
    short angle;

    angle = CalcInterceptAngle(overridePos, &target->pos);
    if (weaponType != -1) {
        unsigned short guidance = gWeapons[weaponType].guidance;
        if (guidance == 0xffff || guidance == kGuidanceTurret ||
            (unsigned short)(guidance - 6) <= 3) {
            float targetX;
            float dx;
            float dy;
            float speed;
            float dist;
            FloatPoint lead;

            targetX = target->pos.x;
            dx = targetX;
            dy = target->pos.y;
            if (overridePos == 0) {
                dx -= self->pos.x;
                dy -= self->pos.y;
            } else {
                dx -= overridePos->x;
                dy -= overridePos->y;
            }
            dist = FUN_004eed20(dx * dx + dy * dy);
            speed = gWeapons[weaponType].speed100;
            if (guidance == kGuidanceRocket) {
                if (dist <= speed * 19.59)
                    speed = dist / (speed * 0.316);
                else
                    speed = (dist - speed * 19.59) / speed + 2.066666666666667;
            } else {
                speed = dist / speed;
            }
            lead.x = (target->vel.x - self->vel.x) * speed + targetX;
            lead.y = (target->vel.y - self->vel.y) * speed + target->pos.y;
            angle = CalcInterceptAngle(overridePos, &lead);
        }
    }
    return angle;
}

/* CalcObjectLeadAngle (mac _CalcObjectLeadAngle). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043b8c0
short CalcObjectLeadAngle(ShipObj *self, FloatPoint *targetPos, FloatPoint *targetVel,
                   short weaponType, FloatPoint *overridePos)
{
    short angle;

    angle = CalcInterceptAngle(overridePos, targetPos);
    if (weaponType != -1) {
        unsigned short guidance = gWeapons[weaponType].guidance;
        if (guidance == 0xffff || guidance == kGuidanceTurret ||
            (unsigned short)(guidance - 7) <= 2) {
            float targetX;
            float dx;
            float dy;
            float speed;
            float dist;
            FloatPoint lead;

            targetX = targetPos->x;
            dx = targetX;
            dy = targetPos->y;
            if (overridePos == 0) {
                dx -= self->pos.x;
                dy -= self->pos.y;
            } else {
                dx -= overridePos->x;
                dy -= overridePos->y;
            }
            speed = gWeapons[weaponType].speed100;
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
            angle = CalcInterceptAngle(overridePos, &lead);
        }
    }
    return angle;
}

/* CalcSpobLeadAngle. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x0043ba30
short CalcSpobLeadAngle(SpobRecord *spob, ShipObj *target)
{
    FloatPoint spobPos;
    short weaponType;
    short angle;

    spobPos.x = spob->pos.h;
    spobPos.y = spob->pos.v;
    angle = CalcInterceptAngle(&spobPos, &target->pos);
    weaponType = *(short *)((char *)spob + 0x2c);
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
        speed = gWeapons[weaponType].speed100;
        dist = FUN_004eed20(dx * dx + dy * dy);
        if (gWeapons[weaponType].guidance == kGuidanceRocket) {
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
    return angle;
}
