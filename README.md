# Industrial Supply Quant Engine

**A multi-language industrial supply-chain and data-processing project**

## Overview

Industrial Supply Quant Engine is a software development project exploring how programming, data processing, and numerical optimisation can be applied to industrial supply-chain workflows.

The project brings together C++, Qt, Python, and MATLAB in a multi-language development environment. It is designed to demonstrate practical software engineering skills, desktop application development, data processing, and numerical computing.

## Technologies

| Technology | Application |
|---|---|
| C++ | Core programming and application logic |
| Qt 6 | Desktop graphical user interface |
| Python | Data generation and telemetry processing |
| MATLAB | Numerical and cost optimisation |
| CMake | C++ project configuration |
| Git and GitHub | Version control and project management |

## Project Structure

```text
industrial-supply-quant-engine/
│
├── SupplyChainProj/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── mainwindow.cpp
│   ├── mainwindow.h
│   └── mainwindow.ui
│
├── main/
│   ├── main.cpp
│   └── main.cbp
│
├── supply_chain_gen.py
├── telemetry_processor.py
├── optimize_costs.m
├── MATLAB_Engine/
│   ├── run_analysis.m
│   ├── optimize_costs.m
│   ├── predict_material_prices.m
│   ├── production_regression.m
│   ├── calculate_EOQ.m
│   ├── linear_programming.m
│   ├── process_control.m
│   ├── moving_average.m
│   ├── gradient_descent.m
│   ├── numerical_methods.m
│   ├── functions/                 # compatibility helpers
│   └── results/                   # generated locally; not committed
│
├── telemetry/                     # local Qt log; not committed
│
├── .gitignore
└── README.md
```

Telemetry logs, inventory snapshots, generated CSV reports, and plant-specific category settings are created locally and excluded from Git. No operational data is included in this repository.

## Main Components

### 1. C++ and Qt Desktop Application

The `SupplyChainProj` directory contains a Qt-based C++ desktop application.

It uses:
- C++ for application logic.
- Qt for graphical user interface development.
- CMake for project configuration and building.

### 2. Python Data Processing

The Python components include:

- `supply_chain_gen.py` – a supply-chain data generation script.
- `telemetry_processor.py` – a telemetry processing script.

These scripts form part of the project's data-processing workflow.

### 3. MATLAB Numerical Optimisation

The `optimize_costs.m` script provides a MATLAB component for exploring cost optimisation and numerical computing in a supply-chain context.

## Getting Started

### Requirements

- Git
- A C++ compiler compatible with your Qt installation
- Qt 6 and Qt Creator
- CMake
- Python 3
- MATLAB (for the MATLAB component)

### Clone the Repository

```bash
git clone https://github.com/njabulo0191/industrial-supply-quant-engine.git
```

Navigate into the project:

```bash
cd industrial-supply-quant-engine
```

### Run the Qt Application

1. Open Qt Creator.
2. Select **File → Open File or Project**.
3. Open `SupplyChainProj/CMakeLists.txt`.
4. Configure the project using an installed Qt kit.
5. Build and run the application.

### Python Data Tools

The Qt application calls `telemetry_processor.py` after a material is recorded. To use the processor manually, first run the Qt app and capture an item, then run this from the project root:

```bash
python telemetry_processor.py
```

`supply_chain_gen.py` is an optional standalone demo-data generator; it is separate from the Qt telemetry pipeline.

### Run the MATLAB Analysis Pipeline

Capture or edit items in the Qt application first. Qt stores the action history in `historical_material_tracking.csv` and the latest state of each item in `current_inventory.csv`. MATLAB reads both files; the random-data `supply_chain_gen.py` simulator remains a separate demo source.

From MATLAB, set the current folder to `MATLAB_Engine` and execute:

```matlab
run_analysis
```

Regression uses Qt event history. Material cost/efficiency screening uses current inventory and compares rows only within matching metric units. The Qt form does not capture throughput, so MATLAB does not calculate throughput metrics. EOQ uses annual demand, ordering cost, and holding cost entered for each item in Qt; leave all three blank to exclude that item. Inventory batch quantity is not treated as annual demand. The older `eoq_parameters.csv` is not used by this pipeline.

`run_analysis` also creates `price_predictions.csv` and `production_analysis.csv` from the Qt event history. Price forecasts are linear trend extrapolations by production stream, material category, and exact variant; variants without at least two distinct timestamps are listed with an insufficient-history status. Production analysis reports event observations, latest recorded quantity, unit-cost trend, and efficiency. It does not estimate throughput because the Qt app does not collect production counts. The same run produces `process_control.csv` for efficiency and `moving_average.csv` for unit cost by variant.

The standalone numerical modules are reusable functions. `linear_programming` solves a bounded procurement problem when items are interchangeable and use the same unit; the current Qt categories do not supply those constraints, so it is not run automatically. `gradient_descent` accepts an objective and its gradient. `numerical_methods` provides bisection and Newton root finding. `moving_average` returns a trailing moving average for a numeric vector.

The Qt app opens with PET, RGB, RefPET, and Can stream choices, then shows that stream’s material categories. RefPET and RGB track syrup, labels, closures, bottles, crates, and pallets; Can tracks syrup, cans, top lids, shrinkwrapper, stretch wrapper, and pallets. PET keeps the existing categories. Selectors capture flavour, container size, and packaging colour where appropriate, and the editable choices allow line-specific variants. Custom categories and their SI/stock units are saved per stream in `material_categories.json`. MATLAB price-trend and performance outputs distinguish stream, category, and exact variant.
## Skills Demonstrated

- Object-oriented programming with C++.
- Desktop application development using Qt.
- CMake-based project configuration.
- Python scripting and data processing.
- MATLAB numerical computing.
- Working with CSV datasets.
- Version control using Git and GitHub.
- Multi-language software project organisation.

## Future Improvements

Potential areas for further development include:

- Integrating the different language components into a unified workflow.
- Improving data validation and error handling.
- Adding visual analytics and reporting.
- Developing additional cost optimisation algorithms.
- Adding automated tests.
- Improving technical documentation.
- Adding application screenshots and demonstrations.

## Author

**Njabulo Nkonyane**

Mathematics and Computer Science student at the University of South Africa (UNISA).

GitHub: [@njabulo0191](https://github.com/njabulo0191)

---

*This is an independent educational and portfolio project inspired by industrial supply-chain and manufacturing workflows.*
