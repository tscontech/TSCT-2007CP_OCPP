﻿#include <assert.h>
#include <malloc.h>
#include <string.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"

static const char bgName[] = "ITUBackground";

bool ituBackgroundClone(ITUWidget* widget, ITUWidget** cloned)
{
    ITUIcon* icon = (ITUIcon*)widget;
    assert(widget);
    assert(cloned);
    ITU_ASSERT_THREAD();

    if (*cloned == NULL)
    {
        ITUWidget* newWidget = malloc(sizeof(ITUBackground));
        if (newWidget == NULL)
            return false;

        memcpy(newWidget, widget, sizeof(ITUBackground));
        newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
        *cloned = newWidget;
    }

    return ituIconClone(widget, cloned);
}

void ituBackgroundDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    int destx, desty;
    uint8_t desta;
    ITURectangle prevClip;
    ITUBackground* bg = (ITUBackground*) widget;
    ITURectangle* rect = (ITURectangle*) &widget->rect;
    ITUSurface* bgSurf = NULL;
	int bigSide = ((rect->width >= rect->height) ? (rect->width) : (rect->height));
    assert(bg);
    assert(dest);

    destx = rect->x + x;
    desty = rect->y + y;
    desta = alpha * widget->color.alpha / 255;
    desta = desta * widget->alpha / 255;

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
	if (!bg->icon.surf)
	{
		ituWidgetUpdate(&bg->icon, ITU_EVENT_LOAD, 0, 0, 0);
		//printf("Background %s load surf before draw. %d\n", widget->name, __LINE__);
	}
#endif

    if (widget->angle == 0 && !((widget->flags & ITU_STRETCH) && widget->tree.child && bg->orgWidth && bg->orgHeight && (bg->orgWidth != rect->width || bg->orgHeight != rect->height)))
        ituWidgetSetClipping(widget, dest, x, y, &prevClip);

    if (!bg->icon.surf || 
        (bg->icon.surf->width < widget->rect.width || bg->icon.surf->height < widget->rect.height) ||
        (bg->icon.surf->format == ITU_ARGB1555 || bg->icon.surf->format == ITU_ARGB4444 || bg->icon.surf->format == ITU_ARGB8888))
    {
        if (desta == 255)
        {
            if ((widget->flags & ITU_STRETCH) && widget->tree.child && bg->orgWidth && bg->orgHeight && (bg->orgWidth != rect->width || bg->orgHeight != rect->height))
            {
                bgSurf = ituCreateSurface(bg->orgWidth, bg->orgHeight, 0, dest->format, NULL, 0);
                if (bgSurf)
                {
                    if (bg->graidentMode == ITU_GF_NONE)
                        ituColorFill(bgSurf, 0, 0, bgSurf->width, bgSurf->height, &widget->color);
                    else
                        ituGradientFill(bgSurf, 0, 0, bgSurf->width, bgSurf->height, &widget->color, &bg->graidentColor, bg->graidentMode);
                }
            }
            else
            {
                if (bg->graidentMode == ITU_GF_NONE)
                    ituColorFill(dest, destx, desty, rect->width, rect->height, &widget->color);
                else
                    ituGradientFill(dest, destx, desty, rect->width, rect->height, &widget->color, &bg->graidentColor, bg->graidentMode);
            }
        }
        else if (desta > 0)
        {
            ITUColor color, graidentColor;

            ituSetColor(&color, 255, widget->color.red, widget->color.green, widget->color.blue);
            ituSetColor(&graidentColor, 255, bg->graidentColor.red, bg->graidentColor.green, bg->graidentColor.blue);

        #ifdef CFG_WIN32_SIMULATOR
            if ((widget->flags & ITU_STRETCH) && widget->tree.child && bg->orgWidth && bg->orgHeight && (bg->orgWidth != rect->width || bg->orgHeight != rect->height))
            {
                bgSurf = ituCreateSurface(bg->orgWidth, bg->orgHeight, 0, dest->format, NULL, 0);
                if (bgSurf)
                {
                    if (bg->graidentMode == ITU_GF_NONE)
                        ituColorFill(bgSurf, 0, 0, bgSurf->width, bgSurf->height, &color);
                    else
                        ituGradientFill(bgSurf, 0, 0, bgSurf->width, bgSurf->height, &color, &graidentColor, bg->graidentMode);
                }
            }
            else
            {
                ITUSurface* surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
                if (surf)
                {
                    if (bg->graidentMode == ITU_GF_NONE)
                        ituColorFill(surf, 0, 0, rect->width, rect->height, &color);
                    else
                        ituGradientFill(surf, 0, 0, rect->width, rect->height, &color, &graidentColor, bg->graidentMode);

                    ituAlphaBlend(dest, destx, desty, rect->width, rect->height, surf, 0, 0, desta);
                    ituDestroySurface(surf);
                }
            }
        #else
            if (widget->tree.child && bg->orgWidth && bg->orgHeight && bg->orgWidth != rect->width && bg->orgHeight != rect->height)
            {
                bgSurf = ituCreateSurface(bg->orgWidth, bg->orgHeight, 0, dest->format, NULL, 0);
                if (bgSurf)
                {
                    if (bg->graidentMode == ITU_GF_NONE)
                        ituColorFill(bgSurf, 0, 0, bgSurf->width, bgSurf->height, &color);
                    else
                        ituGradientFill(bgSurf, 0, 0, bgSurf->width, bgSurf->height, &color, &graidentColor, bg->graidentMode);
                }
            }
            else
            {
                if (bg->graidentMode == ITU_GF_NONE)
                    ituColorFillBlend(dest, destx, desty, rect->width, rect->height, &color, false, true, desta);
                else
                    ituGradientFillBlend(dest, destx, desty, rect->width, rect->height, &color, &graidentColor, bg->graidentMode, false, true, desta);
            }
        #endif
        }
        else if ((widget->flags & ITU_STRETCH) && widget->tree.child && bg->orgWidth && bg->orgHeight && (bg->orgWidth != rect->width || bg->orgHeight != rect->height))
        {
            bgSurf = ituCreateSurface(bg->orgWidth, bg->orgHeight, 0, ITU_ARGB8888, NULL, 0);
            if (bgSurf)
            {
                ITUColor color = { 0, 0, 0, 0 };
                ituColorFill(bgSurf, 0, 0, bg->orgWidth, bg->orgHeight, &color);

            //#ifdef CFG_M2D_ENABLE // workaround hw bitblt with alpha bug for 9850
            //    bgSurf->flags |= 0x80000000;
            //#endif
            }
        }
    }
    
    if (bg->icon.surf)
    {
        desta = alpha * widget->alpha / 255;
        if (desta > 0)
        {
            if (widget->flags & ITU_STRETCH)
            {
#ifdef CFG_WIN32_SIMULATOR
                if (desta == 255)
                {
                    if (widget->angle == 0)
                    {
                        if (widget->transformType == ITU_TRANSFORM_NONE)
                        {
                            if (widget->tree.child && bg->orgWidth && bg->orgHeight && bg->orgWidth != rect->width && bg->orgHeight != rect->height)
                            {
                                if (!bgSurf)
                                    bgSurf = ituCreateSurface(bg->orgWidth, bg->orgHeight, 0, dest->format, NULL, 0);

                                if (bgSurf)
                                {
                                    ituStretchBlt(bgSurf, 0, 0, bgSurf->width, bgSurf->height, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height);
                                }
                            }
                            else
                            {
                                ituStretchBlt(dest, destx, desty, rect->width, rect->height, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height);
                            }
                        }
                        else
                        {
                            ITUSurface* surf = ituCreateSurface(bg->icon.surf->width, bg->icon.surf->height, 0, dest->format, NULL, 0);
                            if (surf)
                            {
                                int w = (bg->icon.surf->width - bg->icon.surf->width * widget->transformX / 100) / 2;
                                int h = (bg->icon.surf->height - bg->icon.surf->height * widget->transformY / 100) / 2;

                                ituStretchBlt(surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height, dest, destx, desty, rect->width, rect->height);

                                switch (widget->transformType)
                                {
                                case ITU_TRANSFORM_TURN_LEFT:
                                    ituTransformBlt(surf, 0, 0, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height, w, h, bg->icon.surf->width - w, 0, bg->icon.surf->width - w, bg->icon.surf->height, w, bg->icon.surf->height - h, true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                    break;

                                case ITU_TRANSFORM_TURN_TOP:
                                    ituTransformBlt(surf, 0, 0, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height, w, h, bg->icon.surf->width - w, h, bg->icon.surf->width, bg->icon.surf->height - h, 0, bg->icon.surf->height - h, true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                    break;

                                case ITU_TRANSFORM_TURN_RIGHT:
                                    ituTransformBlt(surf, 0, 0, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height, w, 0, bg->icon.surf->width - w, h, bg->icon.surf->width - w, bg->icon.surf->height - h, w, bg->icon.surf->height, false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                    break;

                                case ITU_TRANSFORM_TURN_BOTTOM:
                                    ituTransformBlt(surf, 0, 0, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height, 0, h, bg->icon.surf->width, h, bg->icon.surf->width - w, bg->icon.surf->height - h, w, bg->icon.surf->height - h, false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                    break;
                                }
                                ituStretchBlt(dest, destx, desty, rect->width, rect->height, surf, 0, 0, surf->width, surf->height);
                                ituDestroySurface(surf);
                            }
                        }
                    }
                    else
                    {
                        float scaleX = (float)rect->width / bg->icon.surf->width;
                        float scaleY = (float)rect->height / bg->icon.surf->height;

                        ituRotate(dest, destx + rect->width / 2, desty + rect->height / 2, bg->icon.surf, bg->icon.surf->width / 2, bg->icon.surf->height / 2, (float)widget->angle, scaleX, scaleY);
                    }
                }
                else
                {
                    if (widget->tree.child && widget->angle == 0 && bg->orgWidth && bg->orgHeight && bg->orgWidth != rect->width && bg->orgHeight != rect->height)
                    {
                        if (!bgSurf)
                            bgSurf = ituCreateSurface(bg->orgWidth, bg->orgHeight, 0, dest->format, NULL, 0);

                        if (bgSurf)
                        {
                            ituStretchBlt(bgSurf, 0, 0, bgSurf->width, bgSurf->height, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height);
                        }
                    }
                    else
                    {
                        ITUSurface* surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
                        if (surf)
                        {
                            ituBitBlt(surf, 0, 0, rect->width, rect->height, dest, destx, desty);

                            if (widget->angle == 0)
                            {
                                ituStretchBlt(surf, 0, 0, rect->width, rect->height, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height);
                            }
                            else
                            {
                                float scaleX = (float)rect->width / bg->icon.surf->width;
                                float scaleY = (float)rect->height / bg->icon.surf->height;

                                ituRotate(surf, rect->width / 2, rect->height / 2, bg->icon.surf, bg->icon.surf->width / 2, bg->icon.surf->height / 2, (float)widget->angle, scaleX, scaleY);
                            }
                            ituAlphaBlend(dest, destx, desty, rect->width, rect->height, surf, 0, 0, desta);
                            ituDestroySurface(surf);
                        }
                    }
                }
#else
                if (widget->angle == 0 && widget->transformType != ITU_TRANSFORM_NONE)
                {
                    int w = (bg->icon.surf->width - bg->icon.surf->width * widget->transformX / 100) / 2;
                    int h = (bg->icon.surf->height - bg->icon.surf->height * widget->transformY / 100) / 2;                  

                    ITUSurface* surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
                    if (surf)
                    {
                        ituStretchBlt(surf, 0, 0, rect->width, rect->height, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height);                        
                        ituWidgetDrawImpl((ITUWidget*)bg, surf, -rect->x, -rect->y, alpha);

                        switch (widget->transformType)
                        {
                        case ITU_TRANSFORM_TURN_LEFT:
                            ituTransformBlt(dest, destx, desty, surf, 0, 0, rect->width, rect->height, w, h, rect->width - w, 0, rect->width - w, rect->height, w, rect->height - h, true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                            break;

                        case ITU_TRANSFORM_TURN_TOP:
                            ituTransformBlt(dest, destx, desty, surf, 0, 0, rect->width, rect->height, w, h, rect->width - w, h, rect->width, rect->height - h, 0, rect->height - h, true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                            break;

                        case ITU_TRANSFORM_TURN_RIGHT:
                            ituTransformBlt(dest, destx, desty, surf, 0, 0, rect->width, rect->height, w, 0, rect->width - w, h, rect->width - w, rect->height - h, w, rect->height, false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                            break;

                        case ITU_TRANSFORM_TURN_BOTTOM:
                            ituTransformBlt(dest, destx, desty, surf, 0, 0, rect->width, rect->height, 0, h, rect->width, h, rect->width - w, rect->height - h, w, rect->height - h, false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                            break;
                        }

                        ituDestroySurface(surf);
                    }
                }
                else
                {
                    if (widget->tree.child && bg->orgWidth && bg->orgHeight && bg->orgWidth != rect->width && bg->orgHeight != rect->height)
                    {
                        if (!bgSurf)
                            bgSurf = ituCreateSurface(bg->orgWidth, bg->orgHeight, 0, dest->format, NULL, 0);

                        if (bgSurf)
                        {
                            ituStretchBlt(bgSurf, 0, 0, bgSurf->width, bgSurf->height, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height);
                        }
                    }
                    else
                    {
                        float scaleX = (float)rect->width / bg->icon.surf->width;
                        float scaleY = (float)rect->height / bg->icon.surf->height;

                        //printf("scaleX:%f,scaleY:%f (%d %d) (%d %d)\n",scaleX,scaleY,rect->width,rect->height,bg->icon.surf->width,bg->icon.surf->height);
                        ituTransform(
                            dest, destx, desty, rect->width, rect->height,
                            bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height,
                            bg->icon.surf->width / 2, bg->icon.surf->height / 2,
                            scaleX,
                            scaleY,
                            (float)widget->angle,
                            0,
                            true,
                            true,
                            desta);
                    }
                }
#endif
            }
            else
            {
                if (desta == 255)
                {
                    if (widget->angle == 0)
                    {
                        if (widget->transformType == ITU_TRANSFORM_NONE)
                        {
                            ituBitBlt(dest, destx, desty, bg->icon.surf->width, bg->icon.surf->height, bg->icon.surf, 0, 0);
                        }
                        else
                        {
                            int w = (bg->icon.surf->width - bg->icon.surf->width * widget->transformX / 100) / 2;
                            int h = (bg->icon.surf->height - bg->icon.surf->height * widget->transformY / 100) / 2;

                            if (widget->tree.child && bg->orgWidth && bg->orgHeight && bg->orgWidth != rect->width && bg->orgHeight != rect->height)
                            {
                                ITUSurface* surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
                                if (surf)
                                {
                                    ituBitBlt(surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height, bg->icon.surf, 0, 0);
                                    ituWidgetDrawImpl((ITUWidget*)bg, surf, -rect->x, -rect->y, alpha);

                                    switch (widget->transformType)
                                    {
                                    case ITU_TRANSFORM_TURN_LEFT:
                                        ituTransformBlt(dest, destx, desty, surf, 0, 0, rect->width, rect->height, w, h, rect->width - w, 0, rect->width - w, rect->height, w, rect->height - h, true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                        break;

                                    case ITU_TRANSFORM_TURN_TOP:
                                        ituTransformBlt(dest, destx, desty, surf, 0, 0, rect->width, rect->height, w, h, rect->width - w, h, rect->width, rect->height - h, 0, rect->height - h, true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                        break;

                                    case ITU_TRANSFORM_TURN_RIGHT:
                                        ituTransformBlt(dest, destx, desty, surf, 0, 0, rect->width, rect->height, w, 0, rect->width - w, h, rect->width - w, rect->height - h, w, rect->height, false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                        break;

                                    case ITU_TRANSFORM_TURN_BOTTOM:
                                        ituTransformBlt(dest, destx, desty, surf, 0, 0, rect->width, rect->height, 0, h, rect->width, h, rect->width - w, rect->height - h, w, rect->height - h, false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                        break;
                                    }

                                    ituDestroySurface(surf);
                                }

                                if (widget->angle == 0)
                                    ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);

                                return;
                            }
                            else
                            {
                                switch (widget->transformType)
                                {
                                case ITU_TRANSFORM_TURN_LEFT:
                                    ituTransformBlt(dest, destx, desty, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height, w, h, bg->icon.surf->width - w, 0, bg->icon.surf->width - w, bg->icon.surf->height, w, bg->icon.surf->height - h, true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                    break;

                                case ITU_TRANSFORM_TURN_TOP:
                                    ituTransformBlt(dest, destx, desty, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height, w, h, bg->icon.surf->width - w, h, bg->icon.surf->width, bg->icon.surf->height - h, 0, bg->icon.surf->height - h, true, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                    break;

                                case ITU_TRANSFORM_TURN_RIGHT:
                                    ituTransformBlt(dest, destx, desty, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height, w, 0, bg->icon.surf->width - w, h, bg->icon.surf->width - w, bg->icon.surf->height - h, w, bg->icon.surf->height, false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                    break;

                                case ITU_TRANSFORM_TURN_BOTTOM:
                                    ituTransformBlt(dest, destx, desty, bg->icon.surf, 0, 0, bg->icon.surf->width, bg->icon.surf->height, 0, h, bg->icon.surf->width, h, bg->icon.surf->width - w, bg->icon.surf->height - h, w, bg->icon.surf->height - h, false, ITU_PAGEFLOW_FOLD2, widget->transformType);
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
#ifdef CFG_WIN32_SIMULATOR
                        ituRotate(dest, destx + rect->width / 2, desty + rect->height / 2, bg->icon.surf, bg->icon.surf->width / 2, bg->icon.surf->height / 2, (float)widget->angle, 1.0f, 1.0f);
#else
                        ituRotate(dest, destx, desty, bg->icon.surf, bg->icon.surf->width / 2, bg->icon.surf->height / 2, (float)widget->angle, 1.0f, 1.0f);
#endif
                    }
                }
                else
                {
                    ituAlphaBlend(dest, destx, desty, bg->icon.surf->width, bg->icon.surf->height, bg->icon.surf, 0, 0, desta);
                }
            }
        }
    }
    if (widget->angle == 0)
    {
        if (bgSurf)
        {
            ITCTree* node;
            assert(widget->flags & ITU_STRETCH);

            for (node = widget->tree.child; node; node = node->sibling)
            {
                ITUWidget* child = (ITUWidget*)node;

                if (child->visible && ituWidgetIsOverlapClipping(child, dest, destx, desty))
                    ituWidgetDraw(node, bgSurf, 0, 0, alpha);

                child->dirty = false;
            }

            if ((widget->flags & ITU_TRANSFER_ALPHA) == 0)
            {
                desta = alpha * widget->alpha / 255;
            }

#ifdef CFG_WIN32_SIMULATOR
            if (desta == 255)
            {
                ituStretchBlt(dest, destx, desty, rect->width, rect->height, bgSurf, 0, 0, bgSurf->width, bgSurf->height);
            }
            else
            {
                ITUSurface* surf = ituCreateSurface(rect->width, rect->height, 0, dest->format, NULL, 0);
                if (surf)
                {
                    ituBitBlt(surf, 0, 0, rect->width, rect->height, dest, destx, desty);
                    assert(widget->angle == 0);
                    ituStretchBlt(surf, 0, 0, rect->width, rect->height, bgSurf, 0, 0, bgSurf->width, bgSurf->height);

                    ituAlphaBlend(dest, destx, desty, rect->width, rect->height, surf, 0, 0, desta);

                    ituDestroySurface(surf);
                }
            }
#else
            float scaleX = (float)rect->width / bgSurf->width;
            float scaleY = (float)rect->height / bgSurf->height;

            ituTransform(
                dest, destx, desty, rect->width, rect->height,
                bgSurf, 0, 0, bgSurf->width, bgSurf->height,
                0, 0,
                scaleX,
                scaleY,
                0,
                0,
                true,
                true,
                desta);
#endif
        }
        else
        {
            if (!((widget->flags & ITU_STRETCH) && widget->tree.child && bg->orgWidth && bg->orgHeight && (bg->orgWidth != rect->width || bg->orgHeight != rect->height)))
                ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);

            ituWidgetDrawImpl(widget, dest, x, y, alpha);
        }
    }
    else
    {
        ITCTree* node;

		ITUSurface* surf = ituCreateSurface(bigSide, bigSide, 0, ITU_ARGB8888, NULL, 0);

        if (surf)
        {
			int fixX = (bigSide - rect->width) / 2;
			int fixY = (bigSide - rect->height) / 2;
            ITUColor color = { 0, 0, 0, 0 };

			ituColorFill(surf, 0, 0, surf->width, surf->height, &color);

			//workaround for DrawGlyph limit
			//alpha channel will make drawglyph over render under ARGB8888 empty surface
			//pre-rotate dest and take it as surface render reference
			if (widget->angle != 360)
			{
				ITUSurface* surfTmp = ituCreateSurface(bigSide, bigSide, 0, ITU_ARGB8888, NULL, 0);
				if (surfTmp)
				{
					ituColorFill(surfTmp, 0, 0, surfTmp->width, surfTmp->height, &color);
					ituBitBlt(surfTmp, 0, 0, surfTmp->width, surfTmp->height, dest, destx - fixX, desty - fixY);
#ifdef CFG_WIN32_SIMULATOR
					ituRotate(surf, surf->width / 2, surf->height / 2, surfTmp, surfTmp->width / 2, surfTmp->height / 2, 360 - widget->angle, 1.0f, 1.0f);
#else
					ituRotate(surf, 0, 0, surfTmp, surfTmp->width / 2, surfTmp->height / 2, 360 - widget->angle, 1.0f, 1.0f);
#endif
					ituDestroySurface(surfTmp);
				}
			}
			else
			{
				ituBitBlt(surf, 0, 0, surf->width, surf->height, dest, destx - fixX, desty - fixY);
			}

            for (node = widget->tree.child; node; node = node->sibling)
            {
                ITUWidget* child = (ITUWidget*)node;
                if (child->visible && ituWidgetIsOverlapClipping(child, dest, destx, desty))
                    ituWidgetDraw(child, surf, child->rect.x + fixX, child->rect.y + fixY, alpha);

                child->dirty = false;
            }

            if (bgSurf)
            {
            #ifdef CFG_WIN32_SIMULATOR
                ituRotate(dest, destx + rect->width / 2, desty + rect->height / 2, bgSurf, bgSurf->width / 2, bgSurf->height / 2, (float)widget->angle, 1.0f, 1.0f);
            #else
                ituRotate(dest, destx, desty, bgSurf, bgSurf->width / 2, bgSurf->height / 2, (float)widget->angle, 1.0f, 1.0f);
            #endif
            }

        #ifdef CFG_WIN32_SIMULATOR
			ituRotate(dest, destx - fixX + surf->width / 2, desty - fixY + surf->height / 2, surf, surf->width / 2, surf->height / 2, (float)widget->angle, 1.0f, 1.0f);
        #else
            ituRotate(dest, destx - fixX, desty - fixY, surf, surf->width / 2, surf->height / 2, (float)widget->angle, 1.0f, 1.0f);
        #endif
            ituDestroySurface(surf);
        }
    }
    if (bgSurf)
        ituDestroySurface(bgSurf);

    {
        ITCTree* node;

        for (node = widget->tree.child; node; node = node->sibling)
        {
            ITUWidget* child = (ITUWidget*)node;
            if (child->visible && child->type == ITU_CLIPPER)
            {
                ituClipperPostDraw(child, dest, destx, desty, alpha);
            }
        }
    }

#ifdef CFG_ITU_DECOMPRESS_SURFACE_INPLACE
	if (bg->icon.surf)
	{
		ituWidgetUpdate(&bg->icon, ITU_EVENT_RELEASE, 0, 0, 0);
		//printf("Background %s release surf after draw. %d\n", widget->name, __LINE__);
	}
#endif
}

void ituBackgroundInit(ITUBackground* bg)
{
    assert(bg);
    ITU_ASSERT_THREAD();

    memset(bg, 0, sizeof (ITUBackground));

    ituIconInit(&bg->icon);
    
    ituWidgetSetType(bg, ITU_BACKGROUND);
    ituWidgetSetName(bg, bgName);
    ituWidgetSetClone(bg, ituBackgroundClone);
    ituWidgetSetDraw(bg, ituBackgroundDraw);
}

void ituBackgroundLoad(ITUBackground* bg, uint32_t base)
{
    assert(bg);

    ituIconLoad(&bg->icon, base);
    ituWidgetSetClone(bg, ituBackgroundClone);
    ituWidgetSetDraw(bg, ituBackgroundDraw);
}
