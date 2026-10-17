# 10. Party, collection and save system

## Player and party

- Active party: 3 creature slots (`Player.party[3]`) plus live HP per slot (`creatureHPs[3]`); the same HP array is the battle contract.
- Heal party restores occupied slots to derived max and clears transient status; empty slots are skipped.
- Fresh games start from the authored `beta` preset (species 0/3/6 at level 3, kits water/earth/water), seed three stable store instances, and start at the authored tile (11, 5).

## Collection and instance records

The collection is **64 eight-byte stable records** (`StoreRecord`): species id, cumulative exp (u16), folded attack mask, 3 reserved bytes (zero), and an HP byte. HP encoding: 0 means full/derived max, otherwise `HP − 1` — this keeps the legacy "full" case free and encodes damaged instances exactly. Instances are the growth authority: level, max HP, evolution and (planned) kit all derive from the record.

## Persistence architecture

| Layer | Design |
|---|---|
| Snapshot | `SaveFile` v3, 96 bytes: location, 8 flag bytes, party slots + HP, plant stages/pairs/ticker, 24 lure counts, 8 consumable counts, 4 key-item bytes |
| Journal | 16-byte append records (StoreAdd / StoreRemove / PartyAssign); 256 records per sector; torn-tail detection via a final marker byte written last, so an interrupted append is un-overwritable and treated as end-of-log |
| Compaction | Alternating A/B checkpoints with header magic, generation counter, dual checksums and commit bytes; validate, erase the journal, verify blank |
| Sectors | 8 sectors × 4,096 B: main snapshot 0, journal 4096, store A 8192, store B 12288, four reserved sectors |
| Cadence | Explicit checkpoints (no autosave yet). Collection writes journal immediately; an explicit SAVE from the world menu is planned (`jp8.11.23`) |
| Failure handling | A refused write leaves accepted state untouched (lossless refusals); unrecoverable boot conditions halt loudly on the device rather than running with an inconsistent store |

> **Loss window policy.** Until `jp8.11.23` lands, nothing in ordinary play calls a checkpoint: a power loss keeps journaled collection writes but loses snapshot-only state (location, flags, party HP, plants, bag) since the last checkpoint. The beta release gate must pin this policy explicitly.
