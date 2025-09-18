#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void LoadListFolder();
    void Execute(const QString &text);
    void GetMenuInt(const QString &text, std::function<void(int)> callback);
    void GetMenuString(const QString &text, std::function<void(QString)> callback);

private slots:
    void on_execBtn_clicked();
    void on_clearBtn_clicked();
    void on_actionLoad_File_triggered();
    void on_actionLoad_Folder_triggered();
    void on_listView_clicked(const QModelIndex &index);
    void on_actionSet_TF2_Root_Folder_triggered();
    void on_saveBtn_clicked();

private:
    Ui::MainWindow *ui;
    QString tfRootFolder = "";
    QString listViewLoadedFolder = "";
};
#endif // MAINWINDOW_H
