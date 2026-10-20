 #include <iostream>
#include <vector>
#include <string>
#include <iomanip>

// Define a unified structural entity for raw materials
struct RawMaterial {
    std::string category;     // Labels, Shrinkwrapper, Syrup, CO2, etc.
    std::string materialName; // Specific product identifier/brand
    double unitCostZAR;
    double efficiencyRate;
};

class CorporateSupplyChainModel {
private:
    std::vector<RawMaterial> inventoryDatabase;

public:
    // 1. DATA CAPTURE: Add a brand new material to the system
    void captureNewMaterial(const std::string& cat, const std::string& name, double cost, double eff) {
        RawMaterial item = {cat, name, cost, eff};
        inventoryDatabase.push_back(item);
        std::cout << " Successfully Captured: " << name << " under " << cat << std::endl;
    }

    // 2. DATA EDITING: Edit an existing item based on its name identifier
    bool editExistingMaterial(const std::string& name, double newCost, double newEff) {
        for (auto& item : inventoryDatabase) {
            if (item.materialName == name) {
                item.unitCostZAR = newCost;
                item.efficiencyRate = newEff;
                std::cout << " Successfully Updated Material Configuration for: " << name << std::endl;
                return true;
            }
        }
        std::cerr << " Material Name Not Found." << std::endl;
        return false;
    }

    // 3. DASHBOARD RENDERING: Print out the entire multi-material inventory matrix
    void renderDashboardView() const {
        std::cout << "\n========================================================================\n";
        std::cout << " INDUSTRIAL SUPPLY CHAIN INVENTORY & DATA CAPTURE MATRIX\n";
        std::cout << "========================================================================\n";
        std::cout << std::left << std::setw(18) << "CATEGORY"
                  << std::setw(20) << "MATERIAL NAME"
                  << std::setw(18) << "UNIT COST (ZAR)"
                  << "PLANT YIELD (%)\n";
        std::cout << "------------------------------------------------------------------------\n";

        for (const auto& item : inventoryDatabase) {
            std::cout << std::left << std::setw(18) << item.category
                      << std::setw(20) << item.materialName
                      << "R " << std::setw(16) << std::fixed << std::setprecision(2) << item.unitCostZAR
                      << std::fixed << std::setprecision(2) << (item.efficiencyRate * 100.0) << "%\n";
        }
        std::cout << "========================================================================\n\n";
    }
};

int main() {
    CorporateSupplyChainModel erpSystem;

    // Simulating user input data captures
    erpSystem.captureNewMaterial("Syrup", "High-Fructose Batch A", 1250.50, 0.985);
    erpSystem.captureNewMaterial("Labels", "Gloss Chrome Wrap", 4.20, 0.950);
    erpSystem.captureNewMaterial("Shrinkwrapper", "Poly-Clear 500m", 450.00, 0.890);
    erpSystem.captureNewMaterial("CO2", "Liquid Carbonic Tank", 3200.00, 0.999);
    erpSystem.captureNewMaterial("Stretchwrapper", "Poly-Clear 500m", 500.00, 0.890);

    // Displaying initial state
    erpSystem.renderDashboardView();

    // Simulating a user modifying / editing a material via the interface
    std::cout << " User Action: Editing 'Gloss Chrome Wrap' prices due to inflation..." << std::endl;
    erpSystem.editExistingMaterial("Gloss Chrome Wrap", 4.95, 0.965);

    // Render updated matrix layout
    erpSystem.renderDashboardView();

    return 0;
}
