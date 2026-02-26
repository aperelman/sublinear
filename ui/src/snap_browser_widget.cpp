#include "snap_browser_widget.h"
#include "snap_catalog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCoreApplication>
#include <QFileInfo>
#include <QListWidgetItem>

SNAPBrowserWidget::SNAPBrowserWidget(QWidget *parent)
    : QWidget(parent), m_downloadManager(new DownloadManager(this)) {
    QVBoxLayout *layout = new QVBoxLayout(this);

    // שורת חיפוש
    m_searchBar = new QLineEdit();
    m_searchBar->setPlaceholderText("Search SNAP datasets...");
    QPushButton *searchBtn = new QPushButton("Search");

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->addWidget(m_searchBar);
    searchLayout->addWidget(searchBtn);
    layout->addLayout(searchLayout);

    // רשימת דאטהסטים
    m_datasetList = new QListWidget();
    layout->addWidget(m_datasetList);

    // סטטוס ופס התקדמות
    m_statusLabel = new QLabel("Ready");
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setRange(0, 100);

    layout->addWidget(m_statusLabel);
    layout->addWidget(m_progressBar);

    // חיבור כפתור חיפוש
    connect(searchBtn, &QPushButton::clicked, this, &SNAPBrowserWidget::onSearch);
    connect(m_searchBar, &QLineEdit::returnPressed, this, &SNAPBrowserWidget::onSearch);

    // חיבור לחיצה כפולה להורדה
    connect(m_datasetList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item){
        for(const auto& ds : SNAPCatalog::get()) {
            if(QString::fromStdString(ds.name) == item->text()) {
                onDownloadRequested(ds);
                break;
            }
        }
    });

    // טעינה ראשונית של הרשימה
    onSearch();
}

void SNAPBrowserWidget::onSearch() {
    m_datasetList->clear();
    QString filter = m_searchBar->text();
    for(const auto& ds : SNAPCatalog::get()) {
        QString name = QString::fromStdString(ds.name);
        if(filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive)) {
            m_datasetList->addItem(name);
        }
    }
}

void SNAPBrowserWidget::onDownloadRequested(const SNAPDataset& dataset) {
    m_statusLabel->setText("Downloading and decompressing...");
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);

    // קביעת נתיב יעד בתיקיית האפליקציה
    QString fileName = QString::fromStdString(dataset.name).replace(" ", "_") + ".txt";
    QString destPath = QCoreApplication::applicationDirPath() + "/" + fileName;

    // ניתוק חיבורים קודמים כדי למנוע כפילויות במידה והורדנו קובץ קודם לכן
    disconnect(m_downloadManager, &DownloadManager::downloadFinished, nullptr, nullptr);
    disconnect(m_downloadManager, &DownloadManager::downloadError, nullptr, nullptr);
    disconnect(m_downloadManager, &DownloadManager::downloadProgress, nullptr, nullptr);

    // חיבור לסיגנלים לפי download_manager.h
    connect(m_downloadManager, &DownloadManager::downloadFinished, this, [this](const QString& filePath){
        m_progressBar->setVisible(false);
        m_statusLabel->setText("Ready: " + QFileInfo(filePath).fileName());
        emit datasetReady(filePath);
    });

    connect(m_downloadManager, &DownloadManager::downloadError, this, [this](const QString& error){
        m_progressBar->setVisible(false);
        m_statusLabel->setText("Error: " + error);
    });

    connect(m_downloadManager, &DownloadManager::downloadProgress, this, [this](qint64 received, qint64 total){
        if (total > 0) {
            m_progressBar->setValue(static_cast<int>((received * 100) / total));
        }
    });

    // הפעלת ההורדה עם השמות הנכונים מה-Header
    m_downloadManager->downloadFile(QString::fromStdString(dataset.url), destPath);
}