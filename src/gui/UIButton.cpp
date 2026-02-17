#include "UIButton.h"

bool UIButton::contains(float mx, float my) const
{
    if (!clickable || !visible) return false;

    return mx >= pos.x &&
           mx <= pos.x + size.x &&
           my >= pos.y &&
           my <= pos.y + size.y;
}
