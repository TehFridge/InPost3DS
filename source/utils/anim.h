#ifndef ANIM_H
#define ANIM_H

typedef enum {
    EASE_IN,
    EASE_OUT,
    EASE_IN_OUT,
    EASE_SMOOTHSTEP,
    EASE_OUTBACKMM,
} EaseType;

typedef struct {
    float timer;
    float duration;
    float value;
    EaseType type;
} Ease;

void resetEase(Ease *ease);
Ease initEase(float duration, EaseType type);
void updateEase(Ease *ease);

#endif
