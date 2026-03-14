#include "local_files_widget.h"
#include <QVBoxLayout>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QItemSelectionModel>

LocalFilesWidget::LocalFilesWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(8);

    // 1. Setup the Model and View
    fileModel = new QStandardItemModel(this);
    fileView = new QListView(this);
    fileView->setModel(fileModel);
    fileView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fileView->setStyleSheet("QListView { background-color: #ffffff; border: 1px solid #ccc; }");
    
    layout->addWidget(fileView, 1); // List takes all available space

    // 2. Setup the "Open Folder" Button (Bottom)
    btnOpenFolder = new QPushButton("Open Data Folder", this);
    btnOpenFolder->setMinimumHeight(30);
    btnOpenFolder->setStyleSheet("font-weight: bold;");
    
    layout->addWidget(btnOpenFolder);

    // 3. Connections
    connect(btnOpenFolder, &QPushButton::clicked, this, &LocalFilesWidget::onOpenFolderClicked);
    connect(fileView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &LocalFilesWidget::handleSelectionChanged);
}

void LocalFilesWidget::scanDirectory(const QString &path) {
    fileModel->clear();
    QDir directory(path);
    
    // Create directory if it doesn't exist
    if (!directory.exists()) {
        directory.mkpath(".");
    }

    // Filter for common graph formats
    QStringList filters;
    filters << "*.txt" << "*.gz" << "*.csv";
    QStringList files = directory.entryList(filters, QDir::Files | QDir::NoDotAndDotDot);
    
    for (const QString &filename : files) {
        QStandardItem *item = new QStandardItem(filename);
        fileModel->appendRow(item);
    }
}

void LocalFilesWidget::onOpenFolderClicked() {
    // Opens C:\BIU\data in the Windows File Explorer
    QDesktopServices::openUrl(QUrl::fromLocalFile("C:/BIU/data"));
}

void LocalFilesWidget::handleSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    Q_UNUSED(deselected);
    if (!selected.indexes().isEmpty()) {
        QString fileName = fileModel->data(selected.indexes().first()).toString();
        Q_EMIT fileSelected(fileName);
    }
}