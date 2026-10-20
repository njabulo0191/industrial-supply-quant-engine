import csv
import random
import time
from datetime import datetime


def generate_telemetry_row():
    # Simulating industrial packaging throughput metrics
    packaging_materials = ["Polyethylene", "Cardboard", "Aluminum"]
    selected_material = random.choice(packaging_materials)

    # Base pricing logic with simulated market volatility
    base_costs = {"Polyethylene": 15.50, "Cardboard": 8.20, "Aluminum": 22.10}
    market_price = base_costs[selected_material] * random.uniform(0.90, 1.15)

    # Factory throughput telemetry rules
    factory_units_per_hour = random.randint(800, 1500)
    system_efficiency = random.uniform(0.85, 0.99)

    return {
        "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "material_type": selected_material,
        "commodity_cost_zar": round(market_price, 2),
        "throughput_units_hr": factory_units_per_hour,
        "efficiency_rate": round(system_efficiency, 4),
    }


def run_simulator(duration_seconds=10):
    filename = "industrial_supply_telemetry.csv"
    fields = [
        "timestamp",
        "material_type",
        "commodity_cost_zar",
        "throughput_units_hr",
        "efficiency_rate",
    ]

    print(f"🚀 Initialising industrial telemetry engine...")
    print(f"📊 Logging data pipeline straight to local file: {filename}")

    with open(filename, mode="w", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=fields)
        writer.writeheader()

        start_time = time.time()
        while time.time() - start_time < duration_seconds:
            row_data = generate_telemetry_row()
            writer.writerow(row_data)
            print(
                f"   [LOG] {row_data['timestamp']} | Material: {row_data['material_type']} | ZAR: {row_data['commodity_cost_zar']}"
            )
            time.sleep(0.5)  # Simulate 500ms operational intervals

    print(f"✅ Data pipeline closed. File successfully saved to laptop.")


if __name__ == "__main__":
    # Runs a quick 10-second data simulation block
    run_simulator(10)
