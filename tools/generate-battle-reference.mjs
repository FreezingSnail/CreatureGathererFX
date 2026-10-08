#!/usr/bin/env node
// Build self-contained battle reference pages from canonical move data.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const moves = JSON.parse(fs.readFileSync(path.join(root, 'data/json/moves.json'), 'utf8'))
  .filter(move => move.name !== 'none').sort((a, b) => a.id - b.id);
const types = ['spirit', 'water', 'wind', 'earth', 'fire', 'lightning', 'plant', 'elder'];
const effectRows = [];
const cap = s => s[0].toUpperCase() + s.slice(1);
const add = (code, name, group, description, duration) => effectRows.push({ code, name, group, description, duration });
for (const [i, type] of types.entries()) {
  const code = ['DPRSD', 'SOAKED', 'BUFTD', 'SOILED', 'SCRCHD', 'ZAPPED', 'TANGLD', 'REDCD'][i];
  add(code, `${cap(type)} weakened`, 'Element weakened', `Weakens damage scaling for active creatures that have the ${type} type.`, 'Until switched out or the battle ends');
}
for (const [i, type] of types.entries()) {
  const code = ['ENLTND', 'DRNCHD', 'AIRSWPT', 'GRNDED', 'KINDLD', 'CHRGD', 'ENRCHD', 'EVOLVD'][i];
  add(code, `${cap(type)} empowered`, 'Element empowered', `Improves damage scaling for active creatures that have the ${type} type.`, 'Until switched out or the battle ends');
}
for (const [code, name] of [['ATKDWN','Attack'],['DEFDWN','Defense'],['SPCADWN','Special attack'],['SPCDDWN','Special defense'],['SPDDWN','Speed']])
  add(code, `${name} lowered`, 'Stat lowered', `Lowers this stat by one stage, reducing its battle contribution.`, 'Stage remains on that party member');
for (const [code, name] of [['ATKUP','Attack'],['DEFUP','Defense'],['SPCAUP','Special attack'],['SPCDUP','Special defense'],['SPDUP','Speed']])
  add(code, `${name} raised`, 'Stat raised', `Raises this stat by one stage, improving its battle contribution.`, 'Stage remains on that party member');
add('SAPPD', 'Sapped', 'End of turn', 'At each end of turn, loses max HP divided by 16 (minimum 1 HP).', 'Three end-of-turn ticks');
add('INFSED', 'Infused', 'End of turn', 'At each end of turn, recovers max HP divided by 8, up to max HP.', 'Three end-of-turn ticks');
add('PINNED', 'Pinned', 'Turn gate', 'At each attempted action, has a 1-in-3 chance to lose that action.', 'Three end-of-turn ticks');
add('CONCUSED', 'Concussed', 'Turn gate', 'At each attempted action, has a 1-in-4 chance to hit itself instead.', 'Three end-of-turn ticks');

const esc = s => String(s).replace(/[&<>"']/g, c => ({ '&':'&amp;', '<':'&lt;', '>':'&gt;', '"':'&quot;', "'":'&#39;' }[c]));
const slug = s => s.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/(^-|-$)/g, '');
const effectByCode = new Map(effectRows.map(e => [e.code, e]));
const style = `:root{color-scheme:dark;--bg:#000;--panel:#111;--ink:#fff;--muted:#bbb;--line:#666}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:16px/1.5 system-ui,sans-serif}main{max-width:1200px;margin:auto;padding:clamp(18px,4vw,48px)}a{color:#fff}.nav{display:flex;gap:18px;flex-wrap:wrap;margin:0 0 28px}.nav a{font-weight:700}h1{font-size:clamp(34px,6vw,60px);line-height:1;margin:10px 0 12px;letter-spacing:-.05em}h2{font-size:22px;margin:30px 0 12px}.lede{color:var(--muted);max-width:850px}.note{border:1px solid var(--line);padding:14px 16px;background:#111;margin:18px 0;color:#ddd}.tools{display:flex;gap:10px;flex-wrap:wrap;margin:22px 0}.tools input,.tools select{background:#111;color:#fff;border:1px solid #777;border-radius:5px;padding:10px 12px;font:inherit}.tools input{flex:1;min-width:220px}.count{color:var(--muted);font:12px ui-monospace,monospace;margin:8px 0}.table-wrap{overflow:auto;border:1px solid #555}table{border-collapse:collapse;width:100%;min-width:850px}th,td{text-align:left;padding:10px 12px;border-bottom:1px solid #444;vertical-align:top}th{position:sticky;top:0;background:#181818;font:700 11px ui-monospace,monospace;text-transform:uppercase;letter-spacing:.08em}tbody tr:hover{background:#171717}.move-name,.code{font-weight:800}.code{font:700 12px ui-monospace,monospace}.muted{color:var(--muted)}.effect-list{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,330px),1fr));gap:10px}.effect{border:1px solid #555;border-radius:6px;background:#111;padding:14px;scroll-margin-top:16px}.effect h3{margin:3px 0 6px;font-size:17px}.effect p{margin:5px 0;color:#ddd}.effect small{color:var(--muted)}.group{margin-top:30px}.group h2{border-bottom:1px solid #666;padding-bottom:8px}.empty{padding:20px;color:var(--muted)}footer{border-top:1px solid #555;margin-top:44px;padding-top:18px;color:#bbb;font-size:13px}@media(max-width:600px){main{padding:18px}.nav{gap:12px}}`;
const nav = `<nav class="nav" aria-label="Battle reference"><a href="battle-roster-balance.html">← Creature roster</a><a href="battle-moves.html">All moves</a><a href="battle-status-effects.html">Status effects</a><a href="world-tile-assets.html">World tile assets</a><a href="battle-simulator.md">Battle simulator notes</a></nav>`;
function shell(title, subtitle, body, script = '') {
  return `<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>${esc(title)} · Creature Gatherer</title><style>${style}</style></head><body><main>${nav}<header><h1>${esc(title)}</h1><p class="lede">${subtitle}</p></header>${body}<footer>Reference for the current battle data and engine rules. <a href="battle-roster-balance.html">Back to the full creature roster</a>.</footer></main>${script ? `<script>${script}</script>` : ''}</body></html>`;
}

const moveRows = moves.map(move => {
  const effect = effectByCode.get(move.effect);
  const utility = move.power === 0;
  const limit = utility && ['ATKUP','DEFUP','SPCAUP','SPCDUP','SPDUP'].includes(move.effect) ? '3' : move.power >= 10 || move.effect !== 'NONE' ? '2' : 'Unlimited';
  const accuracy = move.accuracy === 100 ? '100%' : `${move.accuracy}%`;
  const effectText = effect ? `<a href="battle-status-effects.html#${slug(effect.code)}">${esc(effect.name)}</a>` : '—';
  return `<tr data-type="${esc(move.type)}" data-search="${esc(`${move.name} ${move.type} ${move.effect} ${move.physical ? 'physical' : 'special'}`.toLowerCase())}"><td class="muted">${move.id}</td><td class="move-name">${esc(move.name)}</td><td>${esc(cap(move.type))}</td><td>${utility ? '—' : move.power}</td><td>${utility ? 'Status' : move.physical ? 'Physical' : 'Special'}</td><td>${accuracy}</td><td>${limit}</td><td>${effectText}</td></tr>`;
}).join('');
const movesBody = `<p class="note">${moves.length} defined moves. Power, category, effect, and listed accuracy come from the authored move data. <strong>Accuracy is currently not rolled by the battle engine</strong>; moves resolve without a miss check. No move uses a charge-up turn: its damage and effects resolve in one action.</p><div class="tools"><input id="moveSearch" type="search" placeholder="Search moves…" aria-label="Search moves"><select id="typeFilter" aria-label="Filter moves by type"><option value="all">All types</option>${['spirit','water','wind','earth','fire','lightning','plant','elder','status'].map(t=>`<option value="${t}">${cap(t)}</option>`).join('')}</select></div><p id="moveCount" class="count" aria-live="polite"></p><div class="table-wrap"><table><thead><tr><th>ID</th><th>Move</th><th>Type</th><th>Power</th><th>Category</th><th>Listed accuracy</th><th>Uses</th><th>Effect</th></tr></thead><tbody id="moves">${moveRows}</tbody></table></div><p class="note">Use limits are per move slot in battle: stat-raising/lowering zero-power moves have 3 uses; moves with power 10+ or an effect have 2; other moves are unlimited. See the <a href="battle-status-effects.html">status reference</a> for effect rules.</p>`;
const moveScript = `const search=document.querySelector('#moveSearch'),filter=document.querySelector('#typeFilter'),rows=[...document.querySelectorAll('#moves tr')],count=document.querySelector('#moveCount');function update(){let q=search.value.trim().toLowerCase(),t=filter.value,n=0;for(const r of rows){let show=(t==='all'||r.dataset.type===t)&&r.dataset.search.includes(q);r.hidden=!show;if(show)n++;}count.textContent=n+' of ${moves.length} moves';}search.addEventListener('input',update);filter.addEventListener('change',update);update();`;
const statusGroups = [...new Set(effectRows.map(e => e.group))];
const statusBody = `<p class="note">${effectRows.length} coded effects. Move effects resolve after the move's damage. Element boosts/weakenings last while that creature remains active; stat stages stay with the party member. Timed effects expire after their listed end-of-turn ticks.</p>${statusGroups.map(group => `<section class="group"><h2>${esc(group)}</h2><div class="effect-list">${effectRows.filter(e => e.group === group).map(e => `<article class="effect" id="${slug(e.code)}"><div class="code">${esc(e.code)}</div><h3>${esc(e.name)}</h3><p>${esc(e.description)}</p><small>Duration: ${esc(e.duration)}</small></article>`).join('')}</div></section>`).join('')}<section class="group"><h2>How the values work</h2><article class="effect"><p>Type effects change the damage modifier for a creature that has the associated type. The battle code applies this status modifier to damage dealt and received, alongside the normal matchup and same-type bonuses.</p><p>Stat stages range from −4 to +4. The current multipliers for damage-related stats are 2/6, 2/5, 2/4, 2/3, 1×, 3/2, 4/2, 5/2, and 6/2.</p><p>Tick effects run at end of turn. Duplicate effects cannot occupy both status slots. Switching clears status effects; stat stages are retained with the party member.</p></article></section>`;
const statusScript = `const cards=[...document.querySelectorAll('.effect')];document.querySelector('#statusSearch').addEventListener('input',e=>{let q=e.target.value.trim().toLowerCase();for(const c of cards)c.hidden=!c.textContent.toLowerCase().includes(q);for(const s of document.querySelectorAll('.group'))s.hidden=!s.querySelector('.effect:not([hidden])');});`;
const statusBodyWithSearch = `<div class="tools"><input id="statusSearch" type="search" placeholder="Find an effect…" aria-label="Search status effects"></div>${statusBody}`;
fs.writeFileSync(path.join(root, 'docs/battle-moves.html'), shell('All moves', `Browse every usable move, its authored combat values, use limit, and linked effect. The empty move-slot sentinel is omitted.`, movesBody, moveScript));
fs.writeFileSync(path.join(root, 'docs/battle-status-effects.html'), shell('Status effects', `Look up every effect code used by moves, including elemental modifiers, stat stages, healing, damage over time, and action disruption.`, statusBodyWithSearch, statusScript));
console.log(`PASS: generated ${moves.length} moves and ${effectRows.length} status effect references.`);
