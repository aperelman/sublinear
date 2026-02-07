#include "graph_list_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

GraphListWidget::GraphListWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    QString defaultDir = findDefaultGraphDirectory();
    if (!defaultDir.isEmpty()) {
        loadGraphsFromDirectory(defaultDir);
    } else {
        // Show message if no graphs found
        statsLabel->setText(
            "<b>No graphs found!</b><br><br>"
            "Please:<br>"
            "1. Click 'Browse...' to select a folder with graph files (.txt, .edges, .graphml)<br>"
            "2. Or create graphs in: ~/src/sublinear/data/snap/"
        );
    }
}

void GraphListWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* headerLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel("<b>SNAP Graph Browser</b>");
    titleLabel->setStyleSheet("font-size: 14pt;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    browseButton = new QPushButton("Browse...");
    refreshButton = new QPushButton("Refresh");
    headerLayout->addWidget(browseButton);
    headerLayout->addWidget(refreshButton);
    mainLayout->addLayout(headerLayout);
    
    auto* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("Filter:"));
    filterEdit = new QLineEdit();
    filterEdit->setPlaceholderText("Search graphs...");
    filterLayout->addWidget(filterEdit);
    mainLayout->addLayout(filterLayout);
    
    auto* listGroup = new QGroupBox("Available Graphs");
    auto* listLayout = new QVBoxLayout(listGroup);
    
    graphList = new QListWidget();
    graphList->setAlternatingRowColors(true);
    listLayout->addWidget(graphList);
    
    statsLabel = new QLabel("Select a graph");
    statsLabel->setStyleSheet("padding: 10px; background-color: #f0f0f0; border: 1px solid #ccc;");
    statsLabel->setWordWrap(true);
    listLayout->addWidget(statsLabel);
    
    mainLayout->addWidget(listGroup);
    
    connect(graphList, &QListWidget::itemSelectionChanged,
            this, &GraphListWidget::onSelectionChanged);
    connect(graphList, &QListWidget::itemDoubleClicked,
            this, &GraphListWidget::onItemDoubleClicked);
    connect(browseButton, &QPushButton::clicked,
            this, &GraphListWidget::onBrowseClicked);
    connect(refreshButton, &QPushButton::clicked,
            this, &GraphListWidget::onRefreshClicked);
    connect(filterEdit, &QLineEdit::textChanged,
            this, &GraphListWidget::onFilterTextChanged);
}

QString GraphListWidget::findDefaultGraphDirectory() {
    QStringList paths = {
        "../../data/snap",
        "../data/snap",
        "./data/snap",
        QDir::homePath() + "/src/sublinear/data/snap"
    };
    
    for (const auto& path : paths) {
        QDir dir(path);
        if (dir.exists()) {
            QStringList filters;
            filters << "*.txt" << "*.edges" << "*.graphml";
            QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
            if (!files.isEmpty()) {
                return dir.absolutePath();
            }
        }
    }
    return QString();
}

void GraphListWidget::loadGraphsFromDirectory(const QString& dirPath) {
    currentDirectory = dirPath;
    graphs.clear();
    
    QDir dir(dirPath);
    if (!dir.exists()) {
        statsLabel->setText(
            "<b>Directory not found!</b><br><br>" +
            dirPath +
            "<br><br>Click 'Browse...' to select a valid directory."
        );
        return;
    }
    
    QStringList filters;
    filters << "*.txt" << "*.edges" << "*.graphml";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);
    
    if (files.isEmpty()) {
        statsLabel->setText(
            "<b>No graph files found!</b><br><br>"
            "Directory: " + dirPath + "<br><br>"
            "Looking for: *.txt, *.edges, *.graphml files<br><br>"
            "Click 'Browse...' to select a different directory."
        );
        return;
    }
    
    for (const auto& fileInfo : files) {
        graphs.append(GraphInfo(fileInfo.absoluteFilePath()));
    }
    
    updateGraphList();
    statsLabel->setText(QString("Found %1 graphs in: %2").arg(graphs.size()).arg(currentDirectory));
}

void GraphListWidget::updateGraphList() {
    graphList->clear();
    filteredGraphs.clear();
    
    QString filter = filterEdit->text().toLower();
    
    for (const auto& graph : graphs) {
        if (filter.isEmpty() || graph.name.toLower().contains(filter)) {
            filteredGraphs.append(graph);
            QString displayText = QString("%1 (%2)").arg(graph.name).arg(graph.fileSizeString());
            auto* item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, QVariant::fromValue(graph.filename));
            graphList->addItem(item);
        }
    }
    
    if (filteredGraphs.isEmpty() && !graphs.isEmpty()) {
        statsLabel->setText("No graphs match the filter");
    }
}

void GraphListWidget::onSelectionChanged() {
    if (!hasSelection()) return;
    GraphInfo graph = currentGraph();
    QString info = QString("<b>%1</b><br>Size: %2<br>Format: %3<br>Path: %4")
        .arg(graph.name).arg(graph.fileSizeString()).arg(graph.format).arg(graph.filename);
    statsLabel->setText(info);
    emit graphSelected(graph);
}

void GraphListWidget::onItemDoubleClicked(QListWidgetItem* item) {
    if (item) {
        GraphInfo graph = currentGraph();
        emit graphDoubleClicked(graph);
    }
}

void GraphListWidget::onBrowseClicked() {
    QString dir = QFileDialog::getExistingDirectory(
        this, "Select Graph Directory",
        currentDirectory.isEmpty() ? QDir::homePath() : currentDirectory
    );
    
    if (!dir.isEmpty()) {
        loadGraphsFromDirectory(dir);
    }
}

void GraphListWidget::onRefreshClicked() {
    if (!currentDirectory.isEmpty()) {
        loadGraphsFromDirectory(currentDirectory);
    } else {
        // Try to find default directory again
        QString defaultDir = findDefaultGraphDirectory();
        if (!defaultDir.isEmpty()) {
            loadGraphsFromDirectory(defaultDir);
        } else {
            QMessageBox::information(this, "No Directory",
                "No graph directory set. Click 'Browse...' to select one.");
        }
    }
}

void GraphListWidget::onFilterTextChanged(const QString& text) {
    updateGraphList();
}

GraphInfo GraphListWidget::currentGraph() const {
    auto* item = graphList->currentItem();
    if (!item) {
        return GraphInfo();
    }
    
    QString filename = item->data(Qt::UserRole).toString();
    
    for (const auto& graph : filteredGraphs) {
        if (graph.filename == filename) {
            return graph;
        }
    }
    
    return GraphInfo();
}

bool GraphListWidget::hasSelection() const {
    return graphList->currentItem() != nullptr;
}
