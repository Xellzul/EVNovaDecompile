/* tiling.c -- SpriteWorld 2.3.1's Tiling.c as the mac 1.1.1 fork built it (mac only; the win port has no tiles). Notes: Research/notes/spriteworld-mac.md */
#include "macport.h"
#include "burger.h"
#include "spriteworld.h"
#include "render.h"

/* SWInitTiling (mac _SWInitTiling, mac-only). STATE: see the mac gate. */
// MAC: _SWInitTiling i386 0x000af116 ppc 0x000ba67c
OSErr SWInitTiling(
    SpriteWorldPtr  spriteWorldP,
    short           tileHeight,
    short           tileWidth,
    short           maxNumTiles)
{
    OSErr   err = noErr;
    Size    arraySize;
    short   tileIndex;

    SW_ASSERT(spriteWorldP != NULL, 64);

    if (spriteWorldP->tilingIsInitialized)
    {
        err = kTilingAlreadyInitialized;
    }

    if (err == noErr)
    {
        spriteWorldP->tilingIsOn = true;
        spriteWorldP->tilingIsInitialized = true;
        spriteWorldP->lastActiveTileLayer = 0;
        spriteWorldP->maxNumTiles = maxNumTiles;
        spriteWorldP->tileHeight = tileHeight;
        spriteWorldP->tileWidth = tileWidth;
    }

    if (err == noErr)
    {
        err = SWInitTilingCache(spriteWorldP);
    }

    if (err == noErr)
    {
        arraySize = maxNumTiles * sizeof(FramePtr);
        spriteWorldP->tileFrameArray = (FramePtr *)NewPtrClear(arraySize);
        err = MemError();
    }

    if (err == noErr)
    {
        arraySize = maxNumTiles * sizeof(short);
        spriteWorldP->curTileImage = (short *)NewPtr(arraySize);
        err = MemError();

        if (err == noErr)
        {
            for (tileIndex = 0; tileIndex < maxNumTiles; tileIndex++)
                spriteWorldP->curTileImage[tileIndex] = tileIndex;
        }
    }

    if (err == noErr)
    {
        arraySize = kNumTileLayers * sizeof(TileMapStructPtr);
        spriteWorldP->tileLayerArray = (TileMapStructPtr *)NewPtrClear(arraySize);
        err = MemError();
    }

    SWSetStickyIfError(err);
    return err;
}

/* SWExitTiling (mac _SWExitTiling, mac-only). STATE: see the mac gate. */
// MAC: _SWExitTiling i386 0x000af95d ppc 0x000baf04
void SWExitTiling(
    SpriteWorldPtr  spriteWorldP)
{
    short   tileIndex;

    SW_ASSERT(spriteWorldP != NULL, 138);

    if (spriteWorldP->tilingIsInitialized)
    {
        tileIndex = spriteWorldP->maxNumTiles;
        while (tileIndex--)
        {
            SWDisposeTile(spriteWorldP, tileIndex);
        }

        DisposePtr((Ptr)spriteWorldP->tileFrameArray);
        DisposePtr((Ptr)spriteWorldP->curTileImage);
        DisposePtr((Ptr)spriteWorldP->tileLayerArray);
        DisposePtr((Ptr)spriteWorldP->changedTiles);
        spriteWorldP->changedTiles = NULL;

        DisposePtr((Ptr)spriteWorldP->tilingCache[0]);
        DisposePtr((Ptr)spriteWorldP->tilingCache);
        spriteWorldP->tilingCache = NULL;

        spriteWorldP->tilingIsInitialized = false;
        spriteWorldP->tilingIsOn = false;
    }
}

/* SWInitTilingCache (mac _SWInitTilingCache, mac-only). STATE: see the mac gate. */
// MAC: _SWInitTilingCache i386 0x000aefa2 ppc 0x000ba4fc
OSErr SWInitTilingCache(
    SpriteWorldPtr  spriteWorldP)
{
    short   row, col, numTilingCacheCols, numTilingCacheRows, *tilingCacheData;
    Size    arraySize;
    OSErr   err = noErr;

    if (spriteWorldP->tilingCache != NULL)
    {
        DisposePtr((Ptr)spriteWorldP->tilingCache[0]);
        DisposePtr((Ptr)spriteWorldP->tilingCache);
    }

    if (spriteWorldP->changedTiles != NULL)
    {
        DisposePtr((Ptr)spriteWorldP->changedTiles);
    }

    numTilingCacheRows = spriteWorldP->backRect.bottom / spriteWorldP->tileHeight;
    numTilingCacheCols = spriteWorldP->backRect.right / spriteWorldP->tileWidth;
    spriteWorldP->numTilingCacheRows = numTilingCacheRows;
    spriteWorldP->numTilingCacheCols = numTilingCacheCols;

    spriteWorldP->changedTilesArraySize = (numTilingCacheRows+1) * (numTilingCacheCols+1);
    arraySize = (Size)(numTilingCacheRows+1) * (numTilingCacheCols+1) * sizeof(Rect);
    spriteWorldP->changedTiles = (Rect *)NewPtr(arraySize);
    err = MemError();

    if (err == noErr)
    {
        arraySize = (Size)numTilingCacheRows * sizeof(short*);
        spriteWorldP->tilingCache = (short **)NewPtr(arraySize);
        err = MemError();

        if (err == noErr)
        {
            arraySize = (Size)numTilingCacheRows * numTilingCacheCols * sizeof(short);
            tilingCacheData = (short *)NewPtr(arraySize);
            err = MemError();

            if (err != noErr)
                DisposePtr((Ptr)spriteWorldP->tilingCache);
        }

        if (err == noErr)
        {
            for (row = 0; row < numTilingCacheRows; row++)
                spriteWorldP->tilingCache[row] =
                    &tilingCacheData[(unsigned long)row * numTilingCacheCols];

            for (row = 0; row < numTilingCacheRows; row++)
                for (col = 0; col < numTilingCacheCols; col++)
                    spriteWorldP->tilingCache[row][col] = -1;
        }
    }

    if (err)
    {
        spriteWorldP->tilingCache = NULL;
        spriteWorldP->changedTiles = NULL;
    }

    return err;
}

/* SWCreateTileMap (mac _SWCreateTileMap, mac-only). STATE: see the mac gate. */
// MAC: _SWCreateTileMap i386 0x000af2d2 ppc 0x000ba870
OSErr SWCreateTileMap(
    TileMapStructPtr    *tileMapStructPP,
    short               numTileMapRows,
    short               numTileMapCols)
{
    TileMapStructPtr    tileMapStructP;
    Size                arraySize;
    OSErr               err = noErr;

    tileMapStructP = (TileMapStructPtr)NewPtrClear(sizeof(TileMapStruct));
    err = MemError();

    if (err == noErr)
    {
        tileMapStructP->numRows = numTileMapRows;
        tileMapStructP->numCols = numTileMapCols;

        arraySize = numTileMapRows * sizeof(short*);
        tileMapStructP->arrayOfPointersH = NewHandle(arraySize);
        err = MemError();
    }

    if (err == noErr)
    {
        arraySize = (numTileMapRows * numTileMapCols + 2) * sizeof(short);
        tileMapStructP->tileMapDataH = NewHandleClear(arraySize);
        err = MemError();
    }

    if (err == noErr)
    {
        SWLockTileMap(tileMapStructP);
    }

    if (err != noErr)
    {
        if (tileMapStructP != NULL)
        {
            DisposeHandle(tileMapStructP->arrayOfPointersH);
            DisposeHandle(tileMapStructP->tileMapDataH);
            DisposePtr((Ptr)tileMapStructP);
            tileMapStructP = NULL;
        }
    }

    *tileMapStructPP = tileMapStructP;

    SWSetStickyIfError(err);
    return err;
}

/* SWDisposeTileMap (mac _SWDisposeTileMap, mac-only). STATE: see the mac gate. */
// MAC: _SWDisposeTileMap i386 0x000af221 ppc 0x000ba774
void SWDisposeTileMap(
    TileMapStructPtr    *tileMapStructPP)
{
    TileMapStructPtr    myTileMapStructP = *tileMapStructPP;

    if (myTileMapStructP != NULL)
    {
        DisposeHandle(myTileMapStructP->arrayOfPointersH);
        DisposeHandle(myTileMapStructP->tileMapDataH);
        DisposePtr((Ptr)myTileMapStructP);
        *tileMapStructPP = NULL;
    }
}

/* SWLockTileMap (mac _SWLockTileMap, mac-only). STATE: see the mac gate. */
// MAC: _SWLockTileMap i386 0x000af262 ppc 0x000ba7d0
void SWLockTileMap(
    TileMapStructPtr    tileMapStructP)
{
    short       **arrayOfPointers;
    short       *tileMapDataP;
    short       row;

    SW_ASSERT(tileMapStructP != NULL, 340);

    HLockHi(tileMapStructP->arrayOfPointersH);
    HLockHi(tileMapStructP->tileMapDataH);

    tileMapDataP = (short*)*tileMapStructP->tileMapDataH;
    arrayOfPointers = (short**)*tileMapStructP->arrayOfPointersH;

    for (row = 0; row < tileMapStructP->numRows; row++)
    {
        arrayOfPointers[row] = &tileMapDataP[row * tileMapStructP->numCols + 2];
    }

    tileMapStructP->tileMap = arrayOfPointers;
    tileMapStructP->isLocked = true;
}

/* SWUnlockTileMap (mac _SWUnlockTileMap, mac-only). STATE: see the mac gate. */
// MAC: _SWUnlockTileMap i386 0x000af3a6 ppc 0x000ba938
void SWUnlockTileMap(
    TileMapStructPtr    tileMapStructP)
{
    SW_ASSERT(tileMapStructP != NULL, 367);

    HUnlock(tileMapStructP->arrayOfPointersH);
    HUnlock(tileMapStructP->tileMapDataH);
    tileMapStructP->tileMap = NULL;
    tileMapStructP->isLocked = false;
}

/* SWInstallTileMap (mac _SWInstallTileMap, mac-only). STATE: see the mac gate. */
// MAC: _SWInstallTileMap i386 0x000af3ef ppc 0x000ba990
void SWInstallTileMap(
    SpriteWorldPtr      spriteWorldP,
    TileMapStructPtr    tileMapStructP,
    short               tileLayer)
{
    short   curLayer;

    SW_ASSERT(spriteWorldP != NULL, 387);
    SW_ASSERT(tileLayer < kNumTileLayers, 388);

    spriteWorldP->tileLayerArray[tileLayer] = tileMapStructP;
    spriteWorldP->lastActiveTileLayer = 0;

    for (curLayer = 0; curLayer < kNumTileLayers; curLayer++)
    {
        if (spriteWorldP->tileLayerArray[curLayer] != NULL)
            spriteWorldP->lastActiveTileLayer = curLayer;
    }

    if (spriteWorldP->lastActiveTileLayer == 0)
    {
        spriteWorldP->tileRectDrawProc = SWDrawTilesInRect;
    }
    else
    {
        spriteWorldP->tileRectDrawProc = SWDrawTileLayersInRect;
    }
}

/* SWLoadTileMap (mac _SWLoadTileMap, mac-only). STATE: see the mac gate. */
// MAC: _SWLoadTileMap i386 0x000af486 ppc 0x000baa58
OSErr SWLoadTileMap(
    ResType             resType,
    TileMapStructPtr    *tileMapStructPP,
    short               resourceID)
{
    TileMapStructPtr    tileMapStructP;
    short               *tileMapData;
    Size                arraySize;
    OSErr               err = noErr;

    tileMapStructP = (TileMapStructPtr)NewPtrClear(sizeof(TileMapStruct));
    err = MemError();

    if (err == noErr)
    {
        tileMapStructP->tileMapDataH = Get1Resource(resType, resourceID);

        if (tileMapStructP->tileMapDataH == NULL)
            err = ResError() ? ResError() : resNotFound;
    }

    if (err == noErr)
    {
        DetachResource(tileMapStructP->tileMapDataH);

        tileMapData = (short*)*tileMapStructP->tileMapDataH;
        tileMapStructP->numRows = tileMapData[0];
        tileMapStructP->numCols = tileMapData[1];

        arraySize = tileMapStructP->numRows * sizeof(short*);
        tileMapStructP->arrayOfPointersH = NewHandle(arraySize);
        err = MemError();
    }

    if (err == noErr)
    {
        SWLockTileMap(tileMapStructP);
    }

    if (err != noErr)
    {
        if (tileMapStructP != NULL)
        {
            DisposeHandle(tileMapStructP->arrayOfPointersH);
            DisposeHandle(tileMapStructP->tileMapDataH);
            DisposePtr((Ptr)tileMapStructP);
            tileMapStructP = NULL;
        }
    }

    *tileMapStructPP = tileMapStructP;

    SWSetStickyIfError(err);
    return err;
}

/* SWSaveTileMap (mac _SWSaveTileMap, mac-only). STATE: see the mac gate. */
// MAC: _SWSaveTileMap i386 0x000af56a ppc 0x000bab4c
OSErr SWSaveTileMap(
    ResType             resType,
    TileMapStructPtr    tileMapStructP,
    short               destResID)
{
    short       tempResID, oldResID;
    short       *tileMapData;
    Handle      tempTileMapH;
    Str255      oldResName = "\p";
    ResType     oldResType;
    OSErr       err = noErr;

    if (tileMapStructP == NULL)
        err = kNullTileMapErr;

    if (err == noErr)
    {
        tileMapData = (short*)*tileMapStructP->tileMapDataH;
        tileMapData[0] = tileMapStructP->numRows;
        tileMapData[1] = tileMapStructP->numCols;

        tempResID = Unique1ID(resType);
        AddResource(tileMapStructP->tileMapDataH, resType, tempResID, "\p");
        err = ResError();
    }

    if (err == noErr)
    {
        do
        {
            tempTileMapH = Get1Resource(resType, destResID);

            if (tempTileMapH != NULL)
            {
                GetResInfo(tempTileMapH, &oldResID, &oldResType, oldResName);
                RemoveResource(tempTileMapH);

                err = ResError();
                SW_ASSERT(ResError() == noErr, 530);
            }
            else if (ResError() != resNotFound && ResError() != noErr)
            {
                err = ResError();
            }
        } while (tempTileMapH != NULL);

        UpdateResFile( CurResFile() );

        SetResInfo(tileMapStructP->tileMapDataH, destResID, oldResName);
        ChangedResource(tileMapStructP->tileMapDataH);

        UpdateResFile( CurResFile() );
        DetachResource(tileMapStructP->tileMapDataH);
    }

    SWSetStickyIfError(err);
    return err;
}

/* SWResizeTileMap (mac _SWResizeTileMap, mac-only). STATE: see the mac gate. */
// MAC: _SWResizeTileMap i386 0x000af71f ppc 0x000baca0
OSErr SWResizeTileMap(
    TileMapStructPtr    oldTileMapStructP,
    short               numNewTileMapRows,
    short               numNewTileMapCols)
{
    TileMapStruct       tempTileMapStruct;
    TileMapStructPtr    newTileMapStructP;
    short               row, numRowsToCopy, numColsToCopy;
    Size                arraySize;
    OSErr               err = noErr;

    SW_ASSERT(numNewTileMapRows > 0 && numNewTileMapCols > 0, 575);

    if (oldTileMapStructP == NULL)
        err = kNullTileMapErr;

    if (err == noErr)
    {
        if (oldTileMapStructP->numRows == numNewTileMapRows &&
            oldTileMapStructP->numCols == numNewTileMapCols)
        {
            return noErr;
        }

        err = SWCreateTileMap(&newTileMapStructP, numNewTileMapRows, numNewTileMapCols);
    }

    if (err == noErr)
    {
        if (oldTileMapStructP->isLocked == false)
            SWLockTileMap(oldTileMapStructP);

        numRowsToCopy = SW_MIN(oldTileMapStructP->numRows, numNewTileMapRows);
        numColsToCopy = SW_MIN(oldTileMapStructP->numCols, numNewTileMapCols);

        arraySize = numColsToCopy * sizeof(short);
        for (row = 0; row < numRowsToCopy; row++)
        {
            BlockMoveData(&oldTileMapStructP->tileMap[row][0],
                &newTileMapStructP->tileMap[row][0], arraySize);
        }

        tempTileMapStruct = *oldTileMapStructP;
        *oldTileMapStructP = *newTileMapStructP;
        *newTileMapStructP = tempTileMapStruct;

        SWDisposeTileMap(&newTileMapStructP);
    }

    SWSetStickyIfError(err);
    return err;
}

/* SWLoadTileFromCicnResource (mac _SWLoadTileFromCicnResource, mac-only). STATE: see the mac gate. */
// MAC: _SWLoadTileFromCicnResource i386 0x000afa31 ppc 0x000bafcc
OSErr SWLoadTileFromCicnResource(
    SpriteWorldPtr  spriteWorldP,
    short           tileID,
    short           cicnID,
    MaskType        maskType,
    Boolean         maskIsPartialMask)
{
    OSErr           err;
    GWorldPtr       saveGWorld;
    GDHandle        saveGDH;
    FramePtr        newFrameP;
    CIconHandle     cIconH;
    Rect            frameRect;
    Boolean         maskIsSolid = false;

    SW_ASSERT(spriteWorldP != NULL, 645);

    err = noErr;
    newFrameP = NULL;

    if ( !spriteWorldP->tilingIsInitialized )
    {
        err = kTilingNotInitialized;
    }
    else if ( tileID < 0 || tileID >= spriteWorldP->maxNumTiles)
    {
        err = kOutOfRangeErr;
    }

    if (maskType == kSolidMask)
    {
        maskIsSolid = true;
        maskType = kNoMask;
    }

    if ( err == noErr )
    {
        err = SWCreateFrameFromCicnResource(spriteWorldP, &newFrameP, cicnID, maskType);

        if ( err == noErr )
        {
            newFrameP->tileMaskIsSolid = maskIsSolid;

            if (newFrameP->frameRect.right > spriteWorldP->tileWidth)
                newFrameP->frameRect.right = spriteWorldP->tileWidth;
            if (newFrameP->frameRect.bottom > spriteWorldP->tileHeight)
                newFrameP->frameRect.bottom = spriteWorldP->tileHeight;

            cIconH = GetCIcon( cicnID );

            if (cIconH != NULL)
            {
                GetGWorld(&saveGWorld, &saveGDH);

                HLock((Handle)cIconH);
                frameRect = ((**cIconH).iconPMap.bounds);
                (**cIconH).iconPMap.baseAddr = *(**cIconH).iconData;
                (**cIconH).iconBMap.baseAddr = (Ptr)&((**cIconH).iconMaskData ) +
                    ((**cIconH).iconMask.rowBytes * (frameRect.bottom-frameRect.top));

                LockPixels( GetGWorldPixMap( newFrameP->framePort ) );
                SetGWorld( newFrameP->framePort, NULL );
                if ( spriteWorldP->pixelDepth > 1 )
                {
                    CopyBits( (BitMap*)(&((**cIconH).iconPMap)),
                        (BitMap*)*GetGWorldPixMap( newFrameP->framePort ),
                        &frameRect, &newFrameP->frameRect, srcCopy, nil);
                }
                else
                {
                    CopyBits( (&((**cIconH).iconBMap)),
                        (BitMap*)*GetGWorldPixMap( newFrameP->framePort ),
                        &frameRect,
                        &newFrameP->frameRect, srcCopy, nil);
                }

                if ( ((maskType & kPixelMask) != 0) && maskIsPartialMask == false )
                    SWFixImageGWorld( newFrameP->framePort, newFrameP->maskPort,  NULL );

                UnlockPixels( GetGWorldPixMap( newFrameP->framePort ) );
                DisposeCIcon( cIconH );
                SetGWorld( saveGWorld, NULL );
            }
            else
            {
                err = MemError();
            }
        }

        if ( err == noErr )
        {
            if (spriteWorldP->tileFrameArray[tileID] != NULL)
            {
                SWDisposeTile( spriteWorldP, tileID );
            }
            spriteWorldP->tileFrameArray[tileID] = newFrameP;
            newFrameP->useCount++;
        }
    }

    if ( err != noErr && newFrameP != NULL )
    {
        SWDisposeFrame(&newFrameP);
    }

    SWSetStickyIfError(err);
    return err;
}

/* SWLoadTilesFromPictResource (mac _SWLoadTilesFromPictResource, mac-only). STATE: see the mac gate. */
// MAC: _SWLoadTilesFromPictResource i386 0x000afcea ppc 0x000bb258
OSErr SWLoadTilesFromPictResource(
    SpriteWorldPtr  spriteWorldP,
    short           startTileID,
    short           endTileID,
    short           pictResID,
    short           maskResID,
    MaskType        maskType,
    Boolean         maskIsPartialMask,
    short           horizBorderWidth,
    short           vertBorderHeight)
{
    OSErr           err;
    short           tileIndex;
    FramePtr        newFrameP;
    GWorldPtr       tempPictGWorldP,
                    tempMaskGWorldP,
                    tempTempMaskGWorldP,
                    currentGWorld;
    GDHandle        currentGDH;
    Rect            currentTileRect;
    short           horizOffset,
                    vertOffset;
    Boolean         allTilesDone,
                    maskIsSolid,
                    needToInvertMask;

    SW_ASSERT(spriteWorldP != NULL, 777);

    err = noErr;
    newFrameP = NULL;
    tempPictGWorldP = NULL;
    tempMaskGWorldP = NULL;
    tempTempMaskGWorldP = NULL;
    maskIsSolid = false;
    needToInvertMask = true;

    if ( !spriteWorldP->tilingIsInitialized )
    {
        err = kTilingNotInitialized;
    }
    else if ( startTileID < 0 || endTileID >= spriteWorldP->maxNumTiles)
    {
        err = kOutOfRangeErr;
    }

    if (maskType == kSolidMask)
    {
        maskIsSolid = true;
        maskType = kNoMask;
    }

    if ( err == noErr )
    {
        err = SWCreateGWorldFromPictResource( spriteWorldP, &tempPictGWorldP, pictResID );

        if ( err == noErr && maskType != kNoMask )
            err = SWCreateGWorldFromPictResource( spriteWorldP, &tempMaskGWorldP, maskResID );

        if (err == noErr)
        {
            if ( pictResID == maskResID && tempMaskGWorldP != NULL )
            {
                err = SWCreatePixelMaskFromGWorld( tempPictGWorldP, tempMaskGWorldP,  NULL );
                needToInvertMask = false;

                if (spriteWorldP->pixelDepth <= 8 && (maskType & kRegionMask) != 0 )
                {
                    SWInvertGWorld(tempMaskGWorldP);
                    needToInvertMask = true;
                }
            }
            else if ( (maskType & kPixelMask) != 0 )
            {
                if (maskIsPartialMask == false)
                    SWFixImageGWorld( tempPictGWorldP, tempMaskGWorldP,  NULL );
            }
        }

        if ( err == noErr )
        {
            err = SWCreateFrameFromGWorldAndRectStart( &tempTempMaskGWorldP,
                    spriteWorldP->tileWidth, spriteWorldP->tileHeight, maskType );
        }

        if ( err == noErr )
        {
            SetRect( &currentTileRect, 0,  0,
                spriteWorldP->tileWidth, spriteWorldP->tileHeight );
            tileIndex = startTileID;
            horizOffset = spriteWorldP->tileWidth + horizBorderWidth;
            vertOffset = spriteWorldP->tileHeight + vertBorderHeight;

            allTilesDone = false;
            while( !allTilesDone && err == noErr )
            {
                err = SWCreateFrameFromGWorldAndRectPartial( &newFrameP, tempPictGWorldP,
                        tempMaskGWorldP, tempTempMaskGWorldP, &currentTileRect, maskType);

                if ( newFrameP != NULL && err == noErr )
                {
                    newFrameP->tileMaskIsSolid = maskIsSolid;

                    if (spriteWorldP->tileFrameArray[tileIndex] != NULL)
                    {
                        SWDisposeTile( spriteWorldP, tileIndex );
                    }
                    spriteWorldP->tileFrameArray[tileIndex] = newFrameP;
                    newFrameP->useCount++;

                    tileIndex++;
                    if (tileIndex > endTileID )
                    {
                        allTilesDone = true;
                    }
                    else
                    {
                        if (tileIndex >= spriteWorldP->maxNumTiles)
                        {
                            err = kOutOfRangeErr;
                        }
                        currentTileRect.left += horizOffset;
                        currentTileRect.right += horizOffset;
                        if ( currentTileRect.right > GetCPortPortRect(tempPictGWorldP)->right )
                        {
                            currentTileRect.left = 0;
                            currentTileRect.right = spriteWorldP->tileWidth;
                            currentTileRect.top += vertOffset;
                            currentTileRect.bottom += vertOffset;
                            if ( currentTileRect.bottom > GetCPortPortRect(tempPictGWorldP)->bottom )
                            {
                                err = kOutOfRangeErr;
                            }
                        }
                    }
                }
            }

            if (err == noErr)
            {
                if ((maskType & kPixelMask) != 0)
                {
                    if (spriteWorldP->pixelDepth <= 8 && needToInvertMask)
                    {
                        GetGWorld( &currentGWorld, &currentGDH );
                        SWInvertGWorld(tempMaskGWorldP);
                        SetGWorld( currentGWorld, NULL );
                    }
                }
                else
                {
                    if (tempMaskGWorldP != NULL)
                    {
                        DisposeGWorld( tempMaskGWorldP );
                        tempMaskGWorldP = NULL;
                    }
                }
            }
        }
        SWCreateFrameFromGWorldAndRectFinish( tempTempMaskGWorldP );
    }

    SWSetStickyIfError(err);
    return err;
}

/* SWDisposeTile (mac _SWDisposeTile, mac-only). STATE: see the mac gate. */
// MAC: _SWDisposeTile i386 0x000af899 ppc 0x000bae28
void SWDisposeTile(
    SpriteWorldPtr  spriteWorldP,
    short           tileID)
{
    short       tileIndex;
    Boolean     gWorldStillInUse;

    SW_ASSERT(spriteWorldP != NULL, 940);
    SW_ASSERT(spriteWorldP->tilingIsInitialized, 941);

    gWorldStillInUse = false;
    tileIndex = spriteWorldP->maxNumTiles;
    while ( tileIndex-- )
    {
        if ( tileIndex != tileID )
        {
            if ( spriteWorldP->tileFrameArray[tileIndex] != NULL &&
                ((spriteWorldP->tileFrameArray[tileIndex])->framePort ==
                (spriteWorldP->tileFrameArray[tileID])->framePort) )
            {
                gWorldStillInUse = true;
            }
        }
    }

    (spriteWorldP->tileFrameArray[tileID])->sharesGWorld = gWorldStillInUse;

    SWDisposeFrame( &spriteWorldP->tileFrameArray[tileID] );
    spriteWorldP->tileFrameArray[tileID] = NULL;
}

/* SWLockTiles (mac _SWLockTiles, mac-only). STATE: see the mac gate. */
// MAC: _SWLockTiles i386 0x000b00ad ppc 0x000bb5c8
void SWLockTiles(
    SpriteWorldPtr  spriteWorldP)
{
    short       tileIndex;

    SW_ASSERT(spriteWorldP != NULL, 975);

    if (spriteWorldP->tilingIsInitialized)
    {
        for (tileIndex = 0; tileIndex < spriteWorldP->maxNumTiles; tileIndex++)
        {
            if (spriteWorldP->tileFrameArray[tileIndex] != NULL)
                SWLockFrame(spriteWorldP->tileFrameArray[tileIndex]);
        }
    }
}

/* SWUnlockTiles (mac _SWUnlockTiles, mac-only). STATE: see the mac gate. */
// MAC: _SWUnlockTiles i386 0x000b0108 ppc 0x000bb64c
void SWUnlockTiles(
    SpriteWorldPtr  spriteWorldP)
{
    short       tileIndex;

    SW_ASSERT(spriteWorldP != NULL, 998);

    if (spriteWorldP->tilingIsInitialized)
    {
        for (tileIndex = 0; tileIndex < spriteWorldP->maxNumTiles; tileIndex++)
        {
            if (spriteWorldP->tileFrameArray[tileIndex] != NULL)
                SWUnlockFrame(spriteWorldP->tileFrameArray[tileIndex]);
        }
    }
}

/* SWCreateExtraBackFrame (mac _SWCreateExtraBackFrame, mac-only). STATE: see the mac gate. */
// MAC: _SWCreateExtraBackFrame i386 0x000b0163 ppc 0x000bb6d0
OSErr SWCreateExtraBackFrame(
    SpriteWorldPtr  spriteWorldP,
    Rect            *frameRect)
{
    OSErr   err = noErr;

    SW_ASSERT(spriteWorldP != NULL, 1022);
    SW_ASSERT(frameRect->right > frameRect->left && frameRect->bottom > frameRect->top, 1023);

    if (spriteWorldP->extraBackFrameP != NULL)
    {
        SWDisposeFrame(&spriteWorldP->extraBackFrameP);
    }

    OffsetRect(frameRect, -frameRect->left, -frameRect->top);

    err = SWCreateFrame(NULL, &spriteWorldP->extraBackFrameP,
            frameRect, spriteWorldP->pixelDepth, kCreateGWorld);

    return err;
}

/* SWDisposeExtraBackFrame (mac _SWDisposeExtraBackFrame, mac-only). STATE: see the mac gate. */
// MAC: _SWDisposeExtraBackFrame i386 0x000b0219 ppc 0x000bb790
void SWDisposeExtraBackFrame(
    SpriteWorldPtr  spriteWorldP)
{
    SW_ASSERT(spriteWorldP != NULL, 1047);
    SWDisposeFrame(&spriteWorldP->extraBackFrameP);
    spriteWorldP->extraBackFrameP = NULL;
}

/* SWSetPortToExtraBackFrame (mac _SWSetPortToExtraBackFrame, mac-only). STATE: see the mac gate. */
// MAC: _SWSetPortToExtraBackFrame i386 0x000b0253 ppc 0x000bb7dc
OSErr SWSetPortToExtraBackFrame(
    SpriteWorldPtr  spriteWorldP)
{
    OSErr   err = noErr;

    SW_ASSERT(spriteWorldP != NULL, 1062);

    if ( spriteWorldP->extraBackFrameP == NULL )
        err = kNilFrameErr;
    else if ( spriteWorldP->extraBackFrameP->isFrameLocked == false)
        err = kNotLockedErr;

    if (err == noErr)
        SetGWorld(spriteWorldP->extraBackFrameP->framePort, nil);

    SWSetStickyIfError( err );
    return err;
}

/* SWSetTilingOn (mac _SWSetTilingOn, mac-only). STATE: see the mac gate. */
// MAC: _SWSetTilingOn i386 0x000b02b0 ppc 0x000bb854
void SWSetTilingOn(
    SpriteWorldPtr  spriteWorldP,
    Boolean         tilingIsOn)
{
    SW_ASSERT(spriteWorldP != NULL, 1085);
    spriteWorldP->tilingIsOn = tilingIsOn;
}

/* SWChangeTileSize (mac _SWChangeTileSize, mac-only). STATE: see the mac gate. */
// MAC: _SWChangeTileSize i386 0x000b02e8 ppc 0x000bb8a0
OSErr SWChangeTileSize(
    SpriteWorldPtr  spriteWorldP,
    short           tileHeight,
    short           tileWidth)
{
    OSErr   err;

    SW_ASSERT(spriteWorldP != NULL, 1101);

    spriteWorldP->tileHeight = tileHeight;
    spriteWorldP->tileWidth = tileWidth;

    err = SWInitTilingCache(spriteWorldP);

    return err;
}

/* SWSetSpriteLayerUnderTileLayer (mac _SWSetSpriteLayerUnderTileLayer, mac-only). STATE: see the mac gate. */
// MAC: _SWSetSpriteLayerUnderTileLayer i386 0x000b033a ppc 0x000bb8f4
void SWSetSpriteLayerUnderTileLayer(
    SpriteLayerPtr  spriteLayerP,
    short           tileLayer)
{
    SW_ASSERT(spriteLayerP != NULL, 1121);
    spriteLayerP->tileLayer = tileLayer;
}

/* SWSetTileMaskDrawProc (mac _SWSetTileMaskDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _SWSetTileMaskDrawProc i386 0x000b0370 ppc 0x000bb940
OSErr SWSetTileMaskDrawProc(
    SpriteWorldPtr  spriteWorldP,
    DrawProcPtr     drawProc)
{
    OSErr   err = noErr;

    SW_ASSERT(spriteWorldP != NULL, 1136);
    SW_ASSERT(drawProc != NULL, 1137);

    if (drawProc == BlitPixieAllBitMaskDrawProc)
        err = k68kOnlyErr;

    if (drawProc == BlitPixiePartialMaskDrawProc ||
        drawProc == BlitPixieAllBitPartialMaskDrawProc)
    {
        err = kBadParameterErr;
    }
    else if (spriteWorldP->pixelDepth < 8)
    {
        if (drawProc == BlitPixieMaskDrawProc)
        {
            err = kWrongDepthErr;
        }
    }

    if ( err == noErr )
    {
        spriteWorldP->tileMaskDrawProc = drawProc;
    }

    SWSetStickyIfError( err );
    return err;
}

/* SWSetPartialMaskDrawProc (mac _SWSetPartialMaskDrawProc, mac-only). STATE: see the mac gate. */
// MAC: _SWSetPartialMaskDrawProc i386 0x000b0414 ppc 0x000bba1c
OSErr SWSetPartialMaskDrawProc(
    SpriteWorldPtr  spriteWorldP,
    DrawProcPtr     drawProc)
{
    OSErr   err = noErr;

    SW_ASSERT(spriteWorldP != NULL, 1177);
    SW_ASSERT(drawProc != NULL, 1178);

    if (drawProc == BlitPixieAllBitPartialMaskDrawProc)
        err = k68kOnlyErr;

    if (spriteWorldP->pixelDepth < 8)
    {
        if (drawProc == BlitPixiePartialMaskDrawProc)
        {
            err = kWrongDepthErr;
        }
    }

    if ( err == noErr )
    {
        spriteWorldP->partialMaskDrawProc = drawProc;
    }

    SWSetStickyIfError( err );
    return err;
}

/* SWSetTileChangeProc (mac _SWSetTileChangeProc, mac-only). STATE: see the mac gate. */
// MAC: _SWSetTileChangeProc i386 0x000b04a1 ppc 0x000bbad0
void SWSetTileChangeProc(
    SpriteWorldPtr      spriteWorldP,
    TileChangeProcPtr   tileChangeProc)
{
    SW_ASSERT(spriteWorldP != NULL, 1211);
    spriteWorldP->tileChangeProc = tileChangeProc;
}

/* SWSetTileMaskSolidMode (mac _SWSetTileMaskSolidMode, mac-only). STATE: see the mac gate. */
// MAC: _SWSetTileMaskSolidMode i386 0x000b04d2 ppc 0x000bbb18
void SWSetTileMaskSolidMode(
    SpriteWorldPtr      spriteWorldP,
    short               tileID,
    Boolean             isSolid)
{
    FramePtr    tileFrameP;

    SW_ASSERT(tileID >= 0 && tileID < spriteWorldP->maxNumTiles, 1227);

    tileFrameP = spriteWorldP->tileFrameArray[tileID];

    SW_ASSERT(tileFrameP != NULL, 1231);

    tileFrameP->tileMaskIsSolid = isSolid;
}

/* SWDrawTilesInBackground (mac _SWDrawTilesInBackground, mac-only). STATE: see the mac gate. */
// MAC: _SWDrawTilesInBackground i386 0x000b0542 ppc 0x000bbb98
OSErr SWDrawTilesInBackground(
    SpriteWorldPtr  spriteWorldP)
{
    GWorldPtr       holdGWorld;
    GDHandle        holdGDH;
    OSErr           err = noErr;

    SW_ASSERT(spriteWorldP != NULL, 1249);

    GetGWorld( &holdGWorld, &holdGDH );

    if ( !spriteWorldP->tilingIsInitialized )
        err = kTilingNotInitialized;

    if (err == noErr)
    {
        (*spriteWorldP->tileRectDrawProc)(spriteWorldP, &spriteWorldP->visScrollRect, true);
    }

    SetGWorld( holdGWorld, NULL );

    SWSetStickyIfError(err);
    return err;
}

/* SWDrawTile (mac _SWDrawTile, mac-only). STATE: see the mac gate. */
// MAC: _SWDrawTile i386 0x000b2197 ppc 0x000bd73c
void SWDrawTile(
    SpriteWorldPtr  spriteWorldP,
    short           dstTileLayer,
    short           tileRow,
    short           tileCol,
    short           tileID)
{
    short       row, col, offscreenTileRow, offscreenTileCol, tileLayer;
    Rect*       visScrollRectP = &spriteWorldP->visScrollRect;
    Rect*       backRectP = &spriteWorldP->backRect;
    short       tileHeight = spriteWorldP->tileHeight;
    short       tileWidth = spriteWorldP->tileWidth;
    Rect        srcRect, dstRect;
    FramePtr    tileFrameP;
    GWorldPtr   saveGWorld;
    GDHandle    saveGDH;
    Boolean     tileClipped, tileHasMask;

    if (spriteWorldP->tileLayerArray[dstTileLayer] == NULL)
        return;

    SW_ASSERT(spriteWorldP != NULL, 1299);
    SW_ASSERT(spriteWorldP->tilingIsInitialized, 1300);
    SW_ASSERT(spriteWorldP->tileMaskDrawProc != NULL, 1301);
    SW_ASSERT(spriteWorldP->offscreenDrawProc != NULL, 1302);
    SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked, 1303);
    SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked, 1304);
    SW_ASSERT(spriteWorldP->tileLayerArray[dstTileLayer] != NULL, 1305);
    SW_ASSERT(spriteWorldP->tileLayerArray[dstTileLayer]->isLocked, 1306);

    SW_ASSERT(dstTileLayer <= spriteWorldP->lastActiveTileLayer, 1309);
    SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[dstTileLayer]->numRows, 1310);
    SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[dstTileLayer]->numCols, 1311);
    SW_ASSERT(tileID < spriteWorldP->maxNumTiles, 1312);

    GetGWorld( &saveGWorld, &saveGDH );
    SetGWorld(spriteWorldP->backFrameP->framePort, nil);

    spriteWorldP->tileLayerArray[dstTileLayer]->tileMap[tileRow][tileCol] = tileID;

    row = tileRow * tileHeight;
    col = tileCol * tileWidth;

    dstRect.bottom = row + tileHeight;
    dstRect.right = col + tileWidth;

    tileClipped = false;
    if (row < visScrollRectP->top)
    {
        dstRect.top = visScrollRectP->top;
        tileClipped = true;
    }
    else
        dstRect.top = row;

    if (col < visScrollRectP->left)
    {
        dstRect.left = visScrollRectP->left;
        tileClipped = true;
    }
    else
        dstRect.left = col;

    if (dstRect.bottom > visScrollRectP->bottom)
    {
        dstRect.bottom = visScrollRectP->bottom;
        tileClipped = true;
    }

    if (dstRect.right > visScrollRectP->right)
    {
        dstRect.right = visScrollRectP->right;
        tileClipped = true;
    }

    if (dstRect.left < dstRect.right && dstRect.top < dstRect.bottom)
    {
        SWAddChangedRect(spriteWorldP, &dstRect);

        if (spriteWorldP->tileLayerArray[0] == NULL)
            tileID = -1;
        else
            tileID = spriteWorldP->tileLayerArray[0]->tileMap[tileRow][tileCol];

        if (tileID >= 0)
        {
            tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
            SW_ASSERT(tileFrameP != NULL, 1377);
            SW_ASSERT(tileFrameP->isFrameLocked, 1378);

            tileHasMask = (tileFrameP->maskPort != NULL || tileFrameP->maskRgn != NULL);
        }
        else
            tileHasMask = false;

        if ( (tileID == -1 || tileHasMask) && spriteWorldP->extraBackFrameP != NULL)
        {
            SWWrapRectFromExtraBackFrame(spriteWorldP, &dstRect);
        }
        else
        {
            dstRect.top -= spriteWorldP->vertScrollRectOffset;
            dstRect.bottom -= spriteWorldP->vertScrollRectOffset;
            dstRect.left -= spriteWorldP->horizScrollRectOffset;
            dstRect.right -= spriteWorldP->horizScrollRectOffset;

            if (dstRect.bottom > backRectP->bottom)
            {
                dstRect.top -= backRectP->bottom;
                dstRect.bottom -= backRectP->bottom;
            }
            else if (dstRect.top < backRectP->top)
            {
                dstRect.top += backRectP->bottom;
                dstRect.bottom += backRectP->bottom;
            }

            if (dstRect.right > backRectP->right)
            {
                dstRect.left -= backRectP->right;
                dstRect.right -= backRectP->right;
            }
            else if (dstRect.left < backRectP->left)
            {
                dstRect.left += backRectP->right;
                dstRect.right += backRectP->right;
            }
        }

        if (tileID >= 0)
        {
            srcRect = tileFrameP->frameRect;

            if (row < visScrollRectP->top)
                srcRect.top += visScrollRectP->top - row;
            if (col < visScrollRectP->left)
                srcRect.left += visScrollRectP->left - col;
            if (row + tileHeight > visScrollRectP->bottom)
                srcRect.bottom -= row + tileHeight - visScrollRectP->bottom;
            if (col + tileWidth > visScrollRectP->right)
                srcRect.right -= col + tileWidth - visScrollRectP->right;

            if (tileHasMask && spriteWorldP->extraBackFrameP != NULL)
            {
                (*spriteWorldP->tileMaskDrawProc)(tileFrameP,
                        spriteWorldP->backFrameP, &srcRect, &dstRect);
            }
            else
            {
                (*spriteWorldP->offscreenDrawProc)(tileFrameP,
                        spriteWorldP->backFrameP, &srcRect, &dstRect);
            }
        }

        for (tileLayer = 1; tileLayer <= spriteWorldP->lastActiveTileLayer; tileLayer++)
        {
            if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
                continue;

            SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[tileLayer]->numRows, 1465);
            SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[tileLayer]->numCols, 1466);

            tileID = spriteWorldP->tileLayerArray[tileLayer]->tileMap[tileRow][tileCol];
            if (tileID < 0)
                continue;

            SW_ASSERT(tileID < spriteWorldP->maxNumTiles, 1472);

            tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
            SW_ASSERT(tileFrameP != NULL, 1475);
            SW_ASSERT(tileFrameP->isFrameLocked, 1476);

            srcRect = tileFrameP->frameRect;

            if (row < visScrollRectP->top)
                srcRect.top += visScrollRectP->top - row;
            if (col < visScrollRectP->left)
                srcRect.left += visScrollRectP->left - col;
            if (row + tileHeight > visScrollRectP->bottom)
                srcRect.bottom -= row + tileHeight - visScrollRectP->bottom;
            if (col + tileWidth > visScrollRectP->right)
                srcRect.right -= col + tileWidth - visScrollRectP->right;

            if (tileFrameP->maskPort == NULL && tileFrameP->maskRgn == NULL)
            {
                (*spriteWorldP->offscreenDrawProc)(tileFrameP,
                        spriteWorldP->backFrameP, &srcRect, &dstRect);
            }
            else
            {
                (*spriteWorldP->tileMaskDrawProc)(tileFrameP,
                        spriteWorldP->backFrameP, &srcRect, &dstRect);
            }
        }

        SetGWorld(spriteWorldP->workFrameP->framePort, nil);
        (*spriteWorldP->offscreenDrawProc)(spriteWorldP->backFrameP,
                spriteWorldP->workFrameP, &dstRect, &dstRect);

        if (spriteWorldP->lastActiveTileLayer == 0)
        {
            offscreenTileRow = dstRect.top / spriteWorldP->tileHeight;
            offscreenTileCol = dstRect.left / spriteWorldP->tileWidth;

            if (tileClipped || tileID < 0)
            {
                spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] = -1;
            }
            else
            {
                spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] =
                    spriteWorldP->curTileImage[tileID];
            }
        }
    }

    SetGWorld( saveGWorld, NULL );
}

/* SWDrawTilesInRect (mac _SWDrawTilesInRect, mac-only). STATE: see the mac gate. */
// MAC: _SWDrawTilesInRect i386 0x000b0b5a ppc 0x000bc254
void SWDrawTilesInRect(
    SpriteWorldPtr  spriteWorldP,
    Rect*           updateRectP,
    Boolean         optimizingOn)
{
    short       row, col, tileRow, tileCol, tileID;
    short       startRow, startCol, stopRow, stopCol;
    short       offscreenTileRow, offscreenTileCol;
    Rect        srcRect, dstRect;
    FramePtr    tileFrameP;
    Boolean     tileClipped, tileHasMask;
    Rect*       visScrollRectP = &spriteWorldP->visScrollRect;
    Rect*       backRectP = &spriteWorldP->backRect;
    Rect        updateRect = *updateRectP;
    short       tileWidth = spriteWorldP->tileWidth;
    short       tileHeight = spriteWorldP->tileHeight;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized, 1557);
    SW_ASSERT(spriteWorldP->tileMaskDrawProc != NULL, 1558);
    SW_ASSERT(spriteWorldP->offscreenDrawProc != NULL, 1559);
    SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked, 1560);

    SW_ASSERT(spriteWorldP->tileLayerArray[0] != NULL, 1562);
    SW_ASSERT(spriteWorldP->tileLayerArray[0]->isLocked, 1563);
    SW_ASSERT(tileWidth * spriteWorldP->tileLayerArray[0]->numCols < 32767, 1564);
    SW_ASSERT(tileHeight * spriteWorldP->tileLayerArray[0]->numRows < 32767, 1565);

    SetGWorld(spriteWorldP->backFrameP->framePort, nil);

    startRow = updateRect.top / tileHeight;
    startCol = updateRect.left / tileWidth;
    stopRow = (updateRect.bottom-1) / tileHeight;
    stopCol = (updateRect.right-1) / tileWidth;

    row = startRow * tileHeight;
    for (tileRow = startRow; tileRow <= stopRow; tileRow++, row += tileHeight)
    {
        col = startCol * tileWidth;
        for (tileCol = startCol; tileCol <= stopCol; tileCol++, col += tileWidth)
        {
            if (spriteWorldP->tileLayerArray[0] != NULL)
            {
                SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[0]->numRows, 1584);
                SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[0]->numCols, 1585);
            }

            dstRect.bottom = row + tileHeight;
            dstRect.right = col + tileWidth;

            if (row >= visScrollRectP->top && col >= visScrollRectP->left &&
                dstRect.bottom <= visScrollRectP->bottom &&
                dstRect.right <= visScrollRectP->right)
            {
                tileClipped = false;
            }
            else
            {
                tileClipped = true;
            }

            if (row < updateRect.top)
                dstRect.top = updateRect.top;
            else
                dstRect.top = row;

            if (col < updateRect.left)
                dstRect.left = updateRect.left;
            else
                dstRect.left = col;

            if (dstRect.bottom > updateRect.bottom)
                dstRect.bottom = updateRect.bottom;

            if (dstRect.right > updateRect.right)
                dstRect.right = updateRect.right;

            if (spriteWorldP->tileLayerArray[0] == NULL)
                tileID = -1;
            else
                tileID = spriteWorldP->tileLayerArray[0]->tileMap[tileRow][tileCol];

            SW_ASSERT(tileID < spriteWorldP->maxNumTiles, 1625);

            if (tileID >= 0)
            {
                tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
                SW_ASSERT(tileFrameP != NULL, 1631);
                SW_ASSERT(tileFrameP->isFrameLocked, 1632);

                tileHasMask = (tileFrameP->maskPort != NULL || tileFrameP->maskRgn != NULL);
            }
            else
                tileHasMask = false;

            if ( (tileID == -1 || tileHasMask) && spriteWorldP->extraBackFrameP != NULL)
            {
                SWWrapRectFromExtraBackFrame(spriteWorldP, &dstRect);
            }
            else
            {
                dstRect.top -= spriteWorldP->vertScrollRectOffset;
                dstRect.bottom -= spriteWorldP->vertScrollRectOffset;
                dstRect.left -= spriteWorldP->horizScrollRectOffset;
                dstRect.right -= spriteWorldP->horizScrollRectOffset;

                if (dstRect.bottom > backRectP->bottom)
                {
                    dstRect.top -= backRectP->bottom;
                    dstRect.bottom -= backRectP->bottom;
                }
                else if (dstRect.top < backRectP->top)
                {
                    dstRect.top += backRectP->bottom;
                    dstRect.bottom += backRectP->bottom;
                }

                if (dstRect.right > backRectP->right)
                {
                    dstRect.left -= backRectP->right;
                    dstRect.right -= backRectP->right;
                }
                else if (dstRect.left < backRectP->left)
                {
                    dstRect.left += backRectP->right;
                    dstRect.right += backRectP->right;
                }
            }

            offscreenTileRow = dstRect.top / tileHeight;
            offscreenTileCol = dstRect.left / tileWidth;

            if (tileID >= 0)
            {
                srcRect = tileFrameP->frameRect;

                if (row < updateRect.top)
                    srcRect.top += updateRect.top - row;
                if (col < updateRect.left)
                    srcRect.left += updateRect.left - col;
                if (row + tileHeight > updateRect.bottom)
                    srcRect.bottom -= row + tileHeight - updateRect.bottom;
                if (col + tileWidth > updateRect.right)
                    srcRect.right -= col + tileWidth - updateRect.right;

                if (tileHasMask && spriteWorldP->extraBackFrameP != NULL)
                {
                    (*spriteWorldP->tileMaskDrawProc)(tileFrameP,
                            spriteWorldP->backFrameP, &srcRect, &dstRect);

                    spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] = -1;
                }
                else
                {
                    if (spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] !=
                        spriteWorldP->curTileImage[tileID] || !optimizingOn)
                    {
                        (*spriteWorldP->offscreenDrawProc)(tileFrameP,
                                spriteWorldP->backFrameP, &srcRect, &dstRect);

                        if (tileClipped)
                        {
                            spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] = -1;
                        }
                        else
                        {
                            spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] =
                                spriteWorldP->curTileImage[tileID];
                        }
                    }
                }
            }
            else
            {
                spriteWorldP->tilingCache[offscreenTileRow][offscreenTileCol] = -1;

                SW_ASSERT(spriteWorldP->extraBackFrameP != NULL, 1739);
            }
        }
    }
}

/* SWDrawTileLayersInRect (mac _SWDrawTileLayersInRect, mac-only). STATE: see the mac gate. */
// MAC: _SWDrawTileLayersInRect i386 0x000b12b3 ppc 0x000bc900
void SWDrawTileLayersInRect(
    SpriteWorldPtr  spriteWorldP,
    Rect*           updateRectP,
    Boolean         optimizingOn)
{
    #pragma     unused(optimizingOn)
    short       row, col, tileRow, tileCol, tileID;
    short       startRow, startCol, stopRow, stopCol;
    short       tileLayer, startPixelCol;
    Rect        srcRect, dstRect;
    FramePtr    tileFrameP;
    Rect*       visScrollRectP = &spriteWorldP->visScrollRect;
    Rect*       backRectP = &spriteWorldP->backRect;
    Rect        updateRect = *updateRectP;
    short       tileWidth = spriteWorldP->tileWidth;
    short       tileHeight = spriteWorldP->tileHeight;
    Boolean     tileHasMask;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized, 1768);
    SW_ASSERT(spriteWorldP->tileMaskDrawProc != NULL, 1769);
    SW_ASSERT(spriteWorldP->offscreenDrawProc != NULL, 1770);
    SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked, 1771);

    SetGWorld(spriteWorldP->backFrameP->framePort, nil);

    startRow = updateRect.top / tileHeight;
    startCol = updateRect.left / tileWidth;
    stopRow = (updateRect.bottom-1) / tileHeight;
    stopCol = (updateRect.right-1) / tileWidth;

    startPixelCol = startCol * tileWidth;

    row = startRow * tileHeight;
    for (tileRow = startRow; tileRow <= stopRow; tileRow++, row += tileHeight)
    {
        col = startPixelCol;
        for (tileCol = startCol; tileCol <= stopCol; tileCol++, col += tileWidth)
        {
            if (spriteWorldP->tileLayerArray[0] != NULL)
            {
                SW_ASSERT(spriteWorldP->tileLayerArray[0]->isLocked, 1791);
                SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[0]->numRows, 1792);
                SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[0]->numCols, 1793);
            }

            dstRect.bottom = row + tileHeight;
            dstRect.right = col + tileWidth;

            if (row < updateRect.top)
                dstRect.top = updateRect.top;
            else
                dstRect.top = row;

            if (col < updateRect.left)
                dstRect.left = updateRect.left;
            else
                dstRect.left = col;

            if (dstRect.bottom > updateRect.bottom)
                dstRect.bottom = updateRect.bottom;

            if (dstRect.right > updateRect.right)
                dstRect.right = updateRect.right;

            if (spriteWorldP->tileLayerArray[0] == NULL)
                tileID = -1;
            else
                tileID = spriteWorldP->tileLayerArray[0]->tileMap[tileRow][tileCol];

            SW_ASSERT(tileID < spriteWorldP->maxNumTiles, 1821);

            if (tileID >= 0)
            {
                tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
                SW_ASSERT(tileFrameP != NULL, 1826);
                SW_ASSERT(tileFrameP->isFrameLocked, 1827);

                tileHasMask = (tileFrameP->maskPort != NULL || tileFrameP->maskRgn != NULL);
            }
            else
                tileHasMask = false;

            if ( (tileID == -1 || tileHasMask) && spriteWorldP->extraBackFrameP != NULL)
            {
                SWWrapRectFromExtraBackFrame(spriteWorldP, &dstRect);
            }
            else
            {
                dstRect.top -= spriteWorldP->vertScrollRectOffset;
                dstRect.bottom -= spriteWorldP->vertScrollRectOffset;
                dstRect.left -= spriteWorldP->horizScrollRectOffset;
                dstRect.right -= spriteWorldP->horizScrollRectOffset;

                if (dstRect.bottom > backRectP->bottom)
                {
                    dstRect.top -= backRectP->bottom;
                    dstRect.bottom -= backRectP->bottom;
                }
                else if (dstRect.top < backRectP->top)
                {
                    dstRect.top += backRectP->bottom;
                    dstRect.bottom += backRectP->bottom;
                }

                if (dstRect.right > backRectP->right)
                {
                    dstRect.left -= backRectP->right;
                    dstRect.right -= backRectP->right;
                }
                else if (dstRect.left < backRectP->left)
                {
                    dstRect.left += backRectP->right;
                    dstRect.right += backRectP->right;
                }
            }

            if (tileID >= 0)
            {
                srcRect = tileFrameP->frameRect;

                if (row < updateRect.top)
                    srcRect.top += updateRect.top - row;
                if (col < updateRect.left)
                    srcRect.left += updateRect.left - col;
                if (row + tileHeight > updateRect.bottom)
                    srcRect.bottom -= row + tileHeight - updateRect.bottom;
                if (col + tileWidth > updateRect.right)
                    srcRect.right -= col + tileWidth - updateRect.right;

                if (tileHasMask && spriteWorldP->extraBackFrameP != NULL)
                {
                    (*spriteWorldP->tileMaskDrawProc)(tileFrameP,
                            spriteWorldP->backFrameP, &srcRect, &dstRect);
                }
                else
                {
                    (*spriteWorldP->offscreenDrawProc)(tileFrameP,
                            spriteWorldP->backFrameP, &srcRect, &dstRect);
                }
            }

            for (tileLayer = 1; tileLayer <= spriteWorldP->lastActiveTileLayer; tileLayer++)
            {
                if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
                    continue;

                SW_ASSERT(spriteWorldP->tileLayerArray[tileLayer]->isLocked, 1914);
                SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[tileLayer]->numRows, 1915);
                SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[tileLayer]->numCols, 1916);

                tileID = spriteWorldP->tileLayerArray[tileLayer]->tileMap[tileRow][tileCol];
                if (tileID < 0)
                    continue;

                SW_ASSERT(tileID < spriteWorldP->maxNumTiles, 1922);

                tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
                SW_ASSERT(tileFrameP != NULL, 1925);
                SW_ASSERT(tileFrameP->isFrameLocked, 1926);

                srcRect = tileFrameP->frameRect;

                if (row < updateRect.top)
                    srcRect.top += updateRect.top - row;
                if (col < updateRect.left)
                    srcRect.left += updateRect.left - col;
                if (row + tileHeight > updateRect.bottom)
                    srcRect.bottom -= row + tileHeight - updateRect.bottom;
                if (col + tileWidth > updateRect.right)
                    srcRect.right -= col + tileWidth - updateRect.right;

                if (tileFrameP->maskPort == NULL && tileFrameP->maskRgn == NULL)
                {
                    (*spriteWorldP->offscreenDrawProc)(tileFrameP,
                            spriteWorldP->backFrameP, &srcRect, &dstRect);
                }
                else
                {
                    (*spriteWorldP->tileMaskDrawProc)(tileFrameP,
                            spriteWorldP->backFrameP, &srcRect, &dstRect);
                }
            }
        }
    }
}

/* SWDrawTilesAboveSprite (mac _SWDrawTilesAboveSprite, mac-only). STATE: see the mac gate. */
// MAC: _SWDrawTilesAboveSprite i386 0x000b1a58 ppc 0x000bcfc8
void SWDrawTilesAboveSprite(
    SpriteWorldPtr  spriteWorldP,
    Rect*           updateRectP,
    short           startLayer)
{
    Rect*       backRectP = &spriteWorldP->backRect;
    Rect        updateRect = *updateRectP;
    short       tileLayer;
    short       tileWidth = spriteWorldP->tileWidth;
    short       tileHeight = spriteWorldP->tileHeight;
    short       row, col, tileRow, tileCol, tileID;
    short       startRow, startCol, stopRow, stopCol;
    short       startPixelRow, startPixelCol;
    Rect        srcRect, dstRect;
    FramePtr    tileFrameP;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized, 1980);
    SW_ASSERT(startLayer >= 0, 1981);

    startRow = updateRect.top / tileHeight;
    startCol = updateRect.left / tileWidth;
    stopRow = (updateRect.bottom-1) / tileHeight;
    stopCol = (updateRect.right-1) / tileWidth;

    startPixelRow = startRow * tileHeight;
    startPixelCol = startCol * tileWidth;

    for (tileLayer = startLayer; tileLayer <= spriteWorldP->lastActiveTileLayer; tileLayer++)
    {
        if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
            continue;

        row = startPixelRow;
        for (tileRow = startRow; tileRow <= stopRow; tileRow++, row += tileHeight)
        {
            col = startPixelCol;
            for (tileCol = startCol; tileCol <= stopCol; tileCol++, col += tileWidth)
            {
                SW_ASSERT(tileRow < spriteWorldP->tileLayerArray[tileLayer]->numRows, 2003);
                SW_ASSERT(tileCol < spriteWorldP->tileLayerArray[tileLayer]->numCols, 2004);

                tileID = spriteWorldP->tileLayerArray[tileLayer]->tileMap[tileRow][tileCol];
                if (tileID < 0)
                    continue;

                SW_ASSERT(tileID < spriteWorldP->maxNumTiles, 2010);

                tileFrameP = spriteWorldP->tileFrameArray[spriteWorldP->curTileImage[tileID]];
                SW_ASSERT(tileFrameP != NULL, 2013);

                if (tileLayer == 0 && spriteWorldP->extraBackFrameP == NULL &&
                    tileFrameP->maskPort == NULL && tileFrameP->maskRgn == NULL &&
                    tileFrameP->tileMaskIsSolid == false)
                {
                    continue;
                }

                srcRect = tileFrameP->frameRect;
                dstRect.bottom = row + tileHeight;
                dstRect.right = col + tileWidth;

                if (row < updateRect.top)
                {
                    dstRect.top = updateRect.top;
                    srcRect.top += updateRect.top - row;
                }
                else
                    dstRect.top = row;

                if (col < updateRect.left)
                {
                    dstRect.left = updateRect.left;
                    srcRect.left += updateRect.left - col;
                }
                else
                    dstRect.left = col;

                if (dstRect.bottom > updateRect.bottom)
                {
                    srcRect.bottom -= dstRect.bottom - updateRect.bottom;
                    dstRect.bottom = updateRect.bottom;
                }

                if (dstRect.right > updateRect.right)
                {
                    srcRect.right -= dstRect.right - updateRect.right;
                    dstRect.right = updateRect.right;
                }

                dstRect.top -= spriteWorldP->vertScrollRectOffset;
                dstRect.bottom -= spriteWorldP->vertScrollRectOffset;
                dstRect.left -= spriteWorldP->horizScrollRectOffset;
                dstRect.right -= spriteWorldP->horizScrollRectOffset;

                if (dstRect.bottom > backRectP->bottom)
                {
                    dstRect.top -= backRectP->bottom;
                    dstRect.bottom -= backRectP->bottom;
                }
                else if (dstRect.top < backRectP->top)
                {
                    dstRect.top += backRectP->bottom;
                    dstRect.bottom += backRectP->bottom;
                }

                if (dstRect.right > backRectP->right)
                {
                    dstRect.left -= backRectP->right;
                    dstRect.right -= backRectP->right;
                }
                else if (dstRect.left < backRectP->left)
                {
                    dstRect.left += backRectP->right;
                    dstRect.right += backRectP->right;
                }

                if (tileLayer == 0 && spriteWorldP->extraBackFrameP == NULL)
                {
                    if (tileFrameP->tileMaskIsSolid)
                    {
                        (*spriteWorldP->offscreenDrawProc)(tileFrameP,
                            spriteWorldP->workFrameP, &srcRect, &dstRect);
                    }
                    else
                    {
                        (*spriteWorldP->partialMaskDrawProc)(tileFrameP,
                                spriteWorldP->workFrameP, &srcRect, &dstRect);
                    }
                }
                else
                {
                    if (tileFrameP->maskPort == NULL && tileFrameP->maskRgn == NULL)
                    {
                        (*spriteWorldP->offscreenDrawProc)(tileFrameP,
                                spriteWorldP->workFrameP, &srcRect, &dstRect);
                    }
                    else
                    {
                        (*spriteWorldP->tileMaskDrawProc)(tileFrameP,
                                spriteWorldP->workFrameP, &srcRect, &dstRect);
                    }
                }
            }
        }
    }
}

/* SWResetTilingCache (mac _SWResetTilingCache, mac-only). STATE: see the mac gate. */
// MAC: _SWResetTilingCache i386 0x000b1f1e ppc 0x000bd3ec
void SWResetTilingCache(
    SpriteWorldPtr  spriteWorldP)
{
    short   row, col;

    SW_ASSERT(spriteWorldP != NULL, 2139);
    SW_ASSERT(spriteWorldP->tilingIsInitialized, 2140);

    for (row = 0; row < spriteWorldP->numTilingCacheRows; row++)
    {
        for (col = 0; col < spriteWorldP->numTilingCacheCols; col++)
        {
            spriteWorldP->tilingCache[row][col] = -1;
        }
    }
}

/* SWAddChangedRect (mac _SWAddChangedRect, mac-only). STATE: see the mac gate. */
// MAC: _SWAddChangedRect i386 0x000b1fa1 ppc 0x000bd494
void SWAddChangedRect(
    SpriteWorldPtr  spriteWorldP,
    Rect            *changedRectP)
{
    short   index;
    Rect    *changedTileP;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized, 2164);
    SW_ASSERT(changedRectP->top >= 0 && changedRectP->left >= 0 &&
        changedRectP->right > changedRectP->left && changedRectP->bottom > changedRectP->top, 2166);

    changedTileP = spriteWorldP->changedTiles;
    for ( index = 0; index < spriteWorldP->numTilesChanged; index++, changedTileP++ )
    {
        if ( changedTileP->left <= changedRectP->left &&
             changedTileP->top <= changedRectP->top &&
             changedTileP->right >= changedRectP->right &&
             changedTileP->bottom >= changedRectP->bottom )
        {
            return;
        }
    }

    changedTileP = spriteWorldP->changedTiles;
    for ( index = 0; index < spriteWorldP->numTilesChanged; index++, changedTileP++ )
    {
        if ( changedTileP->top == changedRectP->top &&
             changedTileP->bottom == changedRectP->bottom )
        {
            if ( changedRectP->left <= changedTileP->left &&
                 changedRectP->right >= changedTileP->left ||
                 changedRectP->left <= changedTileP->right &&
                 changedRectP->right >= changedTileP->right )
            {
                changedTileP->left = SW_MIN( changedTileP->left, changedRectP->left );
                changedTileP->right = SW_MAX( changedTileP->right, changedRectP->right );
                return;
            }
        }

        if ( changedTileP->left == changedRectP->left &&
             changedTileP->right == changedRectP->right )
        {
            if ( changedRectP->top <= changedTileP->top &&
                 changedRectP->bottom >= changedTileP->top ||
                 changedRectP->top <= changedTileP->bottom &&
                 changedRectP->bottom >= changedTileP->bottom )
            {
                changedTileP->top = SW_MIN( changedTileP->top, changedRectP->top );
                changedTileP->bottom = SW_MAX( changedTileP->bottom, changedRectP->bottom );
                return;
            }
        }
    }

    if (spriteWorldP->numTilesChanged < spriteWorldP->changedTilesArraySize)
    {
        spriteWorldP->changedTiles[spriteWorldP->numTilesChanged++] = *changedRectP;
    }
    else
    {
        SW_ASSERT(0, 2222);
        SWFlagRectAsChanged(spriteWorldP, changedRectP);
    }
}

/* SWChangeTileImage (mac _SWChangeTileImage, mac-only). STATE: see the mac gate. */
// MAC: _SWChangeTileImage i386 0x000b2a7e ppc 0x000bdffc
void SWChangeTileImage(
    SpriteWorldPtr  spriteWorldP,
    short           tileID,
    short           newImage)
{
    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized, 2237);
    SW_ASSERT(tileID < spriteWorldP->maxNumTiles, 2238);
    SW_ASSERT(newImage < spriteWorldP->maxNumTiles, 2239);

    spriteWorldP->curTileImage[tileID] = newImage;

    SWUpdateTileOnScreen(spriteWorldP, tileID);
}

/* SWUpdateTileOnScreen (mac _SWUpdateTileOnScreen, mac-only). STATE: see the mac gate. */
// MAC: _SWUpdateTileOnScreen i386 0x000b2936 ppc 0x000bdecc
void SWUpdateTileOnScreen(
    SpriteWorldPtr  spriteWorldP,
    short           tileID)
{
    short       tileRow, tileCol, tileLayer;
    short       startRow, startCol, stopRow, stopCol;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized, 2260);

    startRow = spriteWorldP->visScrollRect.top / spriteWorldP->tileHeight;
    startCol = spriteWorldP->visScrollRect.left / spriteWorldP->tileWidth;
    stopRow = (spriteWorldP->visScrollRect.bottom-1) / spriteWorldP->tileHeight;
    stopCol = (spriteWorldP->visScrollRect.right-1) / spriteWorldP->tileWidth;

    for (tileLayer = 0; tileLayer <= spriteWorldP->lastActiveTileLayer; tileLayer++)
    {
        if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
            continue;

        for (tileRow = startRow; tileRow <= stopRow; tileRow++)
        {
            for (tileCol = startCol; tileCol <= stopCol; tileCol++)
            {
                if (tileID == spriteWorldP->tileLayerArray[tileLayer]->tileMap[tileRow][tileCol])
                    SWDrawTile(spriteWorldP, tileLayer, tileRow, tileCol, tileID);
            }
        }
    }
}

/* SWResetCurrentTileImages (mac _SWResetCurrentTileImages, mac-only). STATE: see the mac gate. */
// MAC: _SWResetCurrentTileImages i386 0x000b2b15 ppc 0x000be0a8
void SWResetCurrentTileImages(
    SpriteWorldPtr  spriteWorldP)
{
    short   tileIndex;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized, 2294);

    if (spriteWorldP->tilingIsInitialized)
    {
        for (tileIndex = 0; tileIndex < spriteWorldP->maxNumTiles; tileIndex++)
            spriteWorldP->curTileImage[tileIndex] = tileIndex;
    }
}

/* SWReturnTileUnderPixel (mac _SWReturnTileUnderPixel, mac-only). STATE: see the mac gate. */
// MAC: _SWReturnTileUnderPixel i386 0x000b2b6e ppc 0x000be124
short SWReturnTileUnderPixel(
    SpriteWorldPtr  spriteWorldP,
    short   tileLayer,
    short   pixelCol,
    short   pixelRow)
{
    short   row, col;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized, 2316);
    SW_ASSERT(tileLayer < kNumTileLayers, 2317);

    if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
        return -1;

    row = pixelRow / spriteWorldP->tileHeight;
    col = pixelCol / spriteWorldP->tileWidth;

    if (row < 0 || row >= spriteWorldP->tileLayerArray[tileLayer]->numRows ||
        col < 0 || col >= spriteWorldP->tileLayerArray[tileLayer]->numCols )
    {
        return -1;
    }
    else
    {
        return spriteWorldP->tileLayerArray[tileLayer]->tileMap[row][col];
    }
}

/* SWCheckSpriteWithTiles (mac _SWCheckSpriteWithTiles, mac-only). STATE: see the mac gate. */
// MAC: _SWCheckSpriteWithTiles i386 0x000b2c41 ppc 0x000be200
Boolean SWCheckSpriteWithTiles(
    SpriteWorldPtr  spriteWorldP,
    SpritePtr       srcSpriteP,
    SWTileSearchType searchType,
    Rect            *insetRectP,
    short           startTileLayer,
    short           endTileLayer,
    short           firstTileID,
    short           lastTileID,
    Boolean         fixPosition)
{
    short           row, col, startRow, stopRow, startCol, stopCol;
    TileMapPtr      tileMap;
    Rect            oldFrameRect, destFrameRect;
    Boolean         foundTile = false;
    Boolean         rowLoopTest, colLoopTest;
    short           temp, tileID, tileLayer, rowIncrement, colIncrement;

    SW_ASSERT(spriteWorldP != NULL && spriteWorldP->tilingIsInitialized, 2359);
    SW_ASSERT(startTileLayer >= 0 && endTileLayer < kNumTileLayers, 2360);
    SW_ASSERT(srcSpriteP != NULL, 2361);

    oldFrameRect = srcSpriteP->oldFrameRect;
    destFrameRect = srcSpriteP->destFrameRect;

    if (insetRectP != NULL)
    {
        oldFrameRect.left += insetRectP->left;
        oldFrameRect.top += insetRectP->top;
        oldFrameRect.right -= insetRectP->right;
        oldFrameRect.bottom -= insetRectP->bottom;

        destFrameRect.left += insetRectP->left;
        destFrameRect.top += insetRectP->top;
        destFrameRect.right -= insetRectP->right;
        destFrameRect.bottom -= insetRectP->bottom;
    }

    if (oldFrameRect.top < 0)
        oldFrameRect.top -= spriteWorldP->tileHeight;
    if (oldFrameRect.bottom <= 0)
        oldFrameRect.bottom -= spriteWorldP->tileHeight;

    if (oldFrameRect.left <= 0)
        oldFrameRect.left -= spriteWorldP->tileWidth;
    if (oldFrameRect.right <= 0)
        oldFrameRect.right -= spriteWorldP->tileWidth;

    if (destFrameRect.left < 0)
        destFrameRect.left -= spriteWorldP->tileWidth;
    if (destFrameRect.right <= 0)
        destFrameRect.right -= spriteWorldP->tileWidth;

    if (destFrameRect.top < 0)
        destFrameRect.top -= spriteWorldP->tileHeight;
    if (destFrameRect.bottom <= 0)
        destFrameRect.bottom -= spriteWorldP->tileHeight;

    if (searchType == kSWTopSide)
    {
        startRow = (oldFrameRect.top / spriteWorldP->tileHeight);
        stopRow = destFrameRect.top / spriteWorldP->tileHeight;
        startCol = destFrameRect.left / spriteWorldP->tileWidth;
        stopCol = (destFrameRect.right-1) / spriteWorldP->tileWidth;
        if (fixPosition)
            startRow--;
        if (stopRow > startRow)
            return false;
    }
    else if (searchType == kSWBottomSide)
    {
        startRow = ((oldFrameRect.bottom-1) / spriteWorldP->tileHeight);
        stopRow = (destFrameRect.bottom-1) / spriteWorldP->tileHeight;
        startCol = destFrameRect.left / spriteWorldP->tileWidth;
        stopCol = (destFrameRect.right-1) / spriteWorldP->tileWidth;
        if (fixPosition)
            startRow++;
        if (stopRow < startRow)
            return false;
    }
    else if (searchType == kSWRightSide)
    {
        startCol = ((oldFrameRect.right-1) / spriteWorldP->tileWidth);
        stopCol = (destFrameRect.right-1) / spriteWorldP->tileWidth;
        startRow = destFrameRect.top / spriteWorldP->tileHeight;
        stopRow = (destFrameRect.bottom-1) / spriteWorldP->tileHeight;
        if (fixPosition)
            startCol++;
        if (stopCol < startCol)
            return false;
    }
    else if (searchType == kSWLeftSide)
    {
        startCol = oldFrameRect.left / spriteWorldP->tileWidth;
        stopCol = destFrameRect.left / spriteWorldP->tileWidth;
        startRow = destFrameRect.top / spriteWorldP->tileHeight;
        stopRow = (destFrameRect.bottom-1) / spriteWorldP->tileHeight;
        if (fixPosition)
            startCol--;
        if (stopCol > startCol)
            return false;
    }
    else
    {
        startRow = destFrameRect.top / spriteWorldP->tileHeight;
        stopRow = (destFrameRect.bottom-1) / spriteWorldP->tileHeight;
        startCol = destFrameRect.left / spriteWorldP->tileWidth;
        stopCol = (destFrameRect.right-1) / spriteWorldP->tileWidth;
    }

    if (startRow <= stopRow)
        rowIncrement = 1;
    else
        rowIncrement = -1;

    if (startCol <= stopCol)
        colIncrement = 1;
    else
        colIncrement = -1;

    for (tileLayer = startTileLayer; tileLayer <= endTileLayer; tileLayer++)
    {
        if (spriteWorldP->tileLayerArray[tileLayer] != NULL)
            break;
    }

    if (rowIncrement > 0)
    {
        if (stopRow < 0)
            return false;
        else if (stopRow >= spriteWorldP->tileLayerArray[tileLayer]->numRows)
            stopRow = spriteWorldP->tileLayerArray[tileLayer]->numRows-1;

        if (startRow < 0)
            startRow = 0;
        else if (startRow >= spriteWorldP->tileLayerArray[tileLayer]->numRows)
            return false;
    }
    else
    {
        if (startRow < 0)
            return false;
        else if (startRow >= spriteWorldP->tileLayerArray[tileLayer]->numRows)
            startRow = spriteWorldP->tileLayerArray[tileLayer]->numRows-1;

        if (stopRow < 0)
            stopRow = 0;
        else if (stopRow >= spriteWorldP->tileLayerArray[tileLayer]->numRows)
            return false;
    }

    if (colIncrement > 0)
    {
        if (stopCol < 0)
            return false;
        else if (stopCol >= spriteWorldP->tileLayerArray[tileLayer]->numCols)
            stopCol = spriteWorldP->tileLayerArray[tileLayer]->numCols-1;

        if (startCol < 0)
            startCol = 0;
        else if (startCol >= spriteWorldP->tileLayerArray[tileLayer]->numCols)
            return false;
    }
    else
    {
        if (startCol < 0)
            return false;
        else if (startCol >= spriteWorldP->tileLayerArray[tileLayer]->numCols)
            startCol = spriteWorldP->tileLayerArray[tileLayer]->numCols-1;

        if (stopCol < 0)
            stopCol = 0;
        else if (stopCol >= spriteWorldP->tileLayerArray[tileLayer]->numCols)
            return false;
    }

    if (searchType == kSWTopSide || searchType == kSWBottomSide || searchType == kSWEntireSprite)
    {
        for (tileLayer = startTileLayer; tileLayer <= endTileLayer; tileLayer++)
        {
            if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
                continue;

            tileMap = spriteWorldP->tileLayerArray[tileLayer]->tileMap;

            rowLoopTest = true;
            for (row = startRow; rowLoopTest; row += rowIncrement)
            {
                rowLoopTest = row != stopRow;

                colLoopTest = true;
                for (col = startCol; colLoopTest; col += colIncrement)
                {
                    colLoopTest = col != stopCol;
                    tileID = tileMap[row][col];
                    if (tileID >= firstTileID && tileID <= lastTileID)
                    {
                        foundTile = true;
                        goto exit;
                    }
                }
            }
        }
    }
    else
    {
        for (tileLayer = startTileLayer; tileLayer <= endTileLayer; tileLayer++)
        {
            if (spriteWorldP->tileLayerArray[tileLayer] == NULL)
                continue;

            tileMap = spriteWorldP->tileLayerArray[tileLayer]->tileMap;

            colLoopTest = true;
            for (col = startCol; colLoopTest; col += colIncrement)
            {
                colLoopTest = col != stopCol;

                rowLoopTest = true;
                for (row = startRow; rowLoopTest; row += rowIncrement)
                {
                    rowLoopTest = row != stopRow;
                    tileID = tileMap[row][col];
                    if (tileID >= firstTileID && tileID <= lastTileID)
                    {
                        foundTile = true;
                        goto exit;
                    }
                }
            }
        }
    }

exit:

    if (foundTile && fixPosition)
    {
        if (searchType == kSWTopSide)
        {
            temp = (row+1) * spriteWorldP->tileHeight - destFrameRect.top;
            srcSpriteP->destFrameRect.top += temp;
            srcSpriteP->destFrameRect.bottom += temp;
            srcSpriteP->needsToBeDrawn = true;
        }
        else if (searchType == kSWBottomSide)
        {
            temp = destFrameRect.bottom - row * spriteWorldP->tileHeight;
            srcSpriteP->destFrameRect.top -= temp;
            srcSpriteP->destFrameRect.bottom -= temp;
            srcSpriteP->needsToBeDrawn = true;
        }
        else if (searchType == kSWRightSide)
        {
            temp = destFrameRect.right - col * spriteWorldP->tileWidth;
            srcSpriteP->destFrameRect.left -= temp;
            srcSpriteP->destFrameRect.right -= temp;
            srcSpriteP->needsToBeDrawn = true;
        }
        else if (searchType == kSWLeftSide)
        {
            temp = (col+1) * spriteWorldP->tileWidth - destFrameRect.left;
            srcSpriteP->destFrameRect.left += temp;
            srcSpriteP->destFrameRect.right += temp;
            srcSpriteP->needsToBeDrawn = true;
        }
    }

    return foundTile;
}

/* SWWrapRectToWorkArea (mac _SWWrapRectToWorkArea, mac-only). STATE: see the mac gate. */
// MAC: _SWWrapRectToWorkArea i386 0x000b34e0 ppc 0x000be898
void SWWrapRectToWorkArea(
    SpriteWorldPtr  spriteWorldP,
    Rect*           destRectP)
{
    Rect        destRect = *destRectP;
    Rect        tempDestRect;
    FramePtr    srcFrameP = spriteWorldP->backFrameP;
    FramePtr    dstFrameP = spriteWorldP->workFrameP;

    SW_ASSERT(spriteWorldP != NULL, 2650);
    SW_ASSERT(spriteWorldP->backFrameP->isFrameLocked, 2651);
    SW_ASSERT(spriteWorldP->workFrameP->isFrameLocked, 2652);

    SetGWorld(spriteWorldP->workFrameP->framePort, nil);

    destRect.top -= spriteWorldP->vertScrollRectOffset;
    destRect.bottom -= spriteWorldP->vertScrollRectOffset;
    destRect.left -= spriteWorldP->horizScrollRectOffset;
    destRect.right -= spriteWorldP->horizScrollRectOffset;

    (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP, &destRect, &destRect);

    if (destRect.bottom > dstFrameP->frameRect.bottom)
    {
        tempDestRect.top = destRect.top - dstFrameP->frameRect.bottom;
        tempDestRect.bottom = destRect.bottom - dstFrameP->frameRect.bottom;
        tempDestRect.left = destRect.left;
        tempDestRect.right = destRect.right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDestRect, &tempDestRect);

        if (destRect.right > dstFrameP->frameRect.right)
        {
            tempDestRect.left -= dstFrameP->frameRect.right;
            tempDestRect.right -= dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDestRect, &tempDestRect);
        }
        else if (destRect.left < dstFrameP->frameRect.left)
        {
            tempDestRect.left += dstFrameP->frameRect.right;
            tempDestRect.right += dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDestRect, &tempDestRect);
        }
    }

    if (destRect.right > dstFrameP->frameRect.right)
    {
        tempDestRect.top = destRect.top;
        tempDestRect.bottom = destRect.bottom;
        tempDestRect.left = destRect.left - dstFrameP->frameRect.right;
        tempDestRect.right = destRect.right - dstFrameP->frameRect.right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDestRect, &tempDestRect);
    }
    else if (destRect.left < dstFrameP->frameRect.left)
    {
        tempDestRect.top = destRect.top;
        tempDestRect.bottom = destRect.bottom;
        tempDestRect.left = destRect.left + dstFrameP->frameRect.right;
        tempDestRect.right = destRect.right + dstFrameP->frameRect.right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDestRect, &tempDestRect);
    }

    if (destRect.top < dstFrameP->frameRect.top)
    {
        tempDestRect.top = destRect.top + dstFrameP->frameRect.bottom;
        tempDestRect.bottom = destRect.bottom + dstFrameP->frameRect.bottom;
        tempDestRect.left = destRect.left;
        tempDestRect.right = destRect.right;

        (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
            &tempDestRect, &tempDestRect);

        if (destRect.right > dstFrameP->frameRect.right)
        {
            tempDestRect.left -= dstFrameP->frameRect.right;
            tempDestRect.right -= dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDestRect, &tempDestRect);
        }
        else if (destRect.left < dstFrameP->frameRect.left)
        {
            tempDestRect.left += dstFrameP->frameRect.right;
            tempDestRect.right += dstFrameP->frameRect.right;

            (*spriteWorldP->offscreenDrawProc)(srcFrameP, dstFrameP,
                &tempDestRect, &tempDestRect);
        }
    }
}

/* SWWrapRectFromExtraBackFrame (mac _SWWrapRectFromExtraBackFrame, mac-only). STATE: see the mac gate. */
// MAC: _SWWrapRectFromExtraBackFrame i386 0x000b05c6 ppc 0x000bbc28
void SWWrapRectFromExtraBackFrame(
    SpriteWorldPtr  spriteWorldP,
    Rect            *dstRectP)
{
    FramePtr    srcFrameP = spriteWorldP->extraBackFrameP;
    FramePtr    dstFrameP = spriteWorldP->backFrameP;
    Rect        srcRect, tempSrcRect, tempDstRect;
    short       topClip=0, rightClip=0, bottomClip=0, leftClip=0;
    short       vertBackRectOffset, horizBackRectOffset;

    SW_ASSERT(spriteWorldP != NULL, 2770);
    SW_ASSERT(srcFrameP->isFrameLocked, 2771);
    SW_ASSERT(dstFrameP->isFrameLocked, 2772);

    vertBackRectOffset = spriteWorldP->extraBackFrameP->frameRect.bottom *
        (dstRectP->top / spriteWorldP->extraBackFrameP->frameRect.bottom);

    horizBackRectOffset = spriteWorldP->extraBackFrameP->frameRect.right *
        (dstRectP->left / spriteWorldP->extraBackFrameP->frameRect.right);

    srcRect.top = dstRectP->top - vertBackRectOffset;
    srcRect.bottom = dstRectP->bottom - vertBackRectOffset;
    srcRect.left = dstRectP->left - horizBackRectOffset;
    srcRect.right = dstRectP->right - horizBackRectOffset;

    dstRectP->top -= spriteWorldP->vertScrollRectOffset;
    dstRectP->bottom -= spriteWorldP->vertScrollRectOffset;
    dstRectP->left -= spriteWorldP->horizScrollRectOffset;
    dstRectP->right -= spriteWorldP->horizScrollRectOffset;

    if (dstRectP->bottom > dstFrameP->frameRect.bottom)
    {
        dstRectP->top -= dstFrameP->frameRect.bottom;
        dstRectP->bottom -= dstFrameP->frameRect.bottom;
    }
    else if (dstRectP->top < dstFrameP->frameRect.top)
    {
        dstRectP->top += dstFrameP->frameRect.bottom;
        dstRectP->bottom += dstFrameP->frameRect.bottom;
    }

    if (dstRectP->right > dstFrameP->frameRect.right)
    {
        dstRectP->left -= dstFrameP->frameRect.right;
        dstRectP->right -= dstFrameP->frameRect.right;
    }
    else if (dstRectP->left < dstFrameP->frameRect.left)
    {
        dstRectP->left += dstFrameP->frameRect.right;
        dstRectP->right += dstFrameP->frameRect.right;
    }

    if (srcRect.top < srcFrameP->frameRect.top)
    {
        topClip = srcFrameP->frameRect.top - srcRect.top;
        srcRect.top += topClip;
    }

    if (srcRect.bottom > srcFrameP->frameRect.bottom)
    {
        bottomClip = srcRect.bottom - srcFrameP->frameRect.bottom;
        srcRect.bottom -= bottomClip;
    }

    if (srcRect.left < srcFrameP->frameRect.left)
    {
        leftClip = srcFrameP->frameRect.left - srcRect.left;
        srcRect.left += leftClip;
    }

    if (srcRect.right > srcFrameP->frameRect.right)
    {
        rightClip = srcRect.right - srcFrameP->frameRect.right;
        srcRect.right -= rightClip;
    }

    if (topClip)
    {
        tempSrcRect.right = srcRect.right;
        tempSrcRect.left = srcRect.left;
        tempSrcRect.bottom = srcFrameP->frameRect.bottom;
        tempSrcRect.top = srcFrameP->frameRect.bottom - topClip;

        tempDstRect.top = dstRectP->top;
        tempDstRect.bottom = dstRectP->top + topClip;
        tempDstRect.left = dstRectP->left + leftClip;
        tempDstRect.right = dstRectP->right - rightClip;

        (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);

        if (leftClip)
        {
            tempSrcRect.bottom = srcFrameP->frameRect.bottom;
            tempSrcRect.right = srcFrameP->frameRect.right;
            tempSrcRect.top = srcFrameP->frameRect.bottom - topClip;
            tempSrcRect.left = srcFrameP->frameRect.right - leftClip;

            tempDstRect.left = dstRectP->left;
            tempDstRect.top = dstRectP->top;
            tempDstRect.right = dstRectP->left + leftClip;
            tempDstRect.bottom = dstRectP->top + topClip;

            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);
        }
        else if (rightClip)
        {
            tempSrcRect.bottom = srcFrameP->frameRect.bottom;
            tempSrcRect.left = srcFrameP->frameRect.left;
            tempSrcRect.right = srcFrameP->frameRect.left + rightClip;
            tempSrcRect.top = srcFrameP->frameRect.bottom - topClip;

            tempDstRect.top = dstRectP->top;
            tempDstRect.right = dstRectP->right;
            tempDstRect.bottom = dstRectP->top + topClip;
            tempDstRect.left = dstRectP->right - rightClip;

            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);
        }
    }

    tempDstRect.left = dstRectP->left + leftClip;
    tempDstRect.top = dstRectP->top + topClip;
    tempDstRect.right = dstRectP->right - rightClip;
    tempDstRect.bottom = dstRectP->bottom - bottomClip;

    (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &srcRect, &tempDstRect);

    if (leftClip)
    {
        tempSrcRect.top = srcRect.top;
        tempSrcRect.bottom = srcRect.bottom;
        tempSrcRect.right = srcFrameP->frameRect.right;
        tempSrcRect.left = srcFrameP->frameRect.right - leftClip;

        tempDstRect.left = dstRectP->left;
        tempDstRect.right = dstRectP->left + leftClip;
        tempDstRect.top = dstRectP->top + topClip;
        tempDstRect.bottom = dstRectP->bottom - bottomClip;

        (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);
    }
    else if (rightClip)
    {
        tempSrcRect.top = srcRect.top;
        tempSrcRect.bottom = srcRect.bottom;
        tempSrcRect.left = srcFrameP->frameRect.left;
        tempSrcRect.right = srcFrameP->frameRect.left + rightClip;

        tempDstRect.right = dstRectP->right;
        tempDstRect.left = dstRectP->right - rightClip;
        tempDstRect.top = dstRectP->top + topClip;
        tempDstRect.bottom = dstRectP->bottom - bottomClip;

        (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);
    }

    if (bottomClip)
    {
        tempSrcRect.right = srcRect.right;
        tempSrcRect.left = srcRect.left;
        tempSrcRect.top = srcFrameP->frameRect.top;
        tempSrcRect.bottom = srcFrameP->frameRect.top + bottomClip;

        tempDstRect.bottom = dstRectP->bottom;
        tempDstRect.top = dstRectP->bottom - bottomClip;
        tempDstRect.left = dstRectP->left + leftClip;
        tempDstRect.right = dstRectP->right - rightClip;

        (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);

        if (leftClip)
        {
            tempSrcRect.top = srcFrameP->frameRect.top;
            tempSrcRect.right = srcFrameP->frameRect.right;
            tempSrcRect.bottom = srcFrameP->frameRect.top + bottomClip;
            tempSrcRect.left = srcFrameP->frameRect.right - leftClip;

            tempDstRect.bottom = dstRectP->bottom;
            tempDstRect.left = dstRectP->left;
            tempDstRect.top = dstRectP->bottom - bottomClip;
            tempDstRect.right = dstRectP->left + leftClip;

            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);
        }
        else if (rightClip)
        {
            tempSrcRect.top = srcFrameP->frameRect.top;
            tempSrcRect.left = srcFrameP->frameRect.left;
            tempSrcRect.bottom = srcFrameP->frameRect.top + bottomClip;
            tempSrcRect.right = srcFrameP->frameRect.left + rightClip;

            tempDstRect.bottom = dstRectP->bottom;
            tempDstRect.right = dstRectP->right;
            tempDstRect.top = dstRectP->bottom - bottomClip;
            tempDstRect.left = dstRectP->right - rightClip;

            (*spriteWorldP->screenDrawProc)(srcFrameP, dstFrameP, &tempSrcRect, &tempDstRect);
        }
    }
}
