#include "gameaudio.h"

#include <QSettings>

int GameAudio::sfxPercent()
{
    QSettings s;
    return s.value("audio/sfx", 70).toInt();
}

int GameAudio::bgmPercent()
{
    QSettings s;
    return s.value("audio/bgm", 70).toInt();
}

void GameAudio::playHurt()
{
    if (sfxPercent() <= 0) {
        return;
    }
    // TODO: QSoundEffect / WAV 资源：受伤
}

void GameAudio::playAttack()
{
    if (sfxPercent() <= 0) {
        return;
    }
    // TODO: QSoundEffect：攻击
}

void GameAudio::playCollect()
{
    if (sfxPercent() <= 0) {
        return;
    }
    // TODO: QSoundEffect：吸取小球
}
