#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "ite/itu.h"
#include "itu_cfg.h"
#include "itu_private.h"
#include "ite/itp.h"

static const char cbtName[] = "ITUComboTable";

//use for tree config related

void ituComboTableDebugErr(ITUWidget* widget, int error_line, int error_no)
{
	if (error_no)
	{
		char err_str[100] = "";

		switch (error_no)
		{
		case ITU_CBT_ERR_NOCHILD:
			strcpy(err_str, "without child");
			break;
		case ITU_CBT_ERR_CHILDNOTCOT:
			strcpy(err_str, "first child is not container");
			break;
		default:
			break;
		}

		printf("[CBT_Err][%s][%s][%s][%d]\n", widget->name, err_str, __FILE__, error_line);
	}
	else
	{
		printf("[CBT_Err][%s][%s][%d]\n", widget->name, __FILE__, error_line);
	}
}

bool ituComboTableClone(ITUWidget* widget, ITUWidget** cloned)
{
	assert(widget);
	assert(cloned);
	ITU_ASSERT_THREAD();

	if (*cloned == NULL)
	{
		ITUWidget* newWidget = malloc(sizeof(ITUComboTable));
		if (newWidget == NULL)
			return false;

		memcpy(newWidget, widget, sizeof(ITUComboTable));
		newWidget->tree.child = newWidget->tree.parent = newWidget->tree.sibling = NULL;
		*cloned = newWidget;
	}

	return ituWidgetCloneImpl(widget, cloned);
}

ITUWidget* ComboTableGetVisibleChild(ITUComboTable* cbt)
{
	ITCTree *child, *tree = (ITCTree*)cbt;
	assert(tree);

	for (child = tree->child; child; child = child->sibling)
	{
		if (child)
		{
			ITUWidget* widget = (ITUWidget*)child;

			if (widget->visible)
			{
				return widget;
			}
		}
	}
	return NULL;
}

void ituComboTableExit(ITUWidget* widget)
{
	ITUComboTable* cbt = (ITUComboTable*)widget;
	assert(widget);
	ITU_ASSERT_THREAD();

	if (cbt->surf)
	{
		ituSurfaceRelease(cbt->surf);
		cbt->surf = NULL;
	}
}

//use refParent to find the cloned parent
ITUWidget* ituComboTableFindTargetParent(ITUWidget* cbt_widget, ITUWidget* refParent)
{
	ITUComboTable* cbt = (ITUComboTable*)cbt_widget;
	assert(cbt);
	if (cbt->tmpObj)
	{
		ITCTree* tree = (ITCTree*)cbt->tmpObj;
		ITCTree* node = tree->child;
		char str[100] = "";
		sprintf(str, "%X_%d", (*&refParent), cbt->itemcount - 1);

		if (strcmp(cbt->tmpObj->name, str) == 0)
			return cbt->tmpObj;

		for (; node; node->sibling)
		{
			ITUWidget* checkWidget = (ITUWidget*)node;
			if (checkWidget)
			{
				if (strcmp(checkWidget->name, str) == 0)
					return checkWidget;
			}

			if (node->child)
				ituComboTableFindTargetParent(cbt_widget, refParent);
		}
	}

	return NULL;
}


bool ituComboTableUpdate(ITUWidget* widget, ITUEvent ev, int arg1, int arg2, int arg3)
{
	bool result = false;
	ITUComboTable* cbt = (ITUComboTable*)widget;
	assert(cbt);

	if (ev == ITU_EVENT_LOAD)
	{
		ituComboTableLoadStaticData(cbt);
	}
	else if (ev == ITU_EVENT_LAYOUT)
	{
		ITCTree *tree = (ITCTree*)cbt;
		ITUWidget* first_child_widget = (ITUWidget*)(tree->child);
		int error_num = -1;

		//check setup rule
		if (first_child_widget)
		{
			if (first_child_widget->type != ITU_CONTAINER)
				error_num = ITU_CBT_ERR_CHILDNOTCOT;
			else
				cbt->itemArr[0] = first_child_widget;
		}
		else
			error_num = ITU_CBT_ERR_NOCHILD;

		if (error_num >= 0)
		{
			ituComboTableDebugErr(widget, __LINE__, error_num);
			return false;
		}
		else
		{
			int i = 0;
			int j = cbt->itemcount;
			int p = 0;
			int count = 0;
			cbt->itemcount = 0;
			//check current valid item count
			for (i = 0; i < ITU_WIDGET_CHILD_MAX; i++)
			{
				if (cbt->itemArr[i] != NULL)
					cbt->itemcount++;
				else
					break;
			}
			//grow item upto the target item count
			for (i = cbt->itemcount; i < j; i++)
			{
				if (cbt->itemArr[i] == NULL)
					ituComboTableAddItem(widget, widget);
			}
			//rearrange item position
			p = cbt->currPage * cbt->pagemaxitem;
			for (i = p; i < ITU_WIDGET_CHILD_MAX; i++)
			{
				if (cbt->itemArr[i] != NULL)
				{
					int yPos = 0;
					count++;
					if (i == p)
						continue;
					else
					{
						yPos = (cbt->itemArr[i - 1]->rect.y + cbt->itemArr[i - 1]->rect.height);
						ituWidgetSetPosition(cbt->itemArr[i], cbt->itemArr[i]->rect.x, yPos);
					}

					if (count >= cbt->pagemaxitem)
					{
						count = 0;
						p = i + 1;
						if (cbt->itemArr[p])
							ituWidgetSetPosition(cbt->itemArr[p], cbt->itemArr[p]->rect.x, 0);
						else
							break;
					}
				}
				else
					break;
			}
		}

		cbt->hv_mode = 0;
		widget->dirty = 1;
	}
	else if (ev == ITU_EVENT_MOUSEDOWN)
	{
		if (widget->flags & ITU_DRAGGABLE)
		{
			int x = arg2 - widget->rect.x;
			int y = arg3 - widget->rect.y;
			if (ituWidgetIsInside(widget, x, y))
			{
				if (ituScene->dragged == NULL)
				{
					cbt->initX = cbt->touchX = x;
					cbt->initY = cbt->touchY = y;
					widget->flags |= ITU_DRAGGING;
					ituScene->dragged = widget;
				}

				cbt->lastclock = itpGetTickCount();
			}
		}//if (widget->flags & ITU_DRAGGABLE)
	}
	else if (ev == ITU_EVENT_MOUSEUP)
	{
		if (widget->flags & ITU_DRAGGING)
		{
			cbt->touchX = 0;
			cbt->touchY = 0;
			widget->flags &= ~ITU_DRAGGING;
			ituScene->dragged = NULL;

			cbt->clock = itpGetTickCount();
			if ((cbt->clock - cbt->lastclock) <= ITU_COMBOTABLE_MIT)
			{
				int x = arg2 - widget->rect.x;
				int y = arg3 - widget->rect.y;
				if (ituWidgetIsInside(widget, x, y))
				{
					int i = 0;
					//use currpage to calculate start index of current page
					int p = cbt->currPage * cbt->pagemaxitem;
					for (i = 0; i < cbt->pagemaxitem; i++)
					{
						if (cbt->itemArr[p])
						{
							ITUWidget* item = cbt->itemArr[p];
							//inside check must consider with target rect.x/y
							if (ituWidgetIsInside(item, x - item->rect.x, y - item->rect.y))
							{
								if (cbt->selectIndex >= 0)
									cbt->lastselectIndex = cbt->selectIndex;
								cbt->selectIndex = p;
								//printf("[CBT]select index %d\n", p);
								ituExecActions((ITUWidget*)cbt, cbt->actions, ITU_EVENT_SELECT, cbt->selectIndex);
								break;
							}
						}
						p++;
					}
				}
			}
		}
	}
	else if (ev == ITU_EVENT_MOUSEMOVE)
	{
		if (widget->flags & ITU_DRAGGING)
		{
			int x = arg2 - widget->rect.x;
			int y = arg3 - widget->rect.y;
			int distX = x - cbt->touchX;
			int distY = y - cbt->touchY;
			int Xtd = x - cbt->initX;
			int Ytd = y - cbt->initY;
			bool X_dir = false;//(abs(distX) > abs(distY)) ? (true) : (false);
			bool Y_dir = (abs(distX) < abs(distY)) ? (true) : (false);

			if ((abs(Xtd) >= ITU_DRAG_DISTANCE) || (abs(Ytd) >= ITU_DRAG_DISTANCE))
			{
				ituUnPressWidget(widget);
			}

			if (ituWidgetIsInside(widget, x, y))
			{
				ituComboTableMoveXY(cbt, distX, distY);
				cbt->touchX = x;
				cbt->touchY = y;
				widget->dirty = 1;
			}

		}//if (widget->flags & ITU_DRAGGING)
	}//if (ev == ITU_EVENT_MOUSEMOVE)
	else if (((ev == ITU_EVENT_TOUCHSLIDEUP) || (ev == ITU_EVENT_TOUCHSLIDEDOWN)) && (cbt->hv_mode == 0) && (cbt->sliding == 1))
	{
		if (ev == ITU_EVENT_TOUCHSLIDEUP)
			cbt->inc = -1;
		else
			cbt->inc = 1;
		cbt->comboTableFlags |= ITU_COMBOTABLE_SLIDING;
	}
	else if (ev == ITU_EVENT_TIMER)
	{
		if ((cbt->comboTableFlags & ITU_COMBOTABLE_SLIDING) && (!(widget->flags & ITU_DRAGGING)))
		{
			if (cbt->itemArr[0])
			{
				if (cbt->frame < cbt->totalframe)
				{
					int distX = 0;
					int distY = 0;
					int itemBaseSize = (cbt->hv_mode == 0) ? (cbt->itemArr[0]->rect.height) : (cbt->itemArr[0]->rect.width);
					float stepDist = itemBaseSize * 1.0; //modify here to change the slide distance
					stepDist = cbt->inc * stepDist * exp((cbt->frame * -5.0 / cbt->totalframe));
					distX = (cbt->hv_mode == 0) ? (0) : (stepDist);
					distY = (cbt->hv_mode == 0) ? (stepDist) : (0);
					ituComboTableMoveXY(cbt, distX, distY);
					cbt->frame++;
					widget->dirty = 1;
				}
				else
				{
					cbt->comboTableFlags &= ~ITU_COMBOTABLE_SLIDING;
					cbt->inc = 0;
					cbt->frame = 0;
				}
			}
			else
			{
				cbt->comboTableFlags &= ~ITU_COMBOTABLE_SLIDING;
				cbt->inc = 0;
			}
		}
	}

	result |= widget->dirty;
	result |= ituWidgetUpdateImpl(widget, ev, arg1, arg2, arg3);

	return widget->visible ? result : false;
}

void ituComboTableDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
	int destx, desty;
	uint8_t desta;
	ITUComboTable* cbt = (ITUComboTable*)widget;
	ITURectangle* rect = &widget->rect;
	ITUSurface* bgSurf = NULL;
	ITCTree* node;
	ITURectangle prevClip;
	assert(cbt);
	assert(dest);

	if (!widget->visible)
		return;

	destx = rect->x + x;
	desty = rect->y + y;
	desta = alpha * widget->alpha / 255;

	//when item count is 0, draw nothing
	if (cbt->itemcount == 0)
	{
		return;
	}

	ituWidgetSetClipping(widget, dest, x, y, &prevClip);

	//draw background
	if (cbt->surf)
	{
		if (widget->flags & ITU_STRETCH)
		{
#ifdef CFG_WIN32_SIMULATOR
			if (widget->angle == 0)
			{
				ituStretchBlt(dest, destx, desty, rect->width, rect->height, cbt->surf, 0, 0, cbt->surf->width, cbt->surf->height);
			}
			else
			{
				float scaleX, scaleY;
				if (widget->angle == 90 || widget->angle == 270)
				{
					scaleX = (float)rect->width / cbt->surf->height;
					scaleY = (float)rect->height / cbt->surf->width;
				}
				else
				{
					scaleX = (float)rect->width / cbt->surf->width;
					scaleY = (float)rect->height / cbt->surf->height;
				}
				ituRotate(dest, destx + rect->width / 2, desty + rect->height / 2, cbt->surf, cbt->surf->width / 2, cbt->surf->height / 2, (float)widget->angle, scaleX, scaleY);
			}
#else
			float scaleX = (float)rect->width / cbt->surf->width;
			float scaleY = (float)rect->height / cbt->surf->height;

			ituTransform(
				dest, destx, desty, rect->width, rect->height,
				cbt->surf, 0, 0, cbt->surf->width, cbt->surf->height,
				cbt->surf->width / 2, cbt->surf->height / 2,
				scaleX,
				scaleY,
				(float)widget->angle,
				0,
				true,
				true,
				desta);
#endif
		}
		else
		{
			if (widget->angle == 0)
			{
				ituBitBlt(dest, destx, desty, rect->width, rect->height, cbt->surf, 0, 0);
			}
			else
			{
#ifdef CFG_WIN32_SIMULATOR
				ituRotate(dest, destx + rect->width / 2, desty + rect->height / 2, cbt->surf, cbt->surf->width / 2, cbt->surf->height / 2, (float)widget->angle, 1.0f, 1.0f);
#else
				ituRotate(dest, destx, desty, cbt->surf, cbt->surf->width / 2, cbt->surf->height / 2, (float)widget->angle, 1.0f, 1.0f);
#endif
			}
		}
	}
	else
	{
		ituColorFill(dest, destx, desty, rect->width, rect->height, &widget->color);
	}

	//draw item background
	if (cbt->itemSurf)
	{
		int drawcount = cbt->itemcount - (cbt->currPage * cbt->pagemaxitem);
		if ((drawcount > 0) && cbt->itemArr[0])
		{
			int i = 0;
			for (i = 0; i < drawcount; i++)
			{
				int cid = (cbt->currPage * cbt->pagemaxitem) + i;
				//int itemx = destx;
				//int itemy = desty + (i * cbt->itemArr[0]->rect.height);
				int itemx = destx + cbt->itemArr[cid]->rect.x;
				int itemy = desty + cbt->itemArr[cid]->rect.y;

				//if ((itemy + cbt->itemSurf->height) > (desty + widget->rect.height))
				//	break;
				if (itemy > (desty + widget->rect.height))
					break;

				if (widget->flags & ITU_STRETCH)
				{
#ifdef CFG_WIN32_SIMULATOR
					if (widget->angle == 0)
					{
						ituAlphaBlend(dest, itemx, itemy, cbt->itemSurf->width, cbt->itemSurf->height, cbt->itemSurf, 0, 0, 255);
					}
					else
					{
						float scaleX, scaleY;
						if (widget->angle == 90 || widget->angle == 270)
						{
							scaleX = (float)cbt->itemArr[0]->rect.width / cbt->itemSurf->height;
							scaleY = (float)cbt->itemArr[0]->rect.height / cbt->itemSurf->width;
						}
						else
						{
							scaleX = (float)cbt->itemArr[0]->rect.width / cbt->itemSurf->width;
							scaleY = (float)cbt->itemArr[0]->rect.height / cbt->itemSurf->height;
						}
						ituRotate(dest, itemx + cbt->itemArr[0]->rect.width / 2, itemy + cbt->itemArr[0]->rect.height / 2, cbt->itemSurf, cbt->itemSurf->width / 2, cbt->itemSurf->height / 2, (float)widget->angle, scaleX, scaleY);
					}
#else
					float scaleX = (float)cbt->itemArr[0]->rect.width / cbt->itemSurf->width;
					float scaleY = (float)cbt->itemArr[0]->rect.height / cbt->itemSurf->height;

					ituTransform(
						dest, itemx, itemy, cbt->itemArr[0]->rect.width, cbt->itemArr[0]->rect.height,
						cbt->itemSurf, 0, 0, cbt->itemSurf->width, cbt->itemSurf->height,
						cbt->itemSurf->width / 2, cbt->itemSurf->height / 2,
						scaleX,
						scaleY,
						(float)widget->angle,
						0,
						true,
						true,
						desta);
#endif
				}
			}
		}
	}
	ituSurfaceSetClipping(dest, prevClip.x, prevClip.y, prevClip.width, prevClip.height);
	ituWidgetDrawImpl(widget, dest, x, y, alpha);
}

void ituComboTableOnAction(ITUWidget* widget, ITUActionType action, char* param)
{
	ITUComboTable* cbt = (ITUComboTable*)widget;
	assert(cbt);

	switch (action)
	{
	case ITU_ACTION_NEXT:
		{
			int vCurrPage = ituComboTableGetCurrentPageNumber(cbt);
			if (vCurrPage >= 0)
			{
				int vTargetPage = vCurrPage + 1;
				ituComboTableGotoPage(cbt, vTargetPage);
			}
		}
		break;
	case ITU_ACTION_PREV:
		{
			int vCurrPage = ituComboTableGetCurrentPageNumber(cbt);
			if (vCurrPage >= 0)
			{
				int vTargetPage = vCurrPage - 1;
				ituComboTableGotoPage(cbt, vTargetPage);
			}
		}
		break;
	default:
		ituWidgetOnActionImpl(widget, action, param);
		break;
	}
}

void ituComboTableInit(ITUComboTable* cbt)
{
    assert(cbt);
    ITU_ASSERT_THREAD();

    memset(cbt, 0, sizeof (ITUComboTable));

	ituWidgetInit(&cbt->widget);
    
    ituWidgetSetType(cbt, ITU_COMBOTABLE);
    ituWidgetSetName(cbt, cbtName);
	ituWidgetSetExit(cbt, ituComboTableExit);
	ituWidgetSetUpdate(cbt, ituComboTableUpdate);
	ituWidgetSetDraw(cbt, ituComboTableDraw);
	ituWidgetSetOnAction(cbt, ituComboTableOnAction);
	ituWidgetSetClone(cbt, ituComboTableClone);
}

void ituComboTableLoad(ITUComboTable* cbt, uint32_t base)
{
    assert(cbt);

	ituWidgetLoad(&cbt->widget, base);
	ituWidgetSetExit(cbt, ituComboTableExit);
	ituWidgetSetUpdate(cbt, ituComboTableUpdate);
	ituWidgetSetDraw(cbt, ituComboTableDraw);
	ituWidgetSetOnAction(cbt, ituComboTableOnAction);
	ituWidgetSetClone(cbt, ituComboTableClone);

	//load the background image
	if (cbt->staticSurf)
	{
		ITUSurface* surf = (ITUSurface*)(base + (uint32_t)cbt->staticSurf);
		if (surf->flags & ITU_COMPRESSED)
			cbt->surf = NULL;
		else
			cbt->surf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format, (const uint8_t*)surf->addr, surf->flags);

		cbt->staticSurf = surf;
	}

	//load the item background image
	if (cbt->itemStaticSurf)
	{
		ITUSurface* surf = (ITUSurface*)(base + (uint32_t)cbt->itemStaticSurf);
		if (surf->flags & ITU_COMPRESSED)
			cbt->itemSurf = NULL;
		else
			cbt->itemSurf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format, (const uint8_t*)surf->addr, surf->flags);

		cbt->itemStaticSurf = surf;
	}
}

void ituComboTableAddItem(ITUWidget* cbtWidget, ITUWidget* widget)
{
	bool vPrintfDebug = false;
	ITUComboTable* cbt = (ITUComboTable*)cbtWidget;
	assert(cbt);

	if (cbt->itemcount == 0)
	{
		cbt->itemcount++;
		return;
	}
	else
	{
		ITCTree *tree = (ITCTree*)widget;
		ITCTree* child = tree->child;
		for (child = tree->child; child; child = child->sibling)
		{
			if (child)
			{
				ITUWidget* obj = (ITUWidget*)child;
				ITUWidget* cloned = NULL;
				ITUWidget* test = NULL;
				if (ituWidgetClone(obj, &cloned))
				{
					ITCTree* itc_Cloned = (ITCTree*)cloned;
					char str[100] = "";
					//update itemcount here before make name str
					if ((cloned->type == ITU_CONTAINER) && (widget->type == ITU_COMBOTABLE))
					{
						cbt->itemArr[cbt->itemcount] = cloned;
						cbt->itemcount++;
					}
					if (vPrintfDebug && (cbt->itemcount == 2))
						printf("[CBT][%s] clone [child %s] [parent %s]\n", cbtWidget->name, cloned->name, widget->name);


					//make name str
					sprintf(str, "%X_%d", (*&obj), cbt->itemcount - 1);
					ituWidgetSetName(cloned, str);


					//clear the tree of cloned obj
					if (obj->type != ITU_BUTTON)
					{
						itc_Cloned->parent = NULL;
						itc_Cloned->child = NULL;
						itc_Cloned->sibling = NULL;
					}
					
					if ((cloned->type == ITU_CONTAINER) && (widget->type == ITU_COMBOTABLE))
					{
						cbt->tmpObj = cloned;
						ituWidgetAdd(widget, cloned);

						if (cbt->itemcount >= 2)
						{
							int vRefItemIdx = 0;
							if (cbt->itemcount <= cbt->pagemaxitem) //only one page
							{
								vRefItemIdx = cbt->itemcount - 2;
								ituWidgetSetY(cloned, ituWidgetGetY(cbt->itemArr[vRefItemIdx]) + ituWidgetGetHeight(cbt->itemArr[vRefItemIdx]));
							}
							else
							{
								if ((cbt->itemcount % cbt->pagemaxitem) == 1)
									ituWidgetSetY(cloned, 0);
								else
								{
									vRefItemIdx = cbt->itemcount - 2;
									ituWidgetSetY(cloned, ituWidgetGetY(cbt->itemArr[vRefItemIdx]) + ituWidgetGetHeight(cbt->itemArr[vRefItemIdx]));
								}
								//hide the item that not located in current page
								if (ituComboTableGetMaxPageIndex(cbt) != cbt->currPage)
									ituWidgetSetVisible(cloned, false);
							}
							
						}
					}
					else
					{
						ITUWidget* refParent = ituComboTableFindTargetParent(cbtWidget, widget);
						if (refParent)
							ituWidgetAdd(refParent, cloned);
					}


					if (child->child)
					{
						ITUWidget* source = (ITUWidget*)child;
						ITUWidget* target = (ITUWidget*)child->child;
						if (strlen(target->name))
							ituComboTableAddItem(cbtWidget, source);
					}

					if ((cloned->type == ITU_CONTAINER) && (widget->type == ITU_COMBOTABLE))
						return;
				}

				
			}

			
		}//////
	}
}

void ituComboTableLoadStaticData(ITUComboTable* cbt)
{
	ITUWidget* widget = (ITUWidget*)cbt;
	ITUSurface* surf = NULL;
	ITUSurface* isurf = NULL;

	if (widget->flags & ITU_EXTERNAL)
		return;

	if (cbt->staticSurf && !cbt->surf)
	{
		surf = cbt->staticSurf;

		if (surf->flags & ITU_COMPRESSED)
			cbt->surf = ituSurfaceDecompress(surf);
		else
			cbt->surf = ituCreateSurface(surf->width, surf->height, surf->pitch, surf->format, (const uint8_t*)surf->addr, surf->flags);
	}

	if (cbt->itemStaticSurf && !cbt->itemSurf)
	{
		isurf = cbt->itemStaticSurf;

		if (isurf->flags & ITU_COMPRESSED)
			cbt->itemSurf = ituSurfaceDecompress(isurf);
		else
			cbt->itemSurf = ituCreateSurface(isurf->width, isurf->height, isurf->pitch, isurf->format, (const uint8_t*)isurf->addr, isurf->flags);
	}
}

void ituComboTableMoveXY(ITUComboTable* cbt, int distX, int distY)
{
	if (cbt->itemArr[0])
	{
		int modX = 0;
		int modY = 0;
		int vPageFirstIndex = cbt->currPage * cbt->pagemaxitem;
		int vPageLastIndex = (cbt->currPage < ituComboTableGetMaxPageIndex(cbt)) ?
			(vPageFirstIndex + cbt->pagemaxitem - 1) :
			((cbt->currPage * cbt->pagemaxitem) + (cbt->itemcount % cbt->pagemaxitem) - 1);

		ITUWidget* pPageFirstItem = cbt->itemArr[vPageFirstIndex];
		ITUWidget* pPageLastItem = cbt->itemArr[vPageLastIndex];

		if (distY <= 0)
			modY = ((pPageLastItem->rect.y + distY) < 0) ? (0 - pPageLastItem->rect.y) : (distY);
		else
			modY = ((pPageFirstItem->rect.y + distY) > 0) ? (0 - pPageFirstItem->rect.y) : (distY);

		if ((modX != 0) || (modY != 0))
		{
			int i = vPageFirstIndex;
			while (cbt->itemArr[i] != NULL)
			{
				cbt->itemArr[i]->rect.x += modX;
				cbt->itemArr[i]->rect.y += modY;
				i++;
				if (i > vPageLastIndex)
					break;
			}
		}
	}
}


int ituComboTableGetMaxPageIndex(ITUComboTable* cbt)
{
	assert(cbt);
	if (cbt->itemcount == 0)
	{
		return -1;
	}
	else
	{
		int vMaxPageIdx = (cbt->itemcount / cbt->pagemaxitem);
		if (vMaxPageIdx && (vMaxPageIdx > cbt->pagemaxitem))
			vMaxPageIdx++;
		return vMaxPageIdx;
	}
}

//goto other page (goto current page will reset item position only)
void ituComboTableGotoPage(ITUComboTable* cbt, int page)
{
	assert(cbt);
	if (page < 0)
		return;
	else if (page > ituComboTableGetMaxPageIndex(cbt))
		return;
	else
	{
		int i = cbt->currPage * cbt->pagemaxitem;
		int j = page * cbt->pagemaxitem;
		int x = 0;
		bool vSamePage = (page == cbt->currPage);
		for (; x < cbt->pagemaxitem; x++)
		{
			if (cbt->itemArr[i])
			{
				ituWidgetSetVisible(cbt->itemArr[i], vSamePage);

				if (x == 0)
					ituWidgetSetY(cbt->itemArr[i], 0);
				else
					ituWidgetSetY(cbt->itemArr[i], cbt->itemArr[i - 1]->rect.y + cbt->itemArr[i - 1]->rect.height);
			}

			if ((cbt->itemArr[j]) && (!vSamePage))
			{
				ituWidgetSetVisible(cbt->itemArr[j], true);
			}

			i++;
			j++;
		}
		cbt->currPage = page;
		ituExecActions((ITUWidget*)cbt, cbt->actions, ITU_EVENT_CHANGED, cbt->currPage);
	}
}


void ituComboTableChildArray(ITCTree* tree, ITUWidget** widget_arr)
{
	if (tree)
	{
		ITCTree* child = tree->child;
		for (; child; child = child->sibling)
		{
			if (child)
			{
				ITUWidget* widget = (ITUWidget*)child;
				widget_arr[0] = widget;
				widget_arr++;

				if (child->child)
					ituComboTableChildArray(child, widget_arr);
			}
		}
	}
}


ITUWidget* ituComboTableGetItemChild(ITUComboTable* cbt, int item_index, ITUWidgetType type, int child_index)
{
	assert(cbt);
	if ((item_index < 0) || (item_index >= cbt->itemcount) || (child_index < 0))
		return NULL;
	else
	{
		ITCTree* tree = (ITCTree*)cbt->itemArr[item_index];
		if (tree)
		{
			int i = 0;
			int target_index = -1;
			ITUWidget** widgetArr[128] = { NULL };

			if (child_index >= 128)
				return NULL;

			ituComboTableChildArray(tree, widgetArr);

			for (i = 0; i < 128; i++)
			{
				ITUWidget* child = widgetArr[i];
				if (child)
				{
					if (child->type == type)
						target_index++;
					if (target_index == child_index)
						return child;
				}
				else
					break;
			}

			return NULL;
		}
		else
			return NULL;
	}
}

void ituComboTableDelItem(ITUComboTable* cbt, int item_index)
{
	if (cbt)
	{
		if (cbt->itemArr[0])
		{
			ITCTree *tree = (ITCTree*)cbt;
			ITCTree* child = tree->child;
			int i = 0;
			bool vFound = false;
			for (child = tree->child; child; child = child->sibling)
			{
				ITUWidget* obj = (ITUWidget*)child;
				if (obj)
				{
					if (i == item_index)
					{
						itcTreeRemove(child);
						ituWidgetExit(obj);
						vFound = true;
						break;
					}
					i++;
				}
				else
					continue;
			}

			if (vFound)
			{
				if ((i == 0) && (cbt->itemcount == 1))
				{
					cbt->itemArr[0] = NULL;
					cbt->itemcount = 0;
				}
				else
				{
					int x = 0;
					for (x = i; x < cbt->itemcount; x++)
					{
						if ((x + 1) < ITU_WIDGET_CHILD_MAX)
							cbt->itemArr[x] = cbt->itemArr[x + 1];
						if ((x + 1) == cbt->itemcount)
							cbt->itemArr[x + 1] = NULL;
					}
				}

				if (item_index == cbt->selectIndex)
				{
					cbt->selectIndex = -1;
					cbt->lastselectIndex = -1;
				}
				ituWidgetUpdate(cbt, ITU_EVENT_LAYOUT, 0, 0, 0);
				if (item_index < (cbt->currPage * cbt->pagemaxitem))
					ituComboTableGotoPage(cbt, cbt->currPage);
			}
		}
	}
}

int ituComboTableGetItemIndexOfWidget(ITUComboTable* cbt, ITUWidget* widget, bool currPage)
{
	if (cbt && widget)
	{
		int i = 0;
		int maxloop = (currPage) ? (cbt->pagemaxitem) : (ITU_WIDGET_CHILD_MAX);
		//use currpage to calculate start index of current page
		int p = (currPage) ? (cbt->currPage * cbt->pagemaxitem) : (0);
		ITUWidget* cbtWidget = (ITUWidget*)cbt;
		for (i = 0; i < maxloop; i++)
		{
			if (cbt->itemArr[p])
			{
				ITUWidget* item = cbt->itemArr[p];
				ITCTree* child = (ITCTree*)widget;
				while (child->parent)
				{
					ITUWidget* parent = (ITUWidget*)(child->parent);
					if (parent == item)
						return p;
					else if (parent == cbtWidget)
						break;
					else
						child = child->parent;
				}
			}
			p++;
		}
		return -1;
	}
	else
		return -1;
}

int ituComboTableGetCurrentPageNumber(ITUComboTable* cbt)
{
	if (cbt)
		return cbt->currPage;
	else
		return -1;
}

int ituComboTableGetSelectedIndex(ITUComboTable* cbt, bool current)
{
	if (cbt)
	{
		if (current)
			return cbt->selectIndex;
		else
			return cbt->lastselectIndex;
	}
	else
		return -1;
}