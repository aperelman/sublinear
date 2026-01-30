#include "snap_dataset_cache.h"
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>

QString SnapDatasetCache::getCachePath() {
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir dir(cacheDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return cacheDir + "/snap_datasets.json";
}

bool SnapDatasetCache::cacheExists() {
    return QFile::exists(getCachePath());
}

QDateTime SnapDatasetCache::getCacheTimestamp() {
    QFileInfo info(getCachePath());
    return info.lastModified();
}

bool SnapDatasetCache::saveToCache(const QVector<SnapDataset>& datasets) {
    QJsonDocument doc = datasetsToJson(datasets);
    
    QFile file(getCachePath());
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    return true;
}

QVector<SnapDataset> SnapDatasetCache::loadFromCache() {
    QFile file(getCachePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return loadBuiltInSnapshot();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    return jsonToDatasets(doc);
}

QVector<SnapDataset> SnapDatasetCache::loadBuiltInSnapshot() {
    // Built-in snapshot from SNAP website (as of Jan 2026)
    return {
        // Social Networks
        {
            "Facebook Combined",
            "Facebook social circles from survey participants",
            "https://snap.stanford.edu/data/facebook_combined.txt.gz",
            "facebook_combined.txt",
            4039, 88234,
            "Social Networks"
        },
        {
            "Enron Email",
            "Email communication network from Enron",
            "https://snap.stanford.edu/data/email-Enron.txt.gz",
            "email-Enron.txt",
            36692, 183831,
            "Social Networks"
        },
        {
            "Wiki-Vote",
            "Wikipedia voting network",
            "https://snap.stanford.edu/data/wiki-Vote.txt.gz",
            "wiki-Vote.txt",
            7115, 103689,
            "Social Networks"
        },
        {
            "Slashdot",
            "Slashdot social network (Feb 2009)",
            "https://snap.stanford.edu/data/soc-Slashdot0902.txt.gz",
            "soc-Slashdot0902.txt",
            82168, 948464,
            "Social Networks"
        },
        {
            "Epinions",
            "Epinions social network",
            "https://snap.stanford.edu/data/soc-Epinions1.txt.gz",
            "soc-Epinions1.txt",
            75879, 508837,
            "Social Networks"
        },
        
        // Collaboration Networks
        {
            "CA-GrQc",
            "General Relativity collaboration network",
            "https://snap.stanford.edu/data/ca-GrQc.txt.gz",
            "ca-GrQc.txt",
            5242, 14496,
            "Collaboration"
        },
        {
            "CA-HepTh",
            "High Energy Physics Theory collaboration",
            "https://snap.stanford.edu/data/ca-HepTh.txt.gz",
            "ca-HepTh.txt",
            9877, 25998,
            "Collaboration"
        },
        {
            "CA-HepPh",
            "High Energy Physics Phenomenology collaboration",
            "https://snap.stanford.edu/data/ca-HepPh.txt.gz",
            "ca-HepPh.txt",
            12008, 118521,
            "Collaboration"
        },
        {
            "CA-CondMat",
            "Condensed Matter Physics collaboration",
            "https://snap.stanford.edu/data/ca-CondMat.txt.gz",
            "ca-CondMat.txt",
            23133, 93497,
            "Collaboration"
        },
        
        // Web Graphs
        {
            "Web-Google",
            "Web graph from Google",
            "https://snap.stanford.edu/data/web-Google.txt.gz",
            "web-Google.txt",
            875713, 5105039,
            "Web Graphs"
        },
        {
            "Web-Stanford",
            "Web graph of Stanford.edu",
            "https://snap.stanford.edu/data/web-Stanford.txt.gz",
            "web-Stanford.txt",
            281903, 2312497,
            "Web Graphs"
        },
        
        // Citation Networks
        {
            "Cit-HepTh",
            "High Energy Physics citation network",
            "https://snap.stanford.edu/data/cit-HepTh.txt.gz",
            "cit-HepTh.txt",
            27770, 352807,
            "Citation"
        },
        {
            "Cit-HepPh",
            "High Energy Physics Phenomenology citations",
            "https://snap.stanford.edu/data/cit-HepPh.txt.gz",
            "cit-HepPh.txt",
            34546, 421578,
            "Citation"
        },
        
        // Road Networks
        {
            "Roadnet-PA",
            "Pennsylvania road network",
            "https://snap.stanford.edu/data/roadNet-PA.txt.gz",
            "roadNet-PA.txt",
            1088092, 1541898,
            "Road Networks"
        },
        {
            "Roadnet-TX",
            "Texas road network",
            "https://snap.stanford.edu/data/roadNet-TX.txt.gz",
            "roadNet-TX.txt",
            1379917, 1921660,
            "Road Networks"
        },
        {
            "Roadnet-CA",
            "California road network",
            "https://snap.stanford.edu/data/roadNet-CA.txt.gz",
            "roadNet-CA.txt",
            1965206, 2766607,
            "Road Networks"
        },
        
        // Amazon Networks
        {
            "Amazon-0302",
            "Amazon product co-purchasing network (March 2003)",
            "https://snap.stanford.edu/data/amazon0302.txt.gz",
            "amazon0302.txt",
            262111, 1234877,
            "Product Networks"
        },
        {
            "Amazon-0601",
            "Amazon product co-purchasing network (June 2003)",
            "https://snap.stanford.edu/data/amazon0601.txt.gz",
            "amazon0601.txt",
            403394, 3387388,
            "Product Networks"
        },
        
        // Small test graphs
        {
            "Karate Club",
            "Zachary's Karate Club network",
            "https://snap.stanford.edu/data/karate.txt",
            "karate.txt",
            34, 78,
            "Small Graphs"
        },
        {
            "Dolphins",
            "Dolphin social network",
            "https://snap.stanford.edu/data/dolphins.txt",
            "dolphins.txt",
            62, 159,
            "Small Graphs"
        }
    };
}

QJsonDocument SnapDatasetCache::datasetsToJson(const QVector<SnapDataset>& datasets) {
    QJsonArray array;
    
    for (const auto& ds : datasets) {
        QJsonObject obj;
        obj["name"] = ds.name;
        obj["description"] = ds.description;
        obj["url"] = ds.url;
        obj["filename"] = ds.filename;
        obj["nodes"] = ds.nodes;
        obj["edges"] = ds.edges;
        obj["category"] = ds.category;
        array.append(obj);
    }
    
    QJsonObject root;
    root["datasets"] = array;
    root["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["version"] = "1.0";
    
    return QJsonDocument(root);
}

QVector<SnapDataset> SnapDatasetCache::jsonToDatasets(const QJsonDocument& doc) {
    QVector<SnapDataset> datasets;
    
    QJsonArray array = doc.object()["datasets"].toArray();
    
    for (const auto& val : array) {
        QJsonObject obj = val.toObject();
        
        SnapDataset ds;
        ds.name = obj["name"].toString();
        ds.description = obj["description"].toString();
        ds.url = obj["url"].toString();
        ds.filename = obj["filename"].toString();
        ds.nodes = obj["nodes"].toInt();
        ds.edges = obj["edges"].toInt();
        ds.category = obj["category"].toString();
        
        datasets.append(ds);
    }
    
    return datasets;
}
