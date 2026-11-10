/* body of DisposeClonedSprite: the head is in setup.c at both of its positions */
{
    SpritePtr q;

    if (p == NULL)
        return;
    q = *p;
    if (q == NULL)
        return;
    if (q->isEmptySprite != 0) {
        q->frameArray = NULL;
        q->numFrames = 0;
    }
    SWDisposeSprite(p);
}
