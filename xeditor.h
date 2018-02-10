#ifndef XEDITOR_H
#define XEDITOR_H

#include <QWidget>
#include <QTextEdit>

class XEditor : public QTextEdit
{
    Q_OBJECT
public:
    explicit XEditor(QWidget *parent = 0);
signals:

public slots:
};

#endif // XEDITOR_H
