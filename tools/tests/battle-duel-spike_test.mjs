// Run: node --test tools/tests/battle-duel-spike_test.mjs
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import {
  loadMock, SPIKE_MATCHUP, DEFAULT_MATRIX_ID, DEFAULT_TEAM_SIZE, DEFAULT_MAX_ROUNDS,
  buildDuel, playerPick, playDuel, runMatrix, runReport, parseArgs, POLICIES,
  TEAM_PAIRS, runPairSweep, runPairSweepReport, formatPairSweepReport,
  DUEL_SPIKE_SCHEMA_VERSION, buildJsonReport, buildPairSweepJson, buildProvenance,
  canonicalJson, sha256Hex, fixtureHashes, PROVENANCE_FILES, READ_RATES,
} from '../battle-duel-spike.mjs';

const mock = loadMock();
const REPO = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const TOOL = path.join(REPO, 'tools/battle-duel-spike.mjs');
const HEX64 = /^[0-9a-f]{64}$/;
const REQUIRED_POLICIES = ['random', 'repeat', 'counter', 'rotation', 'swap'];
const TIER_POLICY = 'tier';

test('the balance-contract policies are present', () => {
  for (const policy of REQUIRED_POLICIES) assert.ok(POLICIES.includes(policy), policy);
  assert.ok(POLICIES.includes(TIER_POLICY), 'mirror-tier policy present');
  assert.ok(POLICIES.includes('anti-repeat'), 'anti-repeat retained');
});

test('tier mirrors the AI creature-type tier and is deterministic', () => {
  const { state } = buildDuel({ seed: 4, policy: TIER_POLICY });
  state.duelState.partyIndex = [[37], [6]];
  state.duelState.hp = [[mock.creature(37).maxHp], [mock.creature(6).maxHp]];
  state.duelState.activeSlot = [0, 0];
  const options = mock.duelOptions(state, 0);
  const bonuses = options.map((option) => mock.duelCreatureBonus(state, 0, option.type));
  const bestTier = Math.max(...bonuses);
  assert.ok(options.length > 0);
  // Every tie-break lands on a best-tier option; the tier is the AI's non-read
  // branch (creature-type x1.5/x0.5 against the foe's active types).
  for (const tie of [0, 1, 2, 5]) {
    const pick = playerPick(mock, state, TIER_POLICY, { int: (n) => (n ? tie % n : 0) });
    assert.ok(Number.isInteger(pick) && pick >= 0 && pick < options.length, `pick ${pick}`);
    assert.equal(bonuses[pick], bestTier, `tier pick ${options[pick].type} not best`);
  }
  // Same seed/scenario replays byte-identically through the full duel.
  const a = playDuel({ persona: 'reader', readRate: 0, policy: TIER_POLICY, seed: 42 });
  const b = playDuel({ persona: 'reader', readRate: 0, policy: TIER_POLICY, seed: 42 });
  assert.deepEqual(a, b);
});

test('CLI argument parsing reads the plan command and coerces values', () => {
  const args = parseArgs(['--battles', '1000', '--rounds', '200']);
  assert.equal(args.battles, 1000);
  assert.equal(args.rounds, 200);
  const mixed = parseArgs(['--persona', 'reader', '--policy', 'counter', '--read', '45']);
  assert.equal(mixed.persona, 'reader');
  assert.equal(mixed.policy, 'counter');
  assert.equal(mixed.read, 45);
  const flagOnly = parseArgs(['--battles', '--verbose']);
  assert.equal(flagOnly.battles, true);
});

test('duel defaults are matrix5 / team3 / bonus / cap 200', () => {
  assert.equal(DEFAULT_MATRIX_ID, '5');
  assert.equal(DEFAULT_TEAM_SIZE, 3);
  assert.equal(DEFAULT_MAX_ROUNDS, 200);
  const { state } = buildDuel({ seed: 1 });
  assert.equal(state.duel.matrixId, '5');
  assert.equal(state.duel.teamSize, 3);
  assert.equal(state.duel.scoring, 'bonus');
  assert.equal(state.duelState.partyIndex[0].length, 3);
  assert.equal(state.duelState.partyIndex[1].length, 3);
});

test('playDuel is deterministic and terminates within the round cap', () => {
  const a = playDuel({ persona: 'reader', readRate: 50, policy: 'random', seed: 42 });
  const b = playDuel({ persona: 'reader', readRate: 50, policy: 'random', seed: 42 });
  assert.deepEqual(a, b);
  assert.ok(['WIN', 'LOSE', 'DRAW', 'TIMEOUT'].includes(a.outcome), a.outcome);
  assert.ok(a.rounds > 0 && a.rounds <= DEFAULT_MAX_ROUNDS);
});

test('--rounds cap is honored (rounds never exceed maxRounds)', () => {
  const capped = playDuel({ persona: 'reader', readRate: 100, policy: 'counter', seed: 3, maxRounds: 2 });
  assert.ok(capped.rounds <= 2, `rounds ${capped.rounds}`);
  const one = playDuel({ persona: 'reader', readRate: 100, policy: 'counter', seed: 4, maxRounds: 1 });
  assert.ok(one.rounds <= 1, `rounds ${one.rounds}`);
});

test('runMatrix splits outcomes and reports compact metrics with denominators', () => {
  const result = runMatrix({ battles: 40, persona: 'frequency', readRate: 50, policy: 'counter' });
  assert.equal(result.wins + result.losses + result.draws + result.timeouts, result.battles);
  assert.equal(result.matrixId, '5');
  assert.equal(result.teamSize, 3);
  assert.equal(result.maxRounds, DEFAULT_MAX_ROUNDS);
  assert.ok(result.avgRounds > 0);
  assert.ok(result.medianRounds <= result.p95Rounds);
  // Denominators are explicit: switch rate over rounds, type skew over attacks.
  assert.equal(result.switchRate, result.switches / result.rounds);
  assert.equal(result.attacks, Object.values(result.typeUse).reduce((sum, n) => sum + n, 0));
  assert.equal(result.speciesSamples, Object.values(result.speciesUse).reduce((sum, n) => sum + n, 0));
  assert.ok(result.rounds > 0 && result.attacks > 0 && result.speciesSamples > 0);
  // ohk means a full-HP member KO'd in one hit. Max single hit is 21 and the
  // smallest max HP is 52, so a normal matrix must report zero.
  assert.equal(result.oneHitKOs, 0);
  assert.ok(result.kos >= result.oneHitKOs && result.kos > 0, 'faints counted');
  assert.ok(Object.keys(result.typeUse).length > 0, 'type skew present');
  assert.ok(Object.keys(result.speciesUse).length > 0, 'species skew present');
});

test('repeat and rotation are deterministic over owned options + self history', () => {
  const { state } = buildDuel({ seed: 2, policy: 'repeat' });
  state.duelState.partyIndex = [[37], [6]];
  state.duelState.hp = [[mock.creature(37).maxHp], [mock.creature(6).maxHp]];
  state.duelState.activeSlot = [0, 0];
  const options = mock.duelOptions(state, 0);
  const throwing = { int: () => { throw new Error('repeat/rotation must be deterministic'); } };

  // No revealed history: both fall back to the first owned option.
  state.duelState.history = [];
  assert.equal(playerPick(mock, state, 'repeat', throwing), 0);
  assert.equal(playerPick(mock, state, 'rotation', throwing), 0);

  // Revealed own type -> repeat re-picks that option; rotation advances one.
  const last = options[options.length - 1].type;
  state.duelState.history = [last];
  const firstMatch = options.findIndex((option) => option.type === last);
  assert.ok(firstMatch >= 0);
  assert.equal(playerPick(mock, state, 'repeat', throwing), firstMatch);
  assert.equal(playerPick(mock, state, 'rotation', throwing), (firstMatch + 1) % options.length);

  // Every policy selects only an owned option index, and never reads the hidden pick.
  state.duelState.history = [options[0].type];
  for (const policy of POLICIES) {
    const index = playerPick(mock, state, policy, { int: () => 0 });
    assert.ok(index === -1 || (Number.isInteger(index) && index >= 0 && index < options.length),
      `${policy} -> ${index}`);
  }
});

test('policies fold through duelValue, not the 8-type wheel', () => {
  const { state } = buildDuel({ seed: 1, policy: 'counter' });
  // Sporeling's kit is [spirit, plant] (mask bits ascending). A revealed wind
  // (folded to water in matrix5) is beaten only by plant, so the folded counter
  // must pick the plant option at index 1 - not an index-based 8-type wheel pick.
  state.duelState.partyIndex = [[37], [6]];
  state.duelState.hp = [[mock.creature(37).maxHp], [mock.creature(6).maxHp]];
  state.duelState.activeSlot = [0, 0];
  state.duelState.last = [null, 'wind'];
  state.duelState.history = [];
  assert.deepEqual(mock.duelOptions(state, 0).map((option) => option.type), ['spirit', 'plant']);

  const originalWheelBeats = mock.wheelBeats;
  mock.wheelBeats = () => { throw new Error('wheelBeats must not be used by the folded policies'); };
  try {
    for (const policy of POLICIES) {
      const pick = playerPick(mock, state, policy, { int: () => 0 });
      assert.ok(Number.isInteger(pick), `${policy} pick`);
    }
  } finally {
    mock.wheelBeats = originalWheelBeats;
  }
  const counter = playerPick(mock, state, 'counter', { int: () => 0 });
  assert.equal(counter, 1, `counter folds wind->water and picks plant (${counter})`);
});

test('every policy ignores the opponent current hidden pick (revealed history only)', () => {
  const { state } = buildDuel({ seed: 9, policy: 'counter' });
  state.duelState.partyIndex = [[37], [6]];
  state.duelState.hp = [[mock.creature(37).maxHp], [mock.creature(6).maxHp]];
  state.duelState.activeSlot = [0, 0];
  state.duelState.last = [null, 'wind'];
  state.duelState.history = ['fire'];
  state.duelState.picks = [null, null];
  for (const policy of POLICIES) {
    const clean = playerPick(mock, state, policy, { int: () => 0 });
    // Poison the opponent's hidden current pick: the policy must not observe it.
    state.duelState.picks = [null, 'plant'];
    const poisoned = playerPick(mock, state, policy, { int: () => 0 });
    assert.equal(clean, poisoned, `${policy} observed the hidden pick`);
  }
});

test('structured team-pair seam pins authored presets without touching the model', () => {
  assert.equal(TEAM_PAIRS.length, mock.TEAM_DATA.teams.length ** 2);
  const indicesFor = (id) => mock.TEAM_DATA.teams.find((t) => t.id === id).members
    .map((m) => mock.DATA.creatures.findIndex((c) => c.name === m.creature));

  const { state } = buildDuel({ youPreset: 'balanced', foePreset: 'anchors' });
  assert.deepEqual(state.duelState.partyIndex[0], indicesFor('balanced'));
  assert.deepEqual(state.duelState.partyIndex[1], indicesFor('anchors'));

  const sweep = runPairSweep({
    pairs: [{ you: 'balanced', foe: 'anchors' }, { you: 'support', foe: 'retyped' }],
    battles: 4, maxRounds: 40, persona: 'reader', readRate: 50, policy: 'counter',
  });
  assert.equal(sweep.length, 2);
  assert.equal(sweep[0].you, 'balanced');
  assert.equal(sweep[0].foe, 'anchors');
  assert.equal(sweep[0].result.youPreset, 'balanced');
  assert.equal(sweep[0].result.foePreset, 'anchors');
  assert.equal(sweep[1].result.youPreset, 'support');
  assert.equal(sweep[1].result.wins + sweep[1].result.losses +
    sweep[1].result.draws + sweep[1].result.timeouts, 4);
});

test('runReport uses matrix5/team3 and reports outcomes separately', () => {
  const report = runReport(parseArgs(['--battles', '6', '--rounds', '60']));
  assert.match(report, /matrix5 team3 bonus/);
  assert.match(report, /round cap 60/);
  assert.match(report, /metrics at 50% read/);
  assert.match(report, /\bW\s+\d+\s+L\s+\d+\s+D\s+\d+\s+T\s+\d+/);
  const single = runReport(parseArgs([
    '--battles', '6', '--rounds', '60', '--persona', 'reader', '--policy', 'counter', '--read', '45',
  ]));
  assert.match(single, /reader\/counter@45%/);
  assert.match(single, /species /);
});

test('the pinned singleton isolate stays available for the read-rate spike', () => {
  const result = runMatrix({
    battles: 12, maxRounds: 40, persona: 'reader', readRate: 50,
    policy: 'anti-repeat', matchup: SPIKE_MATCHUP,
  });
  assert.equal(result.wins + result.losses + result.draws + result.timeouts, result.battles);
  assert.ok(result.avgRounds > 0 && result.avgRounds <= 40);
});

test('pair-sweep report runs every sample pair across policies and flags cells', () => {
  const sweep = runPairSweepReport({ battles: 3, maxRounds: 30, persona: 'reader' });
  assert.equal(sweep.rows.length, 25, 'all authored sample-team ordered pairs');
  assert.equal(sweep.policies.length, POLICIES.length);
  for (const row of sweep.rows) {
    assert.equal(row.wins + row.losses + row.draws + row.timeouts, row.total,
      `${row.you}vs${row.foe} outcome split`);
    assert.equal(row.total, 3 * sweep.policies.length * sweep.rates.length);
    assert.ok(row.playerWinRate >= 0 && row.playerWinRate <= 1);
    assert.equal(typeof row.flags.timeouts, 'boolean');
    assert.equal(typeof row.flags.sweep, 'boolean');
  }
  const text = formatPairSweepReport(sweep);
  assert.match(text, /25 pairs/);
  assert.match(text, /flags:/);
});

test('canonicalJson sorts keys and sha256Hex is the standard digest', () => {
  assert.equal(canonicalJson({ b: 1, a: [2, { d: 4, c: 3 }] }), '{"a":[2,{"c":3,"d":4}],"b":1}');
  // Known-answer test: SHA-256("") and SHA-256("abc").
  assert.equal(sha256Hex(''), 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855');
  assert.equal(sha256Hex('abc'), 'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad');
});

test('provenance pins revision, seed keys, seam files and duel fixtures', () => {
  const prov = buildProvenance({ battles: 7 });
  assert.ok(prov.gitRevision === null || /^[0-9a-f]{40}$/.test(prov.gitRevision), prov.gitRevision);
  assert.equal(typeof prov.gitDirty, 'boolean');
  assert.match(prov.nodeVersion, /^v\d+/);
  assert.deepEqual(prov.seedKeys, {
    scheme: 'seed = 1..battles per cell (identical across cells)', battles: 7,
  });
  for (const rel of PROVENANCE_FILES) assert.match(prov.files[rel], HEX64, rel);
  for (const key of ['creatures', 'matrix5', 'teams']) assert.match(prov.fixtures[key], HEX64, key);
  // Fixture hashes are canonical and stable across calls.
  assert.deepEqual(fixtureHashes(), fixtureHashes());
  assert.notEqual(prov.fixtures.creatures, prov.fixtures.teams);
});

test('--json pair-sweep artifact is versioned, complete and self-consistent', () => {
  const report = buildJsonReport(parseArgs(['--json', '--pair-sweep', '--battles', '2', '--rounds', '30']));
  assert.equal(report.schemaVersion, DUEL_SPIKE_SCHEMA_VERSION);
  assert.equal(report.mode, 'pair-sweep');
  assert.equal(report.config.matrixId, '5');
  assert.equal(report.config.teamSize, 3);
  assert.equal(report.config.scoring, 'bonus');
  assert.equal(report.config.maxRounds, 30);
  assert.equal(report.config.battles, 2);
  assert.equal(report.pairs.length, TEAM_PAIRS.length);
  assert.ok(report.flagged.length <= report.pairs.length);
  assert.ok(report.config.policies.includes(TIER_POLICY), 'tier in artifact config');
  let sawTier = false;
  for (const pair of report.pairs) {
    assert.equal(pair.wins + pair.losses + pair.draws + pair.timeouts, pair.total);
    assert.equal(pair.total, 2 * POLICIES.length * READ_RATES.length);
    assert.equal(pair.cells.length, POLICIES.length * READ_RATES.length);
    for (const cell of pair.cells) {
      assert.ok(POLICIES.includes(cell.policy), cell.policy);
      if (cell.policy === TIER_POLICY) sawTier = true;
      assert.ok(READ_RATES.includes(cell.rate), cell.rate);
      assert.deepEqual(cell.seed, { from: 1, to: 2, count: 2 });
      assert.equal(cell.wins + cell.losses + cell.draws + cell.timeouts, cell.battles);
      assert.equal(cell.switchRate, cell.rounds ? cell.switches / cell.rounds : 0);
      assert.equal(cell.attacks, Object.values(cell.typeUse).reduce((sum, n) => sum + n, 0));
      assert.equal(cell.speciesSamples, Object.values(cell.speciesUse).reduce((sum, n) => sum + n, 0));
      assert.ok(cell.medianRounds <= cell.p95Rounds);
    }
  }
  assert.ok(sawTier, 'tier cells present in the JSON artifact');
  // Provenance is present and pinned in the artifact, not added later.
  assert.ok(report.provenance && report.provenance.fixtures);
  assert.equal(report.provenance.seedKeys.battles, 2);
});

test('--json grid artifact emits one persona/policy/rate cell per combination', () => {
  const report = buildJsonReport(parseArgs(['--json', '--battles', '2', '--rounds', '30']));
  assert.equal(report.mode, 'grid');
  assert.equal(report.schemaVersion, DUEL_SPIKE_SCHEMA_VERSION);
  assert.ok(Array.isArray(report.cells) && report.cells.length > 0);
  const first = report.cells[0];
  assert.ok(['reader', 'frequency', 'anti-repeat', 'exploiter'].includes(first.persona));
  assert.ok(POLICIES.includes(first.policy));
  assert.ok(READ_RATES.includes(first.rate));
  assert.equal(first.wins + first.losses + first.draws + first.timeouts, first.battles);
});

test('--json output is deterministic run to run (byte-identical)', () => {
  const a = JSON.stringify(buildJsonReport(parseArgs(['--json', '--pair-sweep', '--battles', '2', '--rounds', '30'])));
  const b = JSON.stringify(buildJsonReport(parseArgs(['--json', '--pair-sweep', '--battles', '2', '--rounds', '30'])));
  assert.equal(a, b);
});

test('--json CLI prints byte-identical JSON for a fixed seed/scenario', () => {
  const run = () => execFileSync(process.execPath, [
    TOOL, '--json', '--pair-sweep', '--battles', '2', '--rounds', '30',
  ], { cwd: REPO, encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 });
  const first = run();
  const second = run();
  assert.equal(first, second, 'fixed scenario must replay byte-identically');
  const parsed = JSON.parse(first);
  assert.equal(parsed.schemaVersion, DUEL_SPIKE_SCHEMA_VERSION);
  assert.ok(parsed.provenance.files['mock/battle-mock.js'], 'provenance present on stdout');
  // Text stays the default when --json is absent.
  const text = execFileSync(process.execPath, [
    TOOL, '--pair-sweep', '--battles', '2', '--rounds', '30',
  ], { cwd: REPO, encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 });
  assert.match(text, /^duel pair sweep/);
  assert.doesNotMatch(text, /"schemaVersion"/);
});

