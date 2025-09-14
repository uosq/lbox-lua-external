#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
    QString tfRootFolder = "";
    QString listViewLoadedFolder = "";

private slots:
    void on_execBtn_clicked();
    void on_clearBtn_clicked();
    void on_actionLoad_File_triggered();
    void on_actionLoad_Folder_triggered();
    void on_listView_clicked(const QModelIndex &index);
    void on_actionSet_TF2_Root_Folder_triggered();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
