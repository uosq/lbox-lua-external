#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <functional>

QT_BEGIN_NAMESPACE
class QTcpServer;
class QTcpSocket;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void Execute(const QString &text);
    void GetMenuInt(const QString &text, std::function<void(int)> callback);
    void GetMenuString(const QString &text, std::function<void(QString)> callback);
    void AppendConsole(const QString &text, QColor color = QColor(255, 255, 255));

private slots:
    void on_execBtn_clicked();
    void on_clearBtn_clicked();
    void on_actionLoad_File_triggered();
    void on_actionLoad_Folder_triggered();
    void on_listView_clicked(const QModelIndex &index);
    void on_actionSet_TF2_Root_Folder_triggered();
    void on_saveBtn_clicked();

    // HTTP Server slots
    void handleNewConnection();

private:
    Ui::MainWindow *ui;
    QString tfRootFolder;
    QString listViewLoadedFolder;
    QTcpServer *httpServer;
    QString script;

    // HTTP Server methods
    void setupHttpServer();
    void handleHttpRequest(QTcpSocket *socket);
    void sendMethodNotAllowedResponse(QTcpSocket *socket);

    void sendScriptResponse(QTcpSocket *socket);
    void setRealtimeText(QTcpSocket *socket, const QString &path);
    void runSetAppendConsole(QTcpSocket *socket, const QString &path);

    void LoadListFolder();
};

#endif // MAINWINDOW_H
