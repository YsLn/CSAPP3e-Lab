#include <stdio.h>

int func(int di, int si, int dx)
{
    int ax = dx -si;
    int cx = ax;

    cx = (unsigned int)cx >> 0x1f;

    ax = ax + cx;
    ax = ax >> 1;
    cx = si + ax;

    if (cx > di) {
        dx = cx - 1;
        ax = func(di, si, dx);
        ax = ax + ax;
        return ax;
    }
    
    ax = 0;

    if (cx >= di) return ax;

    si = cx + 1;
    ax = func(di, si, dx);
    ax = ax + ax + 1;

    return ax;
}

int main()
{
    for (int i = 0; i < 0xf; i++) {
        int ret = func(i, 0, 0xe);
        printf("%2d -> %d", i, ret);
        
        if (ret == 0) {
            printf("    [y]");
        }
        printf("\n");
    }

    return 0;
}

