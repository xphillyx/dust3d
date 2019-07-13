#include <QFont>
#include <QPalette>
#include "scripthighlighter.h"
#include "scripteditwidget.h"

ScriptEditWidget::ScriptEditWidget(QWidget *parent) :
    QTextEdit(parent)
{
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);

    setFont(font);

    new ScriptHighlighter(this->document());
    
    connect(this, &QTextEdit::textChanged, this, [&]() {
        emit scriptChanged(toPlainText());
    });
}
