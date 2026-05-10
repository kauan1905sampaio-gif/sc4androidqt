#include "MainWindow.h"
#include "ViewerWidget.h"
#include "EditorWidget.h"
#include "PreviewWidget.h"
#include "ScParser.h"
#include "ScCombiner.h"
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTabWidget>
#include <QFile>
#include <QApplication>
#include <QPalette>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("SC Editor");
    setMinimumSize(400, 600);
    applyDarkTheme();

    // ── Toolbar ──
    QToolBar *tb = addToolBar("Main");
    tb->setMovable(false);
    m_actOpen    = tb->addAction("📂 Open",    this, &MainWindow::onOpenFile);
    m_actSave    = tb->addAction("💾 Save",    this, &MainWindow::onSaveFile);
    m_actCombine = tb->addAction("🔀 Combine", this, &MainWindow::onCombineFile);
    m_actSave->setEnabled(false);
    m_actCombine->setEnabled(false);

    // ── Tabs ──
    m_tabs    = new QTabWidget(this);
    m_viewer  = new ViewerWidget(this);
    m_editor  = new EditorWidget(this);
    m_preview = new PreviewWidget(this);

    m_tabs->addTab(m_viewer,  "Viewer");
    m_tabs->addTab(m_editor,  "Editor");
    m_tabs->addTab(m_preview, "Preview");
    setCentralWidget(m_tabs);

    connect(m_viewer, &ViewerWidget::exportSelected,
            this, &MainWindow::onExportSelected);
}

void MainWindow::applyDarkTheme() {
    QPalette p;
    p.setColor(QPalette::Window,          QColor(0x0E,0x0E,0x14));
    p.setColor(QPalette::WindowText,      QColor(0xE8,0xE8,0xF0));
    p.setColor(QPalette::Base,            QColor(0x1A,0x1A,0x24));
    p.setColor(QPalette::AlternateBase,   QColor(0x22,0x22,0x2E));
    p.setColor(QPalette::Text,            QColor(0xE8,0xE8,0xF0));
    p.setColor(QPalette::Button,          QColor(0x22,0x22,0x2E));
    p.setColor(QPalette::ButtonText,      QColor(0xE8,0xE8,0xF0));
    p.setColor(QPalette::Highlight,       QColor(0xFF,0xD5,0x4F));
    p.setColor(QPalette::HighlightedText, QColor(0x0E,0x0E,0x14));
    p.setColor(QPalette::Link,            QColor(0x64,0xB5,0xF6));
    qApp->setPalette(p);
    qApp->setStyle("Fusion");
}

void MainWindow::onOpenFile() {
    QString path = QFileDialog::getOpenFileName(this, "Open SC File", "",
                   "SC Files (*.sc);;All Files (*)");
    if (path.isEmpty()) return;
    loadFile(path);
}

void MainWindow::loadFile(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Cannot open file: " + path);
        return;
    }
    QByteArray data = f.readAll();
    auto result = ScParser::parse(data, path);
    if (!result) {
        QMessageBox::critical(this, "Error", "Failed to parse SC file.\n"
            "Make sure the file is not LZMA/ZSTD/LZHAM compressed.");
        return;
    }
    m_currentFile = *result;
    m_fileLoaded = true;
    setWindowTitle("SC Editor — " + QFileInfo(path).fileName());
    m_viewer->setFile(m_currentFile);
    m_editor->setFile(m_currentFile);
    m_preview->setFile(m_currentFile);
    m_actSave->setEnabled(true);
    m_actCombine->setEnabled(true);
    m_tabs->setCurrentIndex(0);
}

void MainWindow::onSaveFile() {
    if (!m_fileLoaded) return;
    QString path = QFileDialog::getSaveFileName(this, "Save SC File", "",
                   "SC Files (*.sc);;All Files (*)");
    if (path.isEmpty()) return;
    QMessageBox::information(this, "Save",
        "Save functionality writes back uncompressed SC v3.\n"
        "Full binary writer coming in next update.");
}

void MainWindow::onCombineFile() {
    if (!m_fileLoaded) return;
    QString path = QFileDialog::getOpenFileName(this, "Open Source SC File", "",
                   "SC Files (*.sc);;All Files (*)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    auto src = ScParser::parse(f.readAll(), path);
    if (!src) { QMessageBox::critical(this, "Error", "Failed to parse source file."); return; }

    // Import all exports from source
    QVector<int> ids;
    for (const auto &exp : src->exports) ids.append(exp.id);
    m_currentFile = ScCombiner::combine(m_currentFile, *src, ids);
    m_viewer->setFile(m_currentFile);
    m_editor->setFile(m_currentFile);
    m_preview->setFile(m_currentFile);
    QMessageBox::information(this, "Done",
        QString("Imported %1 exports from source file.").arg(ids.size()));
}

void MainWindow::onExportSelected(int exportId) {
    m_editor->selectExport(exportId);
    m_preview->selectExport(exportId);
    m_tabs->setCurrentIndex(1); // switch to editor
}
