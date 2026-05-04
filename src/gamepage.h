#pragma once

#include <QElapsedTimer>
#include <QTimer>
#include <QVector>
#include <QWidget>

class QPainter;

class GamePage : public QWidget
{
    Q_OBJECT
public:
    explicit GamePage(QWidget *parent = nullptr);
    void startNewGame();

signals:
    void backToMenuRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    enum class Stage {
        Level1,
        Level2,
        Win,
        Lose
    };

    enum class WeaponType {
        Sword,
        Bow,
        Staff
    };

    struct PlayerState {
        QRectF rect;
        int hp = 25;
        int maxHp = 25;
        int mp = 0;
        int maxMp = 100;
        int baseDamage = 10;
        qint64 invincibleUntilMs = 0;
    };

    struct Obstacle {
        QRectF rect;
        QPointF velocity;
        bool horizontal = true;
    };

    struct Trap {
        QRectF rect;
    };

    struct Orb {
        QRectF rect;
        bool heal = false;
        qreal collectProgress = 0.0;
    };

    struct Projectile {
        QRectF rect;
        QPointF velocity;
        QPointF origin;
        qreal maxRange = 400.0;
        int damage = 0;
        bool fromBoss = false;
    };

    void updateFrame();
    void updateMovement();
    void updateStage1(double dt);
    void updateStage2();
    void updateProjectiles(double dt);
    void spawnObstacleIfNeeded();
    void trySpawnOrbs();
    void applyDamageToPlayer(int damage, bool playSound);
    bool chooseWeapon();
    QString weaponName() const;
    qreal weaponAttackRange() const;
    void drawTopCollectBar(QPainter &painter);
    void drawBars(QPainter &painter);
    void enterLevel2();
    void resolvePlayerAgainstWalls();
    bool isOrbPositionValid(const QRectF &candidate) const;
    void pushPlayerOutOfRect(const QRectF &wall);

    QTimer m_timer;
    QElapsedTimer m_elapsed;
    qint64 m_lastFrameMs = 0;
    qint64 m_lastObstacleSpawnMs = 0;
    qint64 m_lastOrbSpawnMs = 0;
    qint64 m_lastHealOrbSpawnMs = 0;
    qint64 m_lastBossAttackMs = 0;
    qint64 m_lastPlayerAttackMs = 0;
    qint64 m_lastSkillMs = 0;

    Stage m_stage = Stage::Level1;
    WeaponType m_weapon = WeaponType::Sword;
    PlayerState m_player;

    QVector<Obstacle> m_obstacles;
    QVector<Trap> m_traps;
    QVector<Orb> m_orbs;
    QVector<Projectile> m_projectiles;
    QVector<QRectF> m_level2Walls;

    int m_collectedOrbs = 0;
    int m_targetOrbs = 10;
    int m_bossHp = 200;
    int m_bossMaxHp = 200;
    QRectF m_bossRect;

    bool m_upPressed = false;
    bool m_downPressed = false;
    bool m_leftPressed = false;
    bool m_rightPressed = false;
};
