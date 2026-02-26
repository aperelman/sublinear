#ifndef GRAPH_LIST_WIDGET_H
#define GRAPH_LIST_WIDGET_H

#include <QTableWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class GraphListWidget : public QTableWidget {
    Q_OBJECT

public:
    explicit GraphListWidget(QWidget *parent = nullptr);

    // Starts the "Quick Sync" (Names only)
    void fetchNamesOnly();

    signals:
        // Notifies MainWindow to fetch detailed metadata for this specific URL path
        void requestMetadata(const QString& name, const QString& urlPath);

private slots:
    // Handles the network response from SNAP index page
    void onMainPageDownloaded(QNetworkReply* reply);

    // Handles user selection in the table
    void onRowSelected();

private:
    QNetworkAccessManager *m_internalManager=nullptr;
};

#endif // GRAPH_LIST_WIDGET_H