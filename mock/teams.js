// Authored sample teams for the type-count demo (5-type table focus, works on
// any table: member types are retyped for the matchup layer, creatures keep
// their own moves and stats). Browser: window.CGFX_TEAMS; CommonJS: exports.
//
// Team comps come from the permutation census
// (node tools/battle-team-permutations.mjs 5):
//   balanced  = water+earth+plant, water+fire+plant  (adv = dis = 0)
//   dominant  = water x2 + earth, earth x2 + water
//   support   = spirit/fire stacks trend negative; shown for contrast
//   trap      = mono-type is mirror-dead (no internal edge)
(function (root) {
  'use strict';
  const teams = [
    {
      id: 'balanced',
      name: 'Balanced trio',
      note: 'water + earth + plant — the perfectly balanced composition (adv = dis = 0).',
      members: [
        { creature: 'squid', type: 'water' },
        { creature: 'SkitterCrab', type: 'earth' },
        { creature: 'hedge', type: 'plant' },
      ],
    },
    {
      id: 'anchors',
      name: 'Double anchor',
      note: 'water x2 + earth — the dominant composition shape (strong vs the field).',
      members: [
        { creature: 'SquableSnail', type: 'water' },
        { creature: 'bigsquid', type: 'water' },
        { creature: 'ScatterCrab', type: 'earth' },
      ],
    },
    {
      id: 'support',
      name: 'Support stack',
      note: 'spirit + fire + water — spirit and fire each beat only one type, so this leans on water.',
      members: [
        { creature: 'bell', type: 'spirit' },
        { creature: 'CoalGrub', type: 'fire' },
        { creature: 'cloud', type: 'water' },
      ],
    },
    {
      id: 'retyped',
      name: 'Retyped showcase',
      note: 'OrbitRelic retyped to water (raw type is spirit/elder) to show type overrides driving the matchup.',
      members: [
        { creature: 'dragon', type: 'spirit' },
        { creature: 'OrbitRelic', type: 'water' },
        { creature: 'rock', type: 'earth' },
      ],
    },
    {
      id: 'trap',
      name: 'Mono trap (earth x3)',
      note: 'mirror-dead: identical mono-type teams have no internal edge, so no positive matchup exists in the mirror.',
      members: [
        { creature: 'SkitterCrab', type: 'earth' },
        { creature: 'rock', type: 'earth' },
        { creature: 'Shieldshoe', type: 'earth' },
      ],
    },
  ];
  if (typeof module !== 'undefined' && module.exports) module.exports = { teams };
  else root.CGFX_TEAMS = { teams };
})(typeof window !== 'undefined' ? window : this);
