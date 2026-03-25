# Proposal: Single Class at Character Creation

## Problem

Character creation currently asks new players to choose all four mortal classes upfront, in priority order. This is overwhelming for new players who don't yet understand the class system, and it locks in decisions the player isn't ready to make. The class system should be introduced gradually: start with one class, and let the player pick up to three more through gameplay via `gain`.

Additionally, the legacy character migration (`migrate_legacy_class_levels`) currently places non-prime mortal classes in ascending class-index order, which does not preserve the player's actual progression priorities. It should order them by level descending so the most-leveled classes occupy the lower-numbered (higher-priority) slots.

## Approach

### 1. Migration fix: descending level order

In `save/save_players.c`, `migrate_legacy_class_levels()` currently iterates mortal classes in index order after placing the prime. Change it to collect all non-prime mortal classes that have a level recorded, sort them by level descending, then fill slots 1-3 in that order.

This ensures a character who had, for example, 80 levels of Warrior (prime), 60 of Cleric, 20 of Mage, and 5 of Psi ends up with:
- slot 0: Warrior (prime)
- slot 1: Cleric (60)
- slot 2: Mage (20)
- slot 3: Psi (5)

### 2. Login: ask for one class

Change `show_cmenu_to()` and the `CON_GET_NEW_CLASS` handler in `login.c`:

- Update the menu text to ask for a single starting class.
- Parse only one class name from input (not four).
- Set `mortal_class[0]` to the chosen class, `mortal_level[0] = 1`; leave slots 1-3 as -1/0.
- Remove the "4 classes in order" requirement and error messages.

New prompt text (approximate):

```
Character Creation: Starting Class.

Choose your starting class. You may gain up to three more in game
by visiting a trainer and using the 'gain' command.

Abr    Atr    Name
---    ---    ----------
... (class table) ...

Starting class:
```

### 3. `do_gain`: allow gaining a new mortal class

Currently `do_gain` in `act_info.c` only allows leveling in a class the character already has (`char_has_mortal_class`). Add a branch: if the requested class is a mortal class the character does NOT yet have, and they have fewer than 4 mortal slots filled, allow them to gain it. The cost is the first-level exp cost (`exp_to_level(ch, c)` when the class level is 0).

The new slot is appended to the first free `mortal_class[]` slot (the first index where `mortal_class[i] == -1`). The character's `mortal_level[]` for that slot starts at 1 after the first gain.

No additional level requirement is imposed for gaining a second, third, or fourth mortal class beyond having enough exp. This keeps the system simple and lets players progress at their own pace.

### 4. Help entries

Update the following help entries in the database:

- **`gain`**: reflect that new mortal classes can be gained in game (not just leveled up), and that you start with one and can add up to three more.
- **`class`** (or `classes`): remove language about choosing four at creation; say you start with one and earn more.
- **Character creation help** (if any): update to describe the single-class entry point.

No change to remort or adept gain logic.

## Affected files

- `src/save/save_players.c`: `migrate_legacy_class_levels()` sort order
- `src/login.c`: `show_cmenu_to()`, `CON_GET_NEW_CLASS` handler
- `src/act_info.c`: `do_gain()` new-mortal-class branch
- Database: help entries for `gain`, `class`/`classes`, and creation help

## Trade-offs

- Characters migrated from the old save format will retain all their classes, just reordered by level rather than index. The prime class is always slot 0. No data is lost.
- New characters start with only 1 mortal class, which means they have access to fewer skills and spells initially. This is intentional: the learning curve is gentler.
- Players who want a specific class combination still get it, just over time rather than at character creation.
- The exp cost for gaining a new mortal class is the same first-level cost as leveling in it would be (`exp_to_level` with current level 0). This is consistent with how existing classes are costed and requires no new constants.
- `do_worth` and `do_score` already handle partial mortal class slots gracefully (they only display filled slots), so no changes needed there.

## Save revision

No save revision bump required. The migration path (revision <= 15) is already handled. The new data written for revision 16 characters is unchanged (the format supports 1-4 mortal classes). The only change is that newly created characters will have only 1 slot filled instead of 4.
