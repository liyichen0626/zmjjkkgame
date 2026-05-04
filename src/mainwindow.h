#pragma once

#include <QMainWindow>

class GamePage;
class LoginPage;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    QStackedWidget *m_stack = nullptr;
    LoginPage *m_login = nullptr;
    GamePage *m_game = nullptr;
};
