#include "loginpage.h"

#include "settingsdialog.h"

#include <QFont>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

LoginPage::LoginPage(QWidget *parent) : QWidget(parent)
{
    auto *title = new QLabel(tr("康康历险记"), this);
    QFont f = title->font();
    f.setPointSize(22);
    f.setBold(true);
    title->setFont(f);
    title->setAlignment(Qt::AlignCenter);

    auto *startBtn = new QPushButton(tr("开始游戏"), this);
    auto *settingsBtn = new QPushButton(tr("设置"), this);
    auto *rulesBtn = new QPushButton(tr("游戏规则"), this);
    auto *exitBtn = new QPushButton(tr("退出"), this);

    auto *layout = new QVBoxLayout(this);
    layout->addStretch(1);
    layout->addWidget(title);
    layout->addSpacing(24);
    layout->addWidget(startBtn);
    layout->addWidget(settingsBtn);
    layout->addWidget(rulesBtn);
    layout->addWidget(exitBtn);
    layout->addStretch(2);

    connect(startBtn, &QPushButton::clicked, this, &LoginPage::startGameRequested);
    connect(settingsBtn, &QPushButton::clicked, this, &LoginPage::openSettings);
    connect(rulesBtn, &QPushButton::clicked, this, &LoginPage::openRules);
    connect(exitBtn, &QPushButton::clicked, this, &LoginPage::quitRequested);
}

void LoginPage::openSettings()
{
    QSettings settings;
    SettingsDialog dlg(this);
    dlg.setBgmVolume(settings.value("audio/bgm", 70).toInt());
    dlg.setSfxVolume(settings.value("audio/sfx", 70).toInt());
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    settings.setValue("audio/bgm", dlg.bgmVolume());
    settings.setValue("audio/sfx", dlg.sfxVolume());
}

void LoginPage::openRules()
{
    const QString text =
        tr(
            "【第一关】\n"
            "· 使用 WASD 移动，躲避横向与纵向飞来的障碍与地图陷阱；碰到扣 3 点生命。\n"
            "· 场上最多同时存在两个目标小球；需贴近并停留一段时间才能完成吸取；离开小球后吸取进度会保留。\n"
            "· 回血球无进度条，碰到立刻回复 2 点生命。\n"
            "· 收集够指定数量的小球后进入武器选择。\n\n"
            "【第二关】\n"
            "· 你与 Boss 之间有阻挡障碍：你和子弹都无法穿过。\n"
            "· 三种武器攻击射程不同：剑最短，法杖中等，弓最远。\n"
            "· Boss 采用追踪弹幕模式；当前版本 Boss 不对角色造成伤害（便于练走位与输出）。\n"
            "· J 普通攻击，K 技能（消耗 MP）。\n");

    QMessageBox::information(this, tr("游戏规则"), text);
}
