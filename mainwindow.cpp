#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "luasyntaxhighlight.h"
#include "consolesyntaxhighlight.h"
#include <QFileDialog>
#include <QStandardItem>
#include <QMessageBox>
#include <QTimer>
#include <QThread>
#include <QFileSystemWatcher>

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

    QString luaEnv = R"(
local function outputToConsole(...)
    local console <close> = io.open('Executor/console.txt', 'a+')
    if console then
        console:setvbuf('no')
        local args = {...}
        for _, line in pairs (args) do
            console:write(string.format('%s\n', tostring(line)))
        end
        console:flush()
    end
end

local function print(...)
    local args = {...}
    for i = 1, #args do
        if (args[i]) then
            outputToConsole(string.format('[Print] %s', tostring(args[i])))
        end
    end
end

local function error(...)
    local args = {...}
    for i = 1, #args do
        if (args[i]) then
            outputToConsole(string.format('[Error] %s', tostring(args[i])))
        end
    end
end

local function warn(...)
    local args = {...}
    for i = 1, #args do
        if (args[i]) then
            outputToConsole(string.format('[Warn] %s', tostring(args[i])))
        end
    end
end
)";

    QFile file = QFile(tfRootFolder + "/Executor/script.lua");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << luaEnv << "\n" << text;
        file.close();
    }

    qDebug() << "Executed!";
}

void MainWindow::GetMenuInt(const QString &text, std::function<void(int)> callback) {
    QString code = QString(R"(local value = gui.GetValue("%1")
local file <close> = io.open("Executor/returnvalue.txt", "w")
if file then
    file:write(value)
    file:flush()
    file:close()
end)").arg(text);

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

    Execute(code);
}

void MainWindow::GetMenuString(const QString &text, std::function<void(QString)> callback) {
    QString code = QString(R"(
local value = gui.GetValue("%1")
local file <close> = io.open("Executor/returnvalue.txt", "w")
if file then
    file:write(value)
    file:flush()
    file:close()
end
)").arg(text);

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

    Execute(code);
}
