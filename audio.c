#include "engine.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static FILE *ap;
static float phase, nseed;

void audio_open(void)
{
    ap = popen("aplay -q -t raw -f S16_LE -r 22050 -c 1 2>/dev/null", "w");
}

void audio_close(void)
{
    if (ap) {
        pclose(ap);
        ap = NULL;
    }
}

static float frand(void)
{
    nseed = nseed * 1103515245.f + 12345.f;
    return fmodf(nseed * 1e-9f, 2.f) - 1.f;
}

void audio_chunk(const Engine *e, float dt)
{
    int n, i;
    int16_t *buf;
    if (!ap)
        return;
    n = (int)(22050.f * dt);
    if (n < 16)
        n = 16;
    if (n > 3000)
        n = 3000;
    buf = malloc((size_t)n * 2);
    if (!buf)
        return;
    for (i = 0; i < n; i++) {
        float t = (float)i / 22050.f;
        float mech, fire, exh, s;
        phase += e->w / 22050.f;
        mech = 0.08f * sinf(phase * 2.f) * (e->rpm / 4000.f);
        fire = e->fire_str * 0.45f * expf(-t * 80.f) * (0.6f + 0.4f * frand());
        exh = e->exh_open * 0.07f * frand() * (e->rpm > 200 ? 1.f : e->rpm / 200.f);
        s = mech + fire + exh;
        if (s > 1)
            s = 1;
        if (s < -1)
            s = -1;
        buf[i] = (int16_t)(s * 8000);
    }
    fwrite(buf, 2, (size_t)n, ap);
    fflush(ap);
    free(buf);
}
