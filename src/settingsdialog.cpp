#include "settingsdialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("设置"));
    resize(420, 260);

    auto *hint = new QLabel(
        tr("音效包含：受伤、攻击、吸取小球（后期替换为真实音频文件即可生效）。"),
        this);
    hint->setWordWrap(true);

    m_bgmSlider = new QSlider(Qt::Horizontal, this);
    m_bgmSlider->setRange(0, 100);
    m_sfxSlider = new QSlider(Qt::Horizontal, this);
    m_sfxSlider->setRange(0, 100);

    m_bgmLabel = new QLabel(this);
    m_sfxLabel = new QLabel(this);

    auto updateBgmText = [this](int v) { m_bgmLabel->setText(tr("背景音乐: %1%").arg(v)); };
    auto updateSfxText = [this](int v) { m_sfxLabel->setText(tr("音效总音量: %1%").arg(v)); };
    connect(m_bgmSlider, &QSlider::valueChanged, this, updateBgmText);
    connect(m_sfxSlider, &QSlider::valueChanged, this, updateSfxText);
    updateBgmText(m_bgmSlider->value());
    updateSfxText(m_sfxSlider->value());

    auto *form = new QFormLayout();
    form->addRow(m_bgmLabel, m_bgmSlider);
    form->addRow(m_sfxLabel, m_sfxSlider);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(hint);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

int SettingsDialog::bgmVolume() const
{
    return m_bgmSlider->value();
}

int SettingsDialog::sfxVolume() const
{
    return m_sfxSlider->value();
}

void SettingsDialog::setBgmVolume(int value)
{
    m_bgmSlider->setValue(value);
}

void SettingsDialog::setSfxVolume(int value)
{
    m_sfxSlider->setValue(value);
}
