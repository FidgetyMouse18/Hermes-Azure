#include "mic.h"

#define SAMPLE_RATE 48000
#define SAMPLE_BIT_WIDTH 16
#define BLOCK_SIZE_SAMPLES (SAMPLE_RATE / SAMPLE_BIT_WIDTH)
#define BLOCK_COUNT 2

#define BYTES_PER_SAMPLE sizeof(int16_t)
/* Milliseconds to wait for a block to be read. */

#define READ_TIMEOUT 1000

/* Size of a block for 100 ms of audio data. */
#define BLOCK_SIZE(_sample_rate, _number_of_channels) \
    (BYTES_PER_SAMPLE * (_sample_rate / 10) * _number_of_channels)

/* Driver will allocate blocks from this slab to receive audio data into them.
 * Application, after getting a given block from the driver and processing its
 * data, needs to free that block.
 */
#define MAX_BLOCK_SIZE BLOCK_SIZE(SAMPLE_RATE, 1)

static const struct device *dmic = DEVICE_DT_GET(DT_NODELABEL(pdm0));

K_MEM_SLAB_DEFINE_STATIC(mem_slab, MAX_BLOCK_SIZE, BLOCK_COUNT, 4);

struct pcm_stream_cfg stream = {
    .pcm_width = SAMPLE_BIT_WIDTH,
    .mem_slab = &mem_slab,
    .pcm_rate = SAMPLE_RATE,
    .block_size = BLOCK_SIZE_SAMPLES * sizeof(int16_t)};

/* Correct DMIC configuration matching Zephyr sample structure */
static struct dmic_cfg cfg = {
    .io = {
        /* These fields can be used to limit the PDM clock
         * configurations that the driver is allowed to use
         * to those supported by the microphone.
         */
        .min_pdm_clk_freq = 1000000,
        .max_pdm_clk_freq = 3500000,
        .min_pdm_clk_dc = 40,
        .max_pdm_clk_dc = 60,
    },
    .streams = &stream,
    .channel = {
        .req_num_streams = 1,
        .req_num_chan = 1,

    },
};

int mic_init(void)
{
    if (!device_is_ready(dmic))
    {
        printf("DMIC device not ready\n");
        return -ENODEV;
    }

    // cfg.channel.req_num_chan = 1;
    cfg.channel.req_chan_map_lo =
        dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);
    // cfg.streams[0].pcm_rate = SAMPLE_RATE;
    // cfg.streams[0].block_size = BLOCK_SIZE(cfg.streams[0].pcm_rate, cfg.channel.req_num_chan);

    int ret = dmic_configure(dmic, &cfg);
    if (ret != 0)
    {
        printf("DMIC config failed: %d\n", ret);
        return ret;
    }

    return ret;
}

int mic_read(float *dbfs)
{
    int ret = dmic_trigger(dmic, DMIC_TRIGGER_START);
    if (ret < 0)
    {
        printf("START trigger failed: %d\n", ret);
        return ret;
    }

    int16_t *buffer;
    uint32_t size;
    uint32_t accumulator = 0;

    ret = dmic_read(dmic, 0, &buffer, &size, READ_TIMEOUT);
    if (ret < 0)
    {
        printf("read failed: %d\n", ret);
        return ret;
    }

    // printf("%d - got buffer %p of %u bytes\n", i, buffer, size);
    int sample_count = size / sizeof(int16_t);
    for (int j = 0; j < sample_count; j++)
    {
        // int16_t *dt = (int16_t *)buffer;
        accumulator += buffer[j] * buffer[j];
        // printf("got data: %d\n", dt[j]);
    }
    float rms = sqrtf((float)accumulator / (float)sample_count);
    printf("RMS: %0.2f\n", rms);

    k_mem_slab_free(&mem_slab, buffer);
    k_msleep(100);

    ret = dmic_trigger(dmic, DMIC_TRIGGER_STOP);
    if (ret < 0)
    {
        printf("STOP trigger failed: %d\n", ret);
        return ret;
    }

    return 0;
}

// #define MIC_DEBUG_SAMPLES 0

// int mic_read(float *dbfs) {
//     int ret;
//     int64_t sum_squares = 0;
//     size_t total_samples = 0;
//     *dbfs = -120.0f;

//     ret = dmic_trigger(dmic, DMIC_TRIGGER_START);
//     if (ret < 0) {
//         printf("Start failed: %d\n", ret);
//         return ret;
//     }

//     for (int i = 0; i < 2 * BLOCK_COUNT; ++i) {
//         void *buffer;
//         uint32_t size;

//         ret = dmic_read(dmic, 0, &buffer, &size, READ_TIMEOUT);
//         if (ret < 0) {
//             printf("Read failed: %d\n", ret);
//             break;
//         }

//         if (size > 0) {
//             int16_t *samples = (int16_t *)buffer;
//             size_t sample_count = size / sizeof(int16_t);

//             // Debug print first few samples
//             for (int j = 0; j < MIN(MIC_DEBUG_SAMPLES, sample_count); j++) {
//                 printf("Sample %d: %hd\n", j, samples[j]);
//             }

//             for (size_t j = 0; j < sample_count; j++) {
//                 sum_squares += (int32_t)samples[j] * (int32_t)samples[j];
//             }
//             total_samples += sample_count;
//         }

//         k_mem_slab_free(&mem_slab, buffer);
//         k_msleep(10);  // Reduced sleep for better timing
//     }

//     dmic_trigger(dmic, DMIC_TRIGGER_STOP);

//     if (total_samples > 0) {
//         float rms = sqrtf((float)sum_squares / total_samples);
//         *dbfs = 20.0f * log10f(rms / 32768.0f + 1e-12f);  // Avoid log(0)
//         printf("Total samples: %zu, RMS: %.2f, dBFS: %.2f\n",
//                total_samples, rms, *dbfs);
//     }

//     return ret;
// }