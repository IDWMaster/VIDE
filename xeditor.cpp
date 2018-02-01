#include "xeditor.h"
#include <QPaintEvent>
#include <QPainter>
#include <QPaintDevice>
#include <QTimer>

enum PaintOp {
    FULL_REDRAW,
    DRAW_CURSOR
};

class PaintCommand {
public:
    PaintOp op;
    PaintCommand* next = 0;
    PaintCommand(PaintOp op) {
        this->op = op;
    }
};

XEditor::XEditor(QWidget* parent):QWidget(parent) {

}

class XEditorImpl:public XEditor {
Q_OBJECT
public:
    PaintCommand* cmd = 0;
    PaintCommand* end = 0;
    double cursorX = 0;
    double cursorY = 0;
    bool cursorEnable = false;
    QTimer* cursorRefreshTimer;
    void pushcmd(PaintCommand* cmd) {
        if(!this->cmd) {
            this->cmd = cmd;
        }else {
            this->cmd->next = cmd;
        }
        end = cmd;
    }
    void pushredraw() {
        pushcmd(new PaintCommand(FULL_REDRAW));
    }

    int blinkInterval = 500;
    XEditorImpl(QWidget* parent):XEditor(parent) {
        cursorRefreshTimer = new QTimer(this);
        cursorRefreshTimer->setInterval(blinkInterval);
        connect(cursorRefreshTimer,&QTimer::timeout,this,[=](){
            cursorEnable = !cursorEnable;
            pushcmd(new PaintCommand(DRAW_CURSOR));
            update();
        });
        cursorRefreshTimer->start();
        update(); //Force full redraw
    }

protected:
    QPixmap* pbo = 0;
    void paintEvent(QPaintEvent* evt) {
        //Check if we need a viewport refresh
        QPainter painter(this);
        if(pbo == 0) {
            pushredraw(); //Need a redraw.
            pbo = new QPixmap(painter.window().width(),painter.window().height());
        }
        if((pbo->width() != painter.window().width()) || (pbo->height() != painter.window().height())) {
            pushredraw();
        }
        //Paint widget
        doPaint();
        painter.drawPixmap(0,0,*pbo);
    }

    void doPaint() {

        QPainter painter(pbo);
        QBrush background = Qt::black;
        if(!cmd) {
            pushredraw();
        }
        while(this->cmd) {
        PaintCommand* cmd = this->cmd;
        this->cmd = cmd->next;

        switch(cmd->op) {
        case FULL_REDRAW:
            painter.setRenderHint(QPainter::Antialiasing,true);
            painter.fillRect(QRect(0,0,painter.window().width(),painter.window().height()),background);
            pushcmd(new PaintCommand(DRAW_CURSOR));
            break;
        case DRAW_CURSOR:
        {
            if(cursorEnable) {
                painter.fillRect(QRectF(cursorX,cursorY,1,10),Qt::white);
            }else {
                painter.fillRect(QRectF(cursorX,cursorY,1,10),background);
            }
        }
            break;
        }

        delete cmd;
    }
    }

};


XEditor* createEditor(QWidget* parent) {
    return new XEditorImpl(parent);
}

#include "xeditor.moc"
