#ifndef SNAP_BROWSER_WIDGET_H
#define SNAP_BROWSER_WIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QProgressBar>
#include "snap_catalog.h" // חייב להיות כאן עבור SNAPDataset
#include "download_manager.h"

class SNAPBrowserWidget : public QWidget {
    Q_OBJECT
public:
    explicit SNAPBrowserWidget(QWidget *parent = nullptr);

    signals:
        // עכשיו ה-Compiler מכיר את SNAPDataset
        void datasetSelected(const SNAPDataset& dataset);
    void datasetReady(const QString& filePath);
    void analysisProgress(const QString& message);

private slots:
    void onSearch();
    void onDownloadRequested(const SNAPDataset& dataset);

private:
    QLineEdit *m_searchBar;
    QListWidget *m_datasetList;
    DownloadManager *m_downloadManager;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
};

#endif