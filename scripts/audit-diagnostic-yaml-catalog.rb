#!/usr/bin/env ruby
# frozen_string_literal: true

# Checks the structured diagnostic YAML files against the screenshot-derived
# Game Settings catalog. The catalog remains authoritative for visible rows,
# order and native defaults; YAML remains authoritative for offsets, encodings
# and integrity details that are not visible in screenshots.

require "pathname"
require "set"
require "yaml"

ROOT = Pathname.new(__dir__).parent.realpath
CATALOG_PATH = ROOT / "GAME_SETTINGS_CATALOG.md"
YAML_ROOT = ROOT / "data/diagnostic-menus"
EXPECTED_YAML_COUNT = 53

def catalog_rows
  catalog = {}
  current = nil
  CATALOG_PATH.each_line do |line|
    if (match = line.match(/^### .* \(`([^`]+)`\)$/))
      current = match[1]
      catalog[current] = []
    elsif current && line.match?(/^\| \d+ \|/)
      cells = line.strip.delete_prefix("|").delete_suffix("|").split("|").map(&:strip)
      catalog[current] << {label: cells[1], default: cells[2], values: cells[3]}
    end
  end
  catalog
end

def labelled_rows(value)
  case value
  when Hash
    rows = value["label"] && value["key"] && !value["options"].is_a?(Array) ? [value] : []
    rows + value.values.flat_map { |child| labelled_rows(child) }
  when Array
    value.flat_map { |child| labelled_rows(child) }
  else
    []
  end
end

def visible_rows(data, catalog)
  game = data.dig("game", "set")
  menu = data["menus"] || {}
  inherited = labelled_rows(menu).select { |row| row["label"] }

  reference = menu.values.map do |entry|
    entry["inherited_editable_options_from"] if entry.is_a?(Hash)
  end.compact.first
  if reference
    extra = inherited.reject { |row| row["coding_reference"] }
    return catalog.fetch(reference).map { |row| {"label" => row[:label]} } + extra
  end

  if inherited.empty? && data.dig("verification", "menu_parameters") == "identical_to_parent"
    return catalog.fetch(data.dig("game", "parent")).map { |row| {"label" => row[:label]} }
  end

  positioned, ordinary = inherited.partition { |row| row["visible_position"] }
  positioned.sort_by { |row| row["visible_position"] }.each do |row|
    ordinary.insert(row["visible_position"].to_i - 1, row)
  end
  ordinary
end

def rendered(value)
  case value
  when true then "ON"
  when false then "OFF"
  when Hash
    value.key?("setting") ? "##{value['setting']}" : value.values.join(" / ")
  else
    value.to_s
  end
end

def normalized(value)
  value.to_s.upcase
       .gsub(/U\.?S\.?A?\.?\b/, "USA")
       .gsub(/\bJPN\b/, "JAPAN")
       .gsub(/\bEXP\b/, "EXPORT")
       .gsub(/C\.?R\.?T\.?/, "CRT")
       .gsub(/\bINITIAL\b|\bMAX(?: LIFE)?\b/, "")
       .gsub(/[^A-Z0-9#+-]+/, " ").strip
end

def documented_values(cell)
  return [] if ["—", "nessun ciclo dedicato", "non acquisiti"].include?(cell)
  cell.split(",").map(&:strip)
end

catalog = catalog_rows
errors = []
checked = 0
paths = Dir[YAML_ROOT.join("*.yaml")].sort
errors << "expected #{EXPECTED_YAML_COUNT} diagnostic YAML files, found #{paths.length}" unless paths.length == EXPECTED_YAML_COUNT
seen_games = Set.new

paths.each do |path|
  data = YAML.safe_load(File.read(path), permitted_classes: [], aliases: true)
  game = data.dig("game", "set")
  errors << "#{File.basename(path)}: duplicate game #{game.inspect}" unless seen_games.add?(game)
  errors << "#{File.basename(path)}: filename does not match game #{game.inspect}" unless File.basename(path, ".yaml") == game
  expected = catalog[game]
  unless expected
    errors << "#{File.basename(path)}: no catalog section for #{game.inspect}"
    next
  end

  actual = visible_rows(data, catalog)
  if actual.length != expected.length
    errors << "#{game}: #{actual.length} YAML rows, #{expected.length} catalog rows"
    next
  end

  actual.zip(expected).each_with_index do |(row, reference), index|
    unless normalized(row["label"]) == normalized(reference[:label])
      errors << "#{game} row #{index + 1}: #{row['label'].inspect} != #{reference[:label].inspect}"
      next
    end

    default = row["native_default"] || row["default"] || row["observed_native_value"]
    if default && normalized(rendered(default)) != normalized(reference[:default])
      errors << "#{game}:#{row['key']} default #{rendered(default).inspect} != #{reference[:default].inspect}"
    end

    next unless row["values"].is_a?(Array)
    expected_values = documented_values(reference[:values])
    next if expected_values.empty?
    yaml_values = row["values"].map { |value| normalized(rendered(value)) }.to_set
    catalog_values = expected_values.map { |value| normalized(value) }.to_set
    unless yaml_values == catalog_values
      errors << "#{game}:#{row['key']} values differ from the catalog"
    end
  end
  checked += 1
end

abort errors.join("\n") unless errors.empty?
puts "Diagnostic YAML audit passed: #{checked} files match #{CATALOG_PATH.basename}"
