#ifndef __FONT_H__
#define __FONT_H__    


typedef struct 
{
    const char *name;
    const uint8_t *model;

} font_chinese_t;

typedef struct
{
    uint16_t size;
    const uint8_t *ascii_model;
    const char *ascii_map;
    const font_chinese_t *chinese;
} font_t;



extern const font_t font16;
#endif //__FONT_H__