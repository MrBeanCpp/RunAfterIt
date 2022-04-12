#include "lineedit.h"
#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QMimeData>
#include <windows.h>
LineEdit::LineEdit(QWidget* parent) //ui->setupUi(this)会setText(QString()) 所以在构造函数中setText无效
    : QLineEdit(parent)
{
    connect(this, &LineEdit::editingFinished, [=]() {
        setPath(text());
        setEdit(false);
    });

    defaultIcon = iconPro.icon(QFileIconProvider::File);
    act_icon = addAction(defaultIcon, ActionPosition::LeadingPosition);

    setPlaceholderText("Null");

    setEdit(false);
}

void LineEdit::setEdit(bool _isEdit)
{
    if (isEdit != _isEdit) {
        setReadOnly(!_isEdit);
        setFrame(_isEdit);
        setText(_isEdit ? path : fileName);
        this->isEdit = _isEdit;
    }
}

void LineEdit::setPath(const QString& _path)
{
    if (path != _path) {
        this->path = _path;
        QFileInfo info(_path);
        if (QFile::exists(_path)) {
            this->fileName = info.baseName();
            setPlaceholderText("");
            act_icon->setIcon(iconPro.icon(info));
        } else {
            this->fileName = "";
            setPlaceholderText(_path.isEmpty() ? "Null" : "Wrong Path");
            act_icon->setIcon(defaultIcon);
        }
        setText(fileName);
        setToolTip(_path);

        emit pathChanged();
    }
}

QString LineEdit::getPath()
{
    return path;
}

void LineEdit::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    if (isEdit)
        QLineEdit::mouseDoubleClickEvent(event);
    else
        setEdit(true);
}

void LineEdit::focusOutEvent(QFocusEvent* event)
{
    Q_UNUSED(event)
    if (isEdit) //防止二次关闭导致text错误（如右键弹出菜单 + 左键转移焦点 == 2次）
        emit editingFinished();
}

void LineEdit::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    if (!isEdit) //将父对象(item)设置焦点 否则无法选中 焦点被子对象窃取
        event->ignore(); //传递事件给父对象
    return QLineEdit::mousePressEvent(event); //执行本对象的操作
}

void LineEdit::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        //SwitchToThisWindow(HWND(winId()), true); //这句话导致了窗体分裂 因为此处的winId()不是主窗体 而是lineEdit
        //会导致在dragMove之后 新建条目 会在构造函数前 显示第一条item的内容 然后再清空
        //& 分裂后 主窗体接收不到WM_NCHITTEST消息
        setFrame(true);
    } else
        event->ignore();
}

void LineEdit::dropEvent(QDropEvent* event)
{
    QString path = event->mimeData()->urls().at(0).toLocalFile();
    setPath(QDir::toNativeSeparators(path));
    setFrame(false);
}

void LineEdit::dragLeaveEvent(QDragLeaveEvent* event)
{
    Q_UNUSED(event)
    setFrame(false);
}
