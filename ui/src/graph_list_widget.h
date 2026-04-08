#ifndef GRAPH_LIST_WIDGET_H
#define GRAPH_LIST_WIDGET_H

#include <QTableWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class GraphListWidget : public QTableWidget {
    Q_OBJECT

public:
    explicit GraphListWidget(QWidget *parent = nullptr);

    void fetchNamesOnly();

Q_SIGNALS:
    void requestMetadata(const QString& name, const QString& urlPath);

private Q_SLOTS:
    void onMainPageDownloaded(QNetworkReply* reply);
    void onRowSelected();

private:
    QNetworkAccessManager *m_internalManager = nullptr;
};

#endif // GRAPH_LIST_WIDGET_H
