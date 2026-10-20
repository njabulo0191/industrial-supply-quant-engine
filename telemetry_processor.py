import os
import csv

# REMINDER: Update this string path pattern to match your exact laptop build folder
LOG_FILE = r"C:\Users\thinkpad\OneDrive\Desktop\Documentos\VS code\MyProject\SupplyChainProj\build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug\raw_interface_telemetry.log"
SPREADSHEET_FILE = "historical_material_tracking.csv"

def process_interface_telemetry():
    if not os.path.exists(LOG_FILE):
        print(f"⏱️ Waiting for Qt GUI interaction... Start your C++ interface to log data.")
        return

    print(f"📡 Processing active interface logs containing FMCG quantity metrics...")

    # Expanded enterprise column structure for spreadsheet analysis
    headers = [
        "Logged Timestamp",
        "User Action Trigger",
        "Material Classification",
        "Product Name",
        "Unit Cost (ZAR)",
        "Inventory Quantity",
        "Metric Measurement Unit",
        "Plant Target Efficiency",
        "Calculated Total Batch Value",
        "Financial Risk Factor"
    ]

    with open(LOG_FILE, "r") as src, open(SPREADSHEET_FILE, "w", newline="") as dest:
        writer = csv.writer(dest)
        writer.writerow(headers)

        for line in src:
            if not line.strip():
                continue

            # Parse the 8 parameters sent over from the C++ UI window layer
            parts = line.strip().split("|")
            if len(parts) == 8:
                timestamp, action, category, name, cost_str, eff_str, qty_str, unit_type = parts
                
                cost = float(cost_str)
                efficiency = float(eff_str)
                quantity = int(qty_str)

                # Advanced supply chain logic calculations
                total_batch_value = round((cost * quantity), 2)
                risk_factor = round((total_batch_value * (1.0 - efficiency)), 2)

                # Populate your corporate database spreadsheet
                writer.writerow([
                    timestamp,
                    action,
                    category,
                    name,
                    f"R {cost:.2f}",
                    quantity,
                    unit_type,
                    f"{efficiency * 100:.1f}%",
                    f"R {total_batch_value:.2f}",
                    f"R {risk_factor:.2f}"
                ])

    print(f"✅ Spreadsheet data updated successfully: '{SPREADSHEET_FILE}' contains all volume updates.")

if __name__ == "__main__":
    process_interface_telemetry()