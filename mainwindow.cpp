#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "luasyntaxhighlight.h"
#include "consolesyntaxhighlight.h"
#include "luascripts.h"

#include <QFileDialog>
#include <QStandardItem>
#include <QMessageBox>
#include <QTimer>
#include <QThread>
#include <QFileSystemWatcher>
#include <QtNetwork/QNetworkReply>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>

#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    setWindowFlag(Qt::WindowStaysOnTopHint, 1);
    ui->setupUi(this);
    new LuaSyntaxHighlighter(ui->plainTextEdit->document());
    new ConsoleSyntaxHighlight(ui->consoleTextEdit->document());

    QFile settingsFile("settings.txt");
    if (settingsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString rootFolder = QString::fromUtf8(settingsFile.readLine()).trimmed();
        QString listFolder = QString::fromUtf8(settingsFile.readLine()).trimmed();

        tfRootFolder = rootFolder;
        listViewLoadedFolder = listFolder;

        LoadListFolder();

        settingsFile.close();
    } else {
        qDebug() << "Couldn't open settings.txt!";
    }

    QString filePath = tfRootFolder + "/Executor/console.txt";
    QString debugFilePath = tfRootFolder + "/Executor/debug.txt";

    QFileSystemWatcher *watcher1 = new QFileSystemWatcher(this);
    QFileSystemWatcher *watcher2 = new QFileSystemWatcher(this);
    watcher1->addPath(debugFilePath);
    watcher2->addPath(filePath);

    connect(watcher1, &QFileSystemWatcher::fileChanged,
            this, [this, debugFilePath, watcher1]() {
                QFile file(debugFilePath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);

                    // Clear previous text first
                    ui->plainTextEdit_2->clear();

                    // Append line by line
                    while (!in.atEnd()) {
                        QString line = in.readLine();
                        ui->plainTextEdit_2->appendPlainText(line);
                    }

                    file.close();
                }

                // QFileSystemWatcher sometimes stops after a change, re-add the file
                if (!watcher1->files().contains(debugFilePath))
                    watcher1->addPath(debugFilePath);
            });

    connect(watcher2, &QFileSystemWatcher::fileChanged,
            this, [this, filePath, watcher2]() {
                QFile file(filePath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);

                    // Clear previous text first
                    ui->consoleTextEdit->clear();

                    // Append line by line
                    // This makes the file, when updated, go to the last line instead of the first one
                    while (!in.atEnd()) {
                        QString line = in.readLine();
                        ui->consoleTextEdit->append(line);
                    }

                    file.close();
                }

                // QFileSystemWatcher sometimes stops after a change, re-add the file
                if (!watcher2->files().contains(filePath))
                    watcher2->addPath(filePath);
            });

    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply) {
        if (reply->error()) {
            qDebug() << reply->errorString();
            return;
        }

        QByteArray byteArray = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(byteArray, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << parseError.errorString();
        } else {
            if (doc.isArray()) {
                QJsonArray arr = doc.array();

                // Get the scroll area contents widget
                QWidget *container = ui->scrollAreaWidgetContents;
                QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(container->layout());
                if (!layout) {
                    layout = new QVBoxLayout(container);
                    container->setLayout(layout);
                }

                // Clear any previous buttons if necessary
                QLayoutItem *child;
                while ((child = layout->takeAt(0)) != nullptr) {
                    if (child->widget()) {
                        delete child->widget();
                    }
                    delete child;
                }

                for (const QJsonValue &val : std::as_const(arr)) {
                    QJsonObject obj = val.toObject();
                    QString name = obj["name"].toString();
                    QString manifestUrl = obj["download_url"].toString();

                    if (name.endsWith(".json", Qt::CaseInsensitive)) {
                        name.chop(5); // remove ".json"
                    }

                    if (name.isEmpty() || manifestUrl.isEmpty())
                        continue;

                    QPushButton *btn = new QPushButton(name, container);
                    layout->addWidget(btn);

                    connect(btn, &QPushButton::clicked, this, [this, manifestUrl]() {
                        QNetworkRequest manifestRequest;
                        manifestRequest.setUrl(QUrl(manifestUrl));

                        QNetworkReply *manifestReply = manager->get(manifestRequest);

                        connect(manifestReply, &QNetworkReply::finished, this, [this, manifestReply]() {
                            manifestReply->deleteLater();

                            if (manifestReply->error()) {
                                qDebug() << "Manifest request error:" << manifestReply->errorString();
                                return;
                            }

                            QByteArray manifestData = manifestReply->readAll();
                            QJsonParseError parseError;
                            QJsonDocument manifestDoc = QJsonDocument::fromJson(manifestData, &parseError);

                            if (parseError.error != QJsonParseError::NoError) {
                                qWarning() << "Manifest JSON parse error:" << parseError.errorString();
                                return;
                            }

                            if (manifestDoc.isObject()) {
                                QJsonObject manifestObj = manifestDoc.object();
                                QString luaUrl = manifestObj["url"].toString();
                                if (!luaUrl.isEmpty()) {
                                    QString luaCode = QString("load(http.Get(\"%1\"))()").arg(luaUrl);
                                    Execute(luaCode);
                                } else {
                                    qWarning() << "No 'url' field in manifest JSON";
                                }
                            }
                        });
                    });
                }

                layout->addStretch(); // push everything to top
            }
        }
    });

    request.setUrl(QUrl("https://api.github.com/repos/uosq/cheese-bread-pkgs/contents/packages"));
    manager->get(request);
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
    if (console.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&console);
        out << "";
        console.close();
    }

    QFile debug(tfRootFolder + "/Executor/debug.txt");
    if(debug.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&debug);
        out << "";
        debug.close();
    }

    delete ui;
    delete manager;
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
    QString fileName = index.data(Qt::DisplayRole).toString();

    QString fullPath = listViewLoadedFolder + "/" + fileName;

    QFile file(fullPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
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
    tfRootFolder = folderPath;
}

void MainWindow::on_saveBtn_clicked() {
    QString filePath = QFileDialog::getSaveFileName(this, "Save File", "", "All files (*.*);;Lua files (*.lua)");
    QFile file = QFile(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(ui->plainTextEdit->toPlainText().toStdString().c_str());
        file.close();
    }
}

void MainWindow::Execute(const QString &text) {
    if (tfRootFolder == "" || tfRootFolder.isEmpty()) {
        QMessageBox::critical(this, "Error!", tr("You have to select a root folder!\nFile -> Set TF2 Root Folder"));
        return;
    }

    QFile file = QFile(tfRootFolder + "/Executor/script.lua");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << Lua::Env << "\n" << text;
        file.close();
    }

    //qDebug() << "Executed!";
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
