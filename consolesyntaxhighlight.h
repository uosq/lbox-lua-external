#ifndef CONSOLESYNTAXHIGHLIGHT_H
#define CONSOLESYNTAXHIGHLIGHT_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class ConsoleSyntaxHighlight : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit ConsoleSyntaxHighlight(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightRule> highlightingRules;

    QTextCharFormat printFormat;
    QTextCharFormat errFormat;
    QTextCharFormat warnFormat;
};


#endif // CONSOLESYNTAXHIGHLIGHT_H
