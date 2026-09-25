#!/usr/bin/env python3
"""Audit the NVRAM Core Options against the screenshot-derived catalog.

The screenshot catalog owns menu presence, order, native defaults and observed
values. The reviewed workbook owns the parent-game selection. Clone selections
inherit that review, except for the two explicitly reviewed clone-only fields
below and Motor Raid Deluxe's read-only Engine Volume row.
"""

from __future__ import annotations

from collections import defaultdict
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET
import zipfile


ROOT = Path(__file__).resolve().parent.parent
CATALOG = ROOT / "GAME_SETTINGS_CATALOG.md"
WORKBOOK = ROOT / "Docs/revisione_core_options_model2.xlsx"
GAMES_XML = ROOT / "data/games.xml"
CORE_DATA = ROOT / "src/libretro/nvram_settings_data.inc"

# These are the only reviewed clone-only Core Options. All other clone choices
# are inherited from the parent's reviewed workbook selection.
CLONE_ADDITIONS = {
    "daytonas": {"PROMOTE SATURN"},
    "motoraiddx": {"CABINET TYPE"},
}
CLONE_EXCLUSIONS = {
    # The Deluxe menu displays this row as OUT OF USE and cannot select it.
    "motoraiddx": {"ENGINE VOLUME"},
}


def normalized(value: str) -> str:
    value = value.upper().replace("’", "'")
    value = re.sub(r"U\.?S\.?A?\.?\b", "USA", value)
    value = re.sub(r"\bJPN\b", "JAPAN", value)
    value = re.sub(r"\bEXP\b", "EXPORT", value)
    value = re.sub(r"C\.?R\.?T\.?", "CRT", value)
    return re.sub(r"[^A-Z0-9#+-]+", " ", value).strip()


def parse_catalog() -> dict[str, list[dict[str, str]]]:
    result: dict[str, list[dict[str, str]]] = {}
    current = None
    for line in CATALOG.read_text().splitlines():
        heading = re.match(r"^### .* \(`([^`]+)`\)$", line)
        if heading:
            current = heading.group(1)
            result[current] = []
        elif current and re.match(r"^\| \d+ \|", line):
            cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
            result[current].append(
                {"label": cells[1], "default": cells[2], "values": cells[3]}
            )
    return result


def parse_parent_selections() -> dict[str, list[str]]:
    namespace = {"m": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
    selected: dict[str, list[str]] = defaultdict(list)
    with zipfile.ZipFile(WORKBOOK) as archive:
        sheet = ET.fromstring(archive.read("xl/worksheets/sheet1.xml"))
    rows = sheet.findall(".//m:sheetData/m:row", namespace)[1:]
    for row in rows:
        values: dict[str, str] = {}
        for cell in row.findall("m:c", namespace):
            column = re.match(r"[A-Z]+", cell.attrib["r"]).group(0)
            if cell.attrib.get("t") == "inlineStr":
                value = "".join(
                    text.text or "" for text in cell.findall(".//m:t", namespace)
                )
            else:
                raw = cell.find("m:v", namespace)
                value = "" if raw is None else raw.text or ""
            values[column] = value
        if values.get("F") == "Yes":
            selected[values["B"]].append(values["C"])
    return selected


def parse_core_options() -> dict[str, list[dict[str, object]]]:
    source = CORE_DATA.read_text()
    value_arrays: dict[str, list[tuple[str, str]]] = {}
    value_pattern = re.compile(
        r"static constexpr Value (k\w*Values\d*)\[\] = \{(.*?)\n\};", re.S
    )
    for match in value_pattern.finditer(source):
        value_arrays[match.group(1)] = re.findall(
            r'\{"([^"]+)", "([^"]*)",', match.group(2)
        )

    block = source.split("static constexpr Option kOptions[] = {", 1)[1].split(
        "\n};", 1
    )[0]
    option_pattern = re.compile(
        r'^\s*\{"([^"]+)", "([^"]+)", "([^"]+)", '
        r'"((?:[^"\\]|\\.)*)", "([^"]+)", (k\w*Values\d*),',
        re.M,
    )
    options: dict[str, list[dict[str, object]]] = defaultdict(list)
    for match in option_pattern.finditer(block):
        values = value_arrays[match.group(6)]
        options[match.group(1)].append(
            {
                "suffix": match.group(2),
                "label": match.group(3),
                "description": match.group(4),
                "default_key": match.group(5),
                "values": values,
            }
        )
    return options


def documented_values(cell: str) -> list[str]:
    if cell in {"—", "nessun ciclo dedicato", "non acquisiti"}:
        return []
    return [value.strip() for value in cell.split(",")]


def expected_default(game: str, option: dict[str, object], native: str) -> str:
    suffix = str(option["suffix"])
    labels = [label for _, label in option["values"]]
    overrides = {
        ("daytona", "cabinet"): "DELUXE",
        ("daytona93", "cabinet"): "DELUXE",
        ("daytonas", "cabinet"): "DELUXE",
        ("manxtt", "cabinet_type"): "TWIN",
        ("sgt24h", "io_type"): "C",
        # LINK MAX is unavailable while LINK TYPE is NOT LINK. Its first real
        # selectable value is therefore the only meaningful option default.
        ("sgt24h", "link_max"): "2",
    }
    if (game, suffix) in overrides:
        return overrides[(game, suffix)]
    if suffix in {"country", "nation"}:
        return (
            next((label for label in labels if normalized(label) == "USA"), None)
            or next((label for label in labels if normalized(label) == "EXPORT"), native)
        )
    if "link" in suffix or "network" in suffix:
        offline = {"SINGLE", "NOT LINK", "NOTLINK", "STAND ALONE"}
        return next((label for label in labels if normalized(label) in offline), native)
    return native


def main() -> int:
    catalog = parse_catalog()
    parent_selections = parse_parent_selections()
    options = parse_core_options()
    games = ET.parse(GAMES_XML).getroot().findall("game")
    parents = {game.get("name"): game.get("parent") for game in games}
    errors: list[str] = []
    routed = 0

    if set(catalog) != set(parents):
        errors.append(
            "catalog/games.xml set mismatch: "
            f"missing={sorted(set(parents) - set(catalog))}, "
            f"extra={sorted(set(catalog) - set(parents))}"
        )

    for game, rows in catalog.items():
        parent = parents[game]
        option_game = game if options.get(game) else parent
        actual = options.get(option_game, [])
        if actual:
            routed += 1

        row_by_label = {normalized(row["label"]): row for row in rows}
        parent_selected = parent_selections.get(parent or game, [])
        expected_labels = {
            normalized(label)
            for label in parent_selected
            if normalized(label) in row_by_label
        }
        expected_labels |= {
            normalized(label) for label in CLONE_ADDITIONS.get(game, set())
        }
        expected_labels -= {
            normalized(label) for label in CLONE_EXCLUSIONS.get(game, set())
        }

        actual_labels = [normalized(str(option["label"])) for option in actual]
        actual_set = set(actual_labels)
        missing = expected_labels - actual_set
        extra = actual_set - expected_labels
        if missing:
            errors.append(f"{game}: missing selected options {sorted(missing)}")
        if extra:
            errors.append(f"{game}: exposes unselected/absent options {sorted(extra)}")

        positions = [
            list(row_by_label).index(label)
            for label in actual_labels
            if label in row_by_label
        ]
        if len(positions) != len(actual_labels):
            absent = [label for label in actual_labels if label not in row_by_label]
            errors.append(f"{game}: routed catalog {option_game!r} exposes absent rows {absent}")
        elif positions != sorted(positions):
            errors.append(f"{game}: Core Option order differs from the Service Menu")

        for option in actual:
            label = normalized(str(option["label"]))
            row = row_by_label.get(label)
            if not row:
                continue
            value_pairs = list(option["values"])
            documented = documented_values(row["values"])
            if documented:
                expected_values = {normalized(value) for value in documented}
                actual_values = {normalized(label) for _, label in value_pairs}
                if expected_values != actual_values:
                    errors.append(
                        f"{game}:{option['suffix']}: value mismatch "
                        f"missing={sorted(expected_values - actual_values)}, "
                        f"extra={sorted(actual_values - expected_values)}"
                    )

            default_label = next(
                (label for key, label in value_pairs if key == option["default_key"]), None
            )
            wanted_default = expected_default(str(option_game), option, row["default"])
            if default_label is None or normalized(default_label) != normalized(wanted_default):
                errors.append(
                    f"{game}:{option['suffix']}: default {default_label!r}, "
                    f"expected {wanted_default!r}"
                )
            description = str(option["description"])
            if str(option["label"]) not in description or "operator NVRAM" not in description:
                errors.append(f"{game}:{option['suffix']}: incomplete description")

    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(
        "NVRAM Core Option audit passed: "
        f"{len(catalog)} sets, {routed} with NVRAM Settings, "
        f"{len(catalog) - routed} intentionally without them, "
        f"{sum(len(group) for group in options.values())} options"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
