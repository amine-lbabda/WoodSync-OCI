/**
 * @file mainwindow.cpp
 * @author Mohamed Amine Lbabda & Ayoub Gharbi
 * @brief 
 * @version 0.1
 * @date 2026-03-26
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "geminiclient.h"
#include "emaildialog.h"
#include "employes.h"
#include <QDir>
#include <QFileInfo>
// EmailDialog implementation moved to emaildialog.h/emaildialog.cpp

// stackedwidget order in mainwindow.ui:
// 0 LoginPage, 1 CreateAccountPage, 2 GestionStockPage, 3 GestionEmployePage,
// 4 GestionCommandesPage, 5 GestionProduitsPage, 6 GestionReclamationsPage,
// 7 GestionMaterielPage, 8 AjouterMaterielPage, 9 StatistiquePAge, 10 AI_F.
static constexpr int kStackedIndexAiAnalysisPage = 10;
/**
 * @brief Structure pour structurer la réponse générer par OLLAMA
 * 
 */
struct PreventiveScoreVisual {
    QString text;
    QColor bg;
    QColor fg;
};

// Même règles que l’onglet IA : 0–30 vert, 31–60 orange, > 60 rouge ; sinon neutre.
/**
 * @brief Setting up the UI for the animation
 * @param scoreVariant 
 * @return PreventiveScoreVisual 
 */
static PreventiveScoreVisual preventiveScoreVisual(const QVariant &scoreVariant)
{
    PreventiveScoreVisual r;
    r.bg = QColor(QStringLiteral("#f7f6f2"));
    r.fg = QColor(QStringLiteral("#2a2a2a"));
    if (scoreVariant.isNull()) {
        r.text = QStringLiteral("-");
        return r;
    }
    bool ok = false;
    int s = scoreVariant.toInt(&ok);
    if (!ok)
        s = static_cast<int>(scoreVariant.toDouble(&ok));
    if (ok) {
        s = qBound(0, s, 100);
        r.text = QString::number(s);
        if (s <= 30) {
            r.bg = QColor(QStringLiteral("#d4efe4"));
            r.fg = QColor(QStringLiteral("#2a6b5a"));
        } else if (s <= 60) {
            r.bg = QColor(QStringLiteral("#f2d9a8"));
            r.fg = QColor(QStringLiteral("#4a3518"));
        } else {
            r.bg = QColor(QStringLiteral("#f0c8c8"));
            r.fg = QColor(QStringLiteral("#4a2222"));
        }
    } else {
        r.text = scoreVariant.toString().trimmed();
        if (r.text.isEmpty())
            r.text = QStringLiteral("-");
    }
    return r;
}
/**
 * @brief Application des styles 
 * 
 * @param item 
 * @param scoreVariant 
 */
static void styleTableItemForPreventiveScore(QTableWidgetItem *item, const QVariant &scoreVariant)
{
    if (!item)
        return;
    const PreventiveScoreVisual p = preventiveScoreVisual(scoreVariant);
    item->setText(p.text);
    item->setBackground(p.bg);
    item->setForeground(p.fg);
}

struct AiScoreUiStyle {
    QString progressBarSheet;
    QString valueLabelSheet;
    QString riskBadgeSheet;
};
/**
 * @brief Styles de l'UI
 * 
 * @return AiScoreUiStyle 
 */
static AiScoreUiStyle aiScoreUiStyleNeutral()
{
    const QString bar = QStringLiteral(
        "QProgressBar { border: 2px solid rgba(127, 85, 57, 0.28); border-radius: 10px; height: 28px; "
        "background: #f7f6f2; color: #2a2a2a; font-weight: 600; font-size: 12px; }"
        "QProgressBar::chunk { background-color: #d2d6d1; border-radius: 8px; }");
    const QString val = QStringLiteral("color: #2a2a2a; font-weight: bold; font-size: 18px; padding: 6px 0;");
    const QString badge = QStringLiteral(
        "QLabel { background-color: #e8e9e6; color: #2a2a2a; padding: 10px 20px; border-radius: 14px; "
        "font-weight: 700; font-size: 14px; border: 1px solid rgba(0, 0, 0, 0.06); }");
    return { bar, val, badge };
}
// Score préventif : 0–30 vert doux, 31–60 orange doux, > 60 rouge doux (texte foncé pour le contraste).
/**
 * @brief Styles UI
 * 
 * @param score 
 * @return AiScoreUiStyle 
 */
static AiScoreUiStyle aiScoreUiStyleForPreventiveScore(int score)
{
    const int s = qBound(0, score, 100);
    QString chunk;
    QString text;
    if (s <= 30) {
        chunk = QStringLiteral("#d4efe4");
        text = QStringLiteral("#2a6b5a");
    } else if (s <= 60) {
        chunk = QStringLiteral("#f2d9a8");
        text = QStringLiteral("#4a3518");
    } else {
        chunk = QStringLiteral("#f0c8c8");
        text = QStringLiteral("#4a2222");
    }
    const QString bar = QStringLiteral(
        "QProgressBar { border: 2px solid rgba(127, 85, 57, 0.28); border-radius: 10px; height: 28px; "
        "background: #f7f6f2; color: %2; font-weight: 600; font-size: 12px; }"
        "QProgressBar::chunk { background-color: %1; border-radius: 8px; }")
        .arg(chunk, text);
    const QString val = QStringLiteral("color: %1; font-weight: bold; font-size: 18px; padding: 6px 0;").arg(text);
    const QString badge = QStringLiteral(
        "QLabel { background-color: %1; color: %2; padding: 10px 20px; border-radius: 14px; "
        "font-weight: 700; font-size: 14px; border: 1px solid rgba(0, 0, 0, 0.07); }")
        .arg(chunk, text);
    return { bar, val, badge };
}
/**
 * @brief Styles UI
 * 
 * @param bar 
 * @param scoreVal 
 * @param riskBadge 
 * @param score 
 * @param neutral 
 */
static void applyPreventiveScoreAppearance(QProgressBar *bar, QLabel *scoreVal, QLabel *riskBadge, int score,
                                           bool neutral)
{
    const AiScoreUiStyle st = neutral ? aiScoreUiStyleNeutral() : aiScoreUiStyleForPreventiveScore(score);
    if (bar)
        bar->setStyleSheet(st.progressBarSheet);
    if (scoreVal)
        scoreVal->setStyleSheet(st.valueLabelSheet);
    if (riskBadge)
        riskBadge->setStyleSheet(st.riskBadgeSheet);
}

// Oracle : INDICERISQUEPANNE est souvent un NUMBER (code). L'IA renvoie un libellé français → ORA-01722 si on lie du texte.
// Adapter les codes si votre schéma CHECK utilise une autre échelle.
/**
 * @brief Utility function
 * 
 * @param risk 
 * @return int 
 */
static int riskPanneLabelToOracleCode(const QString &risk)
{
    const QString r = risk.trimmed().toLower();
    if (r.contains(QStringLiteral("faible")))
        return 1;
    if (r.contains(QStringLiteral("moyen")))
        return 2;
    if (r.contains(QStringLiteral("critique")))
        return 4;
    if (r.contains(QStringLiteral("élev")) || r.contains(QStringLiteral("elev")))
        return 3;
    return 2;
}
/**
 * @brief Utility function
 * 
 * @param v 
 * @return QString 
 */
static QString riskPanneDbValueToLabel(const QVariant &v)
{
    if (v.isNull())
        return QString();
    bool ok = false;
    const int c = v.toInt(&ok);
    if (!ok)
        return v.toString();
    switch (c) {
    case 1:
        return QStringLiteral("Faible");
    case 2:
        return QStringLiteral("Moyen");
    case 3:
        return QStringLiteral("Élevé");
    case 4:
        return QStringLiteral("Critique");
    default:
        return QString::number(c);
    }
}

static QString resolveFaceModelPathForWindows(const QString &fileName)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir::cleanPath(appDir + "/" + fileName),
        QDir::cleanPath(appDir + "/../" + fileName),
        QDir::cleanPath(appDir + "/../../" + fileName),
        QDir::cleanPath(appDir + "/../../../" + fileName),
        QDir::cleanPath(QDir::currentPath() + "/" + fileName)
    };
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path))
            return path;
    }
    return candidates.first();
}

#include "employes.h"
/**
 * @brief Construct a new Main Window:: Main Window object
 * 
 * @param parent 
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_editingMaterialId(-1)
    , ui(new Ui::MainWindow)
    , m_geminiClient(this)
{
    qDebug() << "[CONSTRUCTOR] MainWindow constructor started";
    loadFaceRegistry();

#ifdef Q_OS_WIN
    detPath = resolveFaceModelPathForWindows("face_detection_yunet_2023mar.onnx");
    recPath = resolveFaceModelPathForWindows("face_recognition_sface_2021dec.onnx");
#else
    detPath = "/home/amine/Desktop/WoodSync-OCI/face_detection_yunet_2023mar.onnx";
    recPath = "/home/amine/Desktop/WoodSync-OCI/face_recognition_sface_2021dec.onnx";
#endif

    ignore = run([this](){
        detector = FaceDetectorYN::create(detPath.toStdString(), "", Size(640,480), 0.9f, 0.3f, 5000, DNN_BACKEND_CUDA, DNN_TARGET_CUDA);
        recognizer = FaceRecognizerSF::create(recPath.toStdString(), "", DNN_BACKEND_CUDA, DNN_TARGET_CUDA);
    });

    ui->setupUi(this);

    ui->stackedwidget->setCurrentIndex(0);
    ui->tableWidget_3->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableMaterials->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableMaterials->horizontalHeader()->setMinimumSectionSize(110);
    ui->tableMaterials->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableMaterials->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableMaterials->setColumnHidden(0, true); // ID auto-incrementé, pas affiché

    // Initialize report TextEdit early
    if (ui->rapport_f) {
        m_rapportTextEdit = new QTextEdit(ui->rapport_f);
        m_rapportTextEdit->setObjectName("texte_rapport");
        m_rapportTextEdit->setReadOnly(true);
        m_rapportTextEdit->setPlaceholderText(tr("Sélectionnez une machine dans le tableau puis cliquez sur \"Generer\" pour afficher le rapport."));
        m_rapportTextEdit->setStyleSheet("QTextEdit { font-family: 'Roboto Condensed'; font-size: 13px; padding: 8px; border: 1px solid #7F5539; border-radius: 6px; background: #fdfefe; }");
        QVBoxLayout *rapportLayout = new QVBoxLayout(ui->rapport_f);
        rapportLayout->setContentsMargins(4, 4, 4, 4);
        rapportLayout->addWidget(m_rapportTextEdit);
    }

    // Initialize material filters at startup (not only after employee stats are opened).
    ui->comboStatus_2->clear();
    ui->comboStatus_2->addItem("Tous les status");
    ui->comboStatus_2->addItem("Bien");
    ui->comboStatus_2->addItem("Moyenne");
    ui->comboStatus_2->addItem("Mauvaise");
    ui->comboStatus_2->addItem("Critique");

    connect(ui->searchBar_2, &QLineEdit::textChanged, this, [this](const QString &) {
        loadMachines();
    });
    connect(ui->comboWorkshop_2, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        loadMachines();
    });
    connect(ui->comboStatus_2, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        loadMachines();
    });
    connect(ui->stackedwidget, &QStackedWidget::currentChanged, this, [this](int index) {
        if (!QSqlDatabase::database().isOpen())
            return;
        if (index == 7)
            loadMachines();
        else if (index == 8)
            loadLast5Machines();
        else if (index == kStackedIndexAiAnalysisPage)
            loadAiMachinesTable();
    });

    QShortcut* rechercherShortCut = new QShortcut(QKeySequence(Qt::Key_Return),this);
    rechercherShortCut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(rechercherShortCut,&QShortcut::activated,this,[this](){
        on_RechercheEmployeBtn_clicked();
    });
    connect(this, &MainWindow::employeeAdded, this,[this](){
        refreshEmployeeTable();
        populateComboBox();
        ui->FrameAjout->close();
        if (ui->Stat->scene()) {
            generateChart();
            generatePie();
            generateChartRepartition();
        }
    });
    connect(this,&MainWindow::employeedeleted,this,[this](){
        refreshEmployeeTable();
        if (ui->Stat->scene()) {
            generateChart();
            generatePie();
            generateChartRepartition();
        }
    });
    populateComboBox();
    ReadPasswordJob* job = new ReadPasswordJob("WoodSync",this);
    job->setKey("session_token");
    connect(job,&ReadPasswordJob::finished,this,&MainWindow::onTokenRead);
    job->start();
    ui->DateNaissance->setDate(QDate::currentDate());
    ui->DateRecrutementEmploye->setDate(QDate::currentDate());
    QList<QPushButton*> allButtons = this->findChildren<QPushButton*>();
    for (QList<QPushButton*>::Iterator it=allButtons.begin();it != allButtons.end();++it) {
        (*it)->setCursor(Qt::PointingHandCursor);
    }
    QList<QDateEdit*> allDates = this->findChildren<QDateEdit*>();
    for (QList<QDateEdit*>::Iterator it=allDates.begin();it != allDates.end();++it) {
        (*it)->setCalendarPopup(true);
        (*it)->setDisplayFormat("yyyy-MM-dd");
        setupCalendar((*it)->calendarWidget());
    }
    if (ui->Stats) {
        ui->Stats->setCurrentIndex(0);
    }
    ui->stackedwidget->setCurrentIndex(0);
    ui->DeconnecterUtilisateur->hide();
    ui->FrameAjout->hide();
    ui->tableView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    configureTableViews();
    QList<QLabel*> labels = this->findChildren<QLabel*>();
    for (QList<QLabel*>::Iterator it = labels.begin(); it != labels.end(); ++it) {
        QLabel* label = *it;
        if (label->objectName().startsWith("Erreur_")) {
            label->hide();
        }
    }
    ui->RoleEmploye->addItem("Séléctionner le rôle","");
    ui->RoleEmploye->setCurrentText("Séléctionnez le rôle");
    ui->DeconnecterUtilisateur->setCheckable(false);
    ui->DeconnecterUtilisateur->setAutoExclusive(false);

}
/**
 * @brief Configuration des tables
 *
 */
void MainWindow::configureTableViews()
{
    if (ui->tableWidget_3 && ui->tableWidget_3->horizontalHeader()) {
        ui->tableWidget_3->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }
    if (ui->tableWidget_2 && ui->tableWidget_2->horizontalHeader()) {
        ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }
    if (ui->tableview && ui->tableview->horizontalHeader()) {
        ui->tableview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }
}
/**
 * @brief Configuration du table des employés 
 * 
 * @param model 
 */
void MainWindow::bindEmployeeTableModel(QSqlQueryModel *model)
{
    if (!ui->tableView) return;

    if (!model) {
        ui->tableView->setModel(nullptr);
        return;
    }

    ui->tableView->setModel(model);
    if (QHeaderView* header = ui->tableView->horizontalHeader()) {
        header->setSectionResizeMode(QHeaderView::Stretch);
        header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    }
    if (QHeaderView* vertical = ui->tableView->verticalHeader()) {
        vertical->setVisible(false);
    }

}
/**
 * @brief Rafraîchir le table
 * 
 * @return true 
 * @return false 
 */
bool MainWindow::refreshEmployeeTable()
{
    QSqlQueryModel* model = Etmp.afficher();
    if (!model) return false;
    bindEmployeeTableModel(model);
    return true;
}
/**
 * @brief Générer des statististiques
 * 
 */
void MainWindow::generateChart()
{
    QChart* chart = Etmp.genererStatistiquesHeures();
    const QColor chartBg(0x414833);

    if(ui->Stat->scene()) delete ui->Stat->scene();

    QGraphicsScene* scene = new QGraphicsScene(this);
    ui->Stat->setScene(scene);
    scene->setBackgroundBrush(QBrush(chartBg));
    ui->Stat->setBackgroundBrush(QBrush(chartBg));


    const QRectF viewRect = ui->Stat->viewport()->rect();
    const qreal outerMargin = 2.0;
    scene->setSceneRect(viewRect);
    chart->resize(viewRect.width() - (outerMargin * 2.0), viewRect.height() - (outerMargin * 2.0));
    chart->setPos(outerMargin, outerMargin);
    scene->addItem(chart);

    ui->Stat->setRenderHint(QPainter::Antialiasing);
    ui->Stat->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->Stat->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);


    ui->Stat->setFrameShape(QFrame::NoFrame);
}
/**
 * @brief Générer du graphique du répartition
 * 
 */
void MainWindow::generateChartRepartition()
{
    QChart* chart = Etmp.genererStatistiquesRepartition();
    const QColor chartBg(0x414833);
    if (ui->Repartition->scene()) {
        delete ui->Repartition->scene();
    }

    QGraphicsScene* scene = new QGraphicsScene(this);
    ui->Repartition->setScene(scene);
    scene->setBackgroundBrush(QBrush(chartBg));
    ui->Repartition->setBackgroundBrush(QBrush(chartBg));
    const QRectF viewRect = ui->Repartition->viewport()->rect();
    const qreal outerMargin = 2.0;
    scene->setSceneRect(viewRect);
    chart->resize(viewRect.width() - (outerMargin * 2.0), viewRect.height() - (outerMargin * 2.0));
    chart->setPos(outerMargin, outerMargin);
    scene->addItem(chart);

    ui->Repartition->setRenderHint(QPainter::Antialiasing);
    ui->Repartition->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->Repartition->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->Repartition->setFrameShape(QFrame::NoFrame);
}
/**
 * @brief Générer du graphique du répartition
 * 
 */
void MainWindow::generatePie()
{
    QChart* chart = Etmp.genererStatistiquesNaissances();
    const QColor chartBg(0x414833);
    if(ui->Pie->scene()) delete ui->Pie->scene();

    QGraphicsScene* scene = new QGraphicsScene(this);
    ui->Pie->setScene(scene);
    scene->setBackgroundBrush(QBrush(chartBg));
    ui->Pie->setBackgroundBrush(QBrush(chartBg));
    const QRectF viewRect = ui->Pie->viewport()->rect();
    const qreal outerMargin = 2.0;
    scene->setSceneRect(viewRect);
    chart->resize(viewRect.width() - (outerMargin * 2.0), viewRect.height() - (outerMargin * 2.0));
    chart->setPos(outerMargin, outerMargin);
    scene->addItem(chart);

    // Material filters/signals are initialized in constructor.
    // btnResetFilters_2, export_2 : connectSlotsByName uniquement.
    // env_rapport : connexion explicite (évite double exécution comme pour modif_ai).

    // Bouton "Générer" du cadre rapport : objectName gen_rapport ≠ BtnAjouter_27 → connexion explicite requise.
    connect(ui->env_rapport, &QPushButton::clicked, this, &MainWindow::openEmailReportDialog);

    // Zone d'affichage du rapport dans rapport_f — déjà initialisée dans le constructeur.

    // Charte graphique du formulaire matériels (même style que les autres formulaires)
    const QString formCharte(
        "QWidget#page_materiel_form { background-color: #EDE0D4; }"
        "QLabel { color: rgb(0, 51, 102); font-family: 'Roboto Condensed'; font-size: 14px; font-weight: 500; }"
        "QLineEdit, QTextEdit, QComboBox, QSpinBox, QDateEdit {"
        "  font-family: 'Roboto Condensed'; font-size: 14px;"
        "  border: 2px solid #7F5539; border-radius: 6px;"
        "  background: #f9f9f9; selection-background-color: #d4af37; }"
        "QLineEdit:focus, QTextEdit:focus, QComboBox:focus, QSpinBox:focus, QDateEdit:focus {"
        "  border: 2px solid #4169E1; background: #ffffff; }"
        "QPushButton#btnSave_2, QPushButton#btnCancel_2 {"
        "  background-color: #414833; color: #E5EEF5; font-weight: bold; font-size: 14px;"
        "  font-family: 'Roboto Condensed'; border: 2px solid #b8860b; border-radius: 8px; }"
        "QPushButton#btnSave_2:hover, QPushButton#btnCancel_2:hover {"
        "  background-color: #656D4A; border: 2px solid #656D4A; }"
    );
    QWidget *pageForm = ui->stackedwidget->widget(8);
    if (pageForm) {
        pageForm->setObjectName("page_materiel_form");
        pageForm->setStyleSheet(formCharte);
    }
    ui->btnSave_2->setStyleSheet(
        "QPushButton { background-color: #414833; color: #E5EEF5; font-weight: bold; font-size: 14px;"
        " font-family: 'Roboto Condensed'; border: 2px solid #b8860b; border-radius: 8px; }"
        "QPushButton:hover { background-color: #656D4A; border: 2px solid #656D4A; }"
    );
    ui->btnCancel_2->setStyleSheet(
        "QPushButton { background-color: #414833; color: #E5EEF5; font-weight: bold; font-size: 14px;"
        " font-family: 'Roboto Condensed'; border: 2px solid #b8860b; border-radius: 8px; }"
        "QPushButton:hover { background-color: #656D4A; border: 2px solid #656D4A; }"
    );

    // ID auto-incrémenté en base : masquer le champ ID du formulaire (ajout / modification)
    ui->lblMaterialID_2->setVisible(false);
    ui->txtMaterialID_2->setVisible(false);
    ui->txtMaterialID_2->setEnabled(false);

    // Graphiques de la page StatistiquePAge
    {
        m_statChartMachinesParAnnee = new QChartView(ui->frameStatMachinesParAnnee);
        m_statChartMachinesParAnnee->setRenderHint(QPainter::Antialiasing);
        auto *layout1 = new QVBoxLayout(ui->frameStatMachinesParAnnee);
        layout1->setContentsMargins(10, 10, 10, 10);
        layout1->addWidget(m_statChartMachinesParAnnee);

        m_statChartMachinesParAtelier = new QChartView(ui->frameStatMachinesParAtelier);
        m_statChartMachinesParAtelier->setRenderHint(QPainter::Antialiasing);
        auto *layout2 = new QVBoxLayout(ui->frameStatMachinesParAtelier);
        layout2->setContentsMargins(10, 10, 10, 10);
        layout2->addWidget(m_statChartMachinesParAtelier);

        m_statChartFrequenceUtilisation = new QChartView(ui->frameStatFrequenceUtilisation);
        m_statChartFrequenceUtilisation->setRenderHint(QPainter::Antialiasing);
        auto *layout3 = new QVBoxLayout(ui->frameStatFrequenceUtilisation);
        layout3->setContentsMargins(10, 10, 10, 10);
        layout3->addWidget(m_statChartFrequenceUtilisation);

        m_statChartEtatSante = new QChartView(ui->frameStatEtatSante);
        m_statChartEtatSante->setRenderHint(QPainter::Antialiasing);
        auto *layout4 = new QVBoxLayout(ui->frameStatEtatSante);
        layout4->setContentsMargins(10, 10, 10, 10);
        layout4->addWidget(m_statChartEtatSante);
    }

    ui->StatistiquePAge->setStyleSheet(
        QStringLiteral(
            "#StatistiquePAge {"
            "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
            "    stop:0 #faf7f2, stop:0.5 #f2e8dc, stop:1 #e5d9cc);"
            "}"
            "QLabel#lblStatsTitle_m {"
            "  color: #352a22;"
            "  font-family: \"Roboto Condensed\";"
            "  font-size: 26px;"
            "  font-weight: 700;"
            "}"
            "QLabel#lblStatsHint {"
            "  color: #6b5d50;"
            "  font-family: \"Roboto Condensed\";"
            "  font-size: 12px;"
            "}"
            "QFrame#frameStatMachinesParAnnee, QFrame#frameStatMachinesParAtelier,"
            "QFrame#frameStatFrequenceUtilisation, QFrame#frameStatEtatSante {"
            "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #c9b49a, stop:1 #a68a64);"
            "  border: 1px solid rgba(90, 70, 55, 0.35);"
            "  border-radius: 16px;"
            "}"
            "QPushButton#btnRefreshStats {"
            "  background-color: #5B7F66;"
            "  color: #fdfefd;"
            "  font-weight: bold;"
            "  font-size: 13px;"
            "  border-radius: 14px;"
            "  padding: 8px 18px;"
            "  font-family: \"Roboto Condensed\";"
            "  border: 2px solid #4a6b54;"
            "}"
            "QPushButton#btnRefreshStats:hover { background-color: #4d6e56; border-color: #3d5a46; }"));

    // Page Analyse IA (kStackedIndexAiAnalysisPage : AI_F)
    ui->tableMaterials_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableMaterials_2->horizontalHeader()->setMinimumSectionSize(80);
    ui->tableMaterials_2->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableMaterials_2->setSelectionMode(QAbstractItemView::SingleSelection);
    // Note: Signal connections for m_geminiClient are set up in openAiAnalysisPage()
    // when the AI analysis page is first opened, not here in the constructor
    // export_3, gen_rapport_AI : connectSlotsByName uniquement.
    // modif_ai connection is set in openAiAnalysisPage() to guarantee hookup
    // when the AI page is actually opened.
    ui->modif_ai->setEnabled(false);
    ui->Pie->setRenderHint(QPainter::Antialiasing);
    ui->Pie->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->Pie->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->Pie->setFrameShape(QFrame::NoFrame);
}
/**
 * @brief Rafraîchir des statistiques
 * 
 */
void MainWindow::refreshStatistics()
{
    generateChart();
    generatePie();
}
/**
 * @brief Préparation des styles des calendriers
 * 
 * @param calendar 
 */
void MainWindow::setupCalendar(QCalendarWidget *calendar) {
    if (!calendar) return;

    // 1. Force the Palette (The "Mechanical" fix)
    QPalette pal = calendar->palette();
    pal.setColor(QPalette::Window, Qt::white);      // Background
    pal.setColor(QPalette::WindowText, Qt::black);  // Text
    pal.setColor(QPalette::Base, Qt::white);        // Grid Background
    pal.setColor(QPalette::Text, Qt::black);        // Date Numbers
    pal.setColor(QPalette::Button, Qt::white);      // Header Buttons
    pal.setColor(QPalette::ButtonText, Qt::black);  // Header Text
    pal.setColor(QPalette::Highlight, QColor(0xd4af37)); // Selection Gold (matches input fields)
    pal.setColor(QPalette::HighlightedText, Qt::black);

    calendar->setPalette(pal);

    // 2. Force the internal view to follow the palette
    if (QAbstractItemView* view = calendar->findChild<QAbstractItemView*>()) {
        view->setPalette(pal);
    }

    // 3. Apply widget-local style to enforce compatibility
    calendar->setStyleSheet("background-color: white; color: black; border: 1px solid #ccc;");
}
/**
 * @brief Destroy the Main Window:: Main Window object
 * 
 */
MainWindow::~MainWindow()
{
    delete ui;
    if (cap.isOpened()) {
        cap.release();
        destroyAllWindows();
    }

}

// =========================
// Material Domain Functions
// =========================
/**
 * @brief Navigation vers la Gestion des stocks 
 * 
 */
void MainWindow::on_GestionStock_clicked()
{
    if (currentId != -1) {
        ui->stackedwidget->setCurrentIndex(2);
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Vous n'êtes pas autorisé !"));
    }

}

/**
 * @brief Navigation vers la Gestion des avis 
 * 
 */
void MainWindow::on_GestionReclamations_clicked()
{
    if (currentId != -1) {
        ui->stackedwidget->setCurrentIndex(6);
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Vous n'êtes pas autorisé !"));
    }
}

/**
 * @brief Navigation vers @see employes.cpp
 * 
 */
void MainWindow::on_GestionEmployes_clicked()
{
    if (currentId != -1) {
        ui->stackedwidget->setCurrentIndex(3);
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Vous n'êtes pas autorisé !"));
    }

}


void MainWindow::on_GestionProduits_clicked()
{
    if (currentId != -1) {
        ui->stackedwidget->setCurrentIndex(4);
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Vous n'êtes pas autorisé !"));
    }

}


void MainWindow::on_GestionMateriels_clicked()
{
    if (currentId != -1) {
        ui->stackedwidget->setCurrentIndex(7);
        loadMachines();
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Vous n'êtes pas autorisé !"));
    }

}


void MainWindow::on_GestionCommandes_clicked()
{
    if (currentId != -1) {
        ui->stackedwidget->setCurrentIndex(5);
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Vous n'êtes pas autorisé !"));
    }

}


void MainWindow::on_btnAdd_2_clicked()
{
    setEditingMachineId(-1);
    clearMachineForm();
    loadLast5Machines();
    ui->stackedwidget->setCurrentIndex(8);
}


void MainWindow::on_btnCancel_2_clicked()
{
    ui->stackedwidget->setCurrentIndex(7);
    loadMachines();
}


void MainWindow::setEditingMachineId(int id)
{
    m_editingMaterialId = id;
}

void MainWindow::clearMachineForm()
{
    ui->txtMaterialName_2->clear();
    ui->txtDescription_2->clear();
    ui->comboWorkshop_4->setCurrentIndex(0);
    ui->comboStatus_4->setCurrentIndex(0);
    ui->spinQuantity_2->setValue(1);
    ui->datePurchase_2->setDate(QDate::currentDate());
    ui->comboHealthStatus_2->setCurrentIndex(0);
    ui->spinNbIncidents_2->setValue(0);
    ui->spinFrequenceUtilisation_2->setValue(0.0);
}

void MainWindow::loadMachines()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui->tableMaterials->setUpdatesEnabled(false);
    ui->tableMaterials->setSortingEnabled(false);
    ui->tableMaterials->setRowCount(0);
    ui->tableMaterials->setColumnCount(14);
    ui->tableMaterials->setHorizontalHeaderLabels(QStringList()
        << tr("ID") << tr("Nom") << tr("Description") << tr("Atelier") << tr("Quantité") << tr("État santé")
        << tr("Date dernier entretien") << tr("Date achat") << tr("Fréq. utilisation") << tr("Nb incidents")
        << tr("Score préventif") << tr("Indice risque") << tr("Commentaires") << tr("Actions"));

    const QString searchName = ui->searchBar_2->text().trimmed();
    const QString workshopFilter = ui->comboWorkshop_2->currentText().trimmed();
    const QString statusFilter = ui->comboStatus_2->currentText().trimmed();

    QString fetchError;
    const QList<Material> materials = Material::fetchAll(
        searchName,
        ui->comboWorkshop_2->currentIndex() > 0 ? workshopFilter : QString(),
        ui->comboStatus_2->currentIndex() > 0 ? statusFilter : QString(),
        &fetchError);
    if (!fetchError.isEmpty()) {
        QApplication::restoreOverrideCursor();
        ui->tableMaterials->setUpdatesEnabled(true);
        QMessageBox::critical(nullptr, tr("Erreur"), tr("Impossible de charger les machines: ") + fetchError);
        return;
    }

    int row = 0;
    for (const Material &mat : materials) {
        ui->tableMaterials->insertRow(row);
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(mat.idMateriel()));
        idItem->setData(Qt::UserRole, mat.idMateriel()); // pour Modifier / Supprimer même si colonne masquée
        ui->tableMaterials->setItem(row, 0, idItem);
        ui->tableMaterials->setItem(row, 1, new QTableWidgetItem(mat.nomMateriel()));
        ui->tableMaterials->setItem(row, 2, new QTableWidgetItem(mat.description()));
        ui->tableMaterials->setItem(row, 3, new QTableWidgetItem(mat.atelier()));
        ui->tableMaterials->setItem(row, 4, new QTableWidgetItem(QString::number(mat.quantite())));
        ui->tableMaterials->setItem(row, 5, new QTableWidgetItem(mat.etatSante()));
        ui->tableMaterials->setItem(row, 6, new QTableWidgetItem(mat.dateDernierEntretien().isValid() ? mat.dateDernierEntretien().toString(Qt::ISODate) : QString()));
        ui->tableMaterials->setItem(row, 7, new QTableWidgetItem(mat.dateAchat().isValid() ? mat.dateAchat().toString(Qt::ISODate) : QString()));
        ui->tableMaterials->setItem(row, 8, new QTableWidgetItem(QString::number(mat.frequenceUtilisation())));
        ui->tableMaterials->setItem(row, 9, new QTableWidgetItem(QString::number(mat.nombreIncidents())));
        auto *scoreItem = new QTableWidgetItem();
        styleTableItemForPreventiveScore(scoreItem, mat.scorePreventif() >= 0 ? QVariant(mat.scorePreventif()) : QVariant());
        ui->tableMaterials->setItem(row, 10, scoreItem);
        ui->tableMaterials->setItem(row, 11, new QTableWidgetItem(riskPanneDbValueToLabel(mat.indiceRisquePanne() >= 0 ? QVariant(mat.indiceRisquePanne()) : QVariant())));
        ui->tableMaterials->setItem(row, 12, new QTableWidgetItem(mat.commentaire()));

        QPushButton *detailsBtn = new QPushButton(tr("Détails"));
        detailsBtn->setProperty("materialId", mat.idMateriel());
        detailsBtn->setStyleSheet("QPushButton { background-color: #414833; color: #FFFFFF; border-radius: 8px; padding: 4px 10px; font-weight: 600; }"
                                  "QPushButton:hover { background-color: #656D4A; }");
        connect(detailsBtn, &QPushButton::clicked, this, &MainWindow::showMaterialActionDetails);
        ui->tableMaterials->setCellWidget(row, 13, detailsBtn); // Actions column
        row++;

        if (row % 100 == 0) {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    }

    ui->tableMaterials->setUpdatesEnabled(true);
    QApplication::restoreOverrideCursor();
}

void MainWindow::loadLast5Machines()
{
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setColumnCount(5);
    ui->tableWidget->setHorizontalHeaderLabels(QStringList()
        << tr("Nom") << tr("Atelier") << tr("Status") << tr("Date d'achat") << tr("Quantité"));
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QString fetchError;
    const QList<Material> latest = Material::fetchLatest(5, &fetchError);
    if (!fetchError.isEmpty()) {
        qWarning() << "loadLast5Machines:" << fetchError;
        QMessageBox::warning(nullptr, tr("Données"),
                             tr("Impossible de charger les dernières machines : %1").arg(fetchError));
        return;
    }

    int row = 0;
    for (const Material &mat : latest) {
        ui->tableWidget->insertRow(row);
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(mat.nomMateriel()));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(mat.atelier()));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(mat.etatSante()));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(mat.dateAchat().isValid() ? mat.dateAchat().toString(Qt::ISODate) : QString()));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::number(mat.quantite())));
        row++;
    }
}

void MainWindow::on_btnResetFilters_2_clicked()
{
    ui->searchBar_2->clear();
    ui->comboWorkshop_2->setCurrentIndex(0);
    ui->comboStatus_2->setCurrentIndex(0);
    loadMachines();
}

void MainWindow::showMaterialActionDetails()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    bool ok = false;
    int id = btn->property("materialId").toInt(&ok);
    if (!ok) return;

    Material mat;
    QString fetchError;
    if (!Material::fetchById(id, &mat, &fetchError)) {
        QMessageBox::critical(nullptr, tr("Erreur"), tr("Impossible de charger les détails: ") + fetchError);
        return;
    }

    // Fenêtre de détails avec couleurs douces (EFDECD, 9F8170, texte 3B3C36)
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Détails machine"));
    dialog.setModal(true);
    dialog.resize(500, 420);
    dialog.setStyleSheet(
        "QDialog { background-color: #EFDECD; }"
        "QLabel#detailTitle { color: #3B3C36; font-size: 18px; font-weight: 700; background-color: #F5F5DC; padding: 6px; border-radius: 4px; }"
        "QLabel#detailSubtitle { color: #3B3C36; font-size: 14px; font-weight: 600; background-color: #F5F5DC; padding: 6px; border-radius: 4px; }"
        "QLabel#detailLabel { color: #3B3C36; font-weight: 600; background-color: #F5F5DC; padding: 4px 6px; border-radius: 4px; }"
        "QLabel#detailValue { color: #3B3C36; background-color: #F5F5DC; padding: 4px 6px; border-radius: 4px; }"
        "QDialogButtonBox QPushButton {"
        "  background-color: #9F8170; color: #EFDECD; border: none; border-radius: 8px;"
        "  padding: 8px 20px; font-weight: 600; font-size: 13px; }"
        "QDialogButtonBox QPushButton:hover { background-color: #8B7355; }"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    QLabel *title = new QLabel(tr("Détails de la machine"));
    title->setObjectName("detailTitle");
    mainLayout->addWidget(title);

    QLabel *subtitle = new QLabel(mat.nomMateriel());
    subtitle->setObjectName("detailSubtitle");
    mainLayout->addWidget(subtitle);

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(10);
    grid->setColumnStretch(1, 1);

    auto addRow = [&grid](int r, const QString &labelText, const QString &valueText) {
        QLabel *lbl = new QLabel(labelText + " :");
        lbl->setObjectName("detailLabel");
        QLabel *val = new QLabel(valueText.isEmpty() ? "-" : valueText);
        val->setWordWrap(true);
        val->setObjectName("detailValue");
        grid->addWidget(lbl, r, 0, Qt::AlignRight | Qt::AlignTop);
        grid->addWidget(val, r, 1, Qt::AlignLeft | Qt::AlignTop);
    };

    int r = 0;
    addRow(r++, tr("Nom"), mat.nomMateriel());
    addRow(r++, tr("Description"), mat.description());
    addRow(r++, tr("Atelier"), mat.atelier());
    addRow(r++, tr("Quantité"), QString::number(mat.quantite()));
    addRow(r++, tr("État santé"), mat.etatSante());
    addRow(r++, tr("Date dernier entretien"), mat.dateDernierEntretien().isValid() ? mat.dateDernierEntretien().toString(Qt::ISODate) : QString());
    addRow(r++, tr("Date achat"), mat.dateAchat().isValid() ? mat.dateAchat().toString(Qt::ISODate) : QString());
    addRow(r++, tr("Fréquence utilisation"), QString::number(mat.frequenceUtilisation()));
    addRow(r++, tr("Nombre incidents"), QString::number(mat.nombreIncidents()));
    {
        const PreventiveScoreVisual pv = preventiveScoreVisual(mat.scorePreventif() >= 0 ? QVariant(mat.scorePreventif()) : QVariant());
        QLabel *lblSc = new QLabel(tr("Score préventif") + QStringLiteral(" :"));
        lblSc->setObjectName(QStringLiteral("detailLabel"));
        QLabel *valSc = new QLabel(pv.text);
        valSc->setWordWrap(true);
        valSc->setStyleSheet(
            QStringLiteral("QLabel { background-color: %1; color: %2; font-weight: 600; padding: 6px 8px; "
                           "border-radius: 6px; border: 1px solid rgba(0, 0, 0, 0.06); }")
                .arg(pv.bg.name(QColor::HexRgb), pv.fg.name(QColor::HexRgb)));
        grid->addWidget(lblSc, r, 0, Qt::AlignRight | Qt::AlignTop);
        grid->addWidget(valSc, r, 1, Qt::AlignLeft | Qt::AlignTop);
        ++r;
    }
    addRow(r++, tr("Indice risque panne"), riskPanneDbValueToLabel(mat.indiceRisquePanne() >= 0 ? QVariant(mat.indiceRisquePanne()) : QVariant()));
    addRow(r++, tr("Commentaires système"), mat.commentaire());

    mainLayout->addLayout(grid);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    mainLayout->addWidget(buttons, 0, Qt::AlignRight);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);

    dialog.exec();
}

void MainWindow::on_export_2_clicked()
{
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Exporter la liste des matériels"),
        "materiels.csv",
        tr("CSV Files (*.csv)"));

    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, tr("Erreur"), tr("Impossible d'écrire le fichier CSV."));
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    auto csvEscape = [](const QString &value) {
        QString v = value;
        v.replace('"', "\"\"");
        return QString("\"%1\"").arg(v);
    };

    QStringList headers;
    for (int c = 0; c < ui->tableMaterials->columnCount(); ++c) {
        if (c == 0) continue;   // Pas d'ID dans l'export
        if (c == 13) continue;  // Skip Actions column
        QTableWidgetItem *h = ui->tableMaterials->horizontalHeaderItem(c);
        headers << csvEscape(h ? h->text() : QString("Col%1").arg(c));
    }
    out << headers.join(',') << "\n";

    for (int r = 0; r < ui->tableMaterials->rowCount(); ++r) {
        QStringList row;
        for (int c = 0; c < ui->tableMaterials->columnCount(); ++c) {
            if (c == 0) continue;   // Pas d'ID dans l'export
            if (c == 13) continue;  // Skip Actions column
            QTableWidgetItem *item = ui->tableMaterials->item(r, c);
            row << csvEscape(item ? item->text() : QString());
        }
        out << row.join(',') << "\n";
    }

    file.close();
    QMessageBox::information(nullptr, tr("Succès"), tr("Export CSV terminé avec succès."));
}

void MainWindow::on_import_f_clicked()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Importer des matériels"), QString(),
                                                      tr("Fichiers CSV (*.csv)"));
    if (path.isEmpty())
        return;

    QString err;
    int n = 0;
    if (!Material::importFromCsvFile(path, &err, &n)) {
        QMessageBox::warning(nullptr, tr("Fichier non supportable"), err);
        return;
    }

    QMessageBox::information(nullptr, tr("Import"), tr("%1 ligne(s) importée(s).").arg(n));
    loadMachines();
    loadLast5Machines();
}

void MainWindow::on_btnSave_2_clicked()
{
    ui->btnSave_2->setEnabled(false);

    const QString nom = ui->txtMaterialName_2->text().trimmed();
    const QString atelier = ui->comboWorkshop_4->currentText().trimmed();
    const QString etatSante = ui->comboHealthStatus_2->currentText().trimmed();
    const QString description = ui->txtDescription_2->toPlainText().trimmed();
    const QDate dateAchat = ui->datePurchase_2->date();
    const QDate dateDernierEntretien = dateAchat;
    const double frequenceUtilisation = ui->spinFrequenceUtilisation_2->value();
    const int nbIncidents = ui->spinNbIncidents_2->value();

    if (nom.isEmpty()) {
        QMessageBox::warning(nullptr, tr("Champ requis"), tr("Veuillez saisir le nom du matériel."));
        ui->btnSave_2->setEnabled(true);
        return;
    }

    if (ui->comboWorkshop_4->currentIndex() <= 0 || atelier.isEmpty()) {
        QMessageBox::warning(nullptr, tr("Champ requis"), tr("Veuillez sélectionner un atelier."));
        ui->btnSave_2->setEnabled(true);
        return;
    }

    if (etatSante.isEmpty()) {
        QMessageBox::warning(nullptr, tr("Champ requis"), tr("Veuillez sélectionner l'état de santé."));
        ui->btnSave_2->setEnabled(true);
        return;
    }

    if (!dateDernierEntretien.isValid() || !dateAchat.isValid()) {
        QMessageBox::warning(nullptr, tr("Date invalide"), tr("Veuillez vérifier les dates (format yyyy-MM-dd)."));
        ui->btnSave_2->setEnabled(true);
        return;
    }

    Material material;
    material.setNomMateriel(nom);
    material.setDescription(description);
    material.setAtelier(atelier);
    material.setQuantite(ui->spinQuantity_2->value());
    material.setEtatSante(etatSante);
    material.setDateDernierEntretien(dateDernierEntretien);
    material.setDateAchat(dateAchat);
    material.setFrequenceUtilisation(frequenceUtilisation);
    material.setNombreIncidents(nbIncidents);

    if (m_editingMaterialId < 0) {
        QMessageBox::StandardButton btn = QMessageBox::question(nullptr, tr("Confirmation"),
            tr("Confirmer l'ajout de cette machine ?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (btn != QMessageBox::Yes) {
            ui->btnSave_2->setEnabled(true);
            return;
        }

        QApplication::setOverrideCursor(Qt::WaitCursor);

        QString dbError;
        if (!material.insert(&dbError)) {
            QMessageBox::critical(nullptr, tr("Erreur"), tr("Échec de l'ajout: ") + dbError);
            QApplication::restoreOverrideCursor();
            ui->btnSave_2->setEnabled(true);
            return;
        }
        QApplication::restoreOverrideCursor();
        QMessageBox::information(nullptr, tr("Succès"), tr("Machine ajoutée avec succès."));
    } else {
        QMessageBox::StandardButton btn = QMessageBox::question(nullptr, tr("Confirmation"),
            tr("Confirmez la modification de cette machine ?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (btn != QMessageBox::Yes) {
            ui->btnSave_2->setEnabled(true);
            return;
        }

        QApplication::setOverrideCursor(Qt::WaitCursor);

        material.setIdMateriel(m_editingMaterialId);
        QString dbError;
        if (!material.update(&dbError)) {
            QMessageBox::critical(nullptr, tr("Erreur"), tr("Échec de la modification: ") + dbError);
            QApplication::restoreOverrideCursor();
            ui->btnSave_2->setEnabled(true);
            return;
        }
        QApplication::restoreOverrideCursor();
        QMessageBox::information(nullptr, tr("Succès"), tr("Machine modifiée avec succès."));
    }

    clearMachineForm();
    setEditingMachineId(-1);
    ui->stackedwidget->setCurrentIndex(7);
    loadMachines();
    loadLast5Machines();
    ui->btnSave_2->setEnabled(true);
}

void MainWindow::on_modifier_m_clicked()
{
    int row = ui->tableMaterials->currentRow();
    if (row < 0) {
        QMessageBox::warning(nullptr, tr("Sélection"), tr("Veuillez sélectionner une ligne (machine) dans le tableau avant de cliquer Modifier."));
        return;
    }
    QTableWidgetItem *idItem = ui->tableMaterials->item(row, 0);
    if (!idItem) {
        QMessageBox::warning(nullptr, tr("Erreur"), tr("Ligne invalide."));
        return;
    }
    int id = idItem->data(Qt::UserRole).toInt();
    if (id <= 0) {
        bool ok = false;
        id = idItem->text().toInt(&ok);
        if (!ok || id <= 0) {
            QMessageBox::warning(nullptr, tr("Erreur"), tr("ID machine invalide."));
            return;
        }
    }
    setEditingMachineId(id);
    ui->txtMaterialName_2->setText(ui->tableMaterials->item(row, 1) ? ui->tableMaterials->item(row, 1)->text() : QString());
    ui->txtDescription_2->setPlainText(ui->tableMaterials->item(row, 2) ? ui->tableMaterials->item(row, 2)->text() : QString());
    QString atelier = ui->tableMaterials->item(row, 3) ? ui->tableMaterials->item(row, 3)->text() : QString();
    int idx = ui->comboWorkshop_4->findText(atelier);
    ui->comboWorkshop_4->setCurrentIndex(idx >= 0 ? idx : 0);
    ui->spinQuantity_2->setValue((ui->tableMaterials->item(row, 4) ? ui->tableMaterials->item(row, 4)->text() : QString("0")).toInt());
    QString sante = ui->tableMaterials->item(row, 5) ? ui->tableMaterials->item(row, 5)->text() : QString();
    idx = ui->comboHealthStatus_2->findText(sante);
    ui->comboHealthStatus_2->setCurrentIndex(idx >= 0 ? idx : 0);
    QDate d2 = QDate::fromString(ui->tableMaterials->item(row, 7) ? ui->tableMaterials->item(row, 7)->text() : QString(), Qt::ISODate);
    ui->datePurchase_2->setDate(d2.isValid() ? d2 : QDate::currentDate());
    ui->spinFrequenceUtilisation_2->setValue((ui->tableMaterials->item(row, 8) ? ui->tableMaterials->item(row, 8)->text() : QString("0")).toDouble());
    ui->spinNbIncidents_2->setValue((ui->tableMaterials->item(row, 9) ? ui->tableMaterials->item(row, 9)->text() : QString("0")).toInt());
    loadLast5Machines();
    ui->stackedwidget->setCurrentIndex(8);
}

void MainWindow::on_supprimer_m_clicked()
{
    int row = ui->tableMaterials->currentRow();
    if (row < 0) {
        QMessageBox::warning(nullptr, tr("Sélection"), tr("Veuillez sélectionner une ligne (machine) dans le tableau avant de cliquer Supprimer."));
        return;
    }
    QTableWidgetItem *idItem = ui->tableMaterials->item(row, 0);
    if (!idItem) return;
    int id = idItem->data(Qt::UserRole).toInt();
    if (id <= 0) {
        bool ok = false;
        id = idItem->text().toInt(&ok);
        if (!ok || id <= 0) return;
    }
    QMessageBox::StandardButton btn = QMessageBox::question(nullptr, tr("Confirmation"),
        tr("Confirmer la suppression de cette machine ?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (btn != QMessageBox::Yes) return;
    QString dbError;
    if (!Material::removeById(id, &dbError)) {
        QMessageBox::critical(nullptr, tr("Erreur"), tr("Échec de la suppression: ") + dbError);
        return;
    }
    QMessageBox::information(nullptr, tr("Succès"), tr("Machine supprimée avec succès."));
    loadMachines();
}

void MainWindow::on_btnStatsMachines_clicked()
{
    ui->stackedwidget->setCurrentIndex(9);
    reloadMachineStatisticsCharts();
}

void MainWindow::on_btnRefreshStats_clicked()
{
    if (ui->stackedwidget->currentIndex() == 9)
        reloadMachineStatisticsCharts();
}

void MainWindow::reloadMachineStatisticsCharts()
{
    // Defensive init: avoid null/dangling chart view access when navigating to stats page.
    if (!m_statChartMachinesParAnnee && ui->frameStatMachinesParAnnee) {
        m_statChartMachinesParAnnee = new QChartView(ui->frameStatMachinesParAnnee);
        m_statChartMachinesParAnnee->setRenderHint(QPainter::Antialiasing);
        QVBoxLayout *layout1 = qobject_cast<QVBoxLayout*>(ui->frameStatMachinesParAnnee->layout());
        if (!layout1) {
            layout1 = new QVBoxLayout(ui->frameStatMachinesParAnnee);
            layout1->setContentsMargins(10, 10, 10, 10);
        }
        layout1->addWidget(m_statChartMachinesParAnnee);
    }
    if (!m_statChartMachinesParAtelier && ui->frameStatMachinesParAtelier) {
        m_statChartMachinesParAtelier = new QChartView(ui->frameStatMachinesParAtelier);
        m_statChartMachinesParAtelier->setRenderHint(QPainter::Antialiasing);
        QVBoxLayout *layout2 = qobject_cast<QVBoxLayout*>(ui->frameStatMachinesParAtelier->layout());
        if (!layout2) {
            layout2 = new QVBoxLayout(ui->frameStatMachinesParAtelier);
            layout2->setContentsMargins(10, 10, 10, 10);
        }
        layout2->addWidget(m_statChartMachinesParAtelier);
    }
    if (!m_statChartFrequenceUtilisation && ui->frameStatFrequenceUtilisation) {
        m_statChartFrequenceUtilisation = new QChartView(ui->frameStatFrequenceUtilisation);
        m_statChartFrequenceUtilisation->setRenderHint(QPainter::Antialiasing);
        QVBoxLayout *layout3 = qobject_cast<QVBoxLayout*>(ui->frameStatFrequenceUtilisation->layout());
        if (!layout3) {
            layout3 = new QVBoxLayout(ui->frameStatFrequenceUtilisation);
            layout3->setContentsMargins(10, 10, 10, 10);
        }
        layout3->addWidget(m_statChartFrequenceUtilisation);
    }
    if (!m_statChartEtatSante && ui->frameStatEtatSante) {
        m_statChartEtatSante = new QChartView(ui->frameStatEtatSante);
        m_statChartEtatSante->setRenderHint(QPainter::Antialiasing);
        QVBoxLayout *layout4 = qobject_cast<QVBoxLayout*>(ui->frameStatEtatSante->layout());
        if (!layout4) {
            layout4 = new QVBoxLayout(ui->frameStatEtatSante);
            layout4->setContentsMargins(10, 10, 10, 10);
        }
        layout4->addWidget(m_statChartEtatSante);
    }

    if (!m_statChartMachinesParAnnee || !m_statChartMachinesParAtelier ||
        !m_statChartFrequenceUtilisation || !m_statChartEtatSante) {
        qWarning() << "reloadMachineStatisticsCharts: stats chart views are not initialized";
        return;
    }

    QString err;
    if (QChart *yearChart = Material::createMachinesByYearChart(&err)) {
        m_statChartMachinesParAnnee->setChart(yearChart);
        m_statChartMachinesParAnnee->setRenderHint(QPainter::Antialiasing, true);
        m_statChartMachinesParAnnee->setRenderHint(QPainter::TextAntialiasing, true);
        m_statChartMachinesParAnnee->setRenderHint(QPainter::SmoothPixmapTransform, true);
        m_statChartMachinesParAnnee->setRubberBand(QChartView::RectangleRubberBand);
    }
    if (QChart *workshopChart = Material::createMachinesByWorkshopChart(&err)) {
        m_statChartMachinesParAtelier->setChart(workshopChart);
        m_statChartMachinesParAtelier->setRenderHint(QPainter::Antialiasing, true);
        m_statChartMachinesParAtelier->setRenderHint(QPainter::TextAntialiasing, true);
        m_statChartMachinesParAtelier->setRenderHint(QPainter::SmoothPixmapTransform, true);
        m_statChartMachinesParAtelier->setRubberBand(QChartView::RectangleRubberBand);
    }
    if (QChart *freqChart = Material::createUsageAndCountByWorkshopChart(&err)) {
        m_statChartFrequenceUtilisation->setChart(freqChart);
        m_statChartFrequenceUtilisation->setRenderHint(QPainter::Antialiasing, true);
        m_statChartFrequenceUtilisation->setRenderHint(QPainter::TextAntialiasing, true);
        m_statChartFrequenceUtilisation->setRenderHint(QPainter::SmoothPixmapTransform, true);
        m_statChartFrequenceUtilisation->setRubberBand(QChartView::RectangleRubberBand);
    }
    if (QChart *healthChart = Material::createHealthStatusChart(&err)) {
        m_statChartEtatSante->setChart(healthChart);
        m_statChartEtatSante->setRenderHint(QPainter::Antialiasing, true);
        m_statChartEtatSante->setRenderHint(QPainter::TextAntialiasing, true);
        m_statChartEtatSante->setRenderHint(QPainter::SmoothPixmapTransform, true);
        m_statChartEtatSante->setRubberBand(QChartView::RectangleRubberBand);
    }
    if (!err.isEmpty())
        qWarning() << "reloadMachineStatisticsCharts:" << err;
}

void MainWindow::on_retour_clicked()
{
    // Retour vers la page Gestion Materiels
    ui->stackedwidget->setCurrentIndex(7);
}

void MainWindow::on_retour_2_clicked()
{
    // Retour depuis la page Analyse IA (index 10) vers Gestion matériels
    ui->stackedwidget->setCurrentIndex(7);
}

void MainWindow::on_gen_rapport_clicked()
{
    const int row = ui->tableMaterials->currentRow();
    if (row < 0) {
        QMessageBox::information(nullptr, tr("Rapport"), tr("Veuillez sélectionner une machine dans le tableau avant de générer le rapport."));
        return;
    }

    QTableWidgetItem *idItem = ui->tableMaterials->item(row, 0);
    if (!idItem) {
        QMessageBox::warning(nullptr, tr("Erreur"), tr("Impossible d'identifier la machine sélectionnée."));
        return;
    }

    int idMat = idItem->data(Qt::UserRole).toInt();
    if (idMat <= 0)
        idMat = idItem->text().toInt();

    Material mat;
    QString fetchError;
    if (!Material::fetchById(idMat, &mat, &fetchError)) {
        QMessageBox::warning(nullptr, tr("Erreur"), tr("Impossible de charger les données de la machine."));
        return;
    }

    QString nom = mat.nomMateriel().trimmed();
    if (nom.isEmpty()) nom = tr("(sans nom)");
    QString atelier = mat.atelier().trimmed();
    if (atelier.isEmpty()) atelier = tr("(non renseigné)");

    QString etatTexte = mat.etatSante().trimmed();
    if (etatTexte.isEmpty()) etatTexte = tr("non renseigné");
    else {
        const QString lower = etatTexte.toLower();
        if (lower.contains("bon") || lower.contains("good")) etatTexte = tr("bon");
        else if (lower.contains("moyen") || lower.contains("medium")) etatTexte = tr("moyen");
        else if (lower.contains("mauvais") || lower.contains("bad")) etatTexte = tr("mauvais");
    }

    QString dateAchat = tr("(non renseignée)");
    if (mat.dateAchat().isValid())
        dateAchat = QLocale(QLocale::French).toString(mat.dateAchat(), QLocale::ShortFormat);

    QString dateEntretien = tr("(non renseignée)");
    if (mat.dateDernierEntretien().isValid())
        dateEntretien = QLocale(QLocale::French).toString(mat.dateDernierEntretien(), QLocale::ShortFormat);

    QString frequence = QString::number(mat.frequenceUtilisation()).trimmed();
    if (frequence.isEmpty()) frequence = tr("(non renseignée)");

    int nbIncidents = mat.nombreIncidents();

    QString rapport = tr("La  %1 située dans  %2 est actuellement dans un état de santé %3. "
                         "Elle a été achetée le %4 et son dernier entretien a été réalisé le %5 afin d'assurer son bon fonctionnement. "
                         "Cette machine est utilisée avec une fréquence d'environ %6 et joue un rôle important dans les opérations de production de l'atelier. "
                         "Depuis sa mise en service, %7 incident(s) ont été enregistrés. "
                         "Un suivi régulier de la maintenance est recommandé afin de garantir la continuité et l'efficacité de son fonctionnement.")
                         .arg(nom).arg(atelier).arg(etatTexte).arg(dateAchat).arg(dateEntretien).arg(frequence).arg(nbIncidents);

    // Style rapport (aligné sur l'application WoodSync), sans logo
    static const QString reportStyle = QStringLiteral(
        "body { font-family: 'Segoe UI', 'Roboto Condensed', Arial; font-size: 13px; color: #414833; "
        "background-color: #fdfefe; margin: 8px; } "
        ".report-box { background-color: rgb(253, 255, 237); border: 2px solid #7F5539; border-radius: 10px; "
        "padding: 14px; color: #003366; line-height: 1.55; } "
        "h2 { color: #7F5539; font-size: 14px; margin: 0 0 10px 0; }"
    );
    QString bodyEscaped = rapport;
    bodyEscaped.replace(QLatin1String("&"), QLatin1String("&amp;"));
    bodyEscaped.replace(QLatin1String("<"), QLatin1String("&lt;"));
    bodyEscaped.replace(QLatin1String(">"), QLatin1String("&gt;"));
    bodyEscaped.replace(QLatin1String("\n"), QLatin1String("<br>"));
    QString html = QStringLiteral("<html><head><style>%1</style></head><body><div class=\"report-box\"><h2>Rapport machine</h2><p>%2</p></div></body></html>")
                      .arg(reportStyle, bodyEscaped);
    if (m_rapportTextEdit)
        m_rapportTextEdit->setHtml(html);
}

void MainWindow::openEmailReportDialog()
{
    if (!m_rapportTextEdit)
        return;
    QString reportText = m_rapportTextEdit->toPlainText().trimmed();
    if (reportText.isEmpty()) {
        QMessageBox::information(nullptr, tr("Rapport vide"),
            tr("Générez d'abord un rapport (sélectionnez une machine puis cliquez sur \"Generer\") avant d'envoyer par e-mail."));
        return;
    }
    EmailDialog dlg(reportText, this);
    dlg.exec();
}

// =========================
// Employee Domain Functions
// =========================

void MainWindow::on_BtnLogin_clicked()
{
    QString nom = ui->NomLoginMenuisier->text();
    QString prenom = ui->PrenomLoginMenuisier->text();
    QString mdp = ui->MdpLoginMenuisier->text();
    if (!verifForm(nom,prenom,mdp)) {
        return;
    }
    Employes e;
    e.setNom(nom);
    e.setPrenom(prenom);
    e.setMdp(mdp);
    if (e.existanceCompte()) {
        currentId = e.getId();
        persistSessionUser(currentId);
        ui->DeconnecterUtilisateur->setVisible(true);
        ui->SideBar->setVisible(true);
        ui->stackedwidget->setCurrentIndex(3);
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Vérifier votre mdp !"));
        return;
    }

}

void MainWindow::on_BtnLoginFace_clicked()
{
    if (detector.empty() || recognizer.empty()) {
        QMessageBox::critical(this, tr("Face Recognition"), tr("Les modèles de reconnaissance faciale ne sont pas chargés."));
        return;
    }

    ignore = run([this](){
        cap.open(0);
        if (!cap.isOpened()) return;
        Mat frame, faces;
        int matchId = -1;
        while (cap.read(frame)) {
            if (frame.empty()) break;


            detector->setInputSize(frame.size());
            detector->detect(frame, faces);

            if (faces.rows > 0) {
                Mat alignedLive, liveFeature;
                recognizer->alignCrop(frame, faces.row(0), alignedLive);
                recognizer->feature(alignedLive, liveFeature);

                QString matchName = "UNKNOWN";
                double maxScore = 0.0;

                for (vector<FaceTemplate>::iterator it = registry.begin(); it != registry.end(); ++it) {
                    double score = recognizer->match(liveFeature, it->vector);
                    if (score > 0.363 && score > maxScore) {
                        maxScore = score;
                        matchId  = it->id;
                        matchName = it->name;
                    }
                }

                float *f = faces.ptr<float>(0);
                Rect faceRect(f[0], f[1], f[2], f[3]);
                Scalar color = (matchId != -1) ? Scalar(0,255,0) : Scalar(0,0,255);
                rectangle(frame, faceRect, color, 2);
                putText(frame,
                        matchName.toStdString() + " (" + QString::number(maxScore,'f',2).toStdString() + ")",
                        Point(f[0], f[1]-10), FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
            }

            // Display + get key — ALL on main thread
            Mat frameCopy = frame.clone();
            bool shouldBreak = false;
            QMetaObject::invokeMethod(this, [frameCopy, &shouldBreak](){
                imshow("Reconnaissance faciale", frameCopy);
                int key = waitKey(1);
                if (key == 27 || !getWindowProperty("Reconnaissance faciale", WND_PROP_VISIBLE))
                    shouldBreak = true;
            }, Qt::BlockingQueuedConnection);

            if (shouldBreak) break;

            // Show green rectangle for 1 second THEN redirect
            if (matchId != -1) {
                currentId = matchId;
                QThread::msleep(500);
                break;
            }
        }

        cap.release();
        QMetaObject::invokeMethod(this, [this,matchId](){
            persistSessionUser(matchId);
            destroyAllWindows();
            ui->stackedwidget->setCurrentIndex(3);
            ui->DeconnecterUtilisateur->show();
            ui->DeconnecterUtilisateur->clearFocus();
            ui->SideBar->setVisible(true);
            Etmp.setId(matchId);
            refreshEmployeeTable();
        }, Qt::QueuedConnection);
    });

}
void MainWindow::on_ConnectionLink_linkActivated(const QString &link)
{
    if (link == "loginPage") {
        ui->stackedwidget->setCurrentIndex(0);
    }
}


void MainWindow::on_ConnectionLink_2_linkActivated(const QString &link)
{
    if (link == "CreateAccount") {
        ui->stackedwidget->setCurrentIndex(1);
    }
}


void MainWindow::on_AjoutEmploye_clicked()
{
    QString id = ui->SuperviseurEmploye->currentData().toString();
    QString nom = ui->nomEmploye->text();
    QString prenom = ui->PrenomEmploye->text();
    QDate date_naissance = ui->DateNaissance->date();
    QDate date_recrutement = ui->DateRecrutementEmploye->date();
    float heures = ui->HeuresTravailleEmploye->value();
    QString tel = ui->TelEmploye->text();
    QString role = ui->RoleEmploye->currentText();
    if (!verifForm(nom,prenom,role,date_naissance,tel,heures)) {
        return;
    }
    Employes e(nom,prenom,tel,heures,date_recrutement,date_naissance,role);
    if (!id.isEmpty()) {
        e.setIdSupervised(id.toInt());
    }
    if (ui->AjoutEmploye->text() == "Ajouter") {
        bool isSuccessful = e.ajouter();
        if (isSuccessful){
            QMessageBox msg(QMessageBox::Information,
                            tr("Succés"),
                            tr("L'employé a été ajouté avec succés"),
                            QMessageBox::Ok,
                            nullptr);
            msg.setCursor(Qt::PointingHandCursor);
            msg.exec();
            refreshEmployeeTable();
            ui->DateNaissance->setDate(QDate::currentDate());
            ui->DateRecrutementEmploye->setDate(QDate::currentDate());
            ui->nomEmploye->setText("");
            ui->PrenomEmploye->setText("");
            ui->HeuresTravailleEmploye->setValue(0.0);
            ui->TelEmploye->setText("");
            ui->FrameAjout->close();
            emit employeeAdded();
        } else {
            QMessageBox msg(QMessageBox::Critical,
                            tr("Erreur"),
                            tr("Erreur lors du traitement du requête"),
                            QMessageBox::Ok,
                            nullptr);
            msg.setCursor(Qt::PointingHandCursor);
            msg.exec();
        }
    } else if (ui->AjoutEmploye->text() == "Modifier") {
        QModelIndex currentIndex = ui->tableView->currentIndex();
        int row = currentIndex.row();
        int id = ui->tableView->model()->index(row,0).data().toInt();

        bool isSuccessful = e.modifier(id);
        if (isSuccessful) {
            QMessageBox msg(QMessageBox::Information,
                            tr("Succés"),
                            tr("L'employé a été modifié avec succés"),
                            QMessageBox::Ok,
                            nullptr);
            msg.setCursor(Qt::PointingHandCursor);
            msg.exec();
            refreshEmployeeTable();
            ui->DateNaissance->setDate(QDate::currentDate());
            ui->DateRecrutementEmploye->setDate(QDate::currentDate());
            ui->nomEmploye->setText("");
            ui->PrenomEmploye->setText("");
            ui->HeuresTravailleEmploye->setValue(0.0);
            ui->TelEmploye->setText("");
            ui->AjoutEmploye->setText("Ajouter");
            ui->FrameAjout->close();
        } else {
            QMessageBox msg(QMessageBox::Critical,
                            tr("Erreur"),
                            tr("Erreur lors du traitement du requête"),
                            QMessageBox::Ok,
                            nullptr);
            msg.setCursor(Qt::PointingHandCursor);
            msg.exec();
        }
    }

}


void MainWindow::on_SupprimerEmploye_clicked()
{
    QModelIndex currentIndex = ui->tableView->currentIndex();

    if (!currentIndex.isValid()) {
        QMessageBox msg(QMessageBox::Critical,
                        tr("Erreur"),
                        tr("Séléctionnez un employé !"),
                        QMessageBox::Ok,
                        nullptr);
        msg.setCursor(Qt::PointingHandCursor);
        msg.exec();
        return;
    }
    int row = currentIndex.row();
    int id = ui->tableView->model()->index(row,0).data().toInt();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(nullptr,"Confirmation","Êtes-vous sûr de supprimer ce employé?");
    if (reply == QMessageBox::Yes) {
        bool isSuccessful = Etmp.supprimer(id);
        if (isSuccessful) {
            QMessageBox msg(QMessageBox::Information,
                            tr("Succés"),
                            tr("L'employé a été supprimé avec succés"),
                            QMessageBox::Ok,
                            nullptr);
            msg.setCursor(Qt::PointingHandCursor);
            msg.exec();
            emit employeedeleted();
        } else {
            QMessageBox msg(QMessageBox::Critical,
                            tr("Erreur"),
                            tr("Erreur lors du traitement du requête"),
                            QMessageBox::Ok,
                            nullptr);
            msg.setCursor(Qt::PointingHandCursor);
            msg.exec();
        }
    }

}


void MainWindow::on_tableView_doubleClicked(const QModelIndex &index)
{
    int row = index.row();
    QString nom = ui->tableView->model()->index(row,1).data().toString();
    QString prenom = ui->tableView->model()->index(row,2).data().toString();
    QDate dateNaissance = ui->tableView->model()->index(row,6).data().toDate();
    QString role = ui->tableView->model()->index(row,7).data().toString();
    QString tel = ui->tableView->model()->index(row,3).data().toString();
    QDate dateRecrutement = ui->tableView->model()->index(row,5).data().toDate();
    double heures = ui->tableView->model()->index(row,4).data().toDouble();

    ui->DateNaissance->setDate(dateNaissance);
    ui->DateRecrutementEmploye->setDate(dateRecrutement);
    ui->nomEmploye->setText(nom);
    ui->PrenomEmploye->setText(prenom);
    ui->HeuresTravailleEmploye->setValue(heures);
    ui->TelEmploye->setText(tel);
    ui->RoleEmploye->setCurrentText(role);
    ui->FrameAjout->setEnabled(false);
    showFrameAsDialog();
    ui->LabelAjout->setText("Informations sur l'employé");
    ui->AjoutEmploye->hide();
}


void MainWindow::on_InscriptionEmploye_clicked()
{
    QString nom = ui->NomMenuisier->text();
    QString prenom = ui->PrenomMenuisier->text();
    QString pwd = ui->MdpMenuisiser->text();
    QString pwd_confirmation = ui->MdpConfirm->text();
    QDate dateNaissance = ui->DateNaissanceMenuisier->date();
    if (!verifForm(nom,prenom,dateNaissance,pwd,pwd_confirmation)) {
        return;
    }
    QByteArray saltBytes;
    saltBytes.resize(128);
    QRandomGenerator64::securelySeeded().fillRange(reinterpret_cast<uint64_t*>(saltBytes.data()),8);
    QString mdp_salt = saltBytes.toHex();
    QString mdp = QCryptographicHash::hash((pwd+mdp_salt).toUtf8(),QCryptographicHash::Sha512).toHex();
    Employes e(nom,prenom,0,0,QDate::currentDate(),dateNaissance,"Menuisier",mdp,mdp_salt);

    if (e.ajoutCompte()){
        currentId = e.getId();
        persistSessionUser(currentId);
        ui->DeconnecterUtilisateur->setVisible(true);
        QMessageBox::information(nullptr,tr("Succées"),tr("Votre compte a été crée avec succés"));
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr,tr("Reconaissance faciale"),tr("Voulez-vous configurer votre reconnaissance faciale?"),QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            if (detector.empty() || recognizer.empty()) {
                QMessageBox::critical(this, tr("Face Recognition"), tr("Les modèles de reconnaissance faciale ne sont pas chargés."));
                return;
            }
            cap.open(0);
            if (!cap.isOpened()) {
                qDebug() << "Could not open camera!";
                return;
            }
            namedWindow(WINNAME,WINDOW_AUTOSIZE);
            Mat frame, faces;
            while (cap.read(frame)) {
                if (frame.empty()) break;

                // Update detector for the camera frame size
                detector->setInputSize(frame.size());
                detector->detect(frame, faces);
                if (faces.rows > 0) {
                    float *f = faces.ptr<float>(0);
                    Rect faceRect(f[0],f[1],f[2],f[3]);
                    rectangle(frame,faceRect,Scalar(255,255,0),2);
                    displayOverlay(WINNAME,"Appuyez sur S pour sauvgarder votre image !",2000);
                    imshow(WINNAME,frame);
                    int key = waitKey(1);
                    if (key == 's' || key == 'S'){
                        if (faces.rows == 1) {
                            Mat alignedFace,featureVector;
                            recognizer->alignCrop(frame,faces.row(0),alignedFace);
                            recognizer->feature(alignedFace,featureVector);
                            QByteArray data(reinterpret_cast<const char*>(featureVector.data),featureVector.total()*featureVector.elemSize());
                            if (e.ajoutReconaissanceFaciale(data)) {
                                QMessageBox::information(nullptr,tr("Succés"),tr("Profile biométrique sauvgardé avec succés !"));
                                break;
                            } else {
                                QMessageBox::critical(nullptr,tr("Échec"),tr("Une personne doit être visible"));
                            }
                        }
                    }

                    // A. Align and Extract from the LIVE Camera Frame
                }
            }
            cap.release();
            destroyAllWindows();
        }
        ui->stackedwidget->setCurrentIndex(3);
        ui->SideBar->setVisible(true);
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Erreur lors du traitement du votre requête !"));
    }
}

void MainWindow::loadFaceRegistry() {
    registry.clear();
    QSqlQuery query("SELECT IDEMPLOYE, NOM, FACE_EMBEDDING FROM EMPLOYES");
    while (query.next()) {
        QByteArray data = query.value(2).toByteArray();
        if (!data.isEmpty()) {
            FaceTemplate ft;
            ft.id = query.value(0).toInt();
            ft.name = query.value(1).toString();
            ft.vector = Mat(1, 128, CV_32F, const_cast<char*>(data.data())).clone();

            registry.push_back(ft);
        }
    }
}

bool MainWindow::verifForm(const QString& nom,const QString& prenom,const QDate& dateNaissance, const QString& pwd,const QString& pwd_confirmation)
{
    const QDate& dateNaissanceMinim = QDate::currentDate().addYears(-18);
    bool isVerified = true;
    if (nom.trimmed().isEmpty()) {
        ui->Erreur_nom->setText("Le nom est vide !");
        ui->Erreur_nom->show();
        isVerified = false;
    } else {
        ui->Erreur_nom->hide();
    }
    if (prenom.trimmed().isEmpty()) {
        ui->Erreur_prenom->setText("Le prénom est vide !");
        ui->Erreur_prenom->show();
        isVerified = false;
    } else {
        ui->Erreur_prenom->hide();
    }
    if (!dateNaissance.isValid() || dateNaissance > dateNaissanceMinim) {
        ui->Erreur_date->setText("La date est invalide !");
        ui->Erreur_date->show();
        isVerified = false;
    } else {
        ui->Erreur_date->hide();
    }
    if (pwd.trimmed().isEmpty()) {
        ui->Erreur_mdp->setText("Votre mot de passe doit être non nul !");
        ui->Erreur_mdp->show();
        isVerified = false;
    }
    else if (pwd.trimmed().length() <= 8) {
        ui->Erreur_mdp->setText("Votre mot de passe doit contenir au minimum 8 caractéres !");
        ui->Erreur_mdp->show();
        isVerified = false;
    } else {
        ui->Erreur_mdp->hide();
    }
    if (pwd.trimmed() != pwd_confirmation.trimmed()) {
        ui->Erreur_confirmation->setText("Les mots de passes ne sont pas conformes !");
        ui->Erreur_confirmation->show();
        isVerified = false;
    } else {
        ui->Erreur_confirmation->hide();
    }
    return isVerified;
}
bool MainWindow::verifForm(const QString& nom,const QString& prenom,const QString& pwd) {
    bool isVerified = true;
    if (nom.trimmed().isEmpty()) {
        ui->Erreur_login_nom->setText("Le nom est vide !");
        ui->Erreur_login_nom->show();
        isVerified = false;
    }
    if (prenom.trimmed().isEmpty()) {
        ui->Erreur_login_prenom->setText("Le prénom est vide !");
        ui->Erreur_login_prenom->show();
        isVerified = false;
    }
    if (pwd.trimmed().isEmpty()) {
        ui->Erreur_login_mdp->setText("Votre mot de passe doit être non nul !");
        ui->Erreur_login_mdp->show();
        isVerified = false;
    }
    else if (pwd.trimmed().length() <= 8) {
        ui->Erreur_login_mdp->setText("Votre mot de passe doit contenir au minimum 8 caractéres !");
        ui->Erreur_login_mdp->show();
        isVerified = false;
    };
    return isVerified;
}

bool MainWindow::verifForm(const QString& nom,const QString& prenom,const QString& role,const QDate& date,const QString& tel,const double& heures) {
    bool isVerified = true;
    bool isNumeric = false;
    if (nom.trimmed().isEmpty()) {
        ui->Erreur_ajout_nom->setText("Le nom est vide !");
        ui->Erreur_ajout_nom->show();
        isVerified = false;
    } else {
         ui->Erreur_ajout_nom->hide();
    }
    if (prenom.trimmed().isEmpty()) {
        ui->Erreur_ajout_prenom->setText("Le prénom est vide !");
        ui->Erreur_ajout_prenom->show();
        isVerified = false;
    } else {
        ui->Erreur_ajout_prenom->hide();
    }
    if (role.trimmed().isEmpty()) {
        ui->Erreur_ajout_role->setText("Le rôle ne doit pas être vide !");
        ui->Erreur_ajout_role->show();
        isVerified = false;
    } else {
        ui->Erreur_ajout_role->hide();
    }
    if (!date.isValid() || date > QDate::currentDate().addYears(-18)) {
        ui->Erreur_ajout_date->setText("Date invalide !");
        ui->Erreur_ajout_date->show();
        isVerified = false;
    } else {
        ui->Erreur_ajout_date->hide();
    }
    if (tel.isEmpty()) {
        ui->Erreur_ajout_tel->setText("Numéro invalide !");
        ui->Erreur_ajout_tel->show();
        isVerified = false;
    } else if (!tel.isEmpty()) {
        tel.toInt(&isNumeric);
        if (!isNumeric) {
            ui->Erreur_ajout_tel->setText("Numéro invalide !");
            ui->Erreur_ajout_tel->show();
            isVerified = false;
        }
    } else {
        ui->Erreur_ajout_tel->hide();
    }
    if (heures == 0.0f) {
        ui->Erreur_ajout_heures->setText("Heures invalid !");
        ui->Erreur_ajout_heures->show();
        isVerified = false;
    } else {
        ui->Erreur_ajout_heures->hide();
    }
    return isVerified;
}

void MainWindow::applyAuthLayout(bool loggedIn)
{
    if (loggedIn) {
        ui->SideBar->show();
        ui->SideBar->setGeometry(40, 20, 261, 701);
        ui->stackedwidget->setGeometry(320, 20, 1161, 688);
    } else {
        ui->SideBar->hide();
        ui->stackedwidget->setGeometry(20, 20, 1460, 688);
    }

}
void MainWindow::populateComboBox()
{
    ui->SuperviseurEmploye->clear();
    ui->SuperviseurEmploye->addItem("Séléctionnez quelqu'un",-1);
    QSqlQuery query;
    query.prepare("SELECT IDEMPLOYE,NOM,PRENOM FROM EMPLOYES");
    if(query.exec()){
        while (query.next()) {
            QString id = query.value(0).toString();
            QString fullDisplayName = query.value(1).toString() + " " + query.value(2).toString();
            ui->SuperviseurEmploye->addItem(fullDisplayName,id);
        }
    }
}

void MainWindow::persistSessionUser(int userId)
{
    QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    Etmp.saveSessionToken(token,QDate::currentDate().addDays(30),userId);
    WritePasswordJob* job = new WritePasswordJob("WoodSync",this);
    job->setKey("session_token");
    job->setTextData(token);
    job->start();
    QSettings s("WoodSync","WoodSyncApp");
    s.setValue("userId",userId);
    applyAuthLayout(true);
}

void MainWindow::showFrameAsDialog()
{
    ui->FrameAjout->setWindowFlags(Qt::Window | Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    ui->FrameAjout->setWindowModality(Qt::ApplicationModal);
    ui->FrameAjout->setWindowTitle("Ajouter employé");
    ui->FrameAjout->show();
}

void MainWindow::onTokenRead(Job *job)
{
    ReadPasswordJob* readJob = static_cast<ReadPasswordJob*>(job);
    QSettings s("WoodSync","WoodSyncApp");
    int savedId = s.value("userId",-1).toInt();
    if (readJob->error() || readJob->textData().isEmpty() || savedId == -1) {
        ui->stackedwidget->setCurrentIndex(0);
        applyAuthLayout(false);
        return;
    }
    QString token = readJob->textData();
    if (Etmp.validateSessionToken(token,savedId)) {
        currentId =savedId;
        Etmp.setId(currentId);
        if (!refreshEmployeeTable()) {
            return;
        }
        ui->DeconnecterUtilisateur->setVisible(true);
        ui->SideBar->setVisible(true);
        ui->stackedwidget->setCurrentIndex(3);
        applyAuthLayout(true);
        ui->GestionEmployes->setChecked(true);
    } else {
        s.remove("userId");
        DeletePasswordJob* del = new DeletePasswordJob("WoodSync",this);
        del->setKey("session_token");
        del->start();
        ui->DeconnecterUtilisateur->setVisible(false);
        ui->stackedwidget->setCurrentIndex(0);
        Etmp.setId(-1);
        currentId = -1;
        applyAuthLayout(false);
    }
}


void MainWindow::on_ExportEmploye_clicked()
{
    QString filter = "CSV Files (*.csv)";
    QString filePath = QFileDialog::getSaveFileName(this,"Export Employee Data",QDir::homePath() + "/employees.csv",filter);
    if (filePath.isEmpty()) {
        return;
    }
    if (Etmp.exportToCSV(ui->tableView,filePath)) {
        QMessageBox::information(nullptr, "Export Success", "Data saved to: " + filePath);
    } else {
        QMessageBox::critical(nullptr, "Export Failed", "Could not save the file.");
    }
}


void MainWindow::on_ImportEmployes_clicked()
{
    if (Etmp.importCSV(ui->tableView)) {
        QMessageBox::information(nullptr, tr("Succés"),tr("Votre fichier est importé avec succés !"));
        populateComboBox();
    }
}


void MainWindow::on_DeconnecterUtilisateur_clicked()
{
    Etmp.clearSessionToken(currentId);
    DeletePasswordJob* job = new DeletePasswordJob("WoodSync",this);
    job->setKey("session_token");
    job->start();
    QSettings s("WoodSync","WoodSyncApp");
    s.remove("userId");
    currentId = -1;
    ui->DeconnecterUtilisateur->setVisible(false);
    applyAuthLayout(false);
    ui->stackedwidget->setCurrentIndex(0);
    ui->NomLoginMenuisier->setText("");
    ui->PrenomLoginMenuisier->setText("");
    ui->MdpLoginMenuisier->setText("");
    ui->DeconnecterUtilisateur->setChecked(false);
    ui->DeconnecterUtilisateur->clearFocus();

}


void MainWindow::on_RechercheEmployeBtn_clicked()
{
    const QString nom = ui->RechercheEmploye->text().trimmed();
    if (nom.isEmpty()) {
        refreshEmployeeTable();
        return;
    }

    QSqlQueryModel* newModel = Etmp.rechercher(nom);
    if (newModel) {
        bindEmployeeTableModel(newModel);
        if (newModel->rowCount() == 0) {
            QMessageBox::information(nullptr, tr("Recherche"), tr("Aucun employé trouvé."));
        }
    } else {
        QMessageBox::critical(nullptr,tr("Erreur"),tr("Erreur pendant la recherche."));
        return;
    }

}


void MainWindow::on_AjoutDialog_clicked()
{
    ui->DateNaissance->setDate(QDate::currentDate());
    ui->DateRecrutementEmploye->setDate(QDate::currentDate());
    ui->nomEmploye->setText("");
    ui->PrenomEmploye->setText("");
    ui->HeuresTravailleEmploye->setValue(0.0);
    ui->TelEmploye->setText("");
    ui->AjoutEmploye->setText("Ajouter");
    ui->LabelAjout->setText("Ajouter un employé");
    showFrameAsDialog();
}

void MainWindow::on_TrierEmploye_clicked()
{
    QString choice = ui->TriEmploye->currentText();
    QSqlQueryModel* model = Etmp.trier(choice);
    if (model) {
        bindEmployeeTableModel(model);
    }
}


void MainWindow::on_ModifierDIalog_clicked()
{
    QModelIndex selection = ui->tableView->currentIndex();
    qDebug() << selection;
    if (!selection.isValid()) {
        QMessageBox::warning(nullptr,"Erreur","Veuillez séléctionnez un employé !");
        return;
    }
    on_tableView_clicked(selection);
    showFrameAsDialog();
    ui->FrameAjout->setWindowTitle("Modifier employe");
    ui->FrameAjout->setEnabled(true);
    if (ui->AjoutEmploye->isHidden()) {
        ui->AjoutEmploye->show();
    }
}



void MainWindow::on_tableView_clicked(const QModelIndex &index)
{
    int row = index.row();
    QString nom = ui->tableView->model()->index(row,1).data().toString();
    QString prenom = ui->tableView->model()->index(row,2).data().toString();
    QString dateNaissance = ui->tableView->model()->index(row,6).data().toString();
    QDate date_pure = QDate::fromString(dateNaissance,"dd/MM/yyyy");
    QString role = ui->tableView->model()->index(row,8).data().toString();
    QString tel = ui->tableView->model()->index(row,3).data().toString();
    QDate dateRecrutement = ui->tableView->model()->index(row,5).data().toDate();
    double heures = ui->tableView->model()->index(row,4).data().toDouble();
    QString superviseur = ui->tableView->model()->index(row,7).data().toString();

    ui->DateNaissance->setDate(date_pure);
    ui->DateRecrutementEmploye->setDate(dateRecrutement);
    ui->nomEmploye->setText(nom);
    ui->PrenomEmploye->setText(prenom);
    ui->HeuresTravailleEmploye->setValue(heures);
    ui->TelEmploye->setText(tel);
    ui->RoleEmploye->setCurrentText(role);
    if (superviseur != "None") {
        ui->SuperviseurEmploye->setCurrentText(superviseur);
    }
    ui->AjoutEmploye->setText("Modifier");
    ui->LabelAjout->setText("Modifier un employé");
}
void MainWindow::on_StatistiquesEmploye_clicked()
{
    generateChart();
    generateChartRepartition();
    generatePie();
}
void MainWindow::on_EnregistrerStat_clicked()
{
    QGraphicsView* src = (ui->Stats && ui->Stats->currentIndex() == 1) ? ui->Pie : ui->Stat;
    if (!src || !src->scene()) {
        QMessageBox::warning(nullptr, tr("Export"), tr("Aucune statistique a exporter."));
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Enregistrer sous"),
        QDir::homePath(),
        tr("Images (*.png *.jpg *.jpeg)")
        );
    if (filePath.isEmpty()) {
        return;
    }

    const QRect sourceRect = src->viewport()->rect();
    if (sourceRect.isEmpty()) {
        QMessageBox::warning(nullptr, tr("Export"), tr("Taille de l'image invalide."));
        return;
    }

    const qreal scale = 3.0;
    const QSize outSize(
        qRound(sourceRect.width() * scale),
        qRound(sourceRect.height() * scale)
    );

    QImage image(outSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(src->backgroundBrush().color());

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    src->render(
        &painter,
        QRectF(QPointF(0, 0), QSizeF(outSize)),
        sourceRect,
        Qt::IgnoreAspectRatio
        );
    painter.end();

    if (!image.save(filePath)) {
        QMessageBox::critical(nullptr, tr("Export"), tr("Echec de sauvegarde de l'image."));
        return;
    }

    QMessageBox::information(nullptr, tr("Export"), tr("Statistique enregistreée avec succés."));
}
void MainWindow::on_StatSuiv_clicked()
{
    int currentIndex = ui->Stats->currentIndex();
    ui->Stats->setCurrentIndex(currentIndex+1);
}


void MainWindow::on_StatPrev_clicked()
{
    int currentIndex = ui->Stats->currentIndex();
    ui->Stats->setCurrentIndex(currentIndex-1);
}

// ====================
// AI Material Functions
// ====================

void MainWindow::setupAiAnalysisUi()
{
    if (!ui || !ui->rapport_ai) {
        return;
    }
    
    // Clear any existing layout to ensure fresh UI
    if (QLayout *existingLayout = ui->rapport_ai->layout()) {
        QLayoutItem *item;
        while ((item = existingLayout->takeAt(0)) != nullptr) {
            if (item->widget())
                delete item->widget();
            delete item;
        }
        delete existingLayout;
    }
    
    // Reset member pointers as they're about to be recreated
    m_aiStatusLabel = nullptr;
    m_aiScoreBar = nullptr;
    m_aiScoreValue = nullptr;
    m_aiRiskBadge = nullptr;
    m_aiCommentLabel = nullptr;

    auto addSeparator = [](QVBoxLayout *l) {
        l->addSpacing(6);
        auto *line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setStyleSheet(QStringLiteral("QFrame { color: #7F5539; max-height: 2px; }"));
        l->addWidget(line);
        l->addSpacing(10);
    };

    QWidget *scrollInner = new QWidget();
    scrollInner->setObjectName(QStringLiteral("rapport_ai_inner"));
    scrollInner->setStyleSheet(QStringLiteral("#rapport_ai_inner { background-color: rgb(253, 255, 237); }"));

    QVBoxLayout *lay = new QVBoxLayout(scrollInner);
    lay->setContentsMargins(16, 18, 16, 20);
    lay->setSpacing(4);

    m_aiStatusLabel = new QLabel(tr("Sélectionnez une machine dans le tableau, puis cliquez sur « Analyser IA » ou sur le bouton d'une ligne."), scrollInner);
    m_aiStatusLabel->setWordWrap(true);
    m_aiStatusLabel->setMinimumHeight(40);
    m_aiStatusLabel->setStyleSheet(QStringLiteral("color: #4a5248; font-size: 13px; padding: 4px 2px;"));

    addSeparator(lay);

    QLabel *lblScore = new QLabel(tr("Score préventif (0–100)"), scrollInner);
    lblScore->setStyleSheet(QStringLiteral("color: #6b5344; font-weight: 700; font-size: 14px; margin-top: 4px;"));

    m_aiScoreBar = new QProgressBar(scrollInner);
    m_aiScoreBar->setRange(0, 100);
    m_aiScoreBar->setValue(0);
    m_aiScoreBar->setFormat(QStringLiteral("%v / 100"));
    m_aiScoreBar->setTextVisible(true);
    m_aiScoreBar->setMinimumHeight(28);

    m_aiScoreValue = new QLabel(QStringLiteral("—"), scrollInner);

    lay->addWidget(m_aiStatusLabel);
    lay->addWidget(lblScore);
    lay->addSpacing(8);
    QHBoxLayout *hScore = new QHBoxLayout();
    hScore->setSpacing(12);
    hScore->addWidget(m_aiScoreBar, 1);
    hScore->addWidget(m_aiScoreValue);
    lay->addLayout(hScore);

    addSeparator(lay);

    QLabel *lblRisk = new QLabel(tr("Indice de risque"), scrollInner);
    lblRisk->setStyleSheet(QStringLiteral("color: #7F5539; font-weight: 700; font-size: 14px;"));

    m_aiRiskBadge = new QLabel(QStringLiteral("—"), scrollInner);
    m_aiRiskBadge->setAlignment(Qt::AlignCenter);
    m_aiRiskBadge->setMinimumHeight(40);
    m_aiRiskBadge->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    applyPreventiveScoreAppearance(m_aiScoreBar, m_aiScoreValue, m_aiRiskBadge, 0, true);

    lay->addWidget(lblRisk);
    lay->addSpacing(10);
    lay->addWidget(m_aiRiskBadge);

    addSeparator(lay);

    QLabel *lblCom = new QLabel(tr("Commentaire système"), scrollInner);
    lblCom->setStyleSheet(QStringLiteral("color: #7F5539; font-weight: 700; font-size: 14px;"));

    m_aiCommentLabel = new QLabel(QStringLiteral("—"), scrollInner);
    m_aiCommentLabel->setWordWrap(true);
    m_aiCommentLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_aiCommentLabel->setMinimumHeight(72);
    m_aiCommentLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    m_aiCommentLabel->setStyleSheet(
        QStringLiteral("color: #003366; font-size: 13px; line-height: 1.45; padding: 12px 14px; "
                       "background: #fdfefe; border-radius: 10px; border: 2px solid #A68A64;"));

    lay->addWidget(lblCom);
    lay->addSpacing(10);
    lay->addWidget(m_aiCommentLabel);
    lay->addStretch(1);

    QScrollArea *scroll = new QScrollArea(ui->rapport_ai);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(scrollInner);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background-color: rgb(253, 255, 237); border: none; }"));

    QVBoxLayout *outer = new QVBoxLayout(ui->rapport_ai);
    outer->setContentsMargins(4, 4, 4, 4);
    outer->setSpacing(0);
    outer->addWidget(scroll, 1);

    clearAiResultsPanel();
}

void MainWindow::clearAiResultsPanel()
{
    if (m_aiScoreBar)
        m_aiScoreBar->setValue(0);
    if (m_aiScoreValue)
        m_aiScoreValue->setText(QStringLiteral("—"));
    if (m_aiRiskBadge)
        m_aiRiskBadge->setText(QStringLiteral("—"));
    applyPreventiveScoreAppearance(m_aiScoreBar, m_aiScoreValue, m_aiRiskBadge, 0, true);
    if (m_aiCommentLabel)
        m_aiCommentLabel->setText(QStringLiteral("—"));
}

void MainWindow::updateAiResultsPanel(int score, const QString &risk, const QString &comment)
{
    qDebug() << "[UPDATE_UI] updateAiResultsPanel called - score:" << score << "risk:" << risk << "comment:" << comment;
    const int bounded = qBound(0, score, 100);
    if (m_aiScoreBar) {
        qDebug() << "[UPDATE_UI] Setting score bar value to:" << bounded;
        m_aiScoreBar->setValue(bounded);
    } else {
        qDebug() << "[UPDATE_UI] ERROR: m_aiScoreBar is NULL!";
    }
    if (m_aiScoreValue) {
        qDebug() << "[UPDATE_UI] Setting score value text to:" << bounded;
        m_aiScoreValue->setText(QString::number(bounded));
    } else {
        qDebug() << "[UPDATE_UI] ERROR: m_aiScoreValue is NULL!";
    }
    if (m_aiRiskBadge) {
        qDebug() << "[UPDATE_UI] Setting risk badge text to:" << risk;
        m_aiRiskBadge->setText(risk.isEmpty() ? QStringLiteral("—") : risk);
    } else {
        qDebug() << "[UPDATE_UI] ERROR: m_aiRiskBadge is NULL!";
    }
    applyPreventiveScoreAppearance(m_aiScoreBar, m_aiScoreValue, m_aiRiskBadge, bounded, false);
    if (m_aiCommentLabel) {
        qDebug() << "[UPDATE_UI] Setting comment text to:" << comment;
        m_aiCommentLabel->setText(comment.isEmpty() ? QStringLiteral("—") : comment);
    } else {
        qDebug() << "[UPDATE_UI] ERROR: m_aiCommentLabel is NULL!";
    }
}

void MainWindow::openAiAnalysisPage()
{
    setupAiAnalysisUi();
    qDebug() << "[OPEN_AI_PAGE] Setting up Gemini signal connections";
    bool connected1 = connect(&m_geminiClient, &GeminiClient::analysisComplete, this, &MainWindow::onGeminiAnalysisComplete, Qt::DirectConnection);
    bool connected2 = connect(&m_geminiClient, &GeminiClient::analysisFailed, this, &MainWindow::onGeminiAnalysisFailed, Qt::DirectConnection);
    bool connectedSave = connect(ui->modif_ai, &QPushButton::clicked, this, &MainWindow::saveAiAnalysisResults, Qt::UniqueConnection);
    qDebug() << "[OPEN_AI_PAGE] analysisComplete connected:" << connected1;
    qDebug() << "[OPEN_AI_PAGE] analysisFailed connected:" << connected2;
    qDebug() << "[OPEN_AI_PAGE] modif_ai connected:" << connectedSave;
    ui->stackedwidget->setCurrentIndex(kStackedIndexAiAnalysisPage);
    loadAiMachinesTable();
    clearAiResultsPanel();
    m_aiPendingMaterialId = -1;
    ui->modif_ai->setEnabled(false);
    if (m_aiStatusLabel)
        m_aiStatusLabel->setText(tr("Sélectionnez une machine puis lancez l'analyse."));
}

void MainWindow::on_fen_ai_clicked()
{
    openAiAnalysisPage();
}

void MainWindow::loadAiMachinesTable()
{
    QTableWidget *t = ui->tableMaterials_2;
    if (!t) {
        QMessageBox::warning(nullptr, tr("Erreur"), tr("Table machines AI non initialisée."));
        return;
    }
    t->setUpdatesEnabled(false);
    t->setSortingEnabled(false);
    t->setRowCount(0);
    t->setColumnCount(12);
    t->setHorizontalHeaderLabels(QStringList()
        << tr("ID") << tr("Nom") << tr("Atelier") << tr("État santé") << tr("Date achat") << tr("Dernier entretien")
        << tr("Fréq. util.") << tr("Nb incidents") << tr("Score prév.") << tr("Indice risque") << tr("Commentaire") << tr("Actions"));
    t->setColumnHidden(0, true);

    QString fetchError;
    const QList<Material> materials = Material::fetchAllById(&fetchError);
    if (!fetchError.isEmpty()) {
        t->setUpdatesEnabled(true);
        QMessageBox::critical(nullptr, tr("Erreur"), tr("Impossible de charger les machines : ") + fetchError);
        return;
    }

    int row = 0;
    for (const Material &m : materials) {
        const int idMat = m.idMateriel();
        t->insertRow(row);
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(idMat));
        idItem->setData(Qt::UserRole, idMat);
        t->setItem(row, 0, idItem);
        t->setItem(row, 1, new QTableWidgetItem(m.nomMateriel()));
        t->setItem(row, 2, new QTableWidgetItem(m.atelier()));
        t->setItem(row, 3, new QTableWidgetItem(m.etatSante()));
        t->setItem(row, 4, new QTableWidgetItem(m.dateAchat().isValid() ? m.dateAchat().toString(Qt::ISODate) : QString()));
        t->setItem(row, 5, new QTableWidgetItem(m.dateDernierEntretien().isValid() ? m.dateDernierEntretien().toString(Qt::ISODate) : QString()));
        t->setItem(row, 6, new QTableWidgetItem(QString::number(m.frequenceUtilisation())));
        t->setItem(row, 7, new QTableWidgetItem(QString::number(m.nombreIncidents())));
        auto *scoreItemIa = new QTableWidgetItem();
        styleTableItemForPreventiveScore(scoreItemIa, m.scorePreventif() >= 0 ? QVariant(m.scorePreventif()) : QVariant());
        t->setItem(row, 8, scoreItemIa);
        t->setItem(row, 9, new QTableWidgetItem(riskPanneDbValueToLabel(m.indiceRisquePanne() >= 0 ? QVariant(m.indiceRisquePanne()) : QVariant())));
        QString com = m.commentaire();
        if (com.length() > 80)
            com = com.left(77) + QLatin1String("…");
        t->setItem(row, 10, new QTableWidgetItem(com));

        QPushButton *btnIa = new QPushButton(tr("Analyser IA"), t);
        btnIa->setProperty("materialId", idMat);
        btnIa->setStyleSheet(
            "QPushButton { background-color: #414833; color: #FFFFFF; border-radius: 8px; padding: 4px 10px; font-weight: 600; font-size: 12px; }"
            "QPushButton:hover { background-color: #656D4A; }"
            "QPushButton:disabled { background-color: #9e9e9e; }");
        connect(btnIa, &QPushButton::clicked, this, [this, idMat]() { startAiAnalysisForMaterialId(idMat); });
        t->setCellWidget(row, 11, btnIa);
        row++;
    }
    t->setUpdatesEnabled(true);
}

void MainWindow::setAiTableButtonsEnabled(bool enabled)
{
    QTableWidget *t = ui->tableMaterials_2;
    if (!t) return;
    for (int r = 0; r < t->rowCount(); ++r) {
        if (QWidget *w = t->cellWidget(r, 11))
            if (auto *b = qobject_cast<QPushButton *>(w))
                b->setEnabled(enabled);
    }
}

Material MainWindow::fetchMachineDataForAi(int id)
{
    Material m;
    QString err;
    if (!Material::fetchById(id, &m, &err))
        return Material();
    return m;
}

void MainWindow::startAiAnalysisForMaterialId(int materialId)
{
    qDebug() << "[ANALYSIS_START] startAiAnalysisForMaterialId called with materialId:" << materialId;
    if (m_aiAnalysisBusy) {
        qDebug() << "[ANALYSIS_START] Analysis already busy, aborting";
        QMessageBox::information(nullptr, tr("Analyse IA"), tr("Une analyse est déjà en cours. Veuillez patienter."));
        return;
    }
    if (materialId <= 0) {
        qDebug() << "[ANALYSIS_START] Invalid materialId, aborting";
        return;
    }

    Material material = fetchMachineDataForAi(materialId);
    if (!material.isValid()) {
        qDebug() << "[ANALYSIS_START] Material data invalid, aborting";
        QMessageBox::warning(nullptr, tr("Analyse IA"), tr("Impossible de charger les données de cette machine."));
        return;
    }

    qDebug() << "[ANALYSIS_START] Material data loaded successfully";
    m_aiPendingMaterialId = materialId;
    m_aiAnalysisBusy = true;
    ui->gen_rapport_AI->setEnabled(false);
    ui->modif_ai->setEnabled(false);
    setAiTableButtonsEnabled(false);
    if (m_aiStatusLabel)
        m_aiStatusLabel->setText(tr("Analyse en cours avec Ollama…"));
    clearAiResultsPanel();
    if (m_aiCommentLabel)
        m_aiCommentLabel->setText(tr("Veuillez patienter."));

    qDebug() << "[ANALYSIS_START] Calling m_geminiClient.analyzeMachine()";
    m_geminiClient.analyzeMachine(material.toVariantMap());
}

void MainWindow::on_gen_rapport_AI_clicked()
{
    const int row = ui->tableMaterials_2->currentRow();
    if (row < 0) {
        QMessageBox::information(nullptr, tr("Analyse IA"), tr("Sélectionnez d'abord une ligne dans le tableau."));
        return;
    }
    QTableWidgetItem *idItem = ui->tableMaterials_2->item(row, 0);
    if (!idItem) {
        QMessageBox::warning(nullptr, tr("Analyse IA"), tr("Impossible d'identifier la machine sélectionnée."));
        return;
    }
    int id = idItem->data(Qt::UserRole).toInt();
    if (id <= 0)
        id = idItem->text().toInt();
    startAiAnalysisForMaterialId(id);
}

void MainWindow::onGeminiAnalysisComplete(int score, const QString &risk, const QString &comment)
{
    qDebug() << "[SLOT] onGeminiAnalysisComplete called - score:" << score << "risk:" << risk << "comment:" << comment;
    qDebug() << "[SLOT] m_aiScoreBar is:" << (m_aiScoreBar ? "valid" : "NULL");
    qDebug() << "[SLOT] m_aiScoreValue is:" << (m_aiScoreValue ? "valid" : "NULL");
    qDebug() << "[SLOT] m_aiRiskBadge is:" << (m_aiRiskBadge ? "valid" : "NULL");
    qDebug() << "[SLOT] m_aiCommentLabel is:" << (m_aiCommentLabel ? "valid" : "NULL");
    
    m_aiAnalysisBusy = false;
    ui->gen_rapport_AI->setEnabled(true);
    ui->modif_ai->setEnabled(true);
    setAiTableButtonsEnabled(true);
    m_aiPendingScore = score;
    m_aiPendingRisk = risk;
    m_aiPendingComment = comment;
    updateAiResultsPanel(score, risk, comment);
    if (m_aiStatusLabel)
        m_aiStatusLabel->setText(tr("Analyse terminée. Vérifiez les résultats puis cliquez sur « Enregistrer l'analyse »."));
}

void MainWindow::onGeminiAnalysisFailed(const QString &error)
{
    qDebug() << "[SLOT] onGeminiAnalysisFailed called - error:" << error;
    m_aiAnalysisBusy = false;
    ui->gen_rapport_AI->setEnabled(true);
    setAiTableButtonsEnabled(true);
    if (m_aiStatusLabel)
        m_aiStatusLabel->setText(tr("Échec de l'analyse."));
    QMessageBox::warning(nullptr, tr("Analyse IA"), error);
}

void MainWindow::updateAiTableRowForMaterial(int materialId, int score, const QString &risk, const QString &comment)
{
    QTableWidget *t = ui->tableMaterials_2;
    if (!t) return;
    for (int r = 0; r < t->rowCount(); ++r) {
        QTableWidgetItem *idItem = t->item(r, 0);
        if (!idItem) continue;
        int id = idItem->data(Qt::UserRole).toInt();
        if (id <= 0) id = idItem->text().toInt();
        if (id != materialId) continue;
        if (t->item(r, 8))
            styleTableItemForPreventiveScore(t->item(r, 8), QVariant(score));
        if (t->item(r, 9)) t->item(r, 9)->setText(risk);
        QString shortCom = comment;
        if (shortCom.length() > 80)
            shortCom = shortCom.left(77) + QLatin1String("…");
        if (t->item(r, 10)) t->item(r, 10)->setText(shortCom);
        t->viewport()->update();
        break;
    }
}

void MainWindow::saveAiAnalysisResults()
{
    qDebug() << "[SAVE_AI] saveAiAnalysisResults clicked. pending material id:" << m_aiPendingMaterialId;
    if (m_aiPendingMaterialId <= 0) {
        QMessageBox::information(nullptr, tr("Analyse IA"), tr("Aucun résultat d'analyse à enregistrer. Lancez d'abord une analyse."));
        return;
    }

    ui->modif_ai->setEnabled(false);

    QString dbError;
    if (!Material::updateAiIndicators(
            m_aiPendingMaterialId,
            m_aiPendingScore,
            riskPanneLabelToOracleCode(m_aiPendingRisk),
            m_aiPendingComment,
            &dbError)) {
        QMessageBox::critical(nullptr, tr("Erreur"), tr("Échec de la mise à jour : ") + dbError);
        ui->modif_ai->setEnabled(true);
        return;
    }

    updateAiTableRowForMaterial(m_aiPendingMaterialId, m_aiPendingScore, m_aiPendingRisk, m_aiPendingComment);
    if (ui->stackedwidget->currentIndex() == 7)
        loadMachines();

    if (m_aiStatusLabel)
        m_aiStatusLabel->setText(tr("Indicateurs enregistrés en base pour la machine #%1.").arg(m_aiPendingMaterialId));
    QMessageBox::information(nullptr, tr("Analyse IA"), tr("Les indicateurs ont été enregistrés."));
    m_aiPendingMaterialId = -1;
}