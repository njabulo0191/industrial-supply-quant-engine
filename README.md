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
│
├── historical_material_tracking.csv
├── industrial_supply_telemetry.csv
├── optimized_strategy.txt
│
├── .gitignore
└── README.md
```

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
```bash

Navigate into the project:

```bash
cd industrial-supply-quant-engine
```bash

### Run the Qt Application

1. Open Qt Creator.
2. Select **File → Open File or Project**.
3. Open `SupplyChainProj/CMakeLists.txt`.
4. Configure the project using an installed Qt kit.
5. Build and run the application.

### Run the Python Scripts

From the project root:

```bash
python supply_chain_gen.py
```

```bash
python telemetry_processor.py
```

### Run the MATLAB Script

Open MATLAB, navigate to the project directory, and execute:

```matlab
optimize_costs
```

*Note: These are initial run instructions based on the repository structure. Dependencies, input-file requirements, and exact execution behaviour should be verified against the source code.*

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