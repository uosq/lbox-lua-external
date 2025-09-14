#include "luasyntaxhighlight.h"

LuaSyntaxHighlighter::LuaSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Lua keywords
    keywordFormat.setForeground(QColor(0, 255, 255));
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns = {
        "\\band\\b", "\\band\\b", "\\bbreak\\b", "\\bdo\\b",
        "\\belse\\b", "\\belseif\\b", "\\bend\\b", "\\bfalse\\b",
        "\\bfor\\b", "\\bfunction\\b", "\\bif\\b", "\\bin\\b",
        "\\blocal\\b", "\\bnil\\b", "\\bnot\\b", "\\bor\\b",
        "\\brepeat\\b", "\\breturn\\b", "\\bthen\\b", "\\btrue\\b",
        "\\buntil\\b", "\\bwhile\\b"
    };
    for (const QString &pattern : keywordPatterns) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    libraryFormat.setForeground(QColor(188, 255, 166));
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList libraryPatterns = {
        "\\bdraw\\b", "\\bphysics\\b", "\\baimbot\\b",
        "\\bcallbacks\\b", "\\bclient\\b", "\\bclientstate\\b",
        "\\bengine\\b", "\\bentities\\b", "\\bfilesystem\\b",
        "\\bgamecoordinator\\b", "\\bgamerules\\b", "\\bglobals\\b",
        "\\bgui\\b", "\\bhttp\\b", "\\binput\\b", "\\binventory\\b", "\\bitemschema\\b",
        "\\bmaterials\\b", "\\bmodels\\b", "party", "\\bplayerlist\\b",
        "\\brender\\b", "\\bsteam\\b", "\\bvector\\b", "\\bwarp\\b"
    };
    for (const QString &pattern : libraryPatterns) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = libraryFormat;
        highlightingRules.append(rule);
    }

    // Numbers
    numberFormat.setForeground(QColor(255, 166, 247));
    HighlightRule numberRule;
    numberRule.pattern = QRegularExpression("\\b[0-9]+\\b");
    numberRule.format = numberFormat;
    highlightingRules.append(numberRule);

    // Strings
    stringFormat.setForeground(Qt::darkGreen);
    HighlightRule stringRule;
    stringRule.pattern = QRegularExpression("\".*\"|'.*'");
    stringRule.format = stringFormat;
    highlightingRules.append(stringRule);

    // Comments
    commentFormat.setForeground(Qt::darkGray);
    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression("--[^\n]*");
    commentRule.format = commentFormat;
    highlightingRules.append(commentRule);
}

void LuaSyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightRule &rule : std::as_const(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
