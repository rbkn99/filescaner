#include <QtCore/QDirIterator>
#include <QtCore/QCryptographicHash>
#include "scanner.h"


void scanner::get_files() {
    files.clear();
    QDirIterator it(dir.path(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (cancel_state) return;
        it.next();
        QString rel_path = dir.relativeFilePath(it.fileInfo().path()) + "/";
        if (rel_path == "./") {
            rel_path = "";
        }
        files.emplace_back(rel_path + it.fileInfo().fileName(), it.fileInfo().absoluteFilePath(), it.fileInfo().size());
    }
    std::sort(files.begin(), files.end(), [this] (my_file& a, my_file& b) {
        return a.size() < b.size();
    });
}

void scanner::update_progress(size_t i, size_t overall_size) {
    int progress = int((i / (double)overall_size) * 100);
    if (progress > current_progress) {
        current_progress = progress;
        emit progress_updated(progress);
    }
}

void scanner::sort_files() {
    size_t end_i;
    for (size_t i = 0; i < files.size() - 1; i = end_i) {
        if (cancel_state) return;
        end_i = i + 1;
        while (end_i < files.size() && files[i].size() == files[end_i].size()) {
            if (cancel_state) return;
            try {
                files[end_i++].hash();
            }
            catch (const std::runtime_error& e) {
                QString message = (QString)e.what() + " " + files[end_i - 1].path();
                emit exception_occurred(message);
            }
        }
        if (end_i - i > 1) {
            try {
                files[i].hash();
            }
            catch (const std::runtime_error& e) {
                QString message = (QString)e.what() + " " + files[i].path();
                emit exception_occurred(message);
            }
            std::sort(files.begin() + i, files.begin() + end_i, [this] (my_file& a, my_file& b) {
                return a.hash() < b.hash();
            });
        }
        update_progress(i, files.size());
    }
    update_progress(files.size(), files.size());
}

void scanner::find_duplicates() {
    size_t i = 0;
    // files which we couldn't open have empty hash and cluster = 0
    for (; i < files.size() && files[i].hash().isEmpty(); i++);
    clusters_volumes.push_back(i);
    if (i != files.size() && files[i].has_hash()) {
        files[i].set_cluster(1);
        clusters_volumes.push_back(1);
        i++;
    }
    for (; i < files.size(); ++i) {
        if (cancel_state) return;
        if (files[i].has_hash() && files[i - 1].has_hash() && files[i].hash() == files[i - 1].hash()) {
            files[i].set_cluster(files[i - 1].cluster());
            clusters_volumes.back()++;
        }
        else {
            if (!files[i].has_hash()) {
                files[i].set_cluster(0);
                clusters_volumes[0]++;
            }
            else if (i < files.size() - 1 && files[i + 1].has_hash() && files[i].hash() == files[i + 1].hash()) {
                files[i].set_cluster(clusters_volumes.size());
                clusters_volumes.push_back(1);
            }
        }
    }
    std::sort(files.begin(), files.end(), [this] (my_file& a, my_file& b) {
        if (a.size() < b.size()) return false;
        if (a.size() == b.size()) return a.cluster() > b.cluster();
        return true;
    });
}

void scanner::init() {
    current_progress = 0;
    files.clear();
    clusters_volumes.clear();
    cancel_state = false;
}

void scanner::scan(QDir const& dir) {
    if (!dir.exists()) {
        throw std::invalid_argument("Root directory does not exist");
    }
    this->dir = dir;

    emit info_message("Scanning is started...");
    init();
    emit info_message("Collecting information about files...");
    get_files();
    emit info_message("Total number of files: " + QString::number(files.size()));
    emit info_message("Getting hashes of the same files. It can take some time.");
    sort_files();
    emit info_message("Almost done! Just marking the same files");
    find_duplicates();
    emit info_message("Scanning is finished.");
    emit return_results(files);
}

void scanner::delete_files(const vector<QString> &del_files) {
    current_progress = 0;
    cancel_state = false;

    emit info_message("Deleting has been started...");
    for (size_t i = 0; i < del_files.size(); i++) {
        if (cancel_state) return;
        if (!QFile::remove(dir.absolutePath() + "/" + del_files[i])) {
            emit exception_occurred("Cannot delete " + del_files[i]);
        }
        else {
            for (my_file& f: files) {
                if (f.path() == del_files[i]) {
                    f.set_deleted_state(true);
                    clusters_volumes[f.cluster()]--;
                    break;
                }
            }
        }
        update_progress(i, del_files.size());
    }
    update_progress(del_files.size(), del_files.size());
    emit info_message("Deleting has been finished...");
    emit return_results(files);
}

void scanner::cancel() {
    cancel_state = true;
}