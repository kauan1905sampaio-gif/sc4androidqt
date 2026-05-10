#pragma once
#include <QMainWindow>
#include "ScFile.h"

class QListWidget;
class QLabel;
class QTabWidget;
class ViewerWidget;
class EditorWidget;
class PreviewWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
private slots:
    void onOpenFile();
    void onSaveFile();
    void onCombineFile();
    void onExportSelected(int exportId);
private:
    void applyDarkTheme();
    void loadFile(const QString &path);
    void populateViewer();
    QAction *m_actOpen, *m_actSave, *m_actCombine;
    QTabWidget  *m_tabs;
    ViewerWidget  *m_viewer;
    EditorWidget  *m_editor;
    PreviewWidget *m_preview;
    ScFile m_currentFile;
    bool   m_fileLoaded = false;
};
