#ifndef GRAPH_LIST_WIDGET_H
#define GRAPH_LIST_WIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QList>
#include "graph_info.h"

class GraphListWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit GraphListWidget(QWidget* parent = nullptr);
    
    void loadGraphsFromDirectory(const QString& dirPath);
    GraphInfo currentGraph() const;
    bool hasSelection() const;
    
signals:
    void graphSelected(const GraphInfo& graph);
    void graphDoubleClicked(const GraphInfo& graph);
    
private slots:
    void onSelectionChanged();
    void onItemDoubleClicked(QListWidgetItem* item);
    void onBrowseClicked();
    void onRefreshClicked();
    void onFilterTextChanged(const QString& text);
    
private:
    void setupUI();
    void updateGraphList();
    QString findDefaultGraphDirectory();
    
    QListWidget* graphList;
    QLineEdit* filterEdit;
    QLabel* statsLabel;
    QPushButton* browseButton;
    QPushButton* refreshButton;
    
    QString currentDirectory;
    QList<GraphInfo> graphs;
    QList<GraphInfo> filteredGraphs;
};

#endif // GRAPH_LIST_WIDGET_H