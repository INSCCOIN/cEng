#include <stdio.h>
#include <unistd.h>
static FILE *f;
void log_init(void)
{
    unlink("/home/ceng.log");
    f = fopen("/home/ceng.log", "w");
    if (f) {
        fprintf(f, "cEng start\n");
        fflush(f);
    }
}
void log_line(const char *s)
{
    if (!f)
        return;
    fprintf(f, "%s\n", s);
    fflush(f);
}
void log_close(void)
{
    if (!f)
        return;
    fprintf(f, "cEng exit\n");
    fclose(f);
    f = NULL;
}
