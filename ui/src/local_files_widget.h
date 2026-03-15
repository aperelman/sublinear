#ifndef LOCAL_FILES_WIDGET_H
#define LOCAL_FILES_WIDGET_H

#include <QWidget>
#include <QListView>
#include <QStandardItemModel>
#include <QPushButton>

class LocalFilesWidget : public QWidget {
    Q_OBJECT

public:
    explicit LocalFilesWidget(QWidget *parent = nullptr);
    ~LocalFilesWidget() = default;

    // Pass empty string to use the default platform-appropriate data dir
    void scanDirectory(const QString &path = {});

Q_SIGNALS:
    void fileSelected(const QString &fileName);

private Q_SLOTS:
    void handleSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onOpenFolderClicked();

private:
    QListView          *fileView;
    QStandardItemModel *fileModel;
    QPushButton        *btnOpenFolder;
    QString             m_currentDir;
};

#endif // LOCAL_FILES_WIDGET_H
