#ifndef MIC_H
#define MIC_H

#include <zephyr/kernel.h>
#include <zephyr/audio/dmic.h>
#include <math.h>

int mic_init(void);
int mic_read(float *dbfs);

#endif