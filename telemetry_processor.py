import argparse
import csv
import math
import sys
from datetime import datetime
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent
QT_BUILD_ROOT = PROJECT_ROOT / "SupplyChainProj" / "build"
LOG_FILENAME = "raw_interface_telemetry.log"
CANONICAL_LOG_FILE = PROJECT_ROOT / "telemetry" / LOG_FILENAME
HISTORY_FILE = PROJECT_ROOT / "historical_material_tracking.csv"
CURRENT_INVENTORY_FILE = PROJECT_ROOT / "current_inventory.csv"
HEADERS = [
    "timestamp",
    "action",
    "item_id",
    "production_stream",
    "material_type",
    "product_name",
    "unit_cost_zar",
    "inventory_quantity",
    "metric_unit",
    "efficiency_rate",
    "annual_demand_metric_units",
    "ordering_cost_zar_per_order",
    "holding_cost_zar_per_metric_unit_year",
    "total_batch_value_zar",
    "financial_risk_factor_zar",
]


def find_log_file(log_file=None):
    """Return an explicitly requested log or the project's telemetry log."""
    if log_file is not None:
        path = Path(log_file).expanduser()
        if not path.is_absolute():
            path = PROJECT_ROOT / path
        return path.resolve()

    if CANONICAL_LOG_FILE.is_file():
        return CANONICAL_LOG_FILE

    if not QT_BUILD_ROOT.is_dir():
        raise FileNotFoundError(
            f"Qt telemetry log was not found at {CANONICAL_LOG_FILE} or under {QT_BUILD_ROOT}. "
            "Build or run the Qt application first, or pass --log-file."
        )

    matches = sorted(QT_BUILD_ROOT.rglob(LOG_FILENAME))
    if not matches:
        raise FileNotFoundError(
            f"Telemetry log was not found under {QT_BUILD_ROOT}: {LOG_FILENAME}. "
            "Run the Qt application to create it, or pass --log-file."
        )
    if len(matches) > 1:
        paths = "\n  ".join(str(path) for path in matches)
        raise RuntimeError(
            "More than one Qt telemetry log was found. Pass the intended one "
            f"with --log-file:\n  {paths}"
        )
    return matches[0]


def normalize_timestamp(value, log_path, line_number):
    for timestamp_format in ("%Y-%m-%d %H:%M:%S.%f", "%Y-%m-%d %H:%M:%S"):
        try:
            parsed = datetime.strptime(value, timestamp_format)
            return parsed.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
        except ValueError:
            pass
    raise ValueError(
        f"{log_path}, line {line_number}: timestamp must use "
        "YYYY-MM-DD HH:MM:SS with optional milliseconds."
    )


def infer_legacy_unit(category):
    """Recover the Qt category's established unit where the old log omitted it."""
    units = {
        "Syrup": "Kilograms",
        "Hot Glue": "Kilograms",
        "CO2": "Bar/Liters",
        "Treated Water": "Liters",
        "Stretch Wrapper": "Meters",
        "Shrinkwrapper": "Meters",
        "Labels": "Units",
        "Pallets": "Units",
        "Preforms": "Units",
    }
    return units.get(category, "")


def format_quantity(value):
    if value == "":
        return ""
    if value.is_integer():
        return str(int(value))
    return f"{value:.3f}".rstrip("0").rstrip(".")


def parse_records(log_path):
    """Parse current 13-field and older 12-, 9-, 8-, or 6-field Qt logs."""
    rows = []
    legacy_count = 0
    unmatched_edits = 0
    try:
        with log_path.open("r", encoding="utf-8-sig") as source:
            for line_number, raw_line in enumerate(source, start=1):
                line = raw_line.strip()
                if not line:
                    continue

                parts = line.split("|")
                if len(parts) == 13:
                    (timestamp, action, item_id, production_stream, category, name, cost_text,
                     efficiency_text, quantity_text, unit_type, demand_text,
                     ordering_text, holding_text) = parts
                    has_quantity = True
                    eoq_texts = (demand_text, ordering_text, holding_text)
                elif len(parts) == 12:
                    (timestamp, action, item_id, category, name, cost_text,
                     efficiency_text, quantity_text, unit_type, demand_text,
                     ordering_text, holding_text) = parts
                    production_stream = ""
                    has_quantity = True
                    eoq_texts = (demand_text, ordering_text, holding_text)
                    legacy_count += 1
                elif len(parts) == 9:
                    (timestamp, action, item_id, category, name, cost_text,
                     efficiency_text, quantity_text, unit_type) = parts
                    has_quantity = True
                    production_stream = ""
                    eoq_texts = ("", "", "")
                    legacy_count += 1
                elif len(parts) == 8:
                    (timestamp, action, category, name, cost_text,
                     efficiency_text, quantity_text, unit_type) = parts
                    item_id = ""
                    production_stream = ""
                    has_quantity = True
                    eoq_texts = ("", "", "")
                    legacy_count += 1
                elif len(parts) == 6:
                    timestamp, action, category, name, cost_text, efficiency_text = parts
                    item_id = ""
                    production_stream = ""
                    quantity_text = ""
                    unit_type = infer_legacy_unit(category)
                    has_quantity = False
                    eoq_texts = ("", "", "")
                    legacy_count += 1
                else:
                    raise ValueError(
                        f"{log_path}, line {line_number}: expected 6, 8, 9, 12, or 13 "
                        f"pipe-separated fields, found {len(parts)}."
                    )

                timestamp = normalize_timestamp(timestamp, log_path, line_number)
                try:
                    cost = float(cost_text)
                    efficiency = float(efficiency_text)
                    quantity = float(quantity_text) if has_quantity else ""
                    eoq_values = [float(value) if value.strip() else None for value in eoq_texts]
                except ValueError as exc:
                    raise ValueError(
                        f"{log_path}, line {line_number}: cost, efficiency, quantity, or EOQ input is invalid ({exc})."
                    ) from exc

                # Earlier Qt builds serialized untouched optional spin boxes as 0|0|0.
                if eoq_values == [0.0, 0.0, 0.0]:
                    eoq_values = [None, None, None]

                if not math.isfinite(cost) or cost <= 0:
                    raise ValueError(
                        f"{log_path}, line {line_number}: unit cost must be finite and positive."
                    )
                if not math.isfinite(efficiency) or not 0.0 <= efficiency <= 1.0:
                    raise ValueError(
                        f"{log_path}, line {line_number}: efficiency must be a finite number from 0 to 1."
                    )
                if has_quantity and (not math.isfinite(quantity) or quantity < 0):
                    raise ValueError(
                        f"{log_path}, line {line_number}: quantity must be a finite non-negative number."
                    )
                if any(value is not None for value in eoq_values) and not all(value is not None for value in eoq_values):
                    raise ValueError(
                        f"{log_path}, line {line_number}: annual demand, ordering cost, and holding cost "
                        "must all be filled in together."
                    )
                if all(value is not None for value in eoq_values):
                    if any(not math.isfinite(value) or value <= 0 for value in eoq_values):
                        raise ValueError(
                            f"{log_path}, line {line_number}: EOQ inputs must be finite positive numbers."
                        )
                    eoq_values = [f"{value:.2f}" for value in eoq_values]
                else:
                    eoq_values = ["", "", ""]

                if has_quantity:
                    total = f"{cost * quantity:.2f}"
                    risk = f"{cost * quantity * (1.0 - efficiency):.2f}"
                else:
                    total = ""
                    risk = ""

                rows.append({
                    "timestamp": timestamp,
                    "action": action.upper(),
                    "item_id": item_id,
                    "production_stream": production_stream,
                    "material_type": category,
                    "product_name": name,
                    "unit_cost_zar": f"{cost:.2f}",
                    "inventory_quantity": format_quantity(quantity),
                    "metric_unit": unit_type,
                    "efficiency_rate": f"{efficiency:.6f}",
                    "annual_demand_metric_units": eoq_values[0],
                    "ordering_cost_zar_per_order": eoq_values[1],
                    "holding_cost_zar_per_metric_unit_year": eoq_values[2],
                    "total_batch_value_zar": total,
                    "financial_risk_factor_zar": risk,
                    "_line_number": line_number,
                })
    except UnicodeDecodeError as exc:
        raise ValueError(f"Could not decode telemetry log as UTF-8: {log_path}") from exc

    current_by_id = {}
    for row in rows:
        item_id = row["item_id"]
        action = row["action"]
        if item_id:
            if action in {"CAPTURE", "EDIT"}:
                current_by_id[item_id] = row
            elif action == "DELETE":
                current_by_id.pop(item_id, None)
        elif action == "CAPTURE":
            legacy_id = f"legacy-{row['_line_number']}"
            current_by_id[legacy_id] = {**row, "item_id": legacy_id}
        elif action == "EDIT":
            unmatched_edits += 1

    history_rows = [{key: value for key, value in row.items() if not key.startswith("_")} for row in rows]
    current_rows = [{key: value for key, value in row.items() if not key.startswith("_")} for row in current_by_id.values()]
    return history_rows, current_rows, legacy_count, unmatched_edits


def write_csv(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as destination:
        writer = csv.DictWriter(destination, fieldnames=HEADERS)
        writer.writeheader()
        writer.writerows(rows)


def resolve_output_path(value, default_path):
    if value is None:
        return default_path
    path = Path(value).expanduser()
    if not path.is_absolute():
        path = PROJECT_ROOT / path
    return path.resolve()


def process_interface_telemetry(log_file=None, spreadsheet_file=None, current_inventory_file=None):
    log_path = find_log_file(log_file)
    if not log_path.is_file():
        raise FileNotFoundError(f"Telemetry log was not found: {log_path}")

    history_path = resolve_output_path(spreadsheet_file, HISTORY_FILE)
    current_path = resolve_output_path(current_inventory_file, CURRENT_INVENTORY_FILE)
    history_rows, current_rows, legacy_count, unmatched_edits = parse_records(log_path)

    try:
        write_csv(history_path, history_rows)
        write_csv(current_path, current_rows)
    except OSError as exc:
        raise OSError(f"Could not write telemetry CSV: {exc}") from exc

    print(f"Processed {len(history_rows)} event(s) from: {log_path}")
    print(f"Current inventory contains {len(current_rows)} item(s).")
    if legacy_count:
        print(f"{legacy_count} older event(s) did not include all current telemetry fields.")
    if unmatched_edits:
        print(
            f"{unmatched_edits} older edit event(s) could not be matched to an item; "
            "they remain in the history but do not change the current-inventory snapshot."
        )
    print(f"Event history exported to: {history_path}")
    print(f"Current inventory exported to: {current_path}")
    return len(history_rows)


def main():
    parser = argparse.ArgumentParser(
        description="Convert Qt interface logs into event-history and current-inventory CSVs."
    )
    parser.add_argument(
        "--log-file",
        help="Telemetry log to read (relative paths are resolved from this script's project root).",
    )
    parser.add_argument(
        "--output",
        help="Event-history CSV destination (relative paths are resolved from the project root).",
    )
    parser.add_argument(
        "--current-output",
        help="Current-inventory CSV destination (relative paths are resolved from the project root).",
    )
    args = parser.parse_args()

    try:
        process_interface_telemetry(args.log_file, args.output, args.current_output)
    except (FileNotFoundError, RuntimeError, ValueError, OSError) as exc:
        print(f"Telemetry processing failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
