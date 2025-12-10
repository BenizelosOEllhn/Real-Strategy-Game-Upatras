#pragma once

struct Resources {
    int food  = 500;
    int wood  = 500;
    int stone = 250;

    bool Spend(int f, int w, int s)
    {
        if (food < f || wood < w || stone < s) return false;
        food  -= f;
        wood  -= w;
        stone -= s;
        return true;
    }
};
