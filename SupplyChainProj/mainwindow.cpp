#include "mainwindow.h"
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QSpinBox>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QStatusBar>
#include <QUuid>
#include <QStringList>
#include <QGridLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalBlocker>
#include <QSet>
#include "telemetry_project_path.h"

namespace {
QString findPythonInterpreter(QStringList &prefixArguments) {
    const QString localAppData = qEnvironmentVariable("LOCALAPPDATA");
    if (!localAppData.isEmpty()) {
        QDir pythonRoot(QDir(localAppData).filePath("Python"));
        const QStringList runtimes = pythonRoot.entryList(
            QStringList() << "pythoncore-*", QDir::Dirs, QDir::Name | QDir::Reversed);
        for (const QString &runtime : runtimes) {
            const QString candidate = pythonRoot.filePath(runtime + "/python.exe");
            if (QFileInfo(candidate).isExecutable()) {
                return candidate;
            }
        }
    }
    const QString launcher = QStandardPaths::findExecutable("py");
    if (!launcher.isEmpty()) {
        prefixArguments << "-3";
        return launcher;
    }
    for (const QString &name : {QStringLiteral("python"), QStringLiteral("python3")}) {
        const QString candidate = QStandardPaths::findExecutable(name);
        if (!candidate.isEmpty()) {
            return candidate;
        }
    }
    return QString();
}

QStringList parseCsvLine(const QString &line) {
    QStringList fields;
    QString field;
    bool inQuotes = false;
    for (qsizetype i = 0; i < line.size(); ++i) {
        const QChar character = line.at(i);
        if (character == QLatin1Char('"')) {
            if (inQuotes && i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                field += QLatin1Char('"');
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (character == QLatin1Char(',') && !inQuotes) {
            fields << field;
            field.clear();
        } else {
            field += character;
        }
    }
    fields << field;
    return fields;
}
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    loadCategoryDefinitions();
    setupUserInterface();
    setWindowTitle("Industrial Supply Chain & ERP Management Dashboard (CCBSA-Inspired)");
    resize(1080, 820);

    QDir().mkpath(QFileInfo(QStringLiteral(TELEMETRY_LOG_PATH)).absolutePath());
    if (!QFileInfo::exists(QStringLiteral(TELEMETRY_LOG_PATH)) &&
            QFileInfo::exists(QStringLiteral(TELEMETRY_LEGACY_LOG_PATH))) {
        QFile::copy(QStringLiteral(TELEMETRY_LEGACY_LOG_PATH), QStringLiteral(TELEMETRY_LOG_PATH));
    }
    const QString logPath = QStringLiteral(TELEMETRY_LOG_PATH);
    if (QFileInfo::exists(logPath)) {
        QString errorMessage;
        if (!processTelemetryLog(logPath, &errorMessage)) {
            statusBar()->showMessage(errorMessage, 15000);
        }
    }
    loadCurrentInventory();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUserInterface() {
    centralWidget = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(centralWidget);
    pageStack = new QStackedWidget(centralWidget);
    rootLayout->addWidget(pageStack);

    streamPage = new QWidget(pageStack);
    auto *streamLayout = new QVBoxLayout(streamPage);
    auto *streamTitle = new QLabel("Choose a production stream", streamPage);
    streamTitle->setAlignment(Qt::AlignCenter);
    QFont titleFont = streamTitle->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    streamTitle->setFont(titleFont);
    auto *streamDescription = new QLabel("Select the line family to open material tracking.", streamPage);
    streamDescription->setAlignment(Qt::AlignCenter);
    streamLayout->addStretch();
    streamLayout->addWidget(streamTitle);
    streamLayout->addWidget(streamDescription);
    auto *streamGrid = new QGridLayout();
    const QStringList streamNames = {"PET", "RGB", "RefPET", "Can"};
    for (int i = 0; i < streamNames.size(); ++i) {
        auto *streamButton = new QPushButton(streamNames.at(i), streamPage);
        streamButton->setMinimumSize(250, 110);
        QFont streamFont = streamButton->font();
        streamFont.setPointSize(18);
        streamFont.setBold(true);
        streamButton->setFont(streamFont);
        streamGrid->addWidget(streamButton, i / 2, i % 2);
        connect(streamButton, &QPushButton::clicked, this, [this, stream = streamNames.at(i)] {
            showMaterialsForStream(stream);
        });
    }
    streamLayout->addLayout(streamGrid);
    streamLayout->addStretch();
    pageStack->addWidget(streamPage);

    materialsPage = new QWidget(pageStack);
    mainLayout = new QVBoxLayout(materialsPage);
    formLayout = new QFormLayout();

    auto *navigationLayout = new QHBoxLayout();
    auto *backButton = new QPushButton("← Production streams", materialsPage);
    selectedStreamLabel = new QLabel(materialsPage);
    QFont selectedStreamFont = selectedStreamLabel->font();
    selectedStreamFont.setPointSize(14);
    selectedStreamFont.setBold(true);
    selectedStreamLabel->setFont(selectedStreamFont);
    addCategoryButton = new QPushButton("Add Material Category", materialsPage);
    navigationLayout->addWidget(backButton);
    navigationLayout->addWidget(selectedStreamLabel, 1, Qt::AlignCenter);
    navigationLayout->addWidget(addCategoryButton);
    mainLayout->addLayout(navigationLayout);
    connect(backButton, &QPushButton::clicked, this, [this] { pageStack->setCurrentWidget(streamPage); });
    connect(addCategoryButton, &QPushButton::clicked, this, &MainWindow::handleAddMaterialCategory);

    categoryDropdown = new QComboBox(this);
    populateCategoryDropdown();

    materialNameInput = new QLineEdit(this);
    materialNameInput->setPlaceholderText("Select options above or enter a material identifier");

    variantDropdown = new QComboBox(this);
    variantDropdown->setEditable(true);
    colorDropdown = new QComboBox(this);
    colorDropdown->setEditable(true);
    packSizeDropdown = new QComboBox(this);
    packSizeDropdown->setEditable(true);
    variantLabel = new QLabel("Flavour / Product:", this);
    colorLabel = new QLabel("Colour:", this);
    packSizeLabel = new QLabel("Pack size:", this);

    unitCostSpinBox = new QDoubleSpinBox(this);
    unitCostSpinBox->setRange(0.00, 100000000.00);
    unitCostSpinBox->setPrefix("R ");

    efficiencySpinBox = new QDoubleSpinBox(this);
    efficiencySpinBox->setRange(0.00, 100.00);
    efficiencySpinBox->setSuffix(" %");
    efficiencySpinBox->setValue(95.00);

    quantitySpinBox = new QDoubleSpinBox(this);
    quantitySpinBox->setObjectName("quantitySpinBox");
    quantitySpinBox->setRange(0.001, 100000000.0);
    quantitySpinBox->setDecimals(3);
    quantitySpinBox->setValue(100.0);

    formLayout->addRow(new QLabel("Material Category:"), categoryDropdown);
    formLayout->addRow(new QLabel("Material Name / Identifier:"), materialNameInput);
    formLayout->addRow(variantLabel, variantDropdown);
    formLayout->addRow(colorLabel, colorDropdown);
    formLayout->addRow(packSizeLabel, packSizeDropdown);
    formLayout->addRow(new QLabel("Unit Commodity Cost (ZAR):"), unitCostSpinBox);
    formLayout->addRow(new QLabel("Plant Target Yield Efficiency:"), efficiencySpinBox);
    formLayout->addRow(new QLabel("Inventory Quantity:"), quantitySpinBox);

    annualDemandSpinBox = new QDoubleSpinBox(this);
    annualDemandSpinBox->setRange(0.00, 1000000000000.00);
    annualDemandSpinBox->setDecimals(2);
    annualDemandSpinBox->setSuffix(" metric units/year");
    formLayout->addRow(new QLabel("Annual Demand (optional for EOQ):"), annualDemandSpinBox);

    orderingCostSpinBox = new QDoubleSpinBox(this);
    orderingCostSpinBox->setRange(0.00, 100000000.00);
    orderingCostSpinBox->setDecimals(2);
    orderingCostSpinBox->setPrefix("R ");
    formLayout->addRow(new QLabel("Ordering Cost (ZAR/order):"), orderingCostSpinBox);

    holdingCostSpinBox = new QDoubleSpinBox(this);
    holdingCostSpinBox->setRange(0.00, 100000000.00);
    holdingCostSpinBox->setDecimals(2);
    holdingCostSpinBox->setPrefix("R ");
    formLayout->addRow(new QLabel("Holding Cost (ZAR/metric unit/year):"), holdingCostSpinBox);

    buttonLayout = new QHBoxLayout();
    addButton = new QPushButton("Capture New Material", this);
    editButton = new QPushButton("Update Existing Configuration", this);
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(editButton);

    inventoryTable = new QTableWidget(this);
    inventoryTable->setColumnCount(7);
    inventoryTable->setHorizontalHeaderLabels({"Stream", "Category", "Material Variant", "Unit Cost", "Qty", "SI Unit", "Yield Eff."});
    inventoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    inventoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *formWrapper = new QWidget(materialsPage);
    formWrapper->setLayout(formLayout);
    mainLayout->addWidget(formWrapper);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(inventoryTable);
    pageStack->addWidget(materialsPage);
    setCentralWidget(centralWidget);

    connect(addButton, &QPushButton::clicked, this, &MainWindow::handleAddMaterial);
    connect(editButton, &QPushButton::clicked, this, &MainWindow::handleEditMaterial);
    connect(inventoryTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::loadSelectedMaterial);
    connect(categoryDropdown, &QComboBox::currentTextChanged, this, &MainWindow::updateVariantControls);
    connect(variantDropdown, &QComboBox::currentTextChanged, this, &MainWindow::refreshVariantName);
    connect(colorDropdown, &QComboBox::currentTextChanged, this, &MainWindow::refreshVariantName);
    connect(packSizeDropdown, &QComboBox::currentTextChanged, this, &MainWindow::refreshVariantName);
    updateVariantControls();
    pageStack->setCurrentWidget(streamPage);
}

void MainWindow::loadCategoryDefinitions() {
    categoryUnits = {
        {"Labels", "Units"}, {"Stretch Wrapper", "Meters"},
        {"Shrinkwrapper", "Meters"}, {"Pallets", "Units"},
        {"Syrup", "Kilograms"}, {"CO2", "Bar/Liters"},
        {"Treated Water", "Liters"}, {"Hot Glue", "Kilograms"},
        {"Preforms", "Units"}, {"Closures", "Units"},
        {"Bottles", "Units"}, {"Crates", "Units"}, {"Cans", "Units"},
        {"Top Lid", "Units"}
    };
    const QString projectRoot = QStringLiteral(TELEMETRY_PROJECT_ROOT);
    QFile file(QDir(projectRoot).filePath("material_categories.json"));
    if (!file.open(QIODevice::ReadOnly)) return;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    const QJsonObject root = document.object();
    const QJsonObject customByStream = root.value("custom_categories_by_stream").toObject();
    for (auto streamIt = customByStream.begin(); streamIt != customByStream.end(); ++streamIt) {
        const QJsonObject customCategories = streamIt.value().toObject();
        for (auto it = customCategories.begin(); it != customCategories.end(); ++it) {
            const QString unit = it.value().toString().trimmed();
            if (!it.key().trimmed().isEmpty() && !unit.isEmpty())
                streamCategoryUnits[streamIt.key()].insert(it.key(), unit);
        }
    }
    // Migrate categories saved by the previous global-category version.
    const QJsonObject legacyCategories = root.value("custom_categories").toObject();
    for (auto it = legacyCategories.begin(); it != legacyCategories.end(); ++it) {
        const QString unit = it.value().toString().trimmed();
        if (it.key().trimmed().isEmpty() || unit.isEmpty()) continue;
        const QStringList legacyStreams = {"PET", "RGB", "RefPET", "Can"};
        for (const QString &stream : legacyStreams) {
            streamCategoryUnits[stream].insert(it.key(), unit);
        }
    }
}

void MainWindow::saveCategoryDefinitions() {
    QJsonObject customByStream;
    for (auto streamIt = streamCategoryUnits.cbegin(); streamIt != streamCategoryUnits.cend(); ++streamIt) {
        QJsonObject customCategories;
        const QMap<QString, QString> streamCategories = streamIt.value();
        for (auto it = streamCategories.cbegin(); it != streamCategories.cend(); ++it) {
            customCategories.insert(it.key(), it.value());
        }
        customByStream.insert(streamIt.key(), customCategories);
    }
    QJsonObject root;
    root.insert("custom_categories_by_stream", customByStream);
    const QString projectRoot = QStringLiteral(TELEMETRY_PROJECT_ROOT);
    QFile file(QDir(projectRoot).filePath("material_categories.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, "Category Save Error",
                             "Could not save material categories:\n" + file.errorString());
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

QStringList MainWindow::categoriesForStream(const QString& stream) const {
    QStringList categories;
    if (stream == "RefPET" || stream == "RGB") {
        categories = {"Syrup", "Labels", "Closures", "Bottles", "Crates", "Pallets"};
    } else if (stream == "Can") {
        categories = {"Syrup", "Cans", "Top Lid", "Shrinkwrapper", "Stretch Wrapper", "Pallets"};
    } else {
        categories = {"Labels", "Stretch Wrapper", "Shrinkwrapper", "Pallets", "Syrup",
            "CO2", "Treated Water", "Hot Glue", "Preforms", "Closures", "Bottles", "Crates"};
    }
    const auto custom = streamCategoryUnits.constFind(stream);
    if (custom != streamCategoryUnits.cend()) {
        for (auto it = custom.value().cbegin(); it != custom.value().cend(); ++it) {
            if (!categories.contains(it.key())) categories.append(it.key());
        }
    }
    return categories;
}

void MainWindow::populateCategoryDropdown() {
    const QString current = categoryDropdown->currentText();
    categoryDropdown->clear();
    categoryDropdown->addItems(categoriesForStream(selectedStream));
    if (!current.isEmpty() && categoryDropdown->findText(current) >= 0)
        categoryDropdown->setCurrentText(current);
}

QString MainWindow::unitForCategory(const QString& category) const {
    const auto streamCategories = streamCategoryUnits.constFind(selectedStream);
    if (streamCategories != streamCategoryUnits.cend() && streamCategories.value().contains(category))
        return streamCategories.value().value(category);
    return categoryUnits.value(category, "Units");
}

void MainWindow::showMaterialsForStream(const QString& stream) {
    selectedStream = stream;
    selectedStreamLabel->setText("Production stream: " + selectedStream);
    populateCategoryDropdown();
    updateVariantControls();
    pageStack->setCurrentWidget(materialsPage);
    categoryDropdown->setFocus();
}

void MainWindow::updateVariantControls() {
    const QString category = categoryDropdown->currentText();
    const bool labels = category == "Labels";
    const bool syrup = category == "Syrup";
    const bool preform = category == "Preforms";
    const bool wrapper = category == "Stretch Wrapper" || category == "Shrinkwrapper";
    const bool closure = category == "Closures";
    const bool bottleCount = category == "Bottles";
    const bool canCount = category == "Cans";
    const bool fixedName = category == "Crates" || category == "Pallets" || category == "Top Lid";
    const bool usesVariant = labels || syrup || wrapper;
    const bool usesColor = preform || wrapper || closure;
    const bool usesPack = labels || preform || wrapper || closure || bottleCount || canCount;

    variantLabel->setText(syrup ? "Syrup flavour:" : "Drink flavour:");
    colorLabel->setText("Colour:");
    packSizeLabel->setText((bottleCount || canCount) ? "Container size:" : "Pack size:");
    variantLabel->setVisible(usesVariant);
    variantDropdown->setVisible(usesVariant);
    colorLabel->setVisible(usesColor);
    colorDropdown->setVisible(usesColor);
    packSizeLabel->setVisible(usesPack);
    packSizeDropdown->setVisible(usesPack);
    materialNameInput->setReadOnly(usesVariant || usesColor || usesPack || fixedName);
    materialNameInput->setPlaceholderText(usesVariant || usesColor || usesPack || fixedName
        ? "Variant name is assembled from the selections"
        : "Enter a material identifier");

    const QStringList flavors = {
        "Coca-Cola Original Taste", "Coca-Cola Zero Sugar", "Coca-Cola Light",
        "Fanta Orange", "Fanta Pineapple", "Fanta Grape", "Fanta Passion Fruit",
        "Fanta Red Apple", "Sprite Lemon-Lime", "Sprite Zero", "Sprite Mint",
        "Stoney Classic", "Stoney Extra Kwetsa", "Stoney Zero",
        "Sparletta Crème Soda", "Sparletta Iron Brew", "Sparletta Sparberry",
        "Twist Granadilla", "Twist Lemon", "Powerade Orange", "Powerade Berry",
        "Bonaqua Still", "Other"
    };
    const QStringList colors = {"Clear / Natural", "Green", "Blue", "White", "Red", "Other"};
    QStringList sizes;
    if (selectedStream == "RGB") sizes = {"330 ml", "1.25 L", "Other"};
    else if (selectedStream == "RefPET") sizes = {"2 L", "Other"};
    else if (selectedStream == "Can") sizes = {"330 ml", "440 ml", "500 ml", "Other"};
    else sizes = {"250 ml", "330 ml", "440 ml", "500 ml", "750 ml", "1 L", "1.5 L", "2 L", "2.25 L", "Other"};
    const QSignalBlocker blockVariant(variantDropdown);
    const QSignalBlocker blockColor(colorDropdown);
    const QSignalBlocker blockSize(packSizeDropdown);
    variantDropdown->clear();
    colorDropdown->clear();
    packSizeDropdown->clear();
    if (usesVariant) variantDropdown->addItems(flavors);
    if (usesColor) colorDropdown->addItems(colors);
    if (usesPack) packSizeDropdown->addItems(sizes);
    if (usesVariant) variantDropdown->setCurrentIndex(0);
    if (usesColor) colorDropdown->setCurrentIndex(0);
    if (usesPack) packSizeDropdown->setCurrentIndex(0);
    if (!usesVariant && !usesColor && !usesPack && !fixedName) materialNameInput->clear();
    refreshVariantName();
}

void MainWindow::refreshVariantName() {
    const QString category = categoryDropdown->currentText();
    QString name;
    if (category == "Labels") {
        name = variantDropdown->currentText().trimmed() + " — " + packSizeDropdown->currentText().trimmed() + " label";
    } else if (category == "Syrup") {
        name = variantDropdown->currentText().trimmed() + " syrup";
    } else if (category == "Preforms") {
        name = colorDropdown->currentText().trimmed() + " — " + packSizeDropdown->currentText().trimmed() + " preform";
    } else if (category == "Stretch Wrapper" || category == "Shrinkwrapper") {
        name = colorDropdown->currentText().trimmed() + " — " + variantDropdown->currentText().trimmed() +
            " — " + packSizeDropdown->currentText().trimmed() + " " + category.toLower();
    } else if (category == "Closures") {
        name = colorDropdown->currentText().trimmed() + " — " + packSizeDropdown->currentText().trimmed() + " closure";
    } else if (category == "Bottles") {
        name = packSizeDropdown->currentText().trimmed() + " bottle";
    } else if (category == "Cans") {
        name = packSizeDropdown->currentText().trimmed() + " can";
    } else if (category == "Crates") {
        name = "Crate";
    } else if (category == "Pallets") {
        name = "Pallet";
    } else if (category == "Top Lid") {
        name = "Can top lid";
    } else {
        return;
    }
    materialNameInput->setText(name);
}

void MainWindow::handleAddMaterialCategory() {
    if (selectedStream.isEmpty()) {
        QMessageBox::information(this, "Choose a production stream",
                                 "Choose PET, RGB, RefPET, or Can before adding a category.");
        return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle("Add Material Category");
    auto *layout = new QFormLayout(&dialog);
    auto *nameInput = new QLineEdit(&dialog);
    auto *unitInput = new QLineEdit(&dialog);
    nameInput->setPlaceholderText("e.g., Bottle Caps");
    unitInput->setPlaceholderText("e.g., Units, kg, L, m");
    layout->addRow("Category name:", nameInput);
    layout->addRow("SI / stock unit:", unitInput);
    layout->addRow(new QLabel("The inventory quantity is entered for each material record.", &dialog));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;

    const QString name = nameInput->text().trimmed();
    const QString unit = unitInput->text().trimmed();
    if (name.isEmpty() || unit.isEmpty() || name.contains('|') || name.contains('\n') ||
            name.contains('\r') || unit.contains('|') || unit.contains('\n') || unit.contains('\r')) {
        QMessageBox::warning(this, "Category Validation", "Enter a category name and SI / stock unit without pipe characters.");
        return;
    }
    if (categoryUnits.contains(name) || streamCategoryUnits.value(selectedStream).contains(name)) {
        QMessageBox::information(this, "Category Exists", "That material category is already available.");
        return;
    }
    streamCategoryUnits[selectedStream].insert(name, unit);
    categoryDropdown->addItem(name);
    categoryDropdown->setCurrentText(name);
    saveCategoryDefinitions();
    statusBar()->showMessage("Material category saved with unit " + unit, 5000);
}

void MainWindow::handleAddMaterial() {
    QString cat = categoryDropdown->currentText();
    QString name = materialNameInput->text().trimmed();
    double cost = unitCostSpinBox->value();
    double eff = efficiencySpinBox->value();
    double qty = quantitySpinBox->value();
    const double annualDemand = annualDemandSpinBox->value();
    const double orderingCost = orderingCostSpinBox->value();
    const double holdingCost = holdingCostSpinBox->value();

    if (name.isEmpty() || name.contains('|')) {
        QMessageBox::warning(this, "Validation Error",
                             "Enter a material name without the pipe character (|).");
        return;
    }
    if (cost <= 0.0) {
        QMessageBox::warning(this, "Validation Error", "Unit cost must be greater than zero.");
        return;
    }
    const bool anyEoqInput = annualDemand > 0.0 || orderingCost > 0.0 || holdingCost > 0.0;
    const bool allEoqInputs = annualDemand > 0.0 && orderingCost > 0.0 && holdingCost > 0.0;
    if (anyEoqInput && !allEoqInputs) {
        QMessageBox::warning(this, "EOQ Validation",
                             "Enter annual demand, ordering cost, and holding cost together, or leave all three blank.");
        return;
    }
    if (qty <= 0) {
        QMessageBox::warning(this, "Validation Error", "Inventory quantity must be greater than zero.");
        return;
    }

    const QString unitType = unitForCategory(cat);

    // Insert row dynamically into visual table widget
    int rowCount = inventoryTable->rowCount();
    inventoryTable->insertRow(rowCount);
    const QString itemId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto *streamItem = new QTableWidgetItem(selectedStream);
    auto *categoryItem = new QTableWidgetItem(cat);
    categoryItem->setData(Qt::UserRole, itemId);
    categoryItem->setData(Qt::UserRole + 1, annualDemand);
    categoryItem->setData(Qt::UserRole + 2, orderingCost);
    categoryItem->setData(Qt::UserRole + 3, holdingCost);
    inventoryTable->setItem(rowCount, 0, streamItem);
    inventoryTable->setItem(rowCount, 1, categoryItem);
    inventoryTable->setItem(rowCount, 2, new QTableWidgetItem(name));
    inventoryTable->setItem(rowCount, 3, new QTableWidgetItem(QString("R %1").arg(cost, 0, 'f', 2)));
    inventoryTable->setItem(rowCount, 4, new QTableWidgetItem(QString::number(qty, 'f', 3)));
    inventoryTable->setItem(rowCount, 5, new QTableWidgetItem(unitType));
    inventoryTable->setItem(rowCount, 6, new QTableWidgetItem(QString("%1 %").arg(eff, 0, 'f', 2)));

    logToPythonTelemetry("CAPTURE", selectedStream, cat, name, cost, eff, qty, unitType, itemId, annualDemand, orderingCost, holdingCost);
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
    double qty = quantitySpinBox->value();
    const double annualDemand = annualDemandSpinBox->value();
    const double orderingCost = orderingCostSpinBox->value();
    const double holdingCost = holdingCostSpinBox->value();

    if (name.isEmpty() || name.contains('|')) {
        QMessageBox::warning(this, "Validation Error",
                             "Enter a material name without the pipe character (|).");
        return;
    }
    if (cost <= 0.0) {
        QMessageBox::warning(this, "Validation Error", "Unit cost must be greater than zero.");
        return;
    }
    const bool anyEoqInput = annualDemand > 0.0 || orderingCost > 0.0 || holdingCost > 0.0;
    const bool allEoqInputs = annualDemand > 0.0 && orderingCost > 0.0 && holdingCost > 0.0;
    if (anyEoqInput && !allEoqInputs) {
        QMessageBox::warning(this, "EOQ Validation",
                             "Enter annual demand, ordering cost, and holding cost together, or leave all three blank.");
        return;
    }
    if (qty <= 0) {
        QMessageBox::warning(this, "Validation Error", "Inventory quantity must be greater than zero.");
        return;
    }

    const QString unitType = unitForCategory(cat);

    QTableWidgetItem *oldCategoryItem = inventoryTable->item(selectedRow, 1);
    QString itemId = oldCategoryItem ? oldCategoryItem->data(Qt::UserRole).toString() : QString();
    if (itemId.isEmpty()) {
        itemId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    auto *categoryItem = new QTableWidgetItem(cat);
    categoryItem->setData(Qt::UserRole, itemId);
    categoryItem->setData(Qt::UserRole + 1, annualDemand);
    categoryItem->setData(Qt::UserRole + 2, orderingCost);
    categoryItem->setData(Qt::UserRole + 3, holdingCost);
    inventoryTable->setItem(selectedRow, 0, new QTableWidgetItem(selectedStream));
    inventoryTable->setItem(selectedRow, 1, categoryItem);
    inventoryTable->setItem(selectedRow, 2, new QTableWidgetItem(name));
    inventoryTable->setItem(selectedRow, 3, new QTableWidgetItem(QString("R %1").arg(cost, 0, 'f', 2)));
    inventoryTable->setItem(selectedRow, 4, new QTableWidgetItem(QString::number(qty, 'f', 3)));
    inventoryTable->setItem(selectedRow, 5, new QTableWidgetItem(unitType));
    inventoryTable->setItem(selectedRow, 6, new QTableWidgetItem(QString("%1 %").arg(eff, 0, 'f', 2)));

    logToPythonTelemetry("EDIT", selectedStream, cat, name, cost, eff, qty, unitType, itemId, annualDemand, orderingCost, holdingCost);
}

void MainWindow::loadSelectedMaterial() {
    const int row = inventoryTable->currentRow();
    if (row < 0) {
        return;
    }
    const QTableWidgetItem *stream = inventoryTable->item(row, 0);
    const QTableWidgetItem *category = inventoryTable->item(row, 1);
    const QTableWidgetItem *name = inventoryTable->item(row, 2);
    const QTableWidgetItem *costItem = inventoryTable->item(row, 3);
    const QTableWidgetItem *quantityItem = inventoryTable->item(row, 4);
    const QTableWidgetItem *efficiencyItem = inventoryTable->item(row, 6);
    if (!stream || !category || !name || !costItem || !quantityItem || !efficiencyItem) {
        return;
    }
    selectedStream = stream->text() == "Unassigned" ? QString() : stream->text();
    selectedStreamLabel->setText("Production stream: " + (selectedStream.isEmpty() ? "Unassigned legacy item" : selectedStream));
    populateCategoryDropdown();
    if (categoryDropdown->findText(category->text()) < 0) {
        categoryDropdown->addItem(category->text());
        streamCategoryUnits[selectedStream].insert(category->text(), inventoryTable->item(row, 5)->text());
    }
    categoryDropdown->setCurrentText(category->text());
    const QString storedName = name->text();
    if (category->text() == "Labels" || category->text() == "Syrup") {
        QString flavor = storedName;
        QString pack;
        const QStringList parts = storedName.split(" — ");
        if (parts.size() > 1) {
            flavor = parts.first();
            pack = parts.last();
            if (pack.endsWith(" label")) pack.chop(6);
        } else if (flavor.endsWith(" syrup")) {
            flavor.chop(6);
        }
        variantDropdown->setCurrentText(flavor);
        if (!pack.isEmpty()) packSizeDropdown->setCurrentText(pack);
    } else if (category->text() == "Bottles" || category->text() == "Cans") {
        QString pack = storedName;
        const QString suffix = category->text() == "Bottles" ? " bottle" : " can";
        if (pack.endsWith(suffix, Qt::CaseInsensitive)) pack.chop(suffix.size());
        packSizeDropdown->setCurrentText(pack);
    } else if (category->text() == "Preforms" || category->text() == "Stretch Wrapper" ||
               category->text() == "Shrinkwrapper" || category->text() == "Closures") {
        const QStringList parts = storedName.split(" — ");
        if (parts.size() > 1) {
            colorDropdown->setCurrentText(parts.first());
            if ((category->text() == "Stretch Wrapper" || category->text() == "Shrinkwrapper") && parts.size() > 2)
                variantDropdown->setCurrentText(parts.at(1));
            QString pack = parts.last();
            const QStringList suffixes = {" preform", " stretch wrapper", " shrinkwrapper", " closure"};
            for (const QString &suffix : suffixes) {
                if (pack.endsWith(suffix, Qt::CaseInsensitive)) { pack.chop(suffix.size()); break; }
            }
            packSizeDropdown->setCurrentText(pack);
        } else {
            materialNameInput->setText(storedName);
        }
    } else {
        materialNameInput->setText(storedName);
    }
    refreshVariantName();
    QString costText = costItem->text();
    costText.remove('R');
    unitCostSpinBox->setValue(costText.trimmed().toDouble());
    quantitySpinBox->setValue(quantityItem->text().toDouble());
    QString efficiencyText = efficiencyItem->text();
    efficiencyText.remove('%');
    efficiencySpinBox->setValue(efficiencyText.trimmed().toDouble());
    annualDemandSpinBox->setValue(category->data(Qt::UserRole + 1).toDouble());
    orderingCostSpinBox->setValue(category->data(Qt::UserRole + 2).toDouble());
    holdingCostSpinBox->setValue(category->data(Qt::UserRole + 3).toDouble());
}

bool MainWindow::processTelemetryLog(const QString& logPath, QString* errorMessage) {
    const QString projectRoot = QStringLiteral(TELEMETRY_PROJECT_ROOT);
    const QString processorPath = QDir(projectRoot).filePath("telemetry_processor.py");
    if (!QFileInfo::exists(processorPath)) {
        *errorMessage = "The telemetry processor was not found: " + processorPath;
        return false;
    }

    QStringList arguments;
    const QString python = findPythonInterpreter(arguments);
    if (python.isEmpty()) {
        *errorMessage = "Python was not found. Install Python or add it to PATH.";
        return false;
    }
    arguments << processorPath << "--log-file" << logPath;

    QProcess process;
    process.start(python, arguments);
    if (!process.waitForStarted(5000)) {
        *errorMessage = "Could not start the telemetry processor: " + process.errorString();
        return false;
    }
    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished();
        *errorMessage = "The telemetry processor did not finish within 30 seconds.";
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        *errorMessage = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
        if (errorMessage->isEmpty()) {
            *errorMessage = "The telemetry processor exited with code " + QString::number(process.exitCode()) + ".";
        }
        return false;
    }
    return true;
}

void MainWindow::loadCurrentInventory() {
    const QString projectRoot = QStringLiteral(TELEMETRY_PROJECT_ROOT);
    QFile file(QDir(projectRoot).filePath("current_inventory.csv"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
    if (!stream.atEnd()) {
        stream.readLine(); // CSV header
    }
    inventoryTable->setRowCount(0);
    while (!stream.atEnd()) {
        const QStringList fields = parseCsvLine(stream.readLine());
        if (fields.size() != 15) {
            continue;
        }
        bool costOk = false;
        const double cost = fields.at(6).toDouble(&costOk);
        if (!costOk || fields.at(2).isEmpty()) {
            continue;
        }

        const int row = inventoryTable->rowCount();
        inventoryTable->insertRow(row);
        inventoryTable->setItem(row, 0, new QTableWidgetItem(fields.at(3).isEmpty() ? "Unassigned" : fields.at(3)));
        auto *categoryItem = new QTableWidgetItem(fields.at(4));
        categoryItem->setData(Qt::UserRole, fields.at(2));
        categoryItem->setData(Qt::UserRole + 1, fields.at(10));
        categoryItem->setData(Qt::UserRole + 2, fields.at(11));
        categoryItem->setData(Qt::UserRole + 3, fields.at(12));
        inventoryTable->setItem(row, 1, categoryItem);
        inventoryTable->setItem(row, 2, new QTableWidgetItem(fields.at(5)));
        inventoryTable->setItem(row, 3, new QTableWidgetItem(QString("R %1").arg(cost, 0, 'f', 2)));
        inventoryTable->setItem(row, 4, new QTableWidgetItem(fields.at(7)));
        inventoryTable->setItem(row, 5, new QTableWidgetItem(fields.at(8)));
        bool efficiencyOk = false;
        const double efficiency = fields.at(9).toDouble(&efficiencyOk);
        inventoryTable->setItem(row, 6, new QTableWidgetItem(
            efficiencyOk ? QString("%1 %").arg(efficiency * 100.0, 0, 'f', 2) : QString()));
    }
}

void MainWindow::logToPythonTelemetry(const QString& action, const QString& productionStream, const QString& category, const QString& name, double cost, double efficiency, double quantity, const QString& unitType, const QString& itemId, double annualDemand, double orderingCost, double holdingCost) {
    QFile file(QStringLiteral(TELEMETRY_LOG_PATH));
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        QMessageBox::warning(this, "Telemetry Log Error",
                             "Could not append telemetry data:\n" + file.errorString());
        return;
    }

    QTextStream output(&file);
    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    // Blank EOQ fields mean the item is excluded from EOQ analysis.
    const QString demandText = annualDemand > 0.0 ? QString::number(annualDemand, 'f', 2) : QString();
    const QString orderingText = orderingCost > 0.0 ? QString::number(orderingCost, 'f', 2) : QString();
    const QString holdingText = holdingCost > 0.0 ? QString::number(holdingCost, 'f', 2) : QString();
    output << timestamp << "|" << action << "|" << itemId << "|" << productionStream << "|" << category << "|"
           << name << "|" << QString::number(cost, 'f', 2) << "|" << QString::number(efficiency / 100.0, 'f', 6)
           << "|" << QString::number(quantity, 'f', 3) << "|" << unitType << "|" << demandText
           << "|" << orderingText << "|" << holdingText << "\n";
    output.flush();
    if (output.status() != QTextStream::Ok) {
        file.close();
        QMessageBox::warning(this, "Telemetry Log Error",
                             "The telemetry record could not be written completely.");
        return;
    }
    const QString logPath = QFileInfo(file).absoluteFilePath();
    file.close();

    QString errorMessage;
    if (!processTelemetryLog(logPath, &errorMessage)) {
        QMessageBox::warning(this, "Telemetry Export Error", errorMessage);
        return;
    }
    loadCurrentInventory();
    statusBar()->showMessage("Event history and current inventory CSVs updated", 5000);
}
