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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleAddMaterial();
    void handleEditMaterial();
    void logToPythonTelemetry(const QString& action, const QString& category, const QString& name, double cost, double efficiency, int quantity, const QString& unitType);

private:
    // Core Layout Windows
    QWidget *centralWidget;
    QFormLayout *formLayout;
    QHBoxLayout *buttonLayout;
    QVBoxLayout *mainLayout;

    // Interactive Front-End Input Fields
    QComboBox *categoryDropdown;
    QLineEdit *materialNameInput;
    QDoubleSpinBox *unitCostSpinBox;
    QDoubleSpinBox *efficiencySpinBox;

    // Trigger Buttons
    QPushButton *addButton;
    QPushButton *editButton;

    // Data Matrix Dashboard Grid Display
    QTableWidget *inventoryTable;

    void setupUserInterface();
};

#endif // MAINWINDOW_H
