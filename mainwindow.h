/**
 * @file mainwindow.h
 * @author Ayoub Gharbi & Mohamed Amine Lbabda
 * @brief Header file containing the functions to run the main application 
 * @version 0.1
 * @date 2026-03-26
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QSslSocket>
#include <QByteArray>
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMainWindow>
#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <QMessageBox>
#include <QCalendarWidget>
#include <QSqlQuery>
#include <QSqlError>
#include "material.h"
#include "geminiclient.h"

class QNetworkReply;

#include "employes.h"
#include <QTimer>
#include <QRandomGenerator64>
#include <QCryptographicHash>
#include <QSettings>
#include <QFileDialog>
#include <qt6keychain/keychain.h>
#include <QDate>
#include <QShortcut>
#include <QtConcurrent>
#include <QFile>
#include <QUuid>
#include <QChartView>
#include <QTextEdit>
using namespace std;
using namespace cv;
using namespace QKeychain;
using namespace QtConcurrent;
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// --- Fenêtre principale ---
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private slots:
    // Global navigation
    void on_GestionStock_clicked();
    void on_GestionReclamations_clicked();
    void on_GestionEmployes_clicked();
    void on_GestionProduits_clicked();
    void on_GestionMateriels_clicked();
    void on_GestionCommandes_clicked();

    // Auth / account
    void on_BtnLogin_clicked();
    void on_BtnLoginFace_clicked();
    void on_ConnectionLink_linkActivated(const QString &link);
    void on_ConnectionLink_2_linkActivated(const QString &link);

    // Material management
    void on_btnAdd_2_clicked();
    void on_btnCancel_2_clicked();
    void on_btnSave_2_clicked();
    void on_import_f_clicked();
    void on_modifier_m_clicked();
    void on_supprimer_m_clicked();
    void on_btnResetFilters_2_clicked();
    void on_export_2_clicked();
    void on_btnStatsMachines_clicked();
    void on_btnRefreshStats_clicked();
    void on_retour_clicked();
    void on_retour_2_clicked();
    void on_gen_rapport_clicked();

    // AI analysis (materials)
    void on_fen_ai_clicked();
    void on_gen_rapport_AI_clicked();

    // Employee management
    void on_AjoutEmploye_clicked();
    void on_SupprimerEmploye_clicked();
    void on_tableView_doubleClicked(const QModelIndex &index);
    void on_InscriptionEmploye_clicked();
    void on_ExportEmploye_clicked();
    void on_ImportEmployes_clicked();
    void onTokenRead(QKeychain::Job* job);
    void on_DeconnecterUtilisateur_clicked();
    void on_RechercheEmployeBtn_clicked();
    void on_AjoutDialog_clicked();
    void on_TrierEmploye_clicked();
    void on_ModifierDIalog_clicked();
    void on_tableView_clicked(const QModelIndex &index);
    void on_StatistiquesEmploye_clicked();
    void refreshStatistics();
    void on_EnregistrerStat_clicked();
    void on_StatSuiv_clicked();
    void on_StatPrev_clicked();

    // Gemini callbacks
    void onGeminiAnalysisComplete(int score, const QString &risk, const QString &comment);
    void onGeminiAnalysisFailed(const QString &error);

signals:
    void employeeAdded();
    void employeedeleted();

private:
    // Material helpers
    void showMaterialActionDetails();
    void setupCalendar(QCalendarWidget *calendar);
    void loadMachines();
    void loadLast5Machines();
    void clearMachineForm();
    void setEditingMachineId(int id);

    // Material stats/report widgets
    int m_editingMaterialId;
    QChartView *m_statChartMachinesParAnnee = nullptr;
    QChartView *m_statChartMachinesParAtelier = nullptr;
    QChartView *m_statChartFrequenceUtilisation = nullptr;
    QChartView *m_statChartEtatSante = nullptr;
    QTextEdit *m_rapportTextEdit = nullptr;

    // Shared UI state
    // UI and employee management
    Ui::MainWindow *ui = nullptr;

    // Employee domain state/helpers
    Employes Etmp;
    VideoCapture cap;
    Ptr<FaceDetectorYN> detector;
    Ptr<FaceRecognizerSF> recognizer;
    vector<FaceTemplate> registry;
    QString detPath = "/home/amine/Desktop/WoodSync-OCI/face_detection_yunet_2023mar.onnx";
    QString recPath = "/home/amine/Desktop/WoodSync-OCI/face_recognition_sface_2021dec.onnx";
    const string WINNAME="Enregistrement biométrique WoodSync";
    int currentId = -1;

    void populateComboBox();
    void persistSessionUser(int userId);
    void showFrameAsDialog();
    void configureTableViews();
    void bindEmployeeTableModel(QSqlQueryModel* model);
    bool refreshEmployeeTable();
    bool refreshTable(QSqlQueryModel* model);
    void generateChart();
    void generateChartRepartition();
    void generatePie();
    void loadFaceRegistry();
    bool verifForm(const QString& nom,const QString& prenom,const QDate& dateNaissance, const QString& pwd,const QString& pwd_confirmation);
    bool verifForm(const QString& nom,const QString& prenom,const QString& pwd);
    bool verifForm(const QString& nom,const QString& prenom,const QString& role,const QDate& date,const QString& tel,const double& heures);
    void applyAuthLayout(bool loggedIn);

    // AI analysis (materials)
    /** Page Analyse IA dans pageStatsMachines (AI_F). Index 9 = 10e écran après Login (0). */
    void openAiAnalysisPage();
    void saveAiAnalysisResults();
    void openEmailReportDialog();
    void reloadMachineStatisticsCharts();
    void loadAiMachinesTable();
    void setupAiAnalysisUi();
    void updateAiResultsPanel(int score, const QString &risk, const QString &comment);
    void clearAiResultsPanel();
    void updateAiTableRowForMaterial(int materialId, int score, const QString &risk, const QString &comment);
    Material fetchMachineDataForAi(int id);
    void setAiTableButtonsEnabled(bool enabled);
    void startAiAnalysisForMaterialId(int materialId);
    GeminiClient m_geminiClient;
    class QProgressBar *m_aiScoreBar = nullptr;
    class QLabel *m_aiScoreValue = nullptr;
    class QLabel *m_aiRiskBadge = nullptr;
    class QLabel *m_aiCommentLabel = nullptr;
    class QLabel *m_aiStatusLabel = nullptr;
    int m_aiPendingMaterialId = -1;
    int m_aiPendingScore = 0;
    QString m_aiPendingRisk;
    QString m_aiPendingComment;
    bool m_aiAnalysisBusy = false;
};

#endif // MAINWINDOW_H
