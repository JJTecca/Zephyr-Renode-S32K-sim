#ifndef CONFIG_H
#define CONFIG_H

#define TICK_MS        100
#define LEAK_HEAP_SZ   65536
#define BUSY_CAP_US    500000u
#define SDV_CAN_CMD_ID 0x200          /* center -> edge command frames */

#if ROLE_EDGE
static const char *st(enum can_state s);
static void send_telem(uint8_t sig, uint32_t val);
static void apply_cmd(int action);
#else
static void send_cmd(int node, int action);
static int supervisor_step(int src, int alarm, float ttf, int proposed);
static void dense(const float *w, const float *b, const float *in, float *out, int no, int ni, int relu);
static float ae_score(const float x[]);
static void detector_step(uint16_t seq, uint32_t heap_free, uint32_t loop_latency);
#endif

#endif