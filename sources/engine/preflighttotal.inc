/* body of PreflightMemoryUsageTotal: the head is in setup.c at both of its positions */
{
    long engineGlow;
    long runningLights;
    long weaponGlow;
    long total;

    PreflightMemoryUsage(&total, &engineGlow, &runningLights, &weaponGlow);
    if (settings.runningLights)
        total += runningLights;
    if (settings.weaponGlows)
        total += weaponGlow;
    if (settings.engineGlows)
        total += engineGlow;
    return total;
}
