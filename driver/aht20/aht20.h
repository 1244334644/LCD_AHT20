#ifndef __AHT20_H__
#define __AHT20_H_

#include <stdbool.h>
#include <stdint.h>
typedef struct aht20_desc *aht20_desc_t;

void aht20_init(aht20_desc_t aht20);
bool aht20_measure(aht20_desc_t aht20, float *temp, float *humi);


#endif // !__AHT20_H_
