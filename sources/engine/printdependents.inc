/* body of print_dependents: the head is in profile.c at both of its positions */
{
    short i;

    for (i = 0; i < depth * 2; i++)
        fprintf(f, " ");
    fprintf(f, "%s:", name[slot]);
    for (i = strlen(name[slot]) + depth * 2; i < 32; i++)
        fprintf(f, " ");
    fprintf(f, " %8i iterations  %8i ms  %8.1f ms average",
            iterations[slot], totalMilliseconds[slot],
            totalMilliseconds[slot] / (float)iterations[slot]);
    if (dependency[slot] != -1) {
        for (i = 0; i < (depth + 1) * 2; i++)
            fprintf(f, " ");
        fprintf(f, "(%.1f%%)",
                totalMilliseconds[slot] * 100.0 / (float)totalMilliseconds[dependency[slot]]);
    }
    fprintf(f, "\n");
    done[slot] = 1;
    for (i = 0; i < kProfileSlots; i++) {
        if (done[i] == 0 && dependency[i] == slot)
            print_dependents(f, i, depth + 1);
    }
}
