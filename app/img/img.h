#ifndef __IMG_H__
#define __IMG_H__    

typedef struct
{
    uint16_t width;
    uint16_t height;
    const uint8_t *data;

} img_t;


extern const img_t img_welcome;
#endif //__IMG_H__
