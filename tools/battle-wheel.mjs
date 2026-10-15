// 8-type matchup web for the simultaneous-pick duel: a directed graph of
// "beats" edges built from overlapping 3-cycles; undecided pairs stay neutral.
// Shared by the data generator and the matchup solver.
//
// Edges were seeded with four readable triangles and completed by
// tools/battle-wheel-solve.mjs so the solver guarantees, for the current
// roster: every pair of move-sets has at least one positive matchup (identical
// sets get an internal edge; identical singleton sets are exempt data issues).
// 25 edges / 56 ordered pairs decisive (45%); 16 triangles; 55% of pairs are
// deliberately neutral.
//
// Seeded triangles:
//   fire  > plant > water > fire
//   earth > lightning > water > earth
//   fire  > wind  > earth > fire
//   plant > spirit > elder > plant
// Completion edges (preferred causal order where coverage allowed):
//   wind > spirit, plant > lightning, earth > spirit, spirit > water,
//   lightning > wind, earth > plant, wind > plant, spirit > lightning,
//   fire > spirit, water > wind, lightning > fire, water > elder, elder > earth
export const WHEEL = {
  order: ['wind', 'earth', 'lightning', 'water', 'fire', 'plant', 'spirit', 'elder'],
  edges: [
    // seeded triangles
    ['fire', 'plant'], ['plant', 'water'], ['water', 'fire'],
    ['earth', 'lightning'], ['lightning', 'water'], ['water', 'earth'],
    ['fire', 'wind'], ['wind', 'earth'], ['earth', 'fire'],
    ['plant', 'spirit'], ['spirit', 'elder'], ['elder', 'plant'],
    // solver completion edges
    ['wind', 'spirit'], ['plant', 'lightning'], ['earth', 'spirit'],
    ['spirit', 'water'], ['lightning', 'wind'], ['earth', 'plant'],
    ['wind', 'plant'], ['spirit', 'lightning'], ['fire', 'spirit'],
    ['water', 'wind'], ['lightning', 'fire'], ['water', 'elder'],
    ['elder', 'earth'],
  ],
};
