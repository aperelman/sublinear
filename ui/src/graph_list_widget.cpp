#include "graph_list_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>

// Implementation of the Constructor (Fixed LNK2019 for GraphListWidget::GraphListWidget)
GraphListWidget::GraphListWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    currentDirectory = findDefaultGraphDirectory();
    if (!currentDirectory.isEmpty()) {
        loadGraphsFromDirectory(currentDirectory);
    }
}

// Implementation of UI setup and signal connections
void GraphListWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* filterLayout = new QHBoxLayout();

    filterEdit = new QLineEdit();
    filterEdit->setPlaceholderText("Filter graphs...");
    browseButton = new QPushButton("Browse");
    refreshButton = new QPushButton("Refresh");

    filterLayout->addWidget(filterEdit);
    filterLayout->addWidget(browseButton);
    filterLayout->addWidget(refreshButton);

    graphList = new QListWidget();
    statsLabel = new QLabel("Select a graph to see details");
    statsLabel->setWordWrap(true);

    layout->addLayout(filterLayout);
    layout->addWidget(graphList);
    layout->addWidget(statsLabel);

    connect(filterEdit, &QLineEdit::textChanged, this, &GraphListWidget::onFilterTextChanged);
    connect(browseButton, &QPushButton::clicked, this, &GraphListWidget::onBrowseClicked);
    connect(refreshButton, &QPushButton::clicked, this, &GraphListWidget::onRefreshClicked);
    connect(graphList, &QListWidget::itemSelectionChanged, this, &GraphListWidget::onSelectionChanged);
    connect(graphList, &QListWidget::itemDoubleClicked, this, &GraphListWidget::onItemDoubleClicked);
}

QString GraphListWidget::findDefaultGraphDirectory() {
    return QDir::currentPath();
}

// Fixed LNK2019 for currentGraph
GraphInfo GraphListWidget::currentGraph() const {
    int row = graphList->currentRow();
    if (row >= 0 && row < filteredGraphs.size()) {
        return filteredGraphs[row];
    }
    return GraphInfo();
}

// Fixed LNK2019 for hasSelection
bool GraphListWidget::hasSelection() const {
    return graphList->currentRow() >= 0;
}

void GraphListWidget::loadGraphsFromDirectory(const QString& dirPath) {
    currentDirectory = dirPath;
    graphs.clear();
    QDir dir(dirPath);
    if (!dir.exists()) return;

    QStringList filters = {"*.txt", "*.edges", "*.graphml"};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);

    for (const auto& fileInfo : files) {
        GraphInfo info;
        info.name = fileInfo.baseName();
        info.filename = fileInfo.fileName();
        info.localPath = fileInfo.absoluteFilePath();
        info.fileSizeString = QString::number(fileInfo.size() / 1024) + " KB";
        info.format = fileInfo.suffix().toUpper();
        graphs.append(info);
    }
    updateGraphList();
}

void GraphListWidget::updateGraphList() {
    graphList->clear();
    filteredGraphs.clear();
    QString filter = filterEdit->text().toLower();
    
    for (const auto& graph : graphs) {
        if (filter.isEmpty() || graph.name.toLower().contains(filter)) {
            filteredGraphs.append(graph);
            QString displayText = QString("%1 (%2)").arg(graph.name).arg(graph.fileSizeString);
            auto* item = new QListWidgetItem(displayText);
            graphList->addItem(item);
        }
    }
}

// Fixed LNK2019 for Slot implementations
void GraphListWidget::onSelectionChanged() {
    if (!hasSelection()) return;
    GraphInfo graph = currentGraph();
    QString info = QString("<b>%1</b><br>Size: %2<br>Format: %3")
        .arg(graph.name).arg(graph.fileSizeString).arg(graph.format);
    statsLabel->setText(info);
    emit graphSelected(graph);
}

void GraphListWidget::onItemDoubleClicked(QListWidgetItem* item) {
    Q_UNUSED(item);
    if (hasSelection()) {
        emit graphDoubleClicked(currentGraph());
    }
}

void GraphListWidget::onBrowseClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Graph Directory", currentDirectory);
    if (!dir.isEmpty()) {
        loadGraphsFromDirectory(dir);
    }
}

void GraphListWidget::onRefreshClicked() {
    if (!currentDirectory.isEmpty()) {
        loadGraphsFromDirectory(currentDirectory);
    }
}

void GraphListWidget::onFilterTextChanged(const QString& text) {
    Q_UNUSED(text);
    updateGraphList();
}