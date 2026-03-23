# yaml/areas/import/

New areas to INSERT into the database. Each area is a subdirectory named after
its keyword. All six section files should be present (missing files are treated
as empty sections):

```
whispering_forest_preserve/
  area.yaml      # required — area header fields (no id field)
  rooms.yaml     # list of room records
  mobs.yaml      # list of mobile records
  objects.yaml   # list of object records
  resets.yaml    # list of reset records
  shops.yaml     # list of shop records
```

Applied by:
    python3 tools/yaml_apply.py yaml/areas/import/whispering_forest_preserve/

The folder is deleted after successful application.
