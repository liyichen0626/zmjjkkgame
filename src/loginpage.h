#pragma once

#include <QWidget>

class LoginPage : public QWidget
{
    Q_OBJECT
public:
    explicit LoginPage(QWidget *parent = nullptr);

signals:
    void startGameRequested();
    void quitRequested();

private:
    void openSettings();
    void openRules();
};
