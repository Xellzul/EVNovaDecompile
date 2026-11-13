/* body of PreflightMemoryUsageTotal: the head is in setup.c at both of its positions */
{
    int32_t engineGlow;
    int32_t runningLights;
    int32_t weaponGlow;
    int32_t total;

    PreflightMemoryUsage(&total, &engineGlow, &runningLights, &weaponGlow);
    if (settings.runningLights)
        total += runningLights;
    if (settings.weaponGlows)
        total += weaponGlow;
    if (settings.engineGlows)
        total += engineGlow;
    return total;
}
