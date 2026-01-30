#ifndef GRAPH_LIST_WIDGET_H
#define GRAPH_LIST_WIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "graph_info.h"

// Custom widget for each graph list item
class GraphListItemWidget : public QWidget {
    Q_OBJECT

public:
    explicit GraphListItemWidget(const GraphInfo& graph, QWidget *parent = nullptr);
    
    const GraphInfo& getGraphInfo() const { return graphInfo; }
    void setDownloadProgress(int percentage);
    void setDownloaded(bool downloaded);
    void setDownloading(bool downloading);

signals:
    void downloadRequested(const GraphInfo& graph);
    void analyzeRequested(const GraphInfo& graph);

private:
    GraphInfo graphInfo;
    QLabel* nameLabel;
    QLabel* descLabel;
    QLabel* statsLabel;
    QLabel* statusLabel;
    QPushButton* actionButton;
    QProgressBar* progressBar;
};

// Main graph list widget
class GraphListWidget : public QListWidget {
    Q_OBJECT

public:
    explicit GraphListWidget(QWidget *parent = nullptr);
    
    void populateGraphs();
    void updateGraphStatus(const QString& graphName, bool isDownloaded);
    GraphInfo* getSelectedGraph();

signals:
    void graphDownloadRequested(const GraphInfo& graph);
    void graphAnalyzeRequested(const GraphInfo& graph);

private:
    void createGraphItem(const GraphInfo& graph);
};

#endif // GRAPH_LIST_WIDGET_H
