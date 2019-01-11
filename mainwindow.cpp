#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QLabel>
#include <QDebug>
#include <QtWidgets/QTreeWidgetItem>
#include <QtConcurrent/QtConcurrent>
#include <QMetaType>

main_window::main_window(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    QCommonStyle style;
    ui->actionScan_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));
    ui->progressBar->setRange(0, 100);

    clear_gui();

    connect(ui->actionScan_Directory, &QAction::triggered, this, &main_window::select_directory);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::show_about_dialog);
    connect(ui->deleteButton, SIGNAL(clicked()), this, SLOT(delete_files()));
    connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(cancel_clicked()));
    connect(this, SIGNAL(cancel_thread()), &s, SLOT(cancel()));
    connect(this, SIGNAL(exception_occurred(
                                 const QString&)),
            this, SLOT(log_error(
                               const QString&)));

    connect(&s, SIGNAL(exception_occurred(
                               const QString&)),
            this, SLOT(log_error(
                               const QString&)));
    connect(&s, SIGNAL(info_message(
                               const QString&)),
            this, SLOT(log_info(
                               const QString&)));
    connect(&s, SIGNAL(progress_updated(int)),
            this, SLOT(update_progress_bar(int)));
    qRegisterMetaType<vector<my_file>>("vector<my_file>");
    connect(&s, SIGNAL(return_results(
                               const vector<my_file> & )),
            this, SLOT(print_results(
                               const vector<my_file> & )));

    //scan_directory(QDir::homePath());
}

main_window::~main_window() {}

void main_window::select_directory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                    QString(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    scan_directory(dir);
}

void main_window::clear_layout(QLayout *layout) {
    if (!layout)
        return;
    while (auto item = layout->takeAt(0)) {
        delete item->widget();
        clear_layout(item->layout());
    }
}

void main_window::clear_gui() {
    ui->treeWidget->clear();
    clear_layout(ui->verticalLayout);
    ui->progressBar->setValue(0);
    ui->deleteButton->setEnabled(false);
}

void main_window::scan_directory(QString const &dir) {
    emit cancel_thread();
    clear_gui();
    try {
        setWindowTitle(QString("Directory Content - %1").arg(dir));

        QFuture<void> future = QtConcurrent::run(&s, &scanner::scan, dir);
    }
    catch (const std::invalid_argument &e) {
        emit exception_occurred(e.what());
    }
}

void main_window::print_results(const vector<my_file> &files) {
    ui->treeWidget->clear();
    size_t i = 0;
    for (; i < files.size();) {
        if (files[i].cluster() == 0 || s.get_cluster_volume(files[i].cluster()) < 2 || files[i].is_deleted()) {
            i++;
            continue;
        }
        auto *subdir = new QTreeWidgetItem(ui->treeWidget);
        subdir->setText(0, "Cluster " + QString::number(files[i].cluster()));
        ui->treeWidget->addTopLevelItem(subdir);

        qint64 total_cluster_size = 0;
        while (i < files.size()) {
            auto *item = new QTreeWidgetItem(subdir);
            item->setText(0, files[i].path());
            total_cluster_size += files[i].size();
            item->setText(1, convert_to_readable_size(files[i].size()));
            item->setCheckState(2, Qt::Unchecked);
            i++;
            if (i < files.size() && files[i].cluster() != files[i - 1].cluster()) break;
        }
        subdir->setText(1, convert_to_readable_size(total_cluster_size));
    }
    ui->deleteButton->setEnabled(true);
}

void main_window::update_progress_bar(int value) {
    ui->progressBar->setValue(value);
}

void main_window::delete_files() {
    vector<QString> files_for_deleting;
    QTreeWidgetItemIterator it(ui->treeWidget);
    while (*it) {
        if ((*it)->checkState(2) == Qt::Checked) {
            files_for_deleting.push_back((*it)->text(0));
        }
        ++it;
    }
    ui->progressBar->setValue(0);
    emit cancel_thread();
    QFuture<void> future = QtConcurrent::run(&s, &scanner::delete_files, files_for_deleting);
}

QString main_window::convert_to_readable_size(qint64 file_size) {
    QString abbr[] = {"B", "KB", "MB", "GB", "TB"};
    auto db_size = (double) file_size;
    int i = 0;
    while (db_size / 1024 >= 1) {
        db_size /= 1024;
        i++;
    }
    db_size = int(db_size * 100) / 100.0;
    return QString::number(db_size) + " " + abbr[i];
}

void main_window::cancel_clicked() {
    emit cancel_thread();
}

void main_window::log_error(const QString &message) {
    auto label = new QLabel(message);
    label->setStyleSheet("QLabel { color : red; }");
    ui->verticalLayout->addWidget(label);
}

void main_window::log_info(const QString &message) {
    ui->verticalLayout->addWidget(new QLabel(message));
}

void main_window::show_about_dialog() {
    QMessageBox::aboutQt(this);
}
