# yaml/areas/update/

Patches to existing areas in the database. Each area is a subdirectory named
after its keyword. Only include the section files you want to change:

```
midgaard/
  area.yaml      # (optional) fields to change on the area header
  rooms.yaml     # (optional) list of { vnum, ...changed fields }
  mobs.yaml      # (optional) list of { vnum, ...changed fields }
  objects.yaml   # (optional) list of { vnum, ...changed fields }
  resets.yaml    # (optional) full replacement — all resets for the area
  shops.yaml     # (optional) list of { keeper_vnum, ...changed fields }
```

Sub-records (exits, extra_descs, loot, affects, scripts) are fully replaced
when present in the patch — omit them to leave them unchanged.

Applied by:
    python3 tools/yaml_apply.py yaml/areas/update/midgaard/

The folder is deleted after successful application.
