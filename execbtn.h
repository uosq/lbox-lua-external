#ifndef EXECBTN_H
#define EXECBTN_H

#include <QObject>

class ExecBtn : public QObject
{
    Q_OBJECT

public:
    ExecBtn();

public slots:
    void Run();

signals:
    void btnClick();
};

#endif // EXECBTN_H
