#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "consolesyntaxhighlight.h"
#include "luasyntaxhighlight.h"
#include "luascripts.h"

#include <QFileDialog>
#include <QStandardItem>
#include <QMessageBox>
#include <QTimer>
#include <QThread>
#include <QFileSystemWatcher>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>

#include <QInputDialog>

#include <QVBoxLayout>
#include <QLabel>
#include <QFileSystemModel>

#include "firststartup.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , httpServer(new QTcpServer(this))
{
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    ui->setupUi(this);
    new LuaSyntaxHighlighter(ui->plainTextEdit->document());
    new ConsoleSyntaxHighlight(ui->console->document());

    AppendConsole("Startup\n", QColor(100, 100, 255));
    AppendConsole("Will try to load previous data\n");

    // Start HTTP server
    setupHttpServer();

    QFile settingsFile("settings.txt");
    if (settingsFile.exists() && settingsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString rootFolder = QString::fromUtf8(settingsFile.readLine()).trimmed();
        QString listFolder = QString::fromUtf8(settingsFile.readLine()).trimmed();

        tfRootFolder = rootFolder;
        listViewLoadedFolder = listFolder;

        LoadListFolder();

        settingsFile.close();

        AppendConsole("Data loaded successfully\n", QColor(128, 255, 0));
    }

    if (tfRootFolder.isEmpty()) {
        AppendConsole("User has no previous data saved", QColor(220, 20, 60));
        AppendConsole(", will ask for root folder\n", QColor(255, 255, 255));

        FirstStartup startup;
        startup.exec();
        tfRootFolder = startup.GetRootFolder();
        qDebug() << tfRootFolder;
    }

    QFileSystemWatcher *watcher1 = new QFileSystemWatcher(this);
    QString consolePath = tfRootFolder + "/Executor/console.txt";
    watcher1->addPath(consolePath);
    connect(watcher1, &QFileSystemWatcher::fileChanged,
            this, [this, consolePath, watcher1]() {
                // Read only the last line of the file
                QFile file(consolePath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);
                    QString lastLine;
                    QString currentLine;

                    // Read all lines and keep track of the last non-empty one
                    while (!in.atEnd()) {
                        currentLine = in.readLine();
                        if (!currentLine.isEmpty()) {
                            lastLine = currentLine;
                        }
                    }

                    if (!lastLine.isEmpty()) {
                        AppendConsole(lastLine + "\n");
                    }
                    file.close();
                }

                // QFileSystemWatcher sometimes stops after a change, re-add the file
                if (!watcher1->files().contains(consolePath)) {
                    watcher1->addPath(consolePath);
                }
            });

    // watch realtime.txt
    QFileSystemWatcher *watcher2 = new QFileSystemWatcher(this);
    QString realtimePath = tfRootFolder + "/Executor/realtime.txt";
    watcher2->addPath(realtimePath);
    connect(watcher2, &QFileSystemWatcher::fileChanged,
            this, [this, realtimePath, watcher2]() {
                // Read only the last line of the file
                QFile file(realtimePath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);
                    ui->realTimeTextEdit->setPlainText(in.readAll());
                    file.close();
                }

                // QFileSystemWatcher sometimes stops after a change, re-add the file
                if (!watcher2->files().contains(realtimePath)) {
                    watcher2->addPath(realtimePath);
                }
            });

    AppendConsole("Ready!\n", QColor(0, 255, 174));
}

MainWindow::~MainWindow()
{
    QFile settingsFile("settings.txt");
    if (settingsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&settingsFile);
        out << tfRootFolder << "\n";
        out << listViewLoadedFolder;
        settingsFile.close();
    }

    QFile console(tfRootFolder + "/Executor/console.txt");
    if (console.exists() && console.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&console);
        out << "";
        console.close();
    }

    delete ui;
}

void MainWindow::setupHttpServer()
{
    const quint16 port = 8080;

    if (!httpServer->listen(QHostAddress::LocalHost, port)) {
        AppendConsole(QString("Failed to start HTTP server on port %1: %2\n")
                          .arg(port).arg(httpServer->errorString()), QColor(255, 0, 0));
        return;
    }

    connect(httpServer, &QTcpServer::newConnection, this, &MainWindow::handleNewConnection);

    //AppendConsole(QString("HTTP server started on http://localhost:%1\n").arg(port), QColor(0, 255, 0));
}

void MainWindow::handleNewConnection()
{
    QTcpSocket *clientSocket = httpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, [this, clientSocket]() {
        handleHttpRequest(clientSocket);
    });
    connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);
}

void MainWindow::handleHttpRequest(QTcpSocket *socket)
{
    QByteArray requestData = socket->readAll();
    QString request = QString::fromUtf8(requestData);

    // Parse the HTTP request
    QStringList lines = request.split("\r\n");
    if (lines.isEmpty()) {
        socket->close();
        return;
    }

    QString requestLine = lines.first();
    QStringList requestParts = requestLine.split(" ");

    if (requestParts.size() < 2) {
        socket->close();
        return;
    }

    QString method = requestParts[0];
    QString path = requestParts[1];

    //AppendConsole(QString("HTTP %1 %2\n").arg(method, path), QColor(200, 200, 200));

    // Only handle GET requests, return current script text
    // GET -> /getcurrentscript -> return script text
    // GET -> /setrealtime=cooltextwewanttodisplay -> set realtimeTextEdit's plain text to everything after /setrealtime=
    // GET -> /appendconsole=cooltexttodisplay -> set console's plain text to everything after /setconsole
    if (method == "GET") {
        if (path.startsWith("/getcurrentscript"))
            sendScriptResponse(socket);
        else if (path.startsWith("/setrealtime=")) {
            QString encodedText = path.mid(QString("/setrealtime=").length());

            // make spaces, newlines, etc work fine
            QByteArray byteArray = QByteArray::fromPercentEncoding(encodedText.toUtf8());

            // Update the QTextEdit
            ui->realTimeTextEdit->setPlainText(QString::fromUtf8(byteArray));

            QByteArray response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain; charset=utf-8\r\n"
                "Content-Length: 2\r\n"
                "\r\n"
                "OK";

            socket->write(response);
            socket->flush();
            socket->close();
        } else if (path.startsWith("/appendconsole=")) {
            QString encodedText = path.mid(QString("/appendconsole=").length());

            // make spaces, newlines, etc work fine
            QByteArray byteArray = QByteArray::fromPercentEncoding(encodedText.toUtf8());

            // Update the console
            AppendConsole(byteArray + "\n");

            QByteArray response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain; charset=utf-8\r\n"
                "Content-Length: 2\r\n"
                "\r\n"
                "OK";

            socket->write(response);
            socket->flush();
            socket->close();
        } else if (path.startsWith("/setcallbacklist=")) {
            QByteArray response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain; charset=utf-8\r\n"
                "Content-Length: 2\r\n"
                "\r\n"
                "OK";

            socket->write(response);
            socket->flush();
            socket->close();

            QString encodedJson = path.mid(QString("/setcallbacklist=").length());

            // Decode percent encoding
            QByteArray byteArray = QByteArray::fromPercentEncoding(encodedJson.toUtf8());
            QString jsonStr = QString::fromUtf8(byteArray);

            // Parse JSON
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                AppendConsole(QString("Failed to parse callback list JSON: %1\n").arg(parseError.errorString()), QColor(255,0,0));
            } else if (doc.isArray()) {
                QJsonArray arr = doc.array();

                // Access the scroll area contents
                QWidget *container = ui->scrollArea->widget(); // scrollAreaWidgetContents
                if (!container) {
                    container = new QWidget(ui->scrollArea);
                    ui->scrollArea->setWidget(container);
                }

                QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(container->layout());
                if (!layout) {
                    layout = new QVBoxLayout(container);
                    container->setLayout(layout);
                }

                // Clear previous buttons
                QLayoutItem *child;
                while ((child = layout->takeAt(0)) != nullptr) {
                    delete child->widget();
                    delete child;
                }

                // Add new buttons
                for (auto value : std::as_const(arr)) {
                    if (!value.isObject()) continue;

                    QJsonObject obj = value.toObject();
                    QString callbackType = obj["callback"].toString();
                    QString callbackName = obj["name"].toString();

                    QPushButton *btn = new QPushButton("Type: " + callbackType + " | Name: " + callbackName, container);

                    connect(btn, &QPushButton::clicked, this, [this, callbackType, callbackName]() {
                        QString code = QString("callbacks.Unregister('%1', '%2')").arg(callbackType, callbackName);
                        Execute(code);
                    });

                    layout->addWidget(btn);
                }

                // Ensure scroll area updates
                container->adjustSize();
            }
        }
    } else {
        sendMethodNotAllowedResponse(socket);
    }
}

void MainWindow::sendScriptResponse(QTcpSocket *socket)
{
    QByteArray scriptData = script.toUtf8();

    QString response = QString(
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain; charset=utf-8\r\n"
                           "Content-Length: %1\r\n"
                           "\r\n"
                           ).arg(scriptData.length());

    socket->write(response.toUtf8());
    socket->write(scriptData);
    socket->close();

    script.clear();
    //qDebug() << "sent!";
}

void MainWindow::sendMethodNotAllowedResponse(QTcpSocket *socket)
{
    QString response =
        "HTTP/1.1 405 Method Not Allowed\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 18\r\n"
        "\r\n"
        "Method Not Allowed";

    socket->write(response.toUtf8());
    socket->close();
}

void MainWindow::on_execBtn_clicked() {
    Execute(ui->plainTextEdit->toPlainText());
}

void MainWindow::on_clearBtn_clicked() {
    ui->plainTextEdit->setPlainText("");
}

void MainWindow::on_actionLoad_File_triggered() {
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("All Files (*.*);;Lua Files (*.lua);;Text Files (*.txt)"));
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();
            ui->plainTextEdit->setPlainText(content);
        }
    }
}

void MainWindow::LoadListFolder() {
    if (listViewLoadedFolder.isEmpty() || listViewLoadedFolder == "")
        return; // User canceled

    QDir dir(listViewLoadedFolder);

    listViewLoadedFolder = dir.absolutePath();
    //qDebug() << listViewLoadedFolder;

    // Filter for .lua files only
    QStringList filters;
    filters << "*.lua";
    QStringList luaFiles = dir.entryList(filters, QDir::Files);

    // Create a model to populate the list view
    QStandardItemModel* model = new QStandardItemModel(this);

    for (const QString& fileName : std::as_const(luaFiles)) {
        QStandardItem* item = new QStandardItem(fileName);
        model->appendRow(item);
    }

    // Set the model to the QListView
    ui->listView->setModel(model);
}

void MainWindow::on_actionLoad_Folder_triggered() {
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("Select Script Folder"));

    if (folderPath.isEmpty())
        return; // User canceled

    QDir dir(folderPath);

    listViewLoadedFolder = dir.absolutePath();
    //qDebug() << listViewLoadedFolder;

    // Filter for .lua files only
    QStringList filters;
    filters << "*.lua";
    QStringList luaFiles = dir.entryList(filters, QDir::Files);

    // Create a model to populate the list view
    QStandardItemModel* model = new QStandardItemModel(this);

    for (const QString& fileName : std::as_const(luaFiles)) {
        QStandardItem* item = new QStandardItem(fileName);
        model->appendRow(item);
    }

    // Set the model to the QListView
    ui->listView->setModel(model);
}

void MainWindow::on_listView_clicked(const QModelIndex &index) {
    if (listViewLoadedFolder.isEmpty())
        return;

    QString fileName = index.data(Qt::DisplayRole).toString();
    if (fileName.isEmpty())
        return;

    QString fullPath = listViewLoadedFolder + "/" + fileName;

    QFile file(fullPath);

    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        ui->plainTextEdit->setPlainText(content);
        file.close();
    } else {
        qDebug() << "Failed to open file:" << fullPath;
    }
}

void MainWindow::on_actionSet_TF2_Root_Folder_triggered() {
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("Select TF2's Root Folder"));
    if (folderPath.isEmpty())
        return;

    tfRootFolder = folderPath;
}

void MainWindow::on_saveBtn_clicked() {
    QString filePath = QFileDialog::getSaveFileName(this, "Save File", "", "All files (*.*);;Lua files (*.lua)");
    if (filePath.isEmpty())
        return;

    QFile file = QFile(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(ui->plainTextEdit->toPlainText().toStdString().c_str());
        file.close();
    }
}

void MainWindow::Execute(const QString &text) {
    script = text;
}

void MainWindow::GetMenuInt(const QString &text, std::function<void(int)> callback) {
    auto *watcher = new QFileSystemWatcher(this);
    QString retFile = tfRootFolder + "/Executor/returnvalue.txt";

    watcher->addPath(retFile);

    connect(watcher, &QFileSystemWatcher::fileChanged, this,
            [this, watcher, retFile, callback](const QString &) {
                QFile file(retFile);
                int result = -1;
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);
                    result = in.readAll().trimmed().toInt();
                    file.close();
                }
                watcher->deleteLater();
                callback(result);
            });

    Execute(Lua::GetMenuCode.arg(text));
}

void MainWindow::GetMenuString(const QString &text, std::function<void(QString)> callback) {
    auto *watcher = new QFileSystemWatcher(this);
    QString retFile = tfRootFolder + "/Executor/returnvalue.txt";

    watcher->addPath(retFile);

    connect(watcher, &QFileSystemWatcher::fileChanged, this,
            [this, watcher, retFile, callback](const QString &) {
                QFile file(retFile);
                QString result = "";
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);
                    result = in.readAll();
                    file.close();
                }
                watcher->deleteLater();
                callback(result);
            });

    Execute(Lua::GetMenuCode.arg(text));
}

void MainWindow::AppendConsole(const QString &text, QColor color) {
    QTextCharFormat format;
    format.setForeground(color);

    QTextCursor cursor(ui->console->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, format);
}
