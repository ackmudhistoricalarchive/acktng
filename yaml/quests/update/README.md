# yaml/quests/update/

Patches to existing quest templates. Include `id` plus only the fields to change.

Example:
    id: 42
    reward_gold: 750
    max_level: 60

Applied by:
    python3 tools/yaml_apply.py yaml/quests/update/42.yaml

The file is deleted after successful application.
