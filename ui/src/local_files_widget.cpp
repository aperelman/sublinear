#include "local_files_widget.h"
#include <QVBoxLayout>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QItemSelectionModel>
#include <QStandardPaths>

LocalFilesWidget::LocalFilesWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(8);

    fileModel = new QStandardItemModel(this);
    fileView = new QListView(this);
    fileView->setModel(fileModel);
    fileView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fileView->setStyleSheet("QListView { background-color: #ffffff; border: 1px solid #ccc; }");
    layout->addWidget(fileView, 1);

    btnOpenFolder = new QPushButton("Open Data Folder", this);
    btnOpenFolder->setMinimumHeight(30);
    btnOpenFolder->setStyleSheet("font-weight: bold;");
    layout->addWidget(btnOpenFolder);

    connect(btnOpenFolder, &QPushButton::clicked, this, &LocalFilesWidget::onOpenFolderClicked);
    connect(fileView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &LocalFilesWidget::handleSelectionChanged);
}

// Default data directory: <DownloadLocation>/GraphAnalyzer
static QString defaultDataDir() {
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
           + "/GraphAnalyzer";
}

void LocalFilesWidget::scanDirectory(const QString &path) {
    m_currentDir = path.isEmpty() ? defaultDataDir() : path;

    fileModel->clear();
    QDir directory(m_currentDir);

    if (!directory.exists())
        directory.mkpath(".");

    QStringList filters;
    filters << "*.txt" << "*.gz" << "*.csv";
    QStringList files = directory.entryList(filters, QDir::Files | QDir::NoDotAndDotDot);

    for (const QString &filename : files) {
        QStandardItem *item = new QStandardItem(filename);
        fileModel->appendRow(item);
    }
}

void LocalFilesWidget::onOpenFolderClicked() {
    QString dir = m_currentDir.isEmpty() ? defaultDataDir() : m_currentDir;
    QDir().mkpath(dir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void LocalFilesWidget::handleSelectionChanged(const QItemSelection &selected,
                                               const QItemSelection &deselected) {
    Q_UNUSED(deselected);
    if (!selected.indexes().isEmpty()) {
        QString fileName = fileModel->data(selected.indexes().first()).toString();
        Q_EMIT fileSelected(fileName);
    }
}
