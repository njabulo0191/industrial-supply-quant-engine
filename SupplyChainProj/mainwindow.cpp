#include "mainwindow.h"
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QSpinBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUserInterface();
    setWindowTitle("Industrial Supply Chain & ERP Management Dashboard (CCBSA-Inspired)");
    resize(850, 550);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUserInterface() {
    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);
    formLayout = new QFormLayout();

    // 1. Configure the Dropdown for specific industrial raw materials
    categoryDropdown = new QComboBox(this);
    categoryDropdown->addItems({
        "Labels", "Stretch Wrapper", "Shrinkwrapper",
        "Pallets", "Syrup", "CO2", "Treated Water", "Hot Glue", "Preforms"
    });

    materialNameInput = new QLineEdit(this);
    materialNameInput->setPlaceholderText("e.g., Gloss Chrome Wrap, Liquid Carbonic Tank");

    unitCostSpinBox = new QDoubleSpinBox(this);
    unitCostSpinBox->setRange(0.00, 100000000.00);
    unitCostSpinBox->setPrefix("R ");

    efficiencySpinBox = new QDoubleSpinBox(this);
    efficiencySpinBox->setRange(0.00, 100.00);
    efficiencySpinBox->setSuffix(" %");
    efficiencySpinBox->setValue(95.00);

    // NEW FIELD: Quantity capture matching CCBSA operational volumes
    QSpinBox *quantitySpinBox = new QSpinBox(this);
    quantitySpinBox->setObjectName("quantitySpinBox");
    quantitySpinBox->setRange(1, 100000000);
    quantitySpinBox->setValue(100);

    // Populate Form Layout Fields
    formLayout->addRow(new QLabel("Material Category:"), categoryDropdown);
    formLayout->addRow(new QLabel("Material Name / Identifier:"), materialNameInput);
    formLayout->addRow(new QLabel("Unit Commodity Cost (ZAR):"), unitCostSpinBox);
    formLayout->addRow(new QLabel("Plant Target Yield Efficiency:"), efficiencySpinBox);
    formLayout->addRow(new QLabel("Inventory Batch Quantity:"), quantitySpinBox);

    // 2. Action Trigger Buttons
    buttonLayout = new QHBoxLayout();
    addButton = new QPushButton("Capture New Material", this);
    editButton = new QPushButton("Update Existing Configuration", this);
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(editButton);

    // 3. Setup the Grid Table Dashboard Display (Expanded to 6 columns)
    inventoryTable = new QTableWidget(this);
    inventoryTable->setColumnCount(6);
    inventoryTable->setHorizontalHeaderLabels({"Category", "Material Name", "Unit Cost", "Qty", "Unit Metric", "Yield Eff."});
    inventoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    inventoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Assembly lines
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(inventoryTable);
    setCentralWidget(centralWidget);

    // Connect Click Event Listener Signals to Action Slots
    connect(addButton, &QPushButton::clicked, this, &MainWindow::handleAddMaterial);
    connect(editButton, &QPushButton::clicked, this, &MainWindow::handleEditMaterial);
}

void MainWindow::handleAddMaterial() {
    QString cat = categoryDropdown->currentText();
    QString name = materialNameInput->text().trimmed();
    double cost = unitCostSpinBox->value();
    double eff = efficiencySpinBox->value();
    int qty = centralWidget->findChild<QSpinBox*>("quantitySpinBox")->value();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Material Name identifier cannot be blank!");
        return;
    }

    // Dynamic industrial metrics mapping rules
    QString unitType = "Units";
    if (cat == "Syrup" || cat == "Hot Glue") unitType = "Kilograms";
    else if (cat == "CO2") unitType = "Bar/Liters";
    else if (cat == "Treated Water") unitType = "Liters";
    else if (cat == "Stretch Wrapper" || cat == "Shrinkwrapper") unitType = "Meters";
    else if (cat == "Preforms" || cat == "Labels" || cat == "Pallets") unitType = "Units";

    // Insert row dynamically into visual table widget
    int rowCount = inventoryTable->rowCount();
    inventoryTable->insertRow(rowCount);
    inventoryTable->setItem(rowCount, 0, new QTableWidgetItem(cat));
    inventoryTable->setItem(rowCount, 1, new QTableWidgetItem(name));
    inventoryTable->setItem(rowCount, 2, new QTableWidgetItem(QString("R %1").arg(cost, 0, 'f', 2)));
    inventoryTable->setItem(rowCount, 3, new QTableWidgetItem(QString::number(qty)));
    inventoryTable->setItem(rowCount, 4, new QTableWidgetItem(unitType));
    inventoryTable->setItem(rowCount, 5, new QTableWidgetItem(QString("%1 %").arg(eff, 0, 'f', 2)));

    // Log this capture interface action straight to the telemetry pipeline bridge (8 fields payload)
    logToPythonTelemetry("CAPTURE", cat, name, cost, eff, qty, unitType);
    materialNameInput->clear();
}

void MainWindow::handleEditMaterial() {
    int selectedRow = inventoryTable->currentRow();
    if (selectedRow < 0) {
        QMessageBox::information(this, "Selection Required", "Please select a material row from the table below to edit.");
        return;
    }

    QString cat = categoryDropdown->currentText();
    QString name = materialNameInput->text().trimmed();
    double cost = unitCostSpinBox->value();
    double eff = efficiencySpinBox->value();
    int qty = centralWidget->findChild<QSpinBox*>("quantitySpinBox")->value();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Provide a modified text name to update this row.");
        return;
    }

    QString unitType = "Units";
    if (cat == "Syrup" || cat == "Hot Glue") unitType = "Kilograms";
    else if (cat == "CO2") unitType = "Bar/Liters";
    else if (cat == "Treated Water") unitType = "Liters";
    else if (cat == "Stretch Wrapper" || cat == "Shrinkwrapper") unitType = "Meters";

    // Overwrite fields in the visual table row
    inventoryTable->setItem(selectedRow, 0, new QTableWidgetItem(cat));
    inventoryTable->setItem(selectedRow, 1, new QTableWidgetItem(name));
    inventoryTable->setItem(selectedRow, 2, new QTableWidgetItem(QString("R %1").arg(cost, 0, 'f', 2)));
    inventoryTable->setItem(selectedRow, 3, new QTableWidgetItem(QString::number(qty)));
    inventoryTable->setItem(selectedRow, 4, new QTableWidgetItem(unitType));
    inventoryTable->setItem(selectedRow, 5, new QTableWidgetItem(QString("%1 %").arg(eff, 0, 'f', 2)));

    logToPythonTelemetry("EDIT", cat, name, cost, eff, qty, unitType);
}

void MainWindow::logToPythonTelemetry(const QString& action, const QString& category, const QString& name, double cost, double efficiency, int quantity, const QString& unitType) {
    QFile file("raw_interface_telemetry.log");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

        // Expanded pipe-separated string including industrial volumes:
        // TIMESTAMP|ACTION|CATEGORY|NAME|COST|EFFICIENCY|QUANTITY|UNIT_TYPE
        stream << timestamp << "|" << action << "|" << category << "|" << name << "|" << cost << "|" << (efficiency / 100.0) << "|" << quantity << "|" << unitType << "\n";
        file.close();
    }
}


