#include <termios.h>
#include <unistd.h>

static struct termios oldt;
static int raw;

void input_open(void)
{
    struct termios t;
    tcgetattr(0, &oldt);
    t = oldt;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &t);
    raw = 1;
}

void input_close(void)
{
    if (raw)
        tcsetattr(0, TCSANOW, &oldt);
}

int input_read(unsigned char *buf, int n)
{
    return (int)read(0, buf, (size_t)n);
}
