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
    std::string token_text; //Current text for token
    ScopeNode* token_scope = 0; //Current scope for token
    Node* token = 0; //Current token
    size_t token_offset = 0; //Current offset into token
    //Parse and syntax-highlight code at current offset
    void parse() {
        QTextCursor cursor = textCursor();
        if(token) {
            delete token;
            token = 0;
        }
        cursor.movePosition(QTextCursor::PreviousCharacter,QTextCursor::MoveAnchor,token_offset);
        cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,token_text.size());
        setTextCursor(cursor);
        Node** nodes;
        size_t len;
        std::string errstr;
        if(!ctx->parse(token_text.data(),token_scope,&nodes,&len)) {
            //Error -- apply red underline
            errstr = "Illegal expression";
            goto token_err;
        }else {
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
                QString str = fmt.anchorName();
                tinfo->start = QTextCursor(document());
                tinfo->start.setPosition(cursor.position());
                char* insstr = (char*)start+(tsize-token->ide_token_size);
                char prevchar = insstr[tsize];
                insstr[tsize] = 0;
                textCursor().insertText(insstr,fmt); //TODO: Get these cursors figured out. Also; should only insert
                //the number of characters in the token somehow.
                tinfo->start.movePosition(QTextCursor::PreviousCharacter,QTextCursor::MoveAnchor,token->ide_token_size);
                int pos = tinfo->start.position();
                insstr[tsize] = prevchar;
            }
        }
        return;
        token_err:
        QTextCharFormat fmt;
        fmt.setToolTip(errstr.data());
        fmt.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
        fmt.setUnderlineColor(Qt::red);
        fmt.setFontUnderline(true);
        fmt.setAnchorName("");
        textCursor().insertText(token_text.data(),fmt);

    }
    void setTokenInfo() {
        QByteArray byteme = textCursor().charFormat().anchorName().toUtf8();
        std::stringstream ss;
        ss<<byteme.data();
        void* nin = 0;
        ss>>nin;
        Node* n = (Node*)nin;
        if(n) {
            token = n;
            ExtendedTokenInfo* tinfo = (ExtendedTokenInfo*)token->ide_context;
            token_offset = textCursor().position()-tinfo->start.position();
            if(token_offset == token->ide_token_size) {
                token = 0;
                token_offset = 0;
                return;
            }
            QTextCursor selection = tinfo->start;
            selection.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,token->ide_token_size);
            QByteArray bitten = selection.selectedText().toUtf8();
            token_text = bitten.data();
        }
    }
    void insert(const char* text) {
        size_t oldlen = token_text.size();
        if(token_offset == token_text.size()) {
            token_text+=text;
            token_offset+=(token_text.size()-oldlen);
            return;
        }
        std::stringstream ss;
        char prev = token_text[token_offset];
        token_text[token_offset] = 0;
        ss<<token_text.data();
        ss<<text;
        token_text[token_offset] = prev;
        ss<<(token_text.data()+token_offset);
        token_text = ss.str();
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
            QByteArray byteme = e->text().toUtf8();
            insert(byteme.data());
            parse();
            break;
        }
        if(token) {
        if(token_offset == token->ide_token_size) {
                //End of token
                token_offset = 0;
                token = 0;
                token_text.clear();
                setTokenInfo();
            }
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
