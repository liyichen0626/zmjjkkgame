#include "gamepage.h"

#include <QDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QRandomGenerator>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

namespace {
constexpr int kMapWidth = 1280;
constexpr int kMapHeight = 720;
constexpr int kDamagePerHit = 3;
constexpr int kObstacleIntervalMs = 1000;
constexpr int kOrbIntervalMs = 1200;
constexpr int kHealOrbIntervalMs = 5000;
constexpr int kCollectNeedMs = 700;
constexpr int kSkillManaCost = 40;
constexpr int kSkillCooldownMs = 2200;
constexpr int kAttackCooldownMs = 260;
constexpr int kBossAttackIntervalMs = 1000;
}

GamePage::GamePage(QWidget *parent) : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(kMapWidth, kMapHeight);

    connect(&m_timer, &QTimer::timeout, this, &GamePage::updateFrame);
    m_timer.start(16);
    m_elapsed.start();
}

void GamePage::startNewGame()
{
    m_stage = Stage::Level1;
    m_weapon = WeaponType::Sword;
    m_player = PlayerState{};
    m_player.rect = QRectF(80.0, 360.0, 42.0, 42.0);
    m_collectedOrbs = 0;
    m_bossHp = m_bossMaxHp;
    m_bossRect = QRectF(1060.0, 290.0, 100.0, 100.0);

    m_obstacles.clear();
    m_orbs.clear();
    m_projectiles.clear();
    m_traps = {
        {QRectF(300, 200, 40, 40)},
        {QRectF(500, 420, 40, 40)},
        {QRectF(760, 300, 40, 40)},
        {QRectF(980, 500, 40, 40)}
    };

    m_lastFrameMs = m_elapsed.elapsed();
    m_lastObstacleSpawnMs = 0;
    m_lastOrbSpawnMs = 0;
    m_lastHealOrbSpawnMs = 0;
    m_lastBossAttackMs = 0;
    m_lastPlayerAttackMs = 0;
    m_lastSkillMs = -kSkillCooldownMs;

    m_upPressed = m_downPressed = m_leftPressed = m_rightPressed = false;
    setFocus();
}

void GamePage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), m_stage == Stage::Level1 ? QColor(22, 30, 42) : QColor(35, 24, 45));

    for (const Trap &trap : m_traps) {
        painter.fillRect(trap.rect, QColor(180, 40, 40));
    }

    for (const Obstacle &obstacle : m_obstacles) {
        painter.fillRect(obstacle.rect, QColor(240, 150, 55));
    }

    for (const Orb &orb : m_orbs) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(orb.heal ? QColor(70, 220, 90) : QColor(80, 150, 255));
        painter.drawEllipse(orb.rect);
        if (orb.collectProgress > 0.0) {
            painter.setPen(QPen(Qt::white, 2));
            painter.drawArc(orb.rect.adjusted(-3, -3, 3, 3), 90 * 16, static_cast<int>(-360.0 * 16.0 * orb.collectProgress));
        }
    }

    if (m_stage == Stage::Level2 || m_stage == Stage::Win || m_stage == Stage::Lose) {
        painter.fillRect(m_bossRect, QColor(155, 40, 190));
        painter.setPen(Qt::white);
        painter.drawText(QRect(860, 16, 260, 20), Qt::AlignCenter,
                         tr("Boss HP: %1/%2").arg(m_bossHp).arg(m_bossMaxHp));
        painter.fillRect(QRectF(860, 40, 260, 14), QColor(60, 60, 60));
        painter.fillRect(QRectF(860, 40, 260.0 * m_bossHp / m_bossMaxHp, 14), QColor(220, 70, 70));
    }

    for (const Projectile &proj : m_projectiles) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(proj.fromBoss ? QColor(255, 70, 70) : QColor(120, 255, 140));
        painter.drawEllipse(proj.rect);
    }

    painter.fillRect(m_player.rect, QColor(90, 190, 255));
    drawBars(painter);

    painter.setPen(Qt::white);
    if (m_stage == Stage::Level1) {
        painter.drawText(20, 120, tr("第一关: 收集小球 %1 / %2").arg(m_collectedOrbs).arg(m_targetOrbs));
        painter.drawText(20, 145, tr("WASD移动，碰撞障碍/陷阱扣3血，停留0.7秒收集小球"));
    } else if (m_stage == Stage::Level2) {
        painter.drawText(20, 120, tr("第二关: 武器[%1] J普攻 K技能").arg(weaponName()));
    } else if (m_stage == Stage::Win) {
        painter.drawText(rect(), Qt::AlignCenter, tr("胜利！关闭窗口可退出"));
    } else if (m_stage == Stage::Lose) {
        painter.drawText(rect(), Qt::AlignCenter, tr("失败！关闭窗口可退出"));
    }
}

void GamePage::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        return;
    }
    switch (event->key()) {
    case Qt::Key_W: m_upPressed = true; break;
    case Qt::Key_S: m_downPressed = true; break;
    case Qt::Key_A: m_leftPressed = true; break;
    case Qt::Key_D: m_rightPressed = true; break;
    case Qt::Key_J:
        if (m_stage == Stage::Level2 && m_elapsed.elapsed() - m_lastPlayerAttackMs >= kAttackCooldownMs) {
            m_lastPlayerAttackMs = m_elapsed.elapsed();
            Projectile p;
            p.rect = QRectF(m_player.rect.right(), m_player.rect.center().y() - 5, 12, 10);
            p.velocity = QPointF(560, 0);
            p.damage = m_player.baseDamage;
            p.fromBoss = false;
            m_projectiles.push_back(p);
        }
        break;
    case Qt::Key_K:
        if (m_stage == Stage::Level2 &&
            m_elapsed.elapsed() - m_lastSkillMs >= kSkillCooldownMs &&
            m_player.mp >= kSkillManaCost) {
            m_lastSkillMs = m_elapsed.elapsed();
            m_player.mp -= kSkillManaCost;
            if (m_weapon == WeaponType::Sword) {
                m_bossHp = std::max(0, m_bossHp - m_player.baseDamage * 3);
            } else if (m_weapon == WeaponType::Bow) {
                for (int i = -1; i <= 1; ++i) {
                    Projectile p;
                    p.rect = QRectF(m_player.rect.right(), m_player.rect.center().y(), 10, 8);
                    p.velocity = QPointF(580, i * 130);
                    p.damage = m_player.baseDamage + 2;
                    p.fromBoss = false;
                    m_projectiles.push_back(p);
                }
            } else {
                m_bossHp = std::max(0, m_bossHp - m_player.baseDamage * 2);
                m_player.hp = std::min(m_player.maxHp, m_player.hp + 2);
            }
            if (m_bossHp <= 0) {
                m_stage = Stage::Win;
            }
        }
        break;
    default:
        break;
    }
}

void GamePage::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        return;
    }
    switch (event->key()) {
    case Qt::Key_W: m_upPressed = false; break;
    case Qt::Key_S: m_downPressed = false; break;
    case Qt::Key_A: m_leftPressed = false; break;
    case Qt::Key_D: m_rightPressed = false; break;
    default: break;
    }
}

void GamePage::updateFrame()
{
    if (m_stage == Stage::Win || m_stage == Stage::Lose) {
        update();
        return;
    }

    const qint64 now = m_elapsed.elapsed();
    const qint64 deltaMs = std::max<qint64>(1, now - m_lastFrameMs);
    m_lastFrameMs = now;
    const double dt = deltaMs / 1000.0;

    updateMovement();
    updateProjectiles(dt);

    if (m_stage == Stage::Level1) {
        updateStage1(dt);
    } else {
        updateStage2();
    }

    if (m_player.hp <= 0) {
        m_stage = Stage::Lose;
    }
    if (m_bossHp <= 0 && m_stage == Stage::Level2) {
        m_stage = Stage::Win;
    }
    update();
}

void GamePage::updateMovement()
{
    QPointF d(0, 0);
    if (m_upPressed) d.ry() -= 4.5;
    if (m_downPressed) d.ry() += 4.5;
    if (m_leftPressed) d.rx() -= 4.5;
    if (m_rightPressed) d.rx() += 4.5;

    m_player.rect.translate(d);
    m_player.rect.moveLeft(std::clamp(m_player.rect.left(), 0.0, 1280.0 - m_player.rect.width()));
    m_player.rect.moveTop(std::clamp(m_player.rect.top(), 0.0, 720.0 - m_player.rect.height()));
}

void GamePage::updateStage1(double)
{
    spawnObstacleIfNeeded();
    spawnOrbsIfNeeded();

    for (Obstacle &o : m_obstacles) {
        o.rect.translate(o.velocity);
        if (o.rect.intersects(m_player.rect)) {
            applyDamageToPlayer(kDamagePerHit);
        }
    }

    m_obstacles.erase(std::remove_if(m_obstacles.begin(), m_obstacles.end(), [](const Obstacle &o) {
        return o.rect.right() < -70 || o.rect.left() > 1350 || o.rect.bottom() < -70 || o.rect.top() > 790;
    }), m_obstacles.end());

    for (const Trap &trap : m_traps) {
        if (trap.rect.intersects(m_player.rect)) {
            applyDamageToPlayer(kDamagePerHit);
        }
    }

    for (Orb &orb : m_orbs) {
        if (orb.rect.intersects(m_player.rect)) {
            orb.collectProgress = std::min(1.0, orb.collectProgress + (16.0 / kCollectNeedMs));
        } else {
            orb.collectProgress = std::max(0.0, orb.collectProgress - 0.03);
        }
    }

    for (int i = m_orbs.size() - 1; i >= 0; --i) {
        if (m_orbs[i].collectProgress >= 1.0) {
            if (m_orbs[i].heal) {
                m_player.hp = std::min(m_player.maxHp, m_player.hp + 5);
            } else {
                ++m_collectedOrbs;
            }
            m_orbs.remove(i);
        }
    }

    if (m_collectedOrbs >= m_targetOrbs) {
        enterLevel2();
    }
}

void GamePage::updateStage2()
{
    const qint64 now = m_elapsed.elapsed();
    if (now - m_lastBossAttackMs >= kBossAttackIntervalMs) {
        m_lastBossAttackMs = now;
        const QPointF from = m_bossRect.center();
        QPointF dir = m_player.rect.center() - from;
        const qreal len = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
        if (len > 0.01) {
            dir /= len;
        } else {
            dir = QPointF(-1.0, 0.0);
        }
        Projectile p;
        p.rect = QRectF(from.x() - 6, from.y() - 6, 12, 12);
        p.velocity = dir * 260.0;
        p.fromBoss = true;
        p.damage = kDamagePerHit;
        m_projectiles.push_back(p);
    }
}

void GamePage::updateProjectiles(double dt)
{
    for (Projectile &p : m_projectiles) {
        p.rect.translate(p.velocity * dt);
    }

    for (int i = m_projectiles.size() - 1; i >= 0; --i) {
        auto &p = m_projectiles[i];
        if (p.rect.right() < -30 || p.rect.left() > 1310 || p.rect.bottom() < -30 || p.rect.top() > 750) {
            m_projectiles.remove(i);
            continue;
        }
        if (p.fromBoss && p.rect.intersects(m_player.rect)) {
            applyDamageToPlayer(kDamagePerHit);
            m_projectiles.remove(i);
            continue;
        }
        if (!p.fromBoss && p.rect.intersects(m_bossRect)) {
            m_bossHp = std::max(0, m_bossHp - p.damage);
            m_projectiles.remove(i);
        }
    }
}

void GamePage::spawnObstacleIfNeeded()
{
    const qint64 now = m_elapsed.elapsed();
    if (now - m_lastObstacleSpawnMs < kObstacleIntervalMs) {
        return;
    }
    m_lastObstacleSpawnMs = now;

    // Random horizontal or vertical obstacle
    Obstacle o;
    if (QRandomGenerator::global()->bounded(2) == 0) {
        const bool l2r = QRandomGenerator::global()->bounded(2) == 0;
        o.rect = QRectF(l2r ? -60.0 : 1290.0, QRandomGenerator::global()->bounded(20, 680), 50, 20);
        o.velocity = QPointF(l2r ? 7.0 : -7.0, 0.0);
    } else {
        const bool t2b = QRandomGenerator::global()->bounded(2) == 0;
        o.rect = QRectF(QRandomGenerator::global()->bounded(20, 1240), t2b ? -60.0 : 730.0, 20, 50);
        o.velocity = QPointF(0.0, t2b ? 7.0 : -7.0);
    }
    m_obstacles.push_back(o);

    // Fixed obstacle lane every spawn cycle (predictable pattern)
    static int lane = 0;
    const int laneY[4] = {140, 280, 430, 600};
    Obstacle fixed;
    fixed.rect = QRectF(-55.0, laneY[lane], 55, 20);
    fixed.velocity = QPointF(6.0, 0.0);
    lane = (lane + 1) % 4;
    m_obstacles.push_back(fixed);
}

void GamePage::spawnOrbsIfNeeded()
{
    const qint64 now = m_elapsed.elapsed();
    if (now - m_lastOrbSpawnMs >= kOrbIntervalMs) {
        m_lastOrbSpawnMs = now;
        Orb orb;
        orb.heal = false;
        orb.rect = QRectF(QRandomGenerator::global()->bounded(80, 1180),
                          QRandomGenerator::global()->bounded(80, 620), 24, 24);
        m_orbs.push_back(orb);
    }
    if (now - m_lastHealOrbSpawnMs >= kHealOrbIntervalMs) {
        m_lastHealOrbSpawnMs = now;
        if (QRandomGenerator::global()->bounded(100) < 35) {
            Orb orb;
            orb.heal = true;
            orb.rect = QRectF(QRandomGenerator::global()->bounded(120, 1120),
                              QRandomGenerator::global()->bounded(120, 580), 26, 26);
            m_orbs.push_back(orb);
        }
    }
}

void GamePage::applyDamageToPlayer(int damage)
{
    const qint64 now = m_elapsed.elapsed();
    if (now < m_player.invincibleUntilMs) {
        return;
    }
    m_player.hp = std::max(0, m_player.hp - damage);
    m_player.invincibleUntilMs = now + 400;
}

bool GamePage::chooseWeapon()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("选择武器"));
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(tr("选择后进入Boss战：血量会在当前基础上提升"), &dialog));

    auto *sword = new QPushButton(tr("剑: 伤害16, 生命+8"), &dialog);
    auto *bow = new QPushButton(tr("弓: 伤害12, 生命+5"), &dialog);
    auto *staff = new QPushButton(tr("法杖: 伤害10, 生命+10"), &dialog);
    layout->addWidget(sword);
    layout->addWidget(bow);
    layout->addWidget(staff);

    connect(sword, &QPushButton::clicked, &dialog, [this, &dialog]() {
        m_weapon = WeaponType::Sword;
        m_player.baseDamage = 16;
        m_player.maxHp += 8;
        m_player.hp = std::min(m_player.maxHp, m_player.hp + 8);
        dialog.accept();
    });
    connect(bow, &QPushButton::clicked, &dialog, [this, &dialog]() {
        m_weapon = WeaponType::Bow;
        m_player.baseDamage = 12;
        m_player.maxHp += 5;
        m_player.hp = std::min(m_player.maxHp, m_player.hp + 5);
        dialog.accept();
    });
    connect(staff, &QPushButton::clicked, &dialog, [this, &dialog]() {
        m_weapon = WeaponType::Staff;
        m_player.baseDamage = 10;
        m_player.maxHp += 10;
        m_player.hp = std::min(m_player.maxHp, m_player.hp + 10);
        dialog.accept();
    });
    return dialog.exec() == QDialog::Accepted;
}

QString GamePage::weaponName() const
{
    switch (m_weapon) {
    case WeaponType::Sword: return tr("剑");
    case WeaponType::Bow: return tr("弓");
    case WeaponType::Staff: return tr("法杖");
    }
    return tr("未知");
}

void GamePage::drawBars(QPainter &painter)
{
    painter.setBrush(QColor(0, 0, 0, 130));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(12, 12, 280, 74, 8, 8);

    painter.setBrush(QColor(60, 60, 60));
    painter.drawRect(20, 24, 220, 18);
    painter.setBrush(QColor(220, 50, 50));
    painter.drawRect(20, 24, static_cast<int>(220.0 * m_player.hp / m_player.maxHp), 18);
    painter.setPen(Qt::white);
    painter.drawText(QRect(20, 24, 220, 18), Qt::AlignCenter, tr("HP %1/%2").arg(m_player.hp).arg(m_player.maxHp));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(60, 60, 60));
    painter.drawRect(20, 52, 220, 18);
    painter.setBrush(QColor(60, 130, 255));
    painter.drawRect(20, 52, static_cast<int>(220.0 * m_player.mp / m_player.maxMp), 18);
    painter.setPen(Qt::white);
    painter.drawText(QRect(20, 52, 220, 18), Qt::AlignCenter, tr("MP %1/%2").arg(m_player.mp).arg(m_player.maxMp));
}

void GamePage::enterLevel2()
{
    if (!chooseWeapon()) {
        m_stage = Stage::Lose;
        return;
    }
    m_stage = Stage::Level2;
    m_player.mp = m_player.maxMp;
    m_player.rect.moveTo(90, 350);
    m_obstacles.clear();
    m_orbs.clear();
    m_projectiles.clear();
}
