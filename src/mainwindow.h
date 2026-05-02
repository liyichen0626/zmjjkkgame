#pragma once

#include <QMainWindow>

class GamePage;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    GamePage *m_gamePage;
};
