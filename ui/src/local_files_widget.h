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

    // Call this to refresh the list of files on disk
    void scanDirectory(const QString &path);

signals:
    // Signal to tell MainWindow which local file was selected
    void fileSelected(const QString &fileName);

private slots:
    void handleSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onOpenFolderClicked();

private:
    QListView *fileView;
    QStandardItemModel *fileModel;
    QPushButton *btnOpenFolder;
};

#endif // LOCAL_FILES_WIDGET_H