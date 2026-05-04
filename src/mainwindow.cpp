#include "mainwindow.h"

#include "gamepage.h"
#include "loginpage.h"

#include <QApplication>
#include <QStackedWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_stack(new QStackedWidget(this)),
      m_login(new LoginPage(this)),
      m_game(new GamePage(this))
{
    m_stack->addWidget(m_login);
    m_stack->addWidget(m_game);
    setCentralWidget(m_stack);

    setWindowTitle(tr("康康历险记"));
    resize(1280, 720);

    connect(m_login, &LoginPage::startGameRequested, this, [this]() {
        m_game->startNewGame();
        m_stack->setCurrentWidget(m_game);
        m_game->setFocus(Qt::OtherFocusReason);
    });

    connect(m_login, &LoginPage::quitRequested, qApp, &QApplication::quit);

    connect(m_game, &GamePage::backToMenuRequested, this, [this]() {
        m_stack->setCurrentWidget(m_login);
    });
}
