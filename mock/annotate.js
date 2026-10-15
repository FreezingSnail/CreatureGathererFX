// Explainer overlay for the duel windows: numbered chips pinned to each region
// (percentage positions so they track the canvas scale control) plus a legend
// table under the canvas. Toggle: the #annotate checkbox. Kinds:
//   data-duel="wheel"  + data-scoring="power" | "flat" | "bonus"
//   data-duel="stances"
(function (root) {
  'use strict';
  if (typeof document === 'undefined') return;
  const body = document.body && document.body.dataset ? document.body.dataset : {};
  if (body.mode !== 'duel') return;
  const scoring = body.scoring === 'flat' ? 'flat' : body.scoring === 'bonus' ? 'bonus' : 'power';
  const kind = body.duel === 'stances' ? 'stances' : 'wheel';

  const chips = kind === 'stances' ? [
    { at: [18, 28], n: 1 }, { at: [82, 28], n: 2 }, { at: [5, 65], n: 3 },
    { at: [50, 8], n: 4 }, { at: [50, 36], n: 5 }, { at: [40, 66], n: 6 },
    { at: [60, 82], n: 7 }, { at: [28, 82], n: 8 }, { at: [48, 86], n: 9 },
    { at: [93, 78], n: 10 },
  ] : [
    { at: [18, 28], n: 1 }, { at: [82, 28], n: 2 }, { at: [5, 65], n: 3 },
    { at: [50, 8], n: 4 }, { at: [50, 36], n: 5 }, { at: [40, 66], n: 6 },
    { at: [56, 66], n: 7 }, { at: [60, 82], n: 8 }, { at: [44, 80], n: 9 },
    { at: [24, 80], n: 10 }, { at: [93, 78], n: 11 },
  ];

  const wheelLegend = [
    ['1', 'FOE sprite — opponent creature (front view).'],
    ['2', 'Your sprite — your creature (back view).'],
    ['3', 'Type badges (both corners) — the creature\'s types; follows the active type mode.'],
    ['4', 'FOE HP — bar, exact numbers, and team pips (three squares: filled = alive, underline = active).'],
    ['5', 'Your HP — numbers, bar and team pips.'],
    ['6', 'Round number (R#).'],
    ['7', scoring === 'power'
      ? 'Selected move: type icon + power.'
      : 'Selected move: type icon only — moves carry no power in this spelling.'],
    ['8', 'Your picks — attack moves plus the swap cell (switch to the next living member); the white box is the cursor. Switching forfeits the round: the other side lands a default win.'],
    ['9', scoring === 'power'
      ? '✱ on a cell = affinity (move type is your creature\'s type), ×1.5.'
      : scoring === 'bonus'
        ? '↑ on a cell = your move type beats the foe creature\'s type: a winning hit does ×1.5.'
        : 'No markers in this spelling — power and affinity are ignored.'],
    ['10', scoring === 'flat'
      ? 'Result — RPS WIN / RPS LOSE (flat 14 / 4), TRADE (6 both), or DEFAULT WIN (14) when the other side swaps.'
      : scoring === 'bonus'
        ? 'Result — BONUS WIN / BONUS LOSE when the ×1.5 fired (21), else RPS WIN / RPS LOSE (14); TRADE = 6 both; DEFAULT WIN = 14/21 when the other side swaps.'
        : 'Result — WHEEL WIN / WHEEL LOSE (damage scales with power), TRADE, or DEFAULT WIN when the other side swaps.'],
    ['11', 'AI READ — appears when the AI countered your previous type at the read rate.'],
  ];

  const stancesLegend = [
    ['1', 'FOE sprite — opponent creature (front view).'],
    ['2', 'Your sprite — your creature (back view).'],
    ['3', 'Type badges (both corners) — the creature\'s types (merged 4-type cycle here).'],
    ['4', 'FOE HP — bar, plus exact numbers in the duel windows.'],
    ['5', 'Your HP — numbers and bar.'],
    ['6', 'Round number (R#).'],
    ['7', 'Stance row — flame &gt; bloom &gt; tide &gt; stone &gt; flame; the white box is the cursor.'],
    ['8', 'AFF — your affinity stance (×1.5) when the pick matches your creature.'],
    ['9', 'Result — CLASH WIN / CLASH LOSE / TIE.'],
    ['10', 'AI READ — appears when the AI countered your previous pick at the read rate.'],
  ];

  const layer = document.querySelector('[data-annotate-layer]');
  const legend = document.querySelector('[data-annotate-legend]');
  const toggle = document.getElementById('annotate');
  if (!layer || !legend) return;

  layer.innerHTML = chips
    .map(({ at, n }) => `<span class="chip" style="left:${at[0]}%;top:${at[1]}%">${n}</span>`)
    .join('');
  const rows = kind === 'stances' ? stancesLegend : wheelLegend;
  legend.innerHTML = '<table>'
    + rows.map(([n, text]) => `<tr><th>${n}</th><td>${text}</td></tr>`).join('')
    + '</table>';

  const apply = (visible) => {
    layer.hidden = !visible;
    legend.hidden = !visible;
  };
  apply(!toggle || toggle.checked);
  if (toggle) toggle.addEventListener('change', () => apply(toggle.checked));
  root.CGFX_ANNOTATE = { chips, rows, apply };
})(typeof window !== 'undefined' ? window : this);
