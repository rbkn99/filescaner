//
// Created by rbkn99 on 01.01.19.
//

#ifndef FILESCANER_MY_FILE_H
#define FILESCANER_MY_FILE_H


#include <QtCore/QString>

struct my_file {
    explicit my_file(const QString& path, const QString& absolute_path, qint64 size);

    QString path() const {
        return _path;
    }

    QString absolute_path() const {
        return _absolute_path;
    }

    qint64 size() const {
        return _size;
    }

    QString hash();

    bool has_hash() const {
        return _has_hash;
    }

    bool is_deleted() const {
        return _is_deleted;
    }

    size_t cluster() const {
        return _cluster;
    }

    void set_deleted_state(bool state) {
        _is_deleted = state;
    }

    void set_cluster(size_t _cluster) {
        this->_cluster = _cluster;
    }

private:
    QString _path;
    QString _absolute_path;
    qint64 _size;
    QString _hash;
    bool _has_hash;
    size_t _cluster;
    bool _is_deleted;
};

#endif //FILESCANER_MY_FILE_H
