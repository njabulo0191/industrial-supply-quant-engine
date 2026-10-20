# FMCG Industrial Supply Chain Optimization & Enterprise Telemetry Simulator 🚀

An end-to-end, multi-language software architecture designed to model, optimize, and log raw material inventories within a high-volume Fast-Moving Consumer Goods (FMCG) packaging facility. Inspired directly by industrial operations workflows observed during a **FoodBev SETA Packaging Operations Learnership at Coca-Cola Beverages South Africa (CCBSA)**, this project bridges real-world manufacturing telemetry with advanced mathematical modeling and corporate software design patterns.

---

##  System Architecture & Multi-Language Pipeline

The system is engineered as a decoupled, multi-layered data architecture utilizing the unique operational strengths of **Python**, **MATLAB**, and **C++ with the Qt Framework**:

1. **Front-End Presentation Layer (C++ / Qt Framework):** A performance-driven desktop Graphical User Interface (GUI) layout that allows systems operators to capture, monitor, and update critical raw packaging and production materials across operational categories (**Labels, Stretch Wrapper, Shrinkwrapper, Pallets, Syrup, CO2, Treated Water, and Hot Glue**).
2. **Communications Bridge:** Captures live user interaction actions, inputs, and edits, instantly writing event payloads out into pipe-separated data streams.
3. **Data Pipeline Engine (Python):** A continuous backend parser utilizing robust algorithms to ingest streaming log rows, track material volumes across industrial metrics (**Units, Kilograms, Liters, Bar, Meters**), execute automated business logic, and construct a persistent historical tracking ledger database.
4. **Quantitative Mathematical Layer (MATLAB):** Ingests final transactional matrices to execute vector sorting computations, solving cost-minimization constraints and establishing optimal procurement profiles.

---

## 🏭 Industrial Materials Metric Mapping (CCBSA / FoodBev Context)

To accurately simulate a modern bottling facility, raw inventory quantities are dynamically validated and scaled across dedicated industrial metric classes:

| Material Category | Industrial Metric Unit Type | Operational Purpose |
| :--- | :--- | :--- |
| **Syrup** | Kilograms (Bulk Mass) | Core product batching and mixing |
| **CO2** | Bar / Liters (Gas Volume) | Carbonation calibration thresholds |
| **Treated Water** | Liters (Volumetric Liquid) | Main hydration processing line |
| **Labels** | Units (Discrete Count) | Primary product branding applications (e.g., Gloss Chrome Wrap) |
| **Stretch / Shrinkwrapper** | Meters (Linear Roll Length) | Secondary matrix grouping and pallet stability |
| **Pallets** | Units (Discrete Count) | Final warehousing and logistics distribution layout |
| **Hot Glue** | Grams / Kilograms (Adhesive Weight) | Structural carton and case sealing |

---

## 🛠️ Codebase Repository Directory Structure

```text
├── Data_Pipeline_Layer/
│   └── telemetry_processor.py      # Python log ingestion and Excel ledger compiler
├── Mathematical_Analytics/
│   └── optimize_costs.m            # MATLAB vector optimization and linear programming matrix
├── Application_Presentation_UI/
│   ├── main.cpp                    # C++ central execution loop and core object models
│   ├── mainwindow.h                # Qt layout definitions and interactive slots declaration
│   └── mainwindow.cpp              # Event listeners and pipe-separated file streaming logic
└── README.md                       # Comprehensive system documentation manual
```

---

## ⚡ Core Technical Competencies Demonstrated

*   **Object-Oriented Design Patterning (C++):** Implementation of clean structure encapsulation patterns, data validation checks, and memory management constraints.
*   **Event-Driven UI Engineering (Qt Framework):** Structuring responsive layouts, signal-to-slot bindings, and live dashboard table components.
*   **Data Pipeline & Script Automation (Python):** Building file parsing frameworks, automating string transformations, and managing file outputs.
*   **Computational Quantitative Modeling (MATLAB):** Vector matrix manipulation, historical trend processing, and business optimization simulation.
*   **FMCG Operational Domain Knowledge:** Practical understanding of supply lines, asset tracking, quality assurance (QA) validation loops, and manufacturing efficiency metrics observed at CCBSA.