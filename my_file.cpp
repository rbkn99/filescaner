//
// Created by rbkn99 on 01.01.19.
//

#include <QtCore/QFile>
#include <QtCore/QCryptographicHash>
#include "my_file.h"

my_file::my_file(const QString &path, const QString& absolute_path, qint64 size) :
        _path(path), _absolute_path(absolute_path), _size(size), _hash(""), _has_hash(false), _cluster(0), _is_deleted(false) {
}

QString my_file::hash() {
    if (_has_hash) {
        return _hash;
    }
    _has_hash = true;
    QFile f(_absolute_path);
    if (f.open(QFile::ReadOnly)) {
        QCryptographicHash hash(QCryptographicHash::Algorithm::Md5);
        if (hash.addData(&f)) {
            _hash = hash.result();
            return _hash;
        }
        throw std::runtime_error("Cannot read the file");
    }
    throw std::runtime_error("Cannot open the file");
}