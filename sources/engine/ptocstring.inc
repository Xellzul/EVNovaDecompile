/* body of PtoCstring: the head is in crashlog.c at both of its positions */
{
    short len = s[0];
    short i;

    for (i = 0; i < len; i++)
        s[i] = s[i + 1];
    s[len] = 0;
}
