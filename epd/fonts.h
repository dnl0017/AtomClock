
#ifndef __FONTS_H
#define __FONTS_H

/* (32x41) */
#define MAX_HEIGHT_FONT        90// 41
#define MAX_WIDTH_FONT         56// 32
#define OFFSET_BITMAP           

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

//ASCII
typedef struct _tFont
{    
  const uint8_t *table;
  uint16_t Width;
  uint16_t Height;  
} sFONT;

extern sFONT FontBroadway;
extern sFONT FontCurly;
extern sFONT FontOldeEng;
extern sFONT Font7seg;
extern sFONT Font24;
extern sFONT Font20;
extern sFONT Font16;
extern sFONT Font12;
extern sFONT Font8;

#ifdef __cplusplus
}
#endif
  
#endif
