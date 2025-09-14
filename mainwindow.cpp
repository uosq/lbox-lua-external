#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "luasyntaxhighlight.h"
#include <QFileDialog>
#include <QStandardItem>
#include <QMessageBox>
#include <QTimer>
#include <QThread>
#include "filereader.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    setWindowFlag(Qt::WindowStaysOnTopHint, 1);
    ui->setupUi(this);
    new LuaSyntaxHighlighter(ui->plainTextEdit->document());

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

    // Create worker + thread
    FileReader *worker = new FileReader(filePath);
    fileThread = new QThread(this);
    worker->moveToThread(fileThread);

    // Create timer in worker thread
    QTimer *timer = new QTimer;
    timer->setInterval(1000); // every 1 second
    timer->moveToThread(fileThread);

    // Connect signals/slots
    connect(timer, &QTimer::timeout, worker, &FileReader::readFile);
    connect(worker, &FileReader::fileRead, this, &MainWindow::onFileRead);

    // Clean up
    connect(fileThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(fileThread, &QThread::finished, timer, &QObject::deleteLater);

    // Start thread + timer
    connect(fileThread, &QThread::started, timer, QOverload<>::of(&QTimer::start));
    fileThread->start();
}

void MainWindow::onFileRead(const QString &text)
{
    // Runs in the GUI thread
    ui->consoleTextEdit->setPlainText(text);
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

    if (fileThread) {
        fileThread->quit();
        fileThread->wait();
    }

    delete ui;
}

void MainWindow::on_execBtn_clicked() {
    if (tfRootFolder == "" || tfRootFolder.isEmpty()) {
        QMessageBox::critical(this, "Error!", tr("You have to select a root folder!\nFile -> Set TF2 Root Folder"));
        return;
    }

    QString luaEnv = R"(
local function print(...)
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

local function error(...)
    local args = {...}
    for i = 1, #args do
        if (args[i]) then
            print(string.format('[Error] %s', tostring(args[i])))
        end
    end
end

local function warn(...)
    local args = {...}
    for i = 1, #args do
        if (args[i]) then
            print(string.format('[Warn] %s', tostring(args[i])))
        end
    end
end

)";

    QString text = ui->plainTextEdit->toPlainText();

    QFile file = QFile(tfRootFolder + "/Executor/script.lua");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << luaEnv << "\n" << text;
        file.close();
    }

    qDebug() << "Executed!";
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
    if (listViewLoadedFolder.isEmpty())
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
