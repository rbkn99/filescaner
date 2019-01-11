#include <utility>

#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include <vector>
#include <algorithm>
#include <QDir>
#include <QDebug>
#include <QThread>
#include "my_file.h"

using std::string;
using std::vector;

class scanner: public QObject {
    Q_OBJECT

    QDir dir;
    vector<my_file> files;
    vector<size_t> clusters_volumes;
    int current_progress;
    bool cancel_state;

    void init();
    void get_files();
    void sort_files();
    void find_duplicates();


    void update_progress(size_t i, size_t overall_size);

public:
    void scan(QDir const& dir);
    void delete_files(const vector<QString> &del_files);

    size_t get_cluster_volume(size_t cluster) {
        if (cluster < 0 || cluster >= clusters_volumes.size())
            return 0;
        return clusters_volumes[cluster];
    }

    size_t clusters_count() {
        return clusters_volumes.size();
    }

public slots:
    void cancel();

signals:
    void exception_occurred(const QString &message);
    void info_message(const QString& message);
    void progress_updated(int value);
    void return_results(const vector<my_file>& files);
};

#endif // SCANNER_H
