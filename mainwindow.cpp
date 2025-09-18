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

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>

#include <QInputDialog>

#include <QVBoxLayout>
#include <QLabel>
#include <QFileSystemModel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    ui->setupUi(this);
    new LuaSyntaxHighlighter(ui->plainTextEdit->document());
    new ConsoleSyntaxHighlight(ui->console->document());

    AppendConsole("Startup");
    AppendConsole("Will try to load previous data");

    QFile settingsFile("settings.txt");
    if (settingsFile.exists() && settingsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString rootFolder = QString::fromUtf8(settingsFile.readLine()).trimmed();
        QString listFolder = QString::fromUtf8(settingsFile.readLine()).trimmed();

        tfRootFolder = rootFolder;
        listViewLoadedFolder = listFolder;

        LoadListFolder();

        settingsFile.close();

        AppendConsoleWithColor("Data loaded successfully", QColor(128, 255, 0));
    }

    if (tfRootFolder.isEmpty()) {
        AppendConsole("User has no previous data saved, will ask for root folder");

        QDialog *dialog = new QDialog(this);
        dialog->setWindowTitle("First Startup");
        QVBoxLayout *layout = new QVBoxLayout(dialog);
        layout->addWidget(new QLabel("Please select your TF2's root folder"));
        layout->addWidget(new QLabel("(The folder that has the file tf_win64.exe)"));

        QPushButton *button = new QPushButton("Select Folder", dialog);

        connect(button, &QPushButton::clicked, this, [this, dialog](void) {
            QString filePath = QFileDialog::getExistingDirectory(this, tr("Select TF2's Root Folder"));
            if (!filePath.isEmpty()) {
                tfRootFolder = filePath;
                dialog->close();
            }
        });

        layout->addWidget(button);

        dialog->setLayout(layout);
        dialog->exec();
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
                        AppendConsole(lastLine);
                    }
                    file.close();
                }

                // QFileSystemWatcher sometimes stops after a change, re-add the file
                if (!watcher1->files().contains(consolePath)) {
                    watcher1->addPath(consolePath);
                }
            });


    AppendConsoleWithColor("Ready!", QColor(0, 255, 174));
    AppendConsoleWithColor("You can run code now :)", QColor(255, 255, 255));
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

    delete ui;
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
    AppendConsole("Running code...");

    if (tfRootFolder.isEmpty()) {
        QMessageBox::critical(this, "Error!", tr("You have to select a root folder!\nFile -> Set TF2 Root Folder"));
        AppendConsole("Error! You didn't select a root folder!");
        return;
    }

    // Create the Scripts directory if it doesn't exist
    QString scriptsDir = tfRootFolder + "/Executor/Scripts";
    QDir dir;
    if (!dir.exists(scriptsDir)) {
        if (!dir.mkpath(scriptsDir)) {
            QMessageBox::critical(this, "Error!", tr("Failed to create Scripts directory!"));
            AppendConsole("Error! Failed to create Scripts directory!");
            return;
        }
    }

    // Find the next available script number
    int scriptNumber = 0;
    QString fileName;
    do {
        fileName = QString("%1/script-%2.lua").arg(scriptsDir).arg(scriptNumber);
        scriptNumber++;
    } while (QFile::exists(fileName));

    // Create and write to the new script file
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << Lua::Env << "\n" << text;
        file.close();

        AppendConsole(QString("Code executed!"));
    } else {
        QMessageBox::critical(this, "Error!", tr("Failed to create script file!"));
        AppendConsole("Error! Failed to create script file!");
    }
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

void MainWindow::AppendConsoleWithColor(const QString &text, QColor color) {
    QTextCharFormat format;
    format.setForeground(color);

    QTextCursor cursor(ui->console->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText("\n" + text, format);
}

void MainWindow::AppendConsole(const QString &text) {
    ui->console->append(text);
}
