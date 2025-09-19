#include "firststartup.h"
#include "ui_firststartup.h"
#include <QFileDialog>
#include <QMessageBox>

FirstStartup::FirstStartup(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FirstStartup)
{
    ui->setupUi(this);
}

FirstStartup::~FirstStartup()
{
    delete ui;
}

void FirstStartup::on_pushButton_clicked()
{
    QString filePath = QFileDialog::getExistingDirectory(this, tr("Select TF2's Root Folder"));
    if (!filePath.isEmpty()) {

        QFile tf_win64(filePath + "/tf_win64.exe");

        if (tf_win64.exists()) {
            tfRootFolder = filePath;
            close();
        } else {
            QMessageBox::warning(this, "Error!", "No tf_win64.exe found, try again!");
        }
    }
}

QString FirstStartup::GetRootFolder() {
    return tfRootFolder;
}
