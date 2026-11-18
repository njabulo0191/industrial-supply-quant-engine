#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QMap>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleAddMaterial();
    void handleEditMaterial();
    void handleAddMaterialCategory();
    void logToPythonTelemetry(const QString& action, const QString& productionStream, const QString& category, const QString& name, double cost, double efficiency, double quantity, const QString& unitType, const QString& itemId, double annualDemand, double orderingCost, double holdingCost);
    void loadSelectedMaterial();
    void updateVariantControls();
    void refreshVariantName();

private:
    // Core Layout Windows
    QWidget *centralWidget;
    QFormLayout *formLayout;
    QHBoxLayout *buttonLayout;
    QVBoxLayout *mainLayout;
    QStackedWidget *pageStack;
    QWidget *streamPage;
    QWidget *materialsPage;
    QLabel *selectedStreamLabel;

    // Interactive Front-End Input Fields
    QComboBox *categoryDropdown;
    QLineEdit *materialNameInput;
    QComboBox *variantDropdown;
    QComboBox *colorDropdown;
    QComboBox *packSizeDropdown;
    QLabel *variantLabel;
    QLabel *colorLabel;
    QLabel *packSizeLabel;
    QPushButton *addCategoryButton;
    QDoubleSpinBox *quantitySpinBox;
    QDoubleSpinBox *unitCostSpinBox;
    QDoubleSpinBox *efficiencySpinBox;
    QDoubleSpinBox *annualDemandSpinBox;
    QDoubleSpinBox *orderingCostSpinBox;
    QDoubleSpinBox *holdingCostSpinBox;

    // Trigger Buttons
    QPushButton *addButton;
    QPushButton *editButton;

    // Data Matrix Dashboard Grid Display
    QTableWidget *inventoryTable;

    QString selectedStream;
    QMap<QString, QString> categoryUnits;
    QMap<QString, QMap<QString, QString>> streamCategoryUnits;

    void setupUserInterface();
    void showMaterialsForStream(const QString& stream);
    void loadCurrentInventory();
    void loadCategoryDefinitions();
    void saveCategoryDefinitions();
    QStringList categoriesForStream(const QString& stream) const;
    void populateCategoryDropdown();
    QString unitForCategory(const QString& category) const;
    bool processTelemetryLog(const QString& logPath, QString* errorMessage);
};

#endif // MAINWINDOW_H
