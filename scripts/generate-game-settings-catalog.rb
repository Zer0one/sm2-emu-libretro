#!/usr/bin/env ruby
# frozen_string_literal: true

# Generates the authoritative Game Settings reference from archived Service
# Menu screenshots. games.xml supplies identities and parent relations only.
# Diagnostic YAML and the Libretro implementation are intentionally not read.

require "pathname"
require "rexml/document"

ROOT = Pathname.new(__dir__).parent.realpath
PARENT_SCREEN_ROOT = Pathname.new(ENV.fetch(
  "SM2_NVRAM_SCREEN_ROOT",
  File.expand_path("~/Documents/RetroArch/sm2-nvram-analysis")
))
CLONE_SCREEN_ROOT = ROOT / "build-libretro-gpu/validation/clone-service-menu-archive-20260915"
GAP_SCREEN_ROOT = ROOT / "build-libretro-gpu/validation/game-settings-gap-fill-20260920"
OUTPUT = ROOT / "GAME_SETTINGS_CATALOG.md"

Row = Struct.new(:slug, :label, :default, :kind, keyword_init: true)

def row(slug, label, default = "—", kind = "editable")
  Row.new(slug: slug, label: label, default: default, kind: kind)
end

# Row order and defaults are transcribed from each parent base screenshot. The
# slug connects a row to its value-cycle screenshots.
PARENTS = {
  "airwlkrs" => ["Game Options", [
    row("start-button-select", "START BUTTON SELECT", "START BUTTON"),
    row("player-selection", "PLAYER SELECTION", "2P SIMULTANEOUS"),
    row("attract-sound", "ATTRACT SOUND", "OFF"), row("difficulty", "GAME DIFFICULTY", "NORMAL"),
    row("game-time", "GAME TIME", "2:00"), row("replay", "REPLAY", "ON"),
    row("win-freeplay", "WIN FOR FREE PLAY", "CPU PLAY: YES / VS PLAY: YES"),
    row("character-type", "CHARACTER TYPE", "NORMAL")
  ]],
  "bel" => ["Game System", [
    row("coin-credit-setting", "COIN/CREDIT SETTING", "#1"), row("start-credits", "START CREDITS", "1"),
    row("continue-credits", "CONTINUE CREDITS", "1"), row("country", "COUNTRY", "U.S."),
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("difficulty", "DIFFICULTY", "6")
  ]],
  "daytona" => ["Game System", [
    row("link-id", "LINK ID", "MASTER"), row("car-number", "CAR NUMBER", "1"),
    row("cabinet", "CABINET", "TWIN"), row("country", "COUNTRY", "JPN"),
    row("difficulty", "DIFFICULTY", "NORMAL"), row("advertise-sound", "ADVERTISE SOUND", "ON"),
    row("game-mode", "GAME MODE", "NORMAL"), row("rival-arrow", "RIVAL ARROW", "ON")
  ]],
  "desert" => ["Game System", [
    row("coin-credit-setting", "COIN/CREDIT SETTING", "#1"), row("country", "COUNTRY", "JAPAN"),
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("difficulty-desert", "DIFFICULTY: DESERT", "EASY"),
    row("difficulty-canyon", "DIFFICULTY: CANYON", "EASY")
  ]],
  "doa" => ["Game Mode Settings", [
    row("vjcom-set-count", "VJCOM SET COUNT", "2"), row("vjman-set-count", "VJMAN SET COUNT", "2"),
    row("vjcom-difficulty", "VJCOM DIFFICULTY", "NORMAL"), row("vjcom-energy", "VJCOM ENERGY", "NORMAL"),
    row("vjman-energy", "VJMAN ENERGY", "NORMAL"), row("demo-sound", "DEMO SOUND", "ON"),
    row("nation", "NATION", "JAPAN"), row("continue", "CONTINUE", "ON"),
    row("vs-finish", "V.S FINISH", "OFF"), row("burst-mode", "BURST MODE", "OFF")
  ]],
  "dynabb" => ["Game Assignments", [
    row("game-difficulty", "GAME DIFFICULTY", "NORMAL"), row("advertise-sound", "ADVERTISE SOUND", "ON"),
    row("cabinet-type", "CABINET TYPE", "US"), row("favorite", "FAVORITE", "OFF"),
    row("innings", "INNINGS", "1 CREDIT / 2 INNINGS")
  ]],
  "dynabb97" => ["Game Assignments", [
    row("game-difficulty", "GAME DIFFICULTY", "NORMAL"), row("advertise-sound", "ADVERTISE SOUND", "ON"),
    row("cabinet-type", "CABINET TYPE", "US"), row("favorite", "FAVORITE", "OFF"),
    row("innings", "INNINGS", "1 CREDIT / 2 INNINGS")
  ]],
  "dynamcop" => ["Game Assignments", [
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("game-difficulty", "GAME DIFFICULTY", "3"),
    row("life-amount", "LIFE AMOUNT", "104"), row("violence-mode", "VIOLENCE MODE", "ON")
  ]],
  "fvipers" => ["Game Assignment", [
    row("match-count-1p", "MATCH COUNT (1P)", "2"), row("match-count-vs", "MATCH COUNT (VS)", "2"),
    row("difficulty", "DIFFICULTY", "NORMAL"), row("advertise-sound", "ADVERTISE SOUND", "ON"),
    row("continue", "CONTINUE", "ON"), row("country", "COUNTRY", "JAPAN"),
    row("display-type", "DISPLAY TYPE", "PROJECTOR"), row("vs-finish", "VS FINISH", "OFF"),
    row("ranking-mode", "RANKING MODE", "OFF")
  ]],
  "gunblade" => ["Game Assignments", [
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("country", "COUNTRY", "JAPAN"),
    row("game-difficulty", "GAME DIFFICULTY", "4/8"), row("shifting-difficulty", "SHIFTING DIFFICULTY", "4/8"),
    row("player-life", "PLAYER LIFE", "3"), row("gun-reaction", "GUN REACTION", "ON"),
    row("cabinet-type", "CABINET TYPE", "DX")
  ]],
  "hotd" => ["Game Assignments", [
    row("game-difficulty", "GAME DIFFICULTY", "NORMAL"), row("life-setting", "LIFE SETTING", "INITIAL 3 / MAX LIFE 5"),
    row("blood-color", "BLOOD COLOR", "GREEN"), row("advertise-sound", "ADVERTISE SOUND", "ON"),
    row("country", "COUNTRY", "JAPAN")
  ]],
  "hpyagu98" => ["Game Assignments", [
    row("game-difficulty", "GAME DIFFICULTY", "NORMAL"), row("advertise-sound", "ADVERTISE SOUND", "ON"),
    row("cabinet-type", "CABINET TYPE", "US"), row("favorite", "FAVORITE", "OFF"),
    row("innings", "INNINGS", "1 CREDIT / 2 INNINGS")
  ]],
  "indy500" => ["Game Assignments", [
    row("game-difficulty", "GAME DIFFICULTY", "NORMAL"), row("race-mode", "RACE MODE", "NORMAL"),
    row("handicap", "HANDICAP", "ON"), row("advertise-sound", "ADVERTISE SOUND", "OFF"),
    row("country", "COUNTRY", "USA"), row("cabinet-type", "CABINET TYPE", "TWIN"),
    row("network-type", "NETWORK TYPE", "STAND ALONE"), row("cabinet-id", "CABINET ID", "1"),
    row("engine-volume", "ENGINE VOLUME", "3"), row("default-view", "DEFAULT VIEW", "4")
  ]],
  "lastbrnx" => ["Game Assignments", [
    row("game-difficulty", "GAME DIFFICULTY", "1 NORMAL"), row("advertise-sound", "ADVERTISE SOUND", "OFF"),
    row("vs-finish", "VS FINISH", "OFF"), row("survival-mode", "SURVIVAL MODE", "ON"),
    row("match-point-cpu", "MATCH POINT (CPU)", "2"), row("match-point-vs", "MATCH POINT (VS)", "2"),
    row("cut-cross-street", "CUT CROSS STREET", "OFF"), row("display-type", "DISPLAY TYPE", "C.R.T."),
    row("master-volume", "MASTER VOLUME", "3")
  ]],
  "manxtt" => ["Game Assignments", [
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("country", "COUNTRY", "JAPAN"),
    row("cabinet-type", "CABINET TYPE", "DELUXE"), row("link-type", "LINK TYPE", "Not Link"),
    row("bike-color", "BIKE COLOR (No.)", "RED (No.1)"), row("race-mode", "RACE MODE", "RACE"),
    row("laxey-number-of-lap", "LAXEY NUMBER OF LAP", "2"), row("laxey-game-difficulty", "LAXEY GAME DIFFICULTY", "NORMAL"),
    row("laxey-revise-mode", "LAXEY REVISE MODE", "2/3"), row("tt-number-of-lap", "TT NUMBER OF LAP", "2"),
    row("tt-game-difficulty", "TT GAME DIFFICULTY", "NORMAL"), row("tt-revise-mode", "TT REVISE MODE", "2/3"),
    row("start-switch-op", "START SWITCH OP.", "ON")
  ]],
  "motoraid" => ["Game Assignments", [
    row("game-difficulty", "GAME DIFFICULTY", "NORMAL"), row("race-mode", "RACE MODE", "STANDARD"),
    row("enemy-level", "ENEMY LEVEL", "NORMAL"), row("engine-volume", "ENGINE VOLUME", "STANDARD"),
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("country", "COUNTRY", "JAPAN"),
    row("network-type", "NETWORK TYPE", "STAND ALONE"), row("cabinet-id", "CABINET ID", "1")
  ]],
  "overrev" => ["Foundational / Optional Game Setting", [
    row("country", "COUNTRY", "JAPAN"),
    row("hardware-type", "HARDWARE TYPE", "NORMAL (2in1)"),
    row("link-max", "LINK MAX", "NOT LINK"),
    row("link-type", "LINK TYPE", "MASTER CarNo.1"),
    row("time-difficulty", "TIME DIFFICULTY", "3"),
    row("steer-kick-back", "STEER KICK BACK", "3"),
    row("game-difficulty", "GAME DIFFICULTY", "NORMAL"),
    row("demo-sound", "DEMO SOUND", "ON"),
    row("maximum-lap", "MAXIMUM LAP", "NORMAL MODE")
  ]],
  "pltkids" => ["Game Configuring", [
    row("fighters", "Fighters", "3"), row("difficulty", "Difficult", "Normal"),
    row("demo-sound", "Demo Sound", "Off"), row("coin-slot", "Coin Slot", "Same"),
    row("coin-mode", "Coin Mode", "Normal"), row("coin-1", "Coin-1", "1 Coin = 1 Credit"),
    row("coin-2", "Coin-2", "1 Coin = 1 Credit"), row("continue", "Continue", "On")
  ]],
  "rascot2" => ["SEGANET wait", []],
  "rchase2" => ["Game Assignments", [
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("country", "COUNTRY", "JAPAN"),
    row("game-difficulty", "GAME DIFFICULTY", "4/8"), row("shifting-difficulty", "SHIFTING DIFFICULTY", "50 sec EVERY")
  ]],
  "schamp" => ["Game Assignments", [
    row("match-count-1p", "MATCH COUNT (1P)", "2"), row("match-count-vs", "MATCH COUNT (VS)", "2"),
    row("enemy-rank", "ENEMY RANK", "NORMAL"), row("time", "TIME", "30"),
    row("energy-1p", "ENERGY (1P)", "NORMAL"), row("energy-vs", "ENERGY (VS)", "NORMAL"),
    row("barrier", "BARRIER", "5"), row("barrier-reset", "BARRIER RESET", "OFF"),
    row("automatic", "AUTOMATIC", "ON"), row("hyper-mode", "HYPER MODE", "ON"),
    row("damage", "DAMAGE", "NORMAL"), row("advertise-sound", "ADVERTISE SOUND", "ON"),
    row("continue", "CONTINUE", "ON"), row("country", "COUNTRY", "USA"),
    row("display-type", "DISPLAY TYPE", "C.R.T."), row("vs-finish", "VS FINISH", "OFF")
  ]],
  "segawski" => ["Game Assignments", [
    row("game-difficulty", "GAME DIFFICULTY", "NORMAL"), row("advertise-sound", "ADVERTISE SOUND", "OFF")
  ]],
  "sgt24h" => ["Game Assignments", [
    row("link-type", "LINK TYPE", "NOT LINK"), row("link-max", "LINK MAX", "NOT LINK"),
    row("country", "COUNTRY", "JAPAN"), row("game-difficulty", "GAME DIFFICULTY", "NORMAL"),
    row("cpu-car-level", "CPU CAR LEVEL", "2"), row("steering-force", "STEERING FORCE", "2"),
    row("race-mode", "RACE MODE", "NORMAL"), row("game-bgm", "GAME BGM", "ON"),
    row("demo-sound", "DEMO SOUND", "ON"), row("io-type", "I/O TYPE", "A")
  ]],
  "skisuprg" => ["Game Assignments", [
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("country", "COUNTRY", "JAPAN"),
    row("network-type", "NETWORK TYPE", "NO LINK"), row("cabinet-id", "CABINET ID", "1"),
    row("drive-board-power", "DRIVE BOARD POWER", "2"), row("live-display", "LIVE DISPLAY", "ON"),
    row("victoria-display", "VICTORIA DISPLAY", "ON"), row("initial-time", "INITIAL TIME", "10"),
    row("stage-time-white-forest", "STAGE TIME (WHITE FOREST)", "10"),
    row("stage-time-night-valley", "STAGE TIME (NIGHT VALLEY)", "10"),
    row("stage-time-wild-king", "STAGE TIME (WILD KING)", "10")
  ]],
  "skytargt" => ["Game Assignments", [
    row("difficulty", "GAME DIFFICULTY", "NORMAL"), row("advertise-sound", "ADVERTISE SOUND", "ON"),
    row("country", "COUNTRY", "JAPAN"), row("cabinet", "CABINET TYPE", "STANDARD")
  ]],
  "srallyc" => ["Game Assignments", [
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("country", "COUNTRY", "JPN"),
    row("cabinet-type", "CABINET TYPE", "TWIN"), row("link-type", "LINK TYPE", "NOTLINK"),
    row("game-difficulty", "GAME DIFFICULTY", "NORMAL"), row("game-mode", "GAME MODE", "NORMAL")
  ]],
  "stcc" => ["Game Assignments", [
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("url-address", "URL ADDRESS", "ON"),
    row("country", "COUNTRY", "JAPAN"), row("cabinet-type", "CABINET TYPE", "TWIN"),
    row("link-type", "LINK TYPE", "STAND ALONE"), row("difficulty", "DIFFICULTY", "NORMAL"),
    row("game-mode", "GAME MODE", "NORMAL"), row("default-car", "DEFAULT CAR", "RANDOM"),
    row("name-entry", "NAME ENTRY", "BEFORE-3"), row("default-view", "DEFAULT VIEW", "BIRD'S")
  ]],
  "topskatr" => ["Game Assignments", [
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("game-difficulty", "GAME DIFFICULTY", "4/8")
  ]],
  "vcop" => ["Game System", [
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("country", "COUNTRY", "JPN"),
    row("cabinet", "CABINET", "DX"), row("difficulty", "DIFFICULTY", "NORMAL"),
    row("life", "LIFE", "6"), row(nil, "HUMAN TYPE", "NORMAL", "visible-only"),
    row(nil, "RELOAD TYPE", "NORMAL", "visible-only")
  ]],
  "vcop2" => ["Game Assignments", [
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("country", "COUNTRY", "JPN"),
    row("cabinet", "CABINET", "DX"), row("difficulty", "DIFFICULTY", "NORMAL"), row("life", "LIFE", "4")
  ]],
  "vf2" => ["Game Assignment", [
    row("match-count-1p", "MATCH COUNT (1P)", "2"), row("match-count-vs", "MATCH COUNT (VS)", "2"),
    row("difficulty", "DIFFICULTY", "NORMAL"), row(nil, "ENERGY MAX (1P)", "168", "visible-only"),
    row(nil, "ENERGY MAX (VS)", "288", "visible-only"), row(nil, "STAGE WIDTH", "1580", "visible-only"),
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("continue", "CONTINUE", "ON"),
    row("drink", "DRINK", "OK"), row("country", "COUNTRY", "JAPAN"),
    row("display-type", "DISPLAY TYPE", "PROJECTOR"), row("vs-finish", "VS FINISH", "OFF"),
    row("ranking-mode", "RANKING MODE", "OFF"), row("version", "VERSION", "NORMAL")
  ]],
  "von" => ["Game Assignments", [
    row("play-time-1p-stage-1-5", "PLAY TIME 1P (STAGE 1~5)", "90 SECS"),
    row("play-time-1p-penalty", "PLAY TIME 1P (PENALTY)", "90 SECS"),
    row("play-time-1p-stage-6-8", "PLAY TIME 1P (STAGE 6~8)", "90 SECS"),
    row("play-time-1p-last-stage", "PLAY TIME 1P (LAST STG)", "90 SECS"),
    row("play-time-versus", "PLAY TIME VERSUS", "90 SECS"),
    row("match-count-1p-stage-1-5", "MATCH COUNT 1P (STAGE 1~5)", "1"),
    row("match-count-1p-penalty", "MATCH COUNT 1P (PENALTY)", "1"),
    row("match-count-1p-stage-6-8", "MATCH COUNT 1P (STAGE 6~8)", "1"),
    row("match-count-versus", "MATCH COUNT VERSUS", "1"),
    row("network-link-attribute", "NETWORK LINK ATTRIBUTE", "NO LINK"),
    row("winning-by-decision", "WINNING BY DECISION", "ON"), row("game-difficulty", "GAME DIFFICULTY", "NORMAL"),
    row("advertise-sound", "ADVERTISE SOUND", "LOUD"), row("continue", "CONTINUE", "ON"),
    row("replay-and-posing-mode", "REPLAY AND POSING MODE", "REPLAY & POSING"),
    row("ranking-mode", "RANKING MODE", "ON"), row("versus-always-finish", "VERSUS ALWAYS FINISH", "OFF"),
    row("display-brightness", "DISPLAY BRIGHTNESS", "0")
  ]],
  "vstriker" => ["Game System", [
    row("advertise-sound", "ADVERTISE SOUND", "ON"), row("country", "COUNTRY", "JPN"),
    row("monitor", "MONITOR", "CRT"), row("difficulty", "DIFFICULTY", "NORMAL"),
    row("time-set", "TIME SET", "2:00"), row("v-goal-system", "V GOAL SYSTEM", "OFF"),
    row("v-goal-time", "V GOAL TIME SET", "0:15"), row("pk-system", "PK SYSTEM", "OFF"),
    row("pk-member", "PK MEMBER SET", "3"), row("billboard", "BILLBOARD", "ON"),
    row("one-match-mode", "ONE MATCH MODE", "OFF")
  ]],
  "waverunr" => ["Game Assignments", [
    row("game-difficulty", "GAME DIFFICULTY", "NORMAL"), row("race-mode", "RACE MODE", "NORMAL"),
    row("handicap", "HANDICAP", "ON"), row("advertise-sound", "ADVERTISE SOUND", "OFF"),
    row("country", "COUNTRY", "JAPAN"), row("network-type", "NETWORK TYPE", "STAND ALONE"),
    row("cabinet-id", "CABINET ID", "1")
  ]],
  "zerogun" => ["Configuring", [
    row("credit-mode", "Credit Mode", "Same"), row("continue-mode", "Continue Mode", "Normal Mode"),
    row("coin-slot-1", "Coin Slot 1", "1 coin / 1 credit"), row("coin-slot-2", "Coin Slot 2", "1 coin / 1 credit"),
    row("demo-sound", "Demo Sound", "ON"), row("difficulty", "Difficulty", "NORMAL"),
    row("fighters", "Fighters", "3 fighters"), row("extend-points", "Extend Points", "600000"),
    row("ranking-data", "Ranking Data", "Do Initialize")
  ]],
  "zeroguna" => ["Configuring", [
    row("credit-mode", "Credit Mode", "Same"), row("continue-mode", "Continue Mode", "Normal Mode"),
    row("coin-slot-1", "Coin Slot 1", "1 coin / 1 credit"), row("coin-slot-2", "Coin Slot 2", "1 coin / 1 credit"),
    row("demo-sound", "Demo Sound", "ON"), row("difficulty", "Difficulty", "NORMAL"),
    row("fighters", "Fighters", "3 fighters"), row("extend-points", "Extend Points", "600000"),
    row("ranking-data", "Ranking Data", "Do Initialize")
  ]]
}.freeze

# Clone observations are a one-time transcription of the archived clone screen.
CLONES = {
  "daytona93" => {rows: %w[advertise-sound country cabinet difficulty], defaults: {"cabinet" => "DELUXE"}, note: "Reduced four-row menu."},
  "daytonam" => {same: true},
  "daytonas" => {same: true, add: [row("promote-saturn", "PROMOTE SATURN", "COMING SOON")], defaults: {"cabinet" => "UPRIGHT", "country" => "USA"}, note: "Adds PROMOTE SATURN; dedicated cycles show OFF, COMING SOON and AVAILABLE NOW."},
  "daytonase" => {same: true, defaults: {"cabinet" => "SPECIAL"}, note: "Same row set; CABINET defaults to SPECIAL."},
  "doaa" => {same: true}, "doaab" => {same: true}, "doaae" => {same: true, defaults: {"nation" => "EXPORT"}}, "doab" => {same: true},
  "dynamcopb" => {same: true}, "dynamcopc" => {same: true},
  "dyndeka2" => {same: true, add: [row(nil, "HP PASSWORD", "NO PASSWORD", "informational")], note: "HP PASSWORD is visible but informational."},
  "dyndeka2b" => {same: true, add: [row(nil, "HP PASSWORD", "NO PASSWORD", "informational")], note: "HP PASSWORD is visible but informational."},
  "fvipersa" => {same: true}, "fvipersb" => {same: true}, "hotdo" => {same: true},
  "hotdp" => {rows: %w[game-difficulty life-setting blood-color gun-blowback advertise-sound country cabinet-type], defaults: {"game-difficulty" => "NORMAL", "life-setting" => "INITIAL 3 / MAX LIFE 5", "blood-color" => "RED", "gun-blowback" => "ON", "advertise-sound" => "ON", "country" => "JAPAN", "cabinet-type" => "STANDARD"}, labels: {"gun-blowback" => "GUN BLOWBACK", "cabinet-type" => "CABINET TYPE"}, note: "Dedicated clone campaign; adds GUN BLOWBACK and CABINET TYPE."},
  "indy500d" => {rows: %w[game-difficulty race-mode handicap advertise-sound country cabinet-type network-type cabinet-id], defaults: {"country" => "JAPAN", "cabinet-type" => "DELUXE"}, note: "ENGINE VOLUME and DEFAULT VIEW are absent."},
  "indy500to" => {same: true, defaults: {"country" => "JAPAN"}},
  "lastbrnxj" => {same: true}, "lastbrnxu" => {same: true},
  "manxttc" => {same: true, defaults: {"cabinet-type" => "TWIN", "start-switch-op" => "OFF"}},
  "manxttdx" => {rows: %w[advertise-sound country bike-color race-mode laxey-number-of-lap laxey-game-difficulty tt-number-of-lap tt-game-difficulty start-switch-op], defaults: {"start-switch-op" => "OFF"}, note: "CABINET TYPE, LINK TYPE and both REVISE MODE rows are absent."},
  "motoraiddx" => {same: true, add: [row("cabinet-type", "CABINET TYPE", "DELUXE")], defaults: {"engine-volume" => "OUT OF USE"}, note: "Adds CABINET TYPE; ENGINE VOLUME is OUT OF USE for Deluxe."},
  "overrevb" => {same: true, defaults: {"country" => "U.S.A."}, note: "Acquisiti entrambi i sottomenu; l'insieme delle righe coincide con il parent."},
  "overrevba" => {same: true, defaults: {"country" => "U.S.A."}, note: "Acquisiti entrambi i sottomenu; l'insieme delle righe coincide con il parent."},
  "pltkidsa" => {same: true}, "rchase2a" => {same: true},
  "sfight" => {same: true, defaults: {"automatic" => "OFF", "country" => "JAPAN"}},
  "srallycb" => {same: true}, "srallycc" => {same: true},
  "srallycdx" => {rows: %w[advertise-sound country game-difficulty game-mode], note: "DX menu has only four rows; CABINET TYPE and LINK TYPE are absent."},
  "srallycdxa" => {rows: %w[advertise-sound country game-difficulty game-mode], note: "DX menu has only four rows; a dedicated value campaign is archived."},
  "stcca" => {rows: %w[advertise-sound url-address country cabinet-type link-type difficulty game-mode default-car name-entry], defaults: {"country" => "USA (selected capture; native default not established)"}, note: "DEFAULT VIEW is absent. The archived screen intentionally shows COUNTRY=USA and is not a native-default capture."},
  "stccb" => {same: true, defaults: {"default-view" => "DRIVER'S"}},
  "stcco" => {rows: %w[advertise-sound url-address country cabinet-type link-type difficulty game-mode default-car name-entry], defaults: {"country" => "USA (selected capture; native default not established)", "name-entry" => "AFTER-3"}, note: "DEFAULT VIEW is absent. The archived screen intentionally shows COUNTRY=USA and is not a native-default capture."},
  "topskatrj" => {same: true}, "topskatru" => {same: true}, "topskatruo" => {same: true},
  "vcopa" => {same: true}, "vf2a" => {same: true}, "vf2b" => {same: true}, "vf2o" => {same: true},
  "vonj" => {same: true}, "vonr" => {same: true, note: "Game Assignments diventa accessibile dopo il timeout della rete Relay; righe e default coincidono con il parent."}, "vonu" => {same: true},
  "vstrikero" => {rows: %w[advertise-sound country monitor difficulty time-set v-goal-system v-goal-time pk-system pk-member billboard], note: "ONE MATCH MODE is absent."},
  "zerogunaj" => {same: true}, "zerogunj" => {same: true}
}.freeze

def games_from_xml
  document = REXML::Document.new((ROOT / "data/games.xml").read)
  games = {}
  document.elements.each("games/game") do |element|
    games[element.attributes["name"]] = {
      parent: element.attributes["parent"],
      title: element.elements["title"]&.text
    }
  end
  games.each_value { |game| game[:title] ||= games.dig(game[:parent], :title) }
  games
end

def cycle_values(game, slug, root = PARENT_SCREEN_ROOT)
  return [] unless slug
  directory = root / game / "generated/screenshots"
  # Dedicated clone campaigns use .../<set>/screenshots instead.
  directory = root / game / "screenshots" unless directory.directory?
  return [] unless directory.directory?
  prefix = "#{game}-#{slug}-"
  suffix = "-#{slug}"
  directory.children.sort.map do |path|
    stem = path.basename(".png").to_s
    next unless stem.start_with?(prefix) && stem.end_with?(suffix)
    stem.delete_prefix(prefix).delete_suffix(suffix)
  end.compact.uniq
end

def observed_clone_values(game, slug, root)
  # The hotdp campaign intentionally attempted the two parent-only colours.
  # Its "blue" and "purple" captures visibly wrap to RED and GREEN, so the
  # requested filenames are not observed values for this clone.
  return %w[red green] if game == "hotdp" && slug == "blood-color"

  cycle_values(game, slug, root)
end

def display_value(slug, setting = nil)
  return "—" if slug.nil? || slug.empty?
  special = {
    "jpn" => "JPN", "exp" => "EXP", "u-s" => "U.S.", "u-r" => "U/R",
    "sp-u-r" => "SP U/R", "notlink" => "NOTLINK", "not-link" => "NOT LINK",
    "crt" => "CRT", "dx" => "DX", "sd" => "SD", "io" => "I/O",
    "birds" => "BIRD'S", "drivers" => "DRIVER'S"
  }
  return special.fetch(slug) if special.key?(slug)
  return "TIME FREE" if setting == "v-goal-time" && slug == "free"
  return "REAL PK" if setting == "pk-member" && slug == "real"
  return "#{slug} fighter#{slug == '1' ? '' : 's'}" if setting == "fighters" && slug.match?(/\A\d+\z/)
  match = slug.match(/\A(\d+)c-(\d+)c\z/)
  return "#{match[1]} coin#{match[1] == '1' ? '' : 's'} / #{match[2]} credit#{match[2] == '1' ? '' : 's'}" if setting&.start_with?("coin-slot") && match
  match = slug.match(/\A(plus|minus)-(\d+)\z/)
  return "#{match[1] == 'plus' ? '+' : '-'}#{match[2]}" if setting == "display-brightness" && match
  return "##{slug.to_i}" if setting == "coin-credit-setting" && slug.match?(/\A\d+\z/)
  match = slug.match(/\A(\d+)-(\d+)\z/)
  return "#{match[1]}/#{match[2]}" if setting == "life-setting" && match
  match = slug.match(/\A(\d+)-of-(\d+)\z/)
  return "#{match[1]}/#{match[2]}" if match
  match = slug.match(/\A(\d+)m(\d+)\z/)
  return "#{match[1]}:#{match[2]}" if match
  slug.split("-").map { |part| special.fetch(part, part.upcase) }.join(" ")
end

def values_for(game, item)
  cycle_values(game, item.slug).map { |value| display_value(value, item.slug) }
end

def markdown(value)
  value.to_s.gsub("|", "\\|").gsub("\n", " ")
end

def base_screenshot(game)
  directory = PARENT_SCREEN_ROOT / game / "generated/screenshots"
  return nil unless directory.directory?
  directory.children.find { |path| path.basename.to_s.end_with?("-base-base.png") }
end

def clone_screenshots(game)
  gap_directory = GAP_SCREEN_ROOT / game / "screenshots"
  if gap_directory.directory?
    patterns = case game
               when "overrevb", "overrevba" then %w[foundational-settings optional-game-settings]
               when "vonr" then %w[game-assignments.png]
               else []
               end
    selected = gap_directory.children.sort.select do |path|
      patterns.any? { |pattern| path.basename.to_s.include?(pattern) }
    end
    return selected unless selected.empty?
  end
  directory = CLONE_SCREEN_ROOT / game / "screenshots"
  return [] unless directory.directory?
  screenshot = directory.children.sort.find do |path|
    path.basename.to_s.match?(/(settings-main|game-assignments|game-system|game-mode-settings|country-usa)/)
  end
  screenshot ? [screenshot] : []
end

def clone_rows(parent, rule)
  parent_rows = PARENTS.fetch(parent).last
  rows = if rule[:same]
           parent_rows.map(&:dup)
         else
           index = parent_rows.to_h { |item| [item.slug, item] }
           Array(rule[:rows]).map do |slug|
             index[slug]&.dup || row(slug, rule.fetch(:labels, {}).fetch(slug, slug.tr("-", " ").upcase))
           end
         end
  rows.concat(Array(rule[:add]).map(&:dup))
  rows.each do |item|
    item.default = rule.fetch(:defaults, {}).fetch(item.slug, item.default)
    item.label = rule.fetch(:labels, {}).fetch(item.slug, item.label)
  end
  rows
end

games = games_from_xml
parents = games.select { |_name, game| game[:parent].nil? }
clones = games.reject { |_name, game| game[:parent].nil? }

missing_parent_definitions = parents.keys - PARENTS.keys
missing_clone_definitions = clones.keys - CLONES.keys
abort "Missing parent definitions: #{missing_parent_definitions.join(', ')}" unless missing_parent_definitions.empty?
abort "Missing clone definitions: #{missing_clone_definitions.join(', ')}" unless missing_clone_definitions.empty?

missing_parent_screens = PARENTS.keys.reject { |name| name == "rascot2" || base_screenshot(name) }
abort "Missing parent base screenshots: #{missing_parent_screens.join(', ')}" unless missing_parent_screens.empty?
missing_clone_screens = CLONES.keys.reject { |name| !clone_screenshots(name).empty? }
abort "Missing clone screenshots: #{missing_clone_screens.join(', ')}" unless missing_clone_screens.empty?

unmatched_parent_screens = []
PARENTS.each do |game, (_menu, rows)|
  directory = PARENT_SCREEN_ROOT / game / "generated/screenshots"
  next unless directory.directory?
  slugs = rows.map(&:slug).compact
  directory.children.each do |path|
    stem = path.basename(".png").to_s.delete_prefix("#{game}-")
    next if stem == "base-base" || game == "rascot2"
    unmatched_parent_screens << "#{game}/#{path.basename}" unless slugs.any? { |slug| stem.start_with?("#{slug}-") }
  end
end
abort "Unmatched parent screenshots: #{unmatched_parent_screens.join(', ')}" unless unmatched_parent_screens.empty?

lines = []
lines << "# Catalogo autorevole dei Game Settings SEGA Model 2"
lines << ""
lines << "Questo documento trascrive i Service Menu acquisiti nelle campagne screenshot."
lines << "Gli screenshot sono la fonte autorevole per presenza, ordine e default visibile delle righe;"
lines << "i nomi delle acquisizioni cicliche forniscono i valori osservati. `games.xml` viene usato"
lines << "soltanto per nome del set, titolo e relazione parent/clone. Il generatore non legge i file"
lines << "YAML diagnostici né l'implementazione Libretro."
lines << ""
lines << "## Regole di lettura"
lines << ""
lines << "- **Valori osservati** contiene solo valori per i quali esiste uno screenshot della campagna ciclica."
lines << "- **Solo visibile** indica una riga leggibile nella schermata base, ma priva di ciclo dedicato."
lines << "- Per un clone senza campagna ciclica, i valori sono marcati come provenienti dal parent e non vengono presentati come verificati sul clone."
lines << "- Un dato non acquisito non viene dedotto copiandolo dal parent."
lines << ""
lines << "## Copertura"
lines << ""
lines << "| Elemento | Copertura |"
lines << "| --- | ---: |"
lines << "| Set totali in `games.xml` | #{games.length} |"
lines << "| Parent catalogati | #{PARENTS.length} / #{parents.length} |"
lines << "| Cloni catalogati | #{CLONES.length} / #{clones.length} |"
lines << "| Righe parent trascritte | #{PARENTS.values.sum { |_menu, rows| rows.length }} |"
lines << "| Righe parent con ciclo o campagna dedicata | #{PARENTS.values.sum { |_menu, rows| rows.count { |item| item.kind == 'editable' } }} |"
lines << "| Righe parent solo visibili nella schermata base | #{PARENTS.values.sum { |_menu, rows| rows.count { |item| item.kind != 'editable' } }} |"
primary_parent_screens = PARENT_SCREEN_ROOT.children.sum do |directory|
  screens = directory / "generated/screenshots"
  screens.directory? ? screens.children.count { |path| path.extname == ".png" } : 0
end
all_parent_pngs = Dir[PARENT_SCREEN_ROOT.join("**/*.png")].length
lines << "| Screenshot primari della campagna parent indicizzati | #{primary_parent_screens} |"
lines << "| PNG supplementari nell'archivio parent | #{all_parent_pngs - primary_parent_screens} |"
lines << "| Campagne parent con schermata base ordinaria | #{PARENTS.count { |name, (_menu, rows)| !rows.empty? && base_screenshot(name) }} |"
lines << "| Acquisizioni dichiarate incomplete | #{CLONES.count { |_name, rule| rule[:incomplete] }} |"
lines << ""

lines << "## Matrice delle differenze parent/clone"
lines << ""
lines << "Questa matrice è calcolata dalle trascrizioni del catalogo. Non richiede un nuovo confronto visivo."
lines << ""
lines << "| Clone | Parent | Righe aggiunte | Righe rimosse | Default/valori visibili diversi | Stato |"
lines << "| --- | --- | --- | --- | --- | --- |"
CLONES.each do |name, rule|
  parent = games.fetch(name)[:parent]
  if rule[:incomplete]
    lines << "| `#{name}` | `#{parent}` | — | — | — | incompleto |"
    next
  end
  parent_rows = PARENTS.fetch(parent).last
  rows = clone_rows(parent, rule)
  parent_by_slug = parent_rows.reject { |item| item.slug.nil? }.to_h { |item| [item.slug, item] }
  parent_ids = parent_rows.map { |item| item.slug || "@#{item.label}" }
  clone_ids = rows.map { |item| item.slug || "@#{item.label}" }
  added = (clone_ids - parent_ids).map { |identity| identity.delete_prefix("@") }
  removed = (parent_ids - clone_ids).map { |identity| identity.delete_prefix("@") }
  changed = rows.map do |item|
    next if item.slug.nil?
    original = parent_by_slug[item.slug]
    next unless original && item.default != original.default
    "#{item.label}: #{original.default} → #{item.default}"
  end.compact
  lines << "| `#{name}` | `#{parent}` | #{markdown(added.empty? ? '—' : added.join(', '))} | #{markdown(removed.empty? ? '—' : removed.join(', '))} | #{markdown(changed.empty? ? '—' : changed.join('; '))} | acquisito |"
end
lines << ""

lines << "## Parent"
lines << ""
PARENTS.each do |name, (menu, rows)|
  game = games.fetch(name)
  screenshot = base_screenshot(name)
  lines << "### #{game[:title] || name} (`#{name}`)"
  lines << ""
  if name == "rascot2"
    special = GAP_SCREEN_ROOT / name / "screenshots"
    service = special / "rascot2-service-short-service-short.png"
    local_start = special / "rascot2-local-start-test-menu-after-shot1.png"
    test_return = special / "rascot2-local-start-test-menu-local-start-test-menu.png"
    lines << "**Stato:** caso speciale acquisito. La diagnostica mostra l'attesa SEGANET e indica `Push Shot1`; Button 1 avvia correttamente il titolo senza SegaNetCom. Il successivo comando Test torna alla diagnostica SEGANET e non espone un normale menu Game Settings. Non risultano quindi righe Game Settings ordinarie da catalogare nell'emulazione corrente."
    lines << ""
    lines << "**Evidenza:** [diagnostica SEGANET](<#{service}>), [avvio locale dopo Button 1](<#{local_start}>), [ritorno della diagnostica con Test](<#{test_return}>)"
    lines << ""
    next
  end
  lines << "- **Menu:** #{menu}"
  lines << "- **Schermata base:** [apri screenshot](<#{screenshot}>)"
  directory = PARENT_SCREEN_ROOT / name / "generated/screenshots"
  count = directory.children.count { |path| path.extname == ".png" }
  lines << "- **Screenshot della campagna:** #{count}"
  lines << ""
  lines << "| # | Game Setting | Default visibile | Valori osservati negli screenshot | Evidenza |"
  lines << "| ---: | --- | --- | --- | --- |"
  rows.each_with_index do |item, index|
    values = values_for(name, item)
    value_text = if item.kind != "editable"
                   "—"
                 elsif values.empty?
                   "nessun ciclo dedicato"
                 else
                   values.join(", ")
                 end
    evidence = item.kind == "editable" ? (values.empty? ? "base" : "base + ciclo") : item.kind
    lines << "| #{index + 1} | #{markdown(item.label)} | #{markdown(item.default)} | #{markdown(value_text)} | #{evidence} |"
  end
  lines << ""
end

lines << "## Cloni"
lines << ""
lines << "Ogni tabella seguente è la trascrizione strutturata dello screenshot del clone. Quando"
lines << "l'insieme delle righe coincide con il parent, il catalogo lo espande comunque per rendere"
lines << "immediate le differenze. I valori possibili restano attribuiti alla campagna parent finché"
lines << "non esiste una campagna ciclica dedicata al clone."
lines << ""

CLONES.each do |name, rule|
  game = games.fetch(name)
  parent = game[:parent]
  screenshots = clone_screenshots(name)
  lines << "### #{game[:title] || name} (`#{name}`)"
  lines << ""
  lines << "- **Parent:** `#{parent}`"
  unless screenshots.empty?
    links = screenshots.each_with_index.map { |path, index| "[#{screenshots.length == 1 ? 'apri screenshot' : "schermata #{index + 1}"}](<#{path}>)" }
    lines << "- **Schermate acquisite:** #{links.join(', ')}"
  end
  lines << ""
  if rule[:incomplete]
    lines << "**Acquisizione incompleta:** #{rule[:incomplete]}"
    lines << ""
    next
  end
  rows = clone_rows(parent, rule)
  parent_rows = PARENTS.fetch(parent).last
  parent_slugs = parent_rows.map(&:slug).compact
  clone_slugs = rows.map(&:slug).compact
  added = clone_slugs - parent_slugs
  removed = parent_slugs - clone_slugs
  structural = if added.empty? && removed.empty?
                 "nessuna riga aggiunta o rimossa"
               else
                 [added.empty? ? nil : "aggiunte: #{added.join(', ')}", removed.empty? ? nil : "rimosse: #{removed.join(', ')}"].compact.join("; ")
               end
  lines << "- **Differenze strutturali:** #{structural}."
  lines << "- **Nota:** #{rule[:note]}" if rule[:note]
  lines << ""
  lines << "| # | Game Setting | Default/valore visibile nel clone | Valori documentati | Origine valori |"
  lines << "| ---: | --- | --- | --- | --- |"
  rows.each_with_index do |item, index|
    dedicated_root = %w[hotdp srallycdxa].include?(name) ? ROOT / "build-libretro-gpu/validation/nvram-campaigns" : nil
    dedicated_values = dedicated_root ? observed_clone_values(name, item.slug, dedicated_root) : []
    if name == "daytonas" && item.slug == "promote-saturn"
      value_text, source = "OFF, COMING SOON, AVAILABLE NOW", "ciclo clone"
    elsif name == "daytonas" && item.slug == "cabinet"
      value_text, source = "DELUXE, TWIN, SPECIAL, UPRIGHT", "ciclo clone"
    elsif name == "daytona93" && item.slug == "cabinet"
      value_text, source = "DELUXE, UPRIGHT", "ciclo clone"
    elsif name == "motoraiddx" && item.slug == "cabinet-type"
      value_text, source = "DELUXE, TWIN", "ciclo clone"
    elsif !dedicated_values.empty?
      value_text = dedicated_values.map { |value| display_value(value, item.slug) }.join(", ")
      source = "ciclo clone"
    elsif item.kind != "editable"
      value_text, source = "—", item.kind
    else
      parent_row = parent_rows.find { |candidate| candidate.slug == item.slug }
      parent_values = parent_row ? values_for(parent, parent_row) : []
      value_text = parent_values.empty? ? "non acquisiti" : parent_values.join(", ")
      source = parent_values.empty? ? "nessun ciclo" : "campagna parent `#{parent}`"
    end
    lines << "| #{index + 1} | #{markdown(item.label)} | #{markdown(item.default)} | #{markdown(value_text)} | #{source} |"
  end
  lines << ""
end

lines << "## Lacune residue"
lines << ""
incomplete = CLONES.select { |_name, rule| rule[:incomplete] }
if incomplete.empty?
  lines << "Nessuna. Tutti i parent e i cloni presenti in `games.xml` hanno una trascrizione strutturata oppure, nel caso speciale `rascot2`, un esito diagnostico conclusivo senza Game Settings ordinari."
else
  lines << "| Set | Lacuna |"
  lines << "| --- | --- |"
  incomplete.each { |name, rule| lines << "| `#{name}` | #{rule[:incomplete]} |" }
end

OUTPUT.write(lines.join("\n") + "\n")
puts "Wrote #{OUTPUT} (#{PARENTS.length} parents, #{CLONES.length} clones, #{games.length} total sets)"
