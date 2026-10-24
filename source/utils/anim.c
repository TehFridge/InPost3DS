#include "anim.h"

static inline float easeValue(EaseType type, float t)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;

    switch (type)
    {
        case EASE_IN:
            return t * t;

        case EASE_OUT:
            return 1.0f - (1.0f - t) * (1.0f - t);

        case EASE_IN_OUT:
            if (t < 0.5f)
                return 2.0f * t * t;
            return 1.0f - 2.0f * (1.0f - t) * (1.0f - t);

        case EASE_SMOOTHSTEP:
            return t * t * (3.0f - 2.0f * t);

        case EASE_OUTBACKMM:
        {
            return 1.0f + (t - 1.0f) * (t - 1.0f) * (2.70158f * (t - 1.0f) + 1.70158f);
        }

        default:
            break;
    }

    return t;
}

void resetEase(Ease *ease)
{
    ease->timer = 0.0f;
    ease->value = 0.0f;
}

Ease initEase(float duration, EaseType type)
{
    Ease ease = {
        .timer = 0.0f,
        .duration = duration,
        .value = 0.0f,
        .type = type,
    };

    return ease;
}

void updateEase(Ease *ease)
{
    ease->timer += 1.0f / 60.0f;

    if (ease->timer > ease->duration)
        ease->timer = ease->duration;

    ease->value = easeValue(ease->type, ease->timer / ease->duration);
}