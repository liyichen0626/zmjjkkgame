#pragma once

#include <QString>

class GameAudio
{
public:
    static int sfxPercent();
    static int bgmPercent();

    /// 预留：后期放入 WAV/OGG 资源后在此播放。
    static void playHurt();
    static void playAttack();
    static void playCollect();
};
