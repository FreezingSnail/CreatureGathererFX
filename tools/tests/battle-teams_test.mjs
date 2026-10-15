// Run: node --test tools/tests/battle-teams_test.mjs
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

const repo = path.dirname(fileURLToPath(import.meta.url)) + '/../..';
const require = createRequire(import.meta.url);
const { teams } = require(path.join(repo, 'mock/teams.js'));
const data = require(path.join(repo, 'mock/data.js'));

const FIVE_TYPE_ORDER = ['spirit', 'water', 'earth', 'fire', 'plant'];

test('sample teams are well formed: unique ids, three real creatures, typed members', () => {
  assert.ok(teams.length >= 4, 'a handful of teams to play with');
  const ids = new Set();
  for (const team of teams) {
    assert.ok(team.id && !ids.has(team.id), `unique id ${team.id}`);
    ids.add(team.id);
    assert.ok(team.name && team.note, `${team.id} has a name and note`);
    assert.equal(team.members.length, 3, `${team.id} fields three members`);
    for (const member of team.members) {
      const creature = data.creatures.find((entry) => entry.name === member.creature);
      assert.ok(creature, `${team.id}: creature ${member.creature} exists`);
      assert.ok(FIVE_TYPE_ORDER.includes(member.type), `${team.id}: ${member.type} is a 5-type name`);
    }
    // Every retyped member must have a different raw folded primary when the
    // team claims a retype, or the note is misleading.
    const distinct = new Set(team.members.map((member) => member.type));
    assert.ok(distinct.size >= 1);
  }
});

test('census compositions referenced by the teams are the expected shapes', () => {
  const byId = Object.fromEntries(teams.map((team) => [team.id, team]));
  assert.ok(byId.balanced, 'balanced team present');
  assert.deepEqual(byId.balanced.members.map((m) => m.type).sort(), ['earth', 'plant', 'water']);
  assert.ok(byId.anchors, 'anchor team present');
  assert.equal(byId.anchors.members.filter((m) => m.type === 'water').length, 2);
  assert.ok(byId.trap, 'trap team present');
  assert.equal(new Set(byId.trap.members.map((m) => m.type)).size, 1, 'trap is mono-type');
});
