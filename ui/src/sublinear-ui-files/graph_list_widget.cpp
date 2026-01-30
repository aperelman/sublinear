#include "graph_list_widget.h"
#include <QListWidgetItem>

// GraphListItemWidget implementation
GraphListItemWidget::GraphListItemWidget(const GraphInfo& graph, QWidget *parent)
    : QWidget(parent), graphInfo(graph)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(4);
    
    // Top row: Name and action button
    QHBoxLayout* topLayout = new QHBoxLayout();
    
    nameLabel = new QLabel("<b>" + QString::fromStdString(graph.name) + "</b>");
    nameLabel->setStyleSheet("font-size: 14px;");
    topLayout->addWidget(nameLabel);
    
    topLayout->addStretch();
    
    statusLabel = new QLabel();
    statusLabel->setStyleSheet("color: #666; font-size: 12px;");
    topLayout->addWidget(statusLabel);
    
    actionButton = new QPushButton();
    actionButton->setFixedWidth(100);
    if (graph.isDownloaded) {
        actionButton->setText("Analyze");
        statusLabel->setText("✓ Downloaded");
        statusLabel->setStyleSheet("color: green; font-size: 12px;");
        connect(actionButton, &QPushButton::clicked, [this]() {
            emit analyzeRequested(graphInfo);
        });
    } else {
        actionButton->setText("Download");
        statusLabel->setText("Not downloaded");
        connect(actionButton, &QPushButton::clicked, [this]() {
            emit downloadRequested(graphInfo);
        });
    }
    topLayout->addWidget(actionButton);
    
    mainLayout->addLayout(topLayout);
    
    // Description
    descLabel = new QLabel(QString::fromStdString(graph.description));
    descLabel->setStyleSheet("color: #666; font-size: 11px;");
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);
    
    // Stats
    QString statsText = QString("%1 nodes, %2 edges")
        .arg(QString::number(graph.nodes))
        .arg(QString::number(graph.edges));
    statsLabel = new QLabel(statsText);
    statsLabel->setStyleSheet("color: #888; font-size: 10px;");
    mainLayout->addWidget(statsLabel);
    
    // Progress bar (initially hidden)
    progressBar = new QProgressBar();
    progressBar->setMaximum(100);
    progressBar->setVisible(false);
    progressBar->setTextVisible(true);
    mainLayout->addWidget(progressBar);
    
    setLayout(mainLayout);
}

void GraphListItemWidget::setDownloadProgress(int percentage) {
    progressBar->setValue(percentage);
    progressBar->setVisible(true);
    statusLabel->setText(QString("Downloading %1%").arg(percentage));
}

void GraphListItemWidget::setDownloaded(bool downloaded) {
    graphInfo.isDownloaded = downloaded;
    progressBar->setVisible(false);
    
    if (downloaded) {
        actionButton->setText("Analyze");
        statusLabel->setText("✓ Downloaded");
        statusLabel->setStyleSheet("color: green; font-size: 12px;");
        
        disconnect(actionButton, nullptr, nullptr, nullptr);
        connect(actionButton, &QPushButton::clicked, [this]() {
            emit analyzeRequested(graphInfo);
        });
    }
}

void GraphListItemWidget::setDownloading(bool downloading) {
    actionButton->setEnabled(!downloading);
    if (downloading) {
        statusLabel->setText("Downloading...");
        progressBar->setVisible(true);
    } else {
        progressBar->setVisible(false);
    }
}

// GraphListWidget implementation
GraphListWidget::GraphListWidget(QWidget *parent)
    : QListWidget(parent)
{
    setAlternatingRowColors(true);
    setSpacing(2);
}

void GraphListWidget::populateGraphs() {
    clear();
    
    std::vector<GraphInfo> graphs = getAvailableGraphs();
    
    for (const auto& graph : graphs) {
        createGraphItem(graph);
    }
}

void GraphListWidget::createGraphItem(const GraphInfo& graph) {
    QListWidgetItem* item = new QListWidgetItem(this);
    
    GraphListItemWidget* itemWidget = new GraphListItemWidget(graph);
    
    connect(itemWidget, &GraphListItemWidget::downloadRequested,
            this, &GraphListWidget::graphDownloadRequested);
    connect(itemWidget, &GraphListItemWidget::analyzeRequested,
            this, &GraphListWidget::graphAnalyzeRequested);
    
    item->setSizeHint(itemWidget->sizeHint());
    addItem(item);
    setItemWidget(item, itemWidget);
}

void GraphListWidget::updateGraphStatus(const QString& graphName, bool isDownloaded) {
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem* item = this->item(i);
        GraphListItemWidget* widget = qobject_cast<GraphListItemWidget*>(itemWidget(item));
        if (widget && QString::fromStdString(widget->getGraphInfo().name) == graphName) {
            widget->setDownloaded(isDownloaded);
            break;
        }
    }
}

GraphInfo* GraphListWidget::getSelectedGraph() {
    QListWidgetItem* item = currentItem();
    if (!item) return nullptr;
    
    GraphListItemWidget* widget = qobject_cast<GraphListItemWidget*>(itemWidget(item));
    if (widget) {
        return const_cast<GraphInfo*>(&widget->getGraphInfo());
    }
    return nullptr;
}
