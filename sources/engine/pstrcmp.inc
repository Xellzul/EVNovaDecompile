/* body of pstrcmp: the head is in engineutils.c at both of its positions */
#ifdef MAC1010 /* 1.0.10 spelling */
{
    short i;

    if (a[0] != b[0])
        return -1;
    for (i = 0; i < (a[0] != 0 && b[0] != 0); i++) {
        if (a[i] != b[i])
            return -1;
    }
    return 0;
}
#else
{
    short i;
    Boolean empty;

    if (a[0] != b[0])
        return -1;
    empty = (a[0] == 0);
    for (i = 0; i < (!empty && b[0] != 0); i++) {
        if (a[i] != b[i])
            return -1;
    }
    return 0;
}
#endif
