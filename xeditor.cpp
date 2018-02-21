#include "xeditor.h"
#include <QPaintEvent>
#include <QPainter>
#include <QPaintDevice>
#include <QTimer>
#include "VLANG/parser_api.h"
#include <sstream>
#include <QTextBlock>



class ExtendedTokenInfo:public ExternalObject {
public:
    QTextCursor start;
};

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
    ScopeNode* token_scope = 0; //Current scope for token
    Node* token = 0; //Current token
    //Parse and syntax-highlight code at current offset

    QString toUnicode(const char* str) {
        return str;
    }
    size_t unicodeSize(const char* str) {
        return toUnicode(str).length();
    }
    std::string fromUnicode(QString str) {
        QByteArray byteme = str.toUtf8();
        return byteme.data();
    }

    QTextCursor token_select(Node* token) {
       ExtendedTokenInfo* tinfo = (ExtendedTokenInfo*)token->ide_context;
       QTextCursor retval = tinfo->start;
       return retval;
    }
    QTextCursor changedText; //Text to reparse
    void parse() {
        setTokenInfo();
        std::string token_text = fromUnicode(changedText.selectedText());
        if(token) {
            delete token;
            token = 0;
        }

        Node* empty[1];

        Node** nodes;
        size_t len;
        std::string errstr;
        bool ean = ctx->parse(token_text.data(),token_scope,&nodes,&len);
        if(!ean) {
            nodes = empty;
            len = 1;
            nodes[0] = new Node(Invalid);
            nodes[0]->move(token_text.data());
        }



            const char* start = nodes[0]->location;
            size_t tsize = 0;
            for(size_t i = 0;i<len;i++) {
                ExtendedTokenInfo* tinfo = new ExtendedTokenInfo();
                token = nodes[i];
                token_scope = nodes[i]->node_scope;
                token->ide_context = tinfo;
                if(token->ide_token_size == 0) {
                    if(i+1<len) {
                        token->ide_token_size = ((size_t)nodes[i+1]->location)-((size_t)start);
                    }else {
                        token->ide_token_size = token_text.size();
                    }
                }
                tsize+=token->ide_token_size;
                QTextCharFormat fmt;
                std::stringstream ss;
                ss<<(void*)token;
                fmt.setAnchorName(ss.str().data());
                if(token->type == Invalid) {
                    fmt.setToolTip(errstr.data());
                    fmt.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
                    fmt.setUnderlineColor(Qt::red);
                    fmt.setFontUnderline(true);
                }
                QString str = fmt.anchorName();
                tinfo->start = QTextCursor(document());
                tinfo->start.setPosition(changedText.anchor()+(tsize-token->ide_token_size));
                //the number of characters in the token somehow.
                tinfo->start.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,token->ide_token_size); //Select complete block
                int pos = tinfo->start.position();
                token_select(token).setCharFormat(fmt);
        }
    }
    void setTokenInfo() {
        QByteArray byteme = textCursor().charFormat().anchorName().toUtf8();
        std::stringstream ss;
        ss<<byteme.data();
        void* nin = 0;
        ss>>nin;
        Node* n = (Node*)nin;
        token = n;

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
                setTokenInfo();
            }
            return;
        }
        switch(e->key()) {
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            XEditor::keyPressEvent(e);
            setTokenInfo();
            break;
        default:
            //Insert one character of text
            e->accept();
            textCursor().insertText(e->text());
            if(token) {
                if(token->type == Invalid) {
                    //Invalid token -- set bounds
                    changedText = ((ExtendedTokenInfo*)token->ide_context)->start;
                    changedText.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,1);
                }else {
                    changedText.setPosition(textCursor().position()-1);
                    changedText.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,1);
                }
            }else {
                changedText = textCursor();
                changedText.movePosition(QTextCursor::PreviousCharacter);
                changedText.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,1);
            }

            parse();
            break;
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
