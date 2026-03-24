# Proposal: Extract Lua-to-AFFECT_DATA parsing into a shared helper

**Status:** Open
**Date:** 2026-03-24
**Repos affected:** acktng only

---

## Problem

`mud_apply_affect` and `mud_affect_join` in `src/lua/lua_api.c` both parse a Lua table into an `AFFECT_DATA` struct using identical six-field extraction sequences (type, duration, location, modifier, bitvector, duration_type). The two blocks are byte-for-byte duplicates (~18 lines each). Any future field added to `AFFECT_DATA` that Lua scripts need to set must be added in two places, making it easy to silently diverge.

## Approach

Extract the duplicated logic into a file-scoped static helper:

```c
static void lua_to_affect(lua_State *L, int index, AFFECT_DATA *af)
{
   memset(af, 0, sizeof(*af));
   luaL_checktype(L, index, LUA_TTABLE);

   lua_getfield(L, index, "type");
   af->type = (short)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);

   lua_getfield(L, index, "duration");
   af->duration = (short)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);

   lua_getfield(L, index, "location");
   af->location = (short)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);

   lua_getfield(L, index, "modifier");
   af->modifier = (short)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);

   lua_getfield(L, index, "bitvector");
   af->bitvector = (int)luaL_optinteger(L, -1, 0);
   lua_pop(L, 1);

   lua_getfield(L, index, "duration_type");
   af->duration_type = (short)luaL_optinteger(L, -1, DURATION_HOUR);
   lua_pop(L, 1);
}
```

`mud_apply_affect` and `mud_affect_join` each reduce to:

```c
AFFECT_DATA af;
lua_to_affect(L, 2, &af);
affect_to_char(victim, &af);   /* or affect_join */
```

The `luaL_checktype` call inside the helper replaces the explicit `luaL_checktype` in each caller, so external behaviour is unchanged.

## Affected files

- `src/lua/lua_api.c` — add `lua_to_affect` static helper, simplify the two callers

## Trade-offs

No behaviour change; this is a pure internal refactor. No new public symbols, no header changes, no schema changes. The only risk is a mistaken edit during the extraction, caught by the existing build and unit-test suite.

## Out of scope

Adding new `AFFECT_DATA` fields to the Lua API is not part of this proposal.
