#include "mainwindow.h"

#include "gamepage.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_gamePage(new GamePage(this))
{
    setWindowTitle(QStringLiteral("Qt Game"));
    resize(1280, 720);
    setCentralWidget(m_gamePage);
    m_gamePage->startNewGame();
}
