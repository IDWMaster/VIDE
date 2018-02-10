#include "xeditor.h"
#include <QPaintEvent>
#include <QPainter>
#include <QPaintDevice>
#include <QTimer>
#include "VLANG/parser_api.h"
#include <sstream>
#include <QTextBlock>


XEditor::XEditor(QWidget* parent):QTextEdit(parent) {

}

extern thread_local std::map<const char*,Node*> lookup_table;


class XEditorImpl:public XEditor {
Q_OBJECT
public:
    ExternalCompilerContext* ctx;
    XEditorImpl(QWidget* parent):XEditor(parent) {
        setLineWrapMode(LineWrapMode::NoWrap);
        ctx = compiler_new();
    }
    std::string token_text; //Current text for token
    ScopeNode* token_scope = 0; //Current scope for token
    Node* token = 0; //Current token
    size_t token_offset = 0; //Current offset into token
    size_t token_len = 0;
    //Parse and syntax-highlight code
    void parse() {
        if(token) {
            delete token;
        }
        Node** nodes;
        size_t len;
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::KeepAnchor,token_offset);

        setTextCursor(cursor);
        std::string errstr;
        if(!ctx->parse(token_text.data(),token_scope,&nodes,&len)) {
            //Error -- apply red underline
            errstr = "Illegal expression";
            goto token_err;
        }else {
            token = nodes[0];
            token_scope = nodes[0]->node_scope;
            QTextCharFormat fmt;
            std::stringstream ss;
            ss<<(void*)token;
            fmt.setAnchorName(ss.str().data());
            textCursor().insertText(token_text.data(),fmt);
            token_len = token_text.size();
        }
        return;
        token_err:
        token_len = 0;
        QTextCharFormat fmt;
        fmt.setToolTip(errstr.data());
        fmt.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
        fmt.setUnderlineColor(Qt::red);
        fmt.setFontUnderline(true);
        textCursor().insertText(token_text.data(),fmt);

    }
    void setTokenInfo() {
        QByteArray byteme = textCursor().blockCharFormat().anchorName().toUtf8();
        Node* n = (Node*)atoll(byteme.data()); //For whom the atolls
        if(n) {
            token = n;

        }
    }

    void keyPressEvent(QKeyEvent *e) {

        if(e->modifiers() & Qt::ControlModifier) {
            switch(e->key()) {
            case Qt::Key_C:
                //Copy selection
                this->copy();
                break;
            case Qt::Key_V:
                break;
            case Qt::Key_A:
                break;
            case Qt::Key_Left:
            case Qt::Key_Right:
            case Qt::Key_Up:
            case Qt::Key_Down:
                XEditor::keyPressEvent(e);
            }
            return;
        }
        switch(e->key()) {
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            XEditor::keyPressEvent(e);
            break;
        default:
            //Insert one character of text
            e->accept();
            QByteArray byteme = e->text().toUtf8();
            token_text+=byteme.data();
            parse();
            token_offset++;
            break;
        }
        setTokenInfo();
        if(token_offset == token_len) {
            //End of token
            token_offset = 0;
            token_len = 0;
            token = 0;
            token_text.clear();
            //Find next token

        }
    }
    void mousePressEvent(QMouseEvent* evt) {
        evt->accept();
    }
    void contextMenuEvent(QContextMenuEvent* evt) {
        evt->accept();
    }

protected:

};


XEditor* createEditor(QWidget* parent) {
    return new XEditorImpl(parent);
}

#include "xeditor.moc"
