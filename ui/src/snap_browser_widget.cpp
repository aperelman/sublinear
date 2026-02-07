#include "snap_browser_widget.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFile>
#include <QDir>

SnapBrowserWidget::SnapBrowserWidget(QWidget* p) 
    : QWidget(p), dlmgr(new DownloadManager(this)) 
{
    setupUI();
    connect(dlmgr, &DownloadManager::downloadProgress, 
            this, &SnapBrowserWidget::onDownloadProgress);
    connect(dlmgr, &DownloadManager::downloadFinished, 
            this, &SnapBrowserWidget::onDownloadFinished);
    connect(dlmgr, &DownloadManager::downloadError, 
            this, &SnapBrowserWidget::onDownloadError);
}

void SnapBrowserWidget::setupUI() {
    auto* l = new QVBoxLayout(this);
    l->addWidget(new QLabel("<b>SNAP Datasets</b>"));
    
    auto* g = new QGroupBox("Available");
    auto* gl = new QVBoxLayout(g);
    
    list = new QListWidget();
    gl->addWidget(list);
    
    info = new QLabel("Select dataset");
    info->setWordWrap(true);
    gl->addWidget(info);
    
    btn = new QPushButton("Download");
    btn->setEnabled(false);
    gl->addWidget(btn);
    
    progress = new QProgressBar();
    progress->setVisible(false);
    gl->addWidget(progress);
    
    l->addWidget(g);
    
    connect(list, &QListWidget::itemSelectionChanged, 
            this, &SnapBrowserWidget::onDatasetSelected);
    connect(btn, &QPushButton::clicked, 
            this, &SnapBrowserWidget::onDownloadClicked);
}

void SnapBrowserWidget::setDatasets(const QVector<SnapDataset>& d) {
    datasets = d;
    updateList();
}

void SnapBrowserWidget::updateList() {
    list->clear();
    for (int i = 0; i < datasets.size(); ++i) {
        const auto& ds = datasets[i];
        QString t = ds.displayName();
        if (isDownloaded(ds)) t += " ✓";
        auto* item = new QListWidgetItem(t);
        item->setData(Qt::UserRole, i);
        list->addItem(item);
    }
}

QString SnapBrowserWidget::getPath(const SnapDataset& ds) {
    return QDir::homePath() + "/src/sublinear/data/snap/" + ds.filename;
}

bool SnapBrowserWidget::isDownloaded(const SnapDataset& ds) {
    return QFile::exists(getPath(ds));
}

void SnapBrowserWidget::onDatasetSelected() {
    auto* item = list->currentItem();
    if (!item) {
        btn->setEnabled(false);
        return;
    }
    
    int i = item->data(Qt::UserRole).toInt();
    const auto& ds = datasets[i];
    
    info->setText(QString("<b>%1</b><br>%2<br>Nodes: %3, Edges: %4")
        .arg(ds.name).arg(ds.description).arg(ds.nodes).arg(ds.edges));
    
    btn->setEnabled(true);
    btn->setText(isDownloaded(ds) ? "Use" : "Download");
}

void SnapBrowserWidget::onDownloadClicked() {
    auto* item = list->currentItem();
    if (!item) return;
    
    int i = item->data(Qt::UserRole).toInt();
    const auto& ds = datasets[i];
    
    dlPath = getPath(ds);
    
    if (isDownloaded(ds)) {
        emit datasetReady(dlPath);
        return;
    }
    
    btn->setEnabled(false);
    progress->setVisible(true);
    progress->setValue(0);
    info->setText("Downloading...");
    
    dlmgr->downloadFile(ds.url, dlPath);
}

void SnapBrowserWidget::onDownloadProgress(qint64 r, qint64 t) {
    if (t > 0) progress->setValue((r * 100) / t);
}

void SnapBrowserWidget::onDownloadFinished(const QString& f) {
    btn->setEnabled(true);
    progress->setVisible(false);
    updateList();
    
    QMessageBox::information(this, "Done", "Downloaded!");
    emit datasetReady(f);
}

void SnapBrowserWidget::onDownloadError(const QString& e) {
    btn->setEnabled(true);
    progress->setVisible(false);
    QMessageBox::critical(this, "Error", e);
}
