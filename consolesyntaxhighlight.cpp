#include "consolesyntaxhighlight.h"

ConsoleSyntaxHighlight::ConsoleSyntaxHighlight(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightRule printRule;
    printFormat.setForeground(QColor(255, 255, 255));

    printRule.pattern = QRegularExpression("\\bPrint\\b");
    printRule.format = printFormat;
    highlightingRules.append(printRule);

    HighlightRule warnRule;
    warnFormat.setForeground(QColor(255, 153, 0));
    warnRule.pattern = QRegularExpression("\\bWarn\\b");
    warnRule.format = warnFormat;
    highlightingRules.append(warnRule);

    HighlightRule errRule;
    errFormat.setForeground(QColor(255, 100, 100));
    errRule.pattern = QRegularExpression("\\bError\\b");
    errRule.format = errFormat;
    highlightingRules.append(errRule);
}

void ConsoleSyntaxHighlight::highlightBlock(const QString &text)
{
    for (const HighlightRule &rule : std::as_const(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
