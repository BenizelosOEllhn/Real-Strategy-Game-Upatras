#pragma once

struct Resources {
    int food  = 500;
    int wood  = 500;
    int stone = 250;
    int gold  = 300;
    int maxPopulation = 4;

    bool Spend(int f, int w, int s)
    {
        if (food < f || wood < w || stone < s) return false;
        food  -= f;
        wood  -= w;
        stone -= s;
        return true;
    }
};
