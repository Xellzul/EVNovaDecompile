/* body of CenterPict: the head is in engineutils.c at both of its positions */
{
    Rect r;
    Rect frame;

    if (pict != 0) {
        r = *bounds;
        QDGetPictureBounds((PicHandle)pict, &frame);
        bounds->top = ((r.bottom - r.top) - (frame.bottom - frame.top)) / 2 + r.top;
        bounds->bottom = (frame.bottom - frame.top) + bounds->top;
        bounds->left = ((r.right - r.left) - (frame.right - frame.left)) / 2 + r.left;
        bounds->right = (frame.right - frame.left) + bounds->left;
    }
}
