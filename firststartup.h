#ifndef FIRSTSTARTUP_H
#define FIRSTSTARTUP_H

#include <QDialog>
#include <QPushButton>
#include <QString>

namespace Ui {
class FirstStartup;
}

class FirstStartup : public QDialog
{
    Q_OBJECT

public:
    explicit FirstStartup(QWidget *parent = nullptr);
    ~FirstStartup();
    QString GetRootFolder();

private slots:
    void on_pushButton_clicked();

private:
    Ui::FirstStartup *ui;
    QString tfRootFolder;
};

#endif // FIRSTSTARTUP_H
