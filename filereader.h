#ifndef FILEREADER_H
#define FILEREADER_H

#include <QObject>
#include <QFile>
#include <QTextStream>

class FileReader : public QObject {
    Q_OBJECT
public:
    explicit FileReader(const QString &filePath, QObject *parent = nullptr)
        : QObject(parent), m_filePath(filePath) {}

public slots:
    void readFile() {
        QFile file(m_filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            //emit fileRead(QStringLiteral("Failed to open %1").arg(m_filePath));
            return;
        }

        QTextStream in(&file);
        QString allText = in.readAll();   // read entire console.txt
        if (!allText.isEmpty())
            emit fileRead(allText);
    }

signals:
    void fileRead(const QString &text);

private:
    QString m_filePath;
};

#endif // FILEREADER_H
