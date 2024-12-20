#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scene.h"
#include "ctrlboard.h"
/* widgets:
logoLayer
logoBackground
*/
extern ITUScene            theScene;

bool LogoOnEnter(ITUWidget* widget, char* param)
{
    // sleep(5);
    // ituLayerGoto(ituSceneFindWidget(&theScene, "ch2Layer"));

    return true;
}

