#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QLineEdit>
#include <QObject>
#include <QFileIconProvider>
#include <QAction>
class LineEdit : public QLineEdit
{
    Q_OBJECT
public:
    LineEdit(QWidget* parent = nullptr);
    void setEdit(bool _isEdit);
    void setPath(const QString& _path);
    QString getPath(void);

private:
    bool isEdit = true;
    QString path;
    QString fileName;

    QFileIconProvider iconPro;
    QAction* act_icon = nullptr;
    QIcon defaultIcon;

signals:
    void pathChanged(QString newPath);

    // QWidget interface
protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    // QWidget interface
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
};

#endif // LINEEDIT_H
