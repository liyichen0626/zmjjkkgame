#include "gamepage.h"

#include "gameaudio.h"

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
constexpr int kObstacleIntervalMs = 2200;
constexpr qreal kObstacleSpeed = 10.5;
constexpr int kOrbSpawnIntervalMs = 1400;
constexpr int kHealOrbIntervalMs = 9000;
constexpr int kCollectNeedMs = 700;
constexpr int kSkillManaCost = 40;
constexpr int kSkillCooldownMs = 2200;
constexpr int kAttackCooldownMs = 260;
constexpr int kBossAttackIntervalMs = 1000;
constexpr int kMaxOrbsOnScreen = 2;
constexpr qreal kTrapClearance = 72.0;
constexpr int kHealAmount = 2;

constexpr qreal kRangeSword = 185.0;
constexpr qreal kRangeStaff = 340.0;
constexpr qreal kRangeBow = 540.0;
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
    m_level2Walls = {
        QRectF(600.0, 50.0, 36.0, 100.0),
        QRectF(600.0, 200.0, 36.0, 100.0),
        QRectF(600.0, 350.0, 36.0, 100.0),
        QRectF(600.0, 500.0, 36.0, 100.0)
    };

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
    m_lastObstacleSpawnMs = m_elapsed.elapsed() - kObstacleIntervalMs;
    m_lastOrbSpawnMs = m_elapsed.elapsed() - kOrbSpawnIntervalMs;
    m_lastHealOrbSpawnMs = m_elapsed.elapsed() - kHealOrbIntervalMs;
    m_lastBossAttackMs = 0;
    m_lastPlayerAttackMs = 0;
    m_lastSkillMs = -kSkillCooldownMs;

    m_upPressed = m_downPressed = m_leftPressed = m_rightPressed = false;
    setFocus();
}

void GamePage::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), m_stage == Stage::Level1 ? QColor(22, 30, 42) : QColor(32, 22, 40));

    for (const Trap &trap : m_traps) {
        painter.fillRect(trap.rect, QColor(180, 40, 40));
    }

    if (m_stage == Stage::Level2 || m_stage == Stage::Win || m_stage == Stage::Lose) {
        for (const QRectF &w : m_level2Walls) {
            painter.fillRect(w, QColor(55, 55, 70));
        }
    }

    for (const Obstacle &obstacle : m_obstacles) {
        painter.fillRect(obstacle.rect, QColor(240, 150, 55));
    }

    for (const Orb &orb : m_orbs) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(orb.heal ? QColor(60, 220, 100) : QColor(80, 150, 255));
        painter.drawEllipse(orb.rect);
    }

    for (const Projectile &proj : m_projectiles) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(proj.fromBoss ? QColor(255, 90, 90) : QColor(120, 255, 150));
        painter.drawEllipse(proj.rect);
    }

    if (m_stage == Stage::Level2 || m_stage == Stage::Win || m_stage == Stage::Lose) {
        painter.fillRect(m_bossRect, QColor(155, 40, 190));
        painter.setPen(Qt::white);
        painter.drawText(QRect(860, 28, 260, 20), Qt::AlignCenter,
                         tr("Boss HP: %1/%2").arg(m_bossHp).arg(m_bossMaxHp));
        painter.fillRect(QRectF(860, 50, 260, 12), QColor(50, 50, 50));
        if (m_bossMaxHp > 0) {
            painter.fillRect(QRectF(860, 50, 260.0 * m_bossHp / m_bossMaxHp, 12), QColor(220, 70, 70));
        }
    }

    painter.fillRect(m_player.rect, QColor(90, 190, 255));

    if (m_stage == Stage::Level1) {
        drawTopCollectBar(painter);
    }

    drawBars(painter);

    painter.setPen(Qt::white);
    if (m_stage == Stage::Level1) {
        painter.drawText(20, 170, tr("第一关：收集 %1 / %2（场上最多 2 个目标球）").arg(m_collectedOrbs).arg(m_targetOrbs));
        painter.drawText(20, 194, tr("障碍刷新更慢但更快；吸取进度保留；回血球触碰 +%1 HP").arg(kHealAmount));
    } else if (m_stage == Stage::Level2) {
        painter.drawText(20, 118, tr("第二关：%1 | J 普攻 K 技能 | 射程：剑<杖<弓").arg(weaponName()));
        painter.drawText(20, 142, tr("中场障碍阻挡你与子弹；Boss 弹幕不伤血（练习版）"));
    } else if (m_stage == Stage::Win) {
        painter.drawText(rect(), Qt::AlignCenter, tr("胜利！按 Esc 返回菜单"));
    } else if (m_stage == Stage::Lose) {
        painter.drawText(rect(), Qt::AlignCenter, tr("失败！按 Esc 返回菜单"));
    }
}

void GamePage::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        emit backToMenuRequested();
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
            p.origin = m_player.rect.center();
            p.rect = QRectF(m_player.rect.right(), m_player.rect.center().y() - 5, 12, 10);
            p.velocity = QPointF(640, 0);
            p.damage = m_player.baseDamage;
            p.fromBoss = false;
            p.maxRange = weaponAttackRange();
            m_projectiles.push_back(p);
            GameAudio::playAttack();
        }
        break;
    case Qt::Key_K:
        if (m_stage == Stage::Level2 &&
            m_elapsed.elapsed() - m_lastSkillMs >= kSkillCooldownMs &&
            m_player.mp >= kSkillManaCost) {
            m_lastSkillMs = m_elapsed.elapsed();
            m_player.mp -= kSkillManaCost;
            GameAudio::playAttack();
            const qreal skillRange = weaponAttackRange() * 1.15;
            if (m_weapon == WeaponType::Sword) {
                m_bossHp = std::max(0, m_bossHp - m_player.baseDamage * 3);
            } else if (m_weapon == WeaponType::Bow) {
                for (int i = -1; i <= 1; ++i) {
                    Projectile p;
                    p.origin = m_player.rect.center();
                    p.rect = QRectF(m_player.rect.right(), m_player.rect.center().y() - 4, 11, 9);
                    p.velocity = QPointF(620, i * 150);
                    p.damage = m_player.baseDamage + 2;
                    p.fromBoss = false;
                    p.maxRange = skillRange;
                    m_projectiles.push_back(p);
                }
            } else {
                m_bossHp = std::max(0, m_bossHp - m_player.baseDamage * 2);
                m_player.hp = std::min(m_player.maxHp, m_player.hp + 2);
                Projectile p;
                p.origin = m_player.rect.center();
                p.rect = QRectF(m_player.rect.right(), m_player.rect.center().y() - 6, 14, 14);
                p.velocity = QPointF(520, 0);
                p.damage = m_player.baseDamage + 4;
                p.fromBoss = false;
                p.maxRange = skillRange;
                m_projectiles.push_back(p);
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
    if (m_stage == Stage::Level2) {
        resolvePlayerAgainstWalls();
    }
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

void GamePage::resolvePlayerAgainstWalls()
{
    for (int k = 0; k < 4; ++k) {
        for (const QRectF &wall : m_level2Walls) {
            if (m_player.rect.intersects(wall)) {
                pushPlayerOutOfRect(wall);
            }
        }
    }
}

void GamePage::pushPlayerOutOfRect(const QRectF &wall)
{
    const QRectF &p = m_player.rect;
    const qreal overlapLeft = p.right() - wall.left();
    const qreal overlapRight = wall.right() - p.left();
    const qreal overlapTop = p.bottom() - wall.top();
    const qreal overlapBottom = wall.bottom() - p.top();

    const qreal mLeft = (overlapLeft > 0 && overlapLeft < 1000) ? overlapLeft : 1e9;
    const qreal mRight = (overlapRight > 0 && overlapRight < 1000) ? overlapRight : 1e9;
    const qreal mTop = (overlapTop > 0 && overlapTop < 1000) ? overlapTop : 1e9;
    const qreal mBottom = (overlapBottom > 0 && overlapBottom < 1000) ? overlapBottom : 1e9;

    const qreal best = std::min({mLeft, mRight, mTop, mBottom});
    if (best > 900) {
        return;
    }
    if (best == mLeft) {
        m_player.rect.moveRight(wall.left() - 0.5);
    } else if (best == mRight) {
        m_player.rect.moveLeft(wall.right() + 0.5);
    } else if (best == mTop) {
        m_player.rect.moveBottom(wall.top() - 0.5);
    } else {
        m_player.rect.moveTop(wall.bottom() + 0.5);
    }
}

bool GamePage::isOrbPositionValid(const QRectF &candidate) const
{
    const QRectF grown = candidate.adjusted(-kTrapClearance, -kTrapClearance, kTrapClearance, kTrapClearance);
    for (const Trap &trap : m_traps) {
        if (trap.rect.intersects(grown)) {
            return false;
        }
    }
    for (const Obstacle &o : m_obstacles) {
        if (o.rect.intersects(grown)) {
            return false;
        }
    }
    const QRectF orbPad = candidate.adjusted(-28, -28, 28, 28);
    for (const Orb &o : m_orbs) {
        if (orbPad.intersects(o.rect)) {
            return false;
        }
    }
    return true;
}

void GamePage::updateStage1(double)
{
    if (m_stage != Stage::Level1) {
        return;
    }

    spawnObstacleIfNeeded();
    trySpawnOrbs();

    for (Obstacle &o : m_obstacles) {
        o.rect.translate(o.velocity);
        if (o.rect.intersects(m_player.rect)) {
            applyDamageToPlayer(kDamagePerHit, true);
        }
    }

    m_obstacles.erase(std::remove_if(m_obstacles.begin(), m_obstacles.end(), [](const Obstacle &o) {
        return o.rect.right() < -80 || o.rect.left() > 1380 || o.rect.bottom() < -80 || o.rect.top() > 820;
    }), m_obstacles.end());

    for (const Trap &trap : m_traps) {
        if (trap.rect.intersects(m_player.rect)) {
            applyDamageToPlayer(kDamagePerHit, true);
        }
    }

    for (Orb &orb : m_orbs) {
        if (orb.heal) {
            continue;
        }
        if (orb.rect.intersects(m_player.rect)) {
            orb.collectProgress = std::min(1.0, orb.collectProgress + (16.0 / static_cast<double>(kCollectNeedMs)));
        } else {
            orb.collectProgress = std::max(0.0, orb.collectProgress - 0.018);
        }
    }

    for (int i = m_orbs.size() - 1; i >= 0; --i) {
        Orb &orb = m_orbs[i];
        if (orb.heal && orb.rect.intersects(m_player.rect)) {
            m_player.hp = std::min(m_player.maxHp, m_player.hp + kHealAmount);
            m_orbs.remove(i);
            GameAudio::playCollect();
            continue;
        }
        if (!orb.heal && orb.collectProgress >= 1.0) {
            ++m_collectedOrbs;
            m_orbs.remove(i);
            GameAudio::playCollect();
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
        p.origin = from;
        p.rect = QRectF(from.x() - 6, from.y() - 6, 12, 12);
        p.velocity = dir * 280.0;
        p.fromBoss = true;
        p.damage = kDamagePerHit;
        p.maxRange = 900.0;
        m_projectiles.push_back(p);
    }
}

void GamePage::updateProjectiles(double dt)
{
    for (int i = m_projectiles.size() - 1; i >= 0; --i) {
        Projectile &p = m_projectiles[i];
        p.rect.translate(p.velocity * dt);
        const QPointF c = p.rect.center();
        const qreal dist = std::hypot(c.x() - p.origin.x(), c.y() - p.origin.y());
        if (dist > p.maxRange) {
            m_projectiles.remove(i);
            continue;
        }
        if (p.rect.right() < -40 || p.rect.left() > 1320 || p.rect.bottom() < -40 || p.rect.top() > 760) {
            m_projectiles.remove(i);
            continue;
        }

        if (m_stage == Stage::Level2) {
            bool blocked = false;
            for (const QRectF &w : m_level2Walls) {
                if (p.rect.intersects(w)) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) {
                m_projectiles.remove(i);
                continue;
            }
        }

        if (p.fromBoss && p.rect.intersects(m_player.rect)) {
            applyDamageToPlayer(p.damage, true);
            m_projectiles.remove(i);
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

    Obstacle o;
    if (QRandomGenerator::global()->bounded(2) == 0) {
        o.horizontal = true;
        const bool l2r = QRandomGenerator::global()->bounded(2) == 0;
        o.rect = QRectF(l2r ? -70.0 : 1310.0, QRandomGenerator::global()->bounded(24, 660), 56, 22);
        o.velocity = QPointF(l2r ? kObstacleSpeed : -kObstacleSpeed, 0.0);
    } else {
        o.horizontal = false;
        const bool t2b = QRandomGenerator::global()->bounded(2) == 0;
        o.rect = QRectF(QRandomGenerator::global()->bounded(24, 1220), t2b ? -70.0 : 750.0, 22, 56);
        o.velocity = QPointF(0.0, t2b ? kObstacleSpeed : -kObstacleSpeed);
    }
    m_obstacles.push_back(o);

    static int lane = 0;
    const int laneY[4] = {130, 270, 410, 560};
    Obstacle fixed;
    fixed.horizontal = true;
    fixed.rect = QRectF(-60.0, laneY[lane], 58, 22);
    fixed.velocity = QPointF(kObstacleSpeed * 0.95, 0.0);
    lane = (lane + 1) % 4;
    m_obstacles.push_back(fixed);
}

void GamePage::trySpawnOrbs()
{
    const qint64 now = m_elapsed.elapsed();

    auto tryPlaceOrb = [&](bool heal) -> bool {
        if (m_orbs.size() >= kMaxOrbsOnScreen) {
            return false;
        }
        for (int attempt = 0; attempt < 55; ++attempt) {
            const int w = heal ? 26 : 24;
            QRectF cand(QRandomGenerator::global()->bounded(40, 1240 - w),
                        QRandomGenerator::global()->bounded(40, 680 - w), w, w);
            if (!isOrbPositionValid(cand)) {
                continue;
            }
            Orb orb;
            orb.heal = heal;
            orb.rect = cand;
            orb.collectProgress = 0.0;
            m_orbs.push_back(orb);
            return true;
        }
        return false;
    };

    if (now - m_lastOrbSpawnMs >= kOrbSpawnIntervalMs) {
        m_lastOrbSpawnMs = now;
        tryPlaceOrb(false);
    }
    if (now - m_lastHealOrbSpawnMs >= kHealOrbIntervalMs) {
        m_lastHealOrbSpawnMs = now;
        if (QRandomGenerator::global()->bounded(100) < 40) {
            tryPlaceOrb(true);
        }
    }
}

void GamePage::applyDamageToPlayer(int damage, bool playSound)
{
    const qint64 now = m_elapsed.elapsed();
    if (now < m_player.invincibleUntilMs) {
        return;
    }
    m_player.hp = std::max(0, m_player.hp - damage);
    m_player.invincibleUntilMs = now + 420;
    if (playSound && damage > 0) {
        GameAudio::playHurt();
    }
}

void GamePage::drawTopCollectBar(QPainter &painter)
{
    qreal maxProg = 0.0;
    for (const Orb &orb : m_orbs) {
        if (!orb.heal) {
            maxProg = std::max(maxProg, orb.collectProgress);
        }
    }

    const int barH = 7;
    painter.setPen(Qt::NoPen);
    painter.fillRect(0, 0, width(), barH, QColor(15, 15, 20));
    painter.fillRect(0, 0, static_cast<int>(width() * maxProg), barH, QColor(80, 220, 140));
}

bool GamePage::chooseWeapon()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("选择武器"));
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(tr("血量继承第一关；武器影响伤害、射程（剑<杖<弓）"), &dialog));

    auto *sword = new QPushButton(tr("剑：伤害 16，射程最短"), &dialog);
    auto *bow = new QPushButton(tr("弓：伤害 12，射程最远"), &dialog);
    auto *staff = new QPushButton(tr("法杖：伤害 10，射程中等"), &dialog);
    layout->addWidget(sword);
    layout->addWidget(staff);
    layout->addWidget(bow);

    connect(sword, &QPushButton::clicked, &dialog, [this, &dialog]() {
        m_weapon = WeaponType::Sword;
        m_player.baseDamage = 16;
        m_player.maxHp += 8;
        m_player.hp = std::min(m_player.maxHp, m_player.hp + 8);
        dialog.accept();
    });
    connect(staff, &QPushButton::clicked, &dialog, [this, &dialog]() {
        m_weapon = WeaponType::Staff;
        m_player.baseDamage = 10;
        m_player.maxHp += 10;
        m_player.hp = std::min(m_player.maxHp, m_player.hp + 10);
        dialog.accept();
    });
    connect(bow, &QPushButton::clicked, &dialog, [this, &dialog]() {
        m_weapon = WeaponType::Bow;
        m_player.baseDamage = 12;
        m_player.maxHp += 5;
        m_player.hp = std::min(m_player.maxHp, m_player.hp + 5);
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

qreal GamePage::weaponAttackRange() const
{
    switch (m_weapon) {
    case WeaponType::Sword: return kRangeSword;
    case WeaponType::Staff: return kRangeStaff;
    case WeaponType::Bow: return kRangeBow;
    }
    return kRangeStaff;
}

void GamePage::drawBars(QPainter &painter)
{
    painter.setBrush(QColor(0, 0, 0, 130));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(12, 88, 280, 74, 8, 8);

    painter.setBrush(QColor(60, 60, 60));
    painter.drawRect(20, 100, 220, 18);
    painter.setBrush(QColor(220, 50, 50));
    painter.drawRect(20, 100, static_cast<int>(220.0 * m_player.hp / m_player.maxHp), 18);
    painter.setPen(Qt::white);
    painter.drawText(QRect(20, 100, 220, 18), Qt::AlignCenter, tr("HP %1/%2").arg(m_player.hp).arg(m_player.maxHp));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(60, 60, 60));
    painter.drawRect(20, 128, 220, 18);
    painter.setBrush(QColor(60, 130, 255));
    painter.drawRect(20, 128, static_cast<int>(220.0 * m_player.mp / m_player.maxMp), 18);
    painter.setPen(Qt::white);
    painter.drawText(QRect(20, 128, 220, 18), Qt::AlignCenter, tr("MP %1/%2").arg(m_player.mp).arg(m_player.maxMp));
}

void GamePage::enterLevel2()
{
    m_timer.stop();

    if (!chooseWeapon()) {
        m_stage = Stage::Lose;
        m_timer.start(16);
        return;
    }
    m_stage = Stage::Level2;
    m_player.mp = m_player.maxMp;
    m_player.rect.moveTo(80, 330);
    m_obstacles.clear();
    m_orbs.clear();
    m_projectiles.clear();

    m_timer.start(16);
}
