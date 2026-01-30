struct GraphInfo {
    std::string name;
    std::string displayName;
    std::string url;          // SNAP download URL
    std::string filename;     // Local filename
    bool isDownloaded;        // Is it already on disk?
    size_t fileSize;          // Size for progress bar
};