#pragma once

#include <QDialog>

class QSlider;
class QLabel;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    int bgmVolume() const;
    int sfxVolume() const;
    void setBgmVolume(int value);
    void setSfxVolume(int value);

private:
    QSlider *m_bgmSlider = nullptr;
    QSlider *m_sfxSlider = nullptr;
    QLabel *m_bgmLabel = nullptr;
    QLabel *m_sfxLabel = nullptr;
};
