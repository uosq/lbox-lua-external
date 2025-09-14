#include <fstream>
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "luasyntaxhighlight.h"
#include <QFileDialog>
#include <QStandardItem>
#include <QMessageBox>

#define OUTPUT_PATH "/home/tevin/hd/tf2/Team Fortress 2/Executor/script.lua"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    new LuaSyntaxHighlighter(ui->textEdit->document());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_execBtn_clicked() {
    if (tfRootFolder == "") {
        QMessageBox::critical(this, "Error!", tr("You have to select a root folder!\nFile -> Set TF2 Root Folder"));
        return;
    }

    QString text = ui->textEdit->toPlainText();

    QString outputPath = "";
    outputPath.append(tfRootFolder);
    outputPath.append("/Executor/script.lua");
    //qDebug() << outputPath;

    std::ofstream outputFile(outputPath.toStdString());
    if(!outputFile.is_open()) {
        qDebug() << "Failed to open output file!";
        return;
    }

    outputFile << text.toStdString() << std::endl;

    outputFile.close();

    qDebug() << "Executed!";
}

void MainWindow::on_clearBtn_clicked() {
    ui->textEdit->setText("");
}

void MainWindow::on_actionLoad_File_triggered() {
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("All Files (*.*);;Lua Files (*.lua);;Text Files (*.txt)"));
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();
            ui->textEdit->setText(content);
        }
    }
}

void MainWindow::on_actionLoad_Folder_triggered() {
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("Select Folder"));

    if (folderPath.isEmpty())
        return; // User canceled

    QDir dir(folderPath);

    listViewLoadedFolder = dir.absolutePath();
    qDebug() << listViewLoadedFolder;

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

    // Get the folder path (assuming you have stored it somewhere; here we reuse the last loaded folder)
    // If you want, you could store the folder path in a member variable

    QString fullPath = listViewLoadedFolder + "/" + fileName;

    QFile file(fullPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        ui->textEdit->setText(content);
        file.close();
    } else {
        qDebug() << "Failed to open file:" << fullPath;
    }
}

void MainWindow::on_actionSet_TF2_Root_Folder_triggered() {
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("Select Folder"));
    tfRootFolder = folderPath;
}
