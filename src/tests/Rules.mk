UNIT_TEST_TARGETS = \
	tests/unit-test-inlines \
	tests/unit-test-handler \
	tests/unit-test-skills \
	tests/unit-test-skills-chi \
	tests/unit-test-act-flags \
	tests/unit-test-sha256 \
	tests/unit-test-update \
	tests/unit-test-comm \
	tests/unit-test-login \
	tests/unit-test-websocket-validation \
	tests/unit-test-websocket-sanitize \
	tests/unit-test-websocket-json-escape \
	tests/unit-test-sniff-is-tls \
	tests/unit-test-fight \
	tests/unit-test-act-info \
	tests/unit-test-act-move \
	tests/unit-test-cloak \
	tests/unit-test-spendqp \
	tests/unit-test-spell-dam \
	tests/unit-test-email \
	tests/unit-test-pdelete \
	tests/unit-test-rulers \
	tests/unit-test-save \
	tests/unit-test-skills-obj \
	tests/unit-test-skills-combo \
	tests/unit-test-reincarnate \
	tests/unit-test-db \
	tests/unit-test-magic \
	tests/unit-test-mapper \
	tests/unit-test-damage \
	tests/unit-test-buildare \
	tests/unit-test-build \
	tests/unit-test-invasion \
	tests/unit-test-quest \
	tests/unit-test-keep \
	tests/unit-test-act-obj \
	tests/unit-test-ssm \
	tests/unit-test-special \
	tests/unit-test-crusade \
	tests/unit-test-death \
	tests/unit-test-item-generation \
	tests/unit-test-interp \
	tests/unit-test-strfuns \
	tests/unit-test-prompt \
	tests/unit-test-revenant \
	tests/unit-test-adept-skills \
	tests/unit-test-skill-renames \
	tests/unit-test-caravan-travel \
	tests/unit-test-weapon-bond \
	tests/unit-test-overgrowth \
	tests/unit-test-act-clan \
	tests/unit-test-clandata \
	tests/unit-test-sentinel \
	tests/unit-test-npc-dialogue-help \
	tests/unit-test-mssp \
	tests/unit-test-gmcp \
	tests/unit-test-mccp \
	tests/unit-test-chan-history \
	tests/unit-test-lua-engine

$(OBJDIR)/tests/%.o: tests/%.c headers/ack.h
	@mkdir -p $(dir $@)
	$(CC) -c $(C_FLAGS) -I. -o $@ $<

tests/unit-test-sha256: $(OBJDIR)/tests/test_sha256.o $(OBJDIR)/sha256.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

$(OBJDIR)/update.unit-test.o: update.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_UPDATE -o $(OBJDIR)/update.unit-test.o update.c

tests/unit-test-update: $(OBJDIR)/tests/test_update.o $(OBJDIR)/update.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)


$(OBJDIR)/comm.unit-test.o: comm.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_COMM -o $(OBJDIR)/comm.unit-test.o comm.c

tests/unit-test-comm: $(OBJDIR)/tests/test_comm.o $(OBJDIR)/comm.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

$(OBJDIR)/login.unit-test.o: login.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_LOGIN -o $(OBJDIR)/login.unit-test.o login.c

tests/unit-test-login: $(OBJDIR)/tests/test_login.o $(OBJDIR)/login.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

tests/unit-test-websocket-validation: $(OBJDIR)/tests/test_websocket_validation.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

tests/unit-test-websocket-sanitize: $(OBJDIR)/tests/test_websocket_sanitize.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

tests/unit-test-websocket-json-escape: $(OBJDIR)/tests/test_websocket_json_escape.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

tests/unit-test-sniff-is-tls: $(OBJDIR)/tests/test_sniff_is_tls.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

$(OBJDIR)/fight.unit-test.o: fight.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/fight.unit-test.o fight.c

tests/unit-test-fight: $(OBJDIR)/tests/test_fight.o $(OBJDIR)/fight.unit-test.o $(OBJDIR)/cloak.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/act_info.unit-test.o: act_info.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/act_info.unit-test.o act_info.c

tests/unit-test-act-info: $(OBJDIR)/tests/test_act_info.o $(OBJDIR)/act_info.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/act_comm.unit-test.o: act_comm.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/act_comm.unit-test.o act_comm.c

tests/unit-test-chan-history: $(OBJDIR)/tests/test_chan_history.o $(OBJDIR)/act_comm.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)


$(OBJDIR)/act_move.unit-test.o: act_move.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/act_move.unit-test.o act_move.c

tests/unit-test-act-move: $(OBJDIR)/tests/test_act_move.o $(OBJDIR)/act_move.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)


$(OBJDIR)/cloak.unit-test.o: cloak.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_CLOAK -o $(OBJDIR)/cloak.unit-test.o cloak.c

tests/unit-test-cloak: $(OBJDIR)/tests/test_cloak.o $(OBJDIR)/cloak.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

$(OBJDIR)/spendqp.unit-test.o: spendqp.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_SPENDQP -o $(OBJDIR)/spendqp.unit-test.o spendqp.c

tests/unit-test-spendqp: $(OBJDIR)/tests/test_spendqp.o $(OBJDIR)/spendqp.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

$(OBJDIR)/skills_chi.unit-test.o: skills_chi.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/skills_chi.unit-test.o skills_chi.c

tests/unit-test-skills-chi: $(OBJDIR)/tests/test_skills_chi.o $(OBJDIR)/skills_chi.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/spell_dam.unit-test.o: spell_dam.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/spell_dam.unit-test.o spell_dam.c

tests/unit-test-spell-dam: $(OBJDIR)/tests/test_spell_dam.o $(OBJDIR)/spell_dam.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)


$(OBJDIR)/pdelete.unit-test.o: pdelete.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/pdelete.unit-test.o pdelete.c

tests/unit-test-pdelete: $(OBJDIR)/tests/test_pdelete.o $(OBJDIR)/pdelete.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/email.unit-test.o: email.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_EMAIL -o $(OBJDIR)/email.unit-test.o email.c

tests/unit-test-email: $(OBJDIR)/tests/test_email.o $(OBJDIR)/email.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

$(OBJDIR)/save/save_rulers.unit-test.o: save/save_rulers.c headers/ack.h
	@mkdir -p $(dir $@)
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_RULERS -o $(OBJDIR)/save/save_rulers.unit-test.o save/save_rulers.c

tests/unit-test-rulers: $(OBJDIR)/tests/test_rulers.o $(OBJDIR)/save/save_rulers.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)


$(OBJDIR)/save/save.unit-test.o: save/save.c headers/ack.h
	@mkdir -p $(dir $@)
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_SAVE -ffunction-sections -fdata-sections -o $@ $<

$(OBJDIR)/save/save_players.unit-test.o: save/save_players.c headers/ack.h
	@mkdir -p $(dir $@)
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_SAVE -ffunction-sections -fdata-sections -o $@ $<

$(OBJDIR)/save/save_objects.unit-test.o: save/save_objects.c headers/ack.h
	@mkdir -p $(dir $@)
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_SAVE -ffunction-sections -fdata-sections -o $@ $<

$(OBJDIR)/save/save_mobs.unit-test.o: save/save_mobs.c headers/ack.h
	@mkdir -p $(dir $@)
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_SAVE -ffunction-sections -fdata-sections -o $@ $<

$(OBJDIR)/save/save_areas.unit-test.o: save/save_areas.c headers/ack.h
	@mkdir -p $(dir $@)
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_SAVE -ffunction-sections -fdata-sections -o $@ $<

tests/unit-test-save: $(OBJDIR)/tests/test_save.o $(OBJDIR)/save/save.unit-test.o $(OBJDIR)/save/save_players.unit-test.o $(OBJDIR)/save/save_objects.unit-test.o $(OBJDIR)/save/save_mobs.unit-test.o $(OBJDIR)/save/save_areas.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/skills_obj.unit-test.o: skills_obj.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/skills_obj.unit-test.o skills_obj.c

tests/unit-test-skills-obj: $(OBJDIR)/tests/test_skills_obj.o $(OBJDIR)/skills_obj.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/skills_combo.unit-test.o: skills_combo.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/skills_combo.unit-test.o skills_combo.c

tests/unit-test-skills-combo: $(OBJDIR)/tests/test_skills_combo.o $(OBJDIR)/skills_combo.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)


$(OBJDIR)/reincarnate.unit-test.o: reincarnate.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/reincarnate.unit-test.o reincarnate.c

tests/unit-test-reincarnate: $(OBJDIR)/tests/test_reincarnate.o $(OBJDIR)/reincarnate.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/ssm.unit-test.o: ssm.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_SSM -o $(OBJDIR)/ssm.unit-test.o ssm.c

tests/unit-test-ssm: $(OBJDIR)/tests/test_ssm.o $(OBJDIR)/ssm.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

$(OBJDIR)/db.unit-test.o: db.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_DB -ffunction-sections -fdata-sections -o $(OBJDIR)/db.unit-test.o db.c

tests/unit-test-db: $(OBJDIR)/tests/test_db.o $(OBJDIR)/db.unit-test.o $(OBJDIR)/strfuns.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/magic.unit-test.o: magic.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_MAGIC -DUNIT_TEST_MAGIC2 -ffunction-sections -fdata-sections -o $(OBJDIR)/magic.unit-test.o magic.c

$(OBJDIR)/heal.unit-test.o: heal.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/heal.unit-test.o heal.c

tests/unit-test-magic: $(OBJDIR)/tests/test_magic.o $(OBJDIR)/magic.unit-test.o $(OBJDIR)/heal.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/mapper.unit-test.o: mapper.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_MAPPER -ffunction-sections -fdata-sections -o $(OBJDIR)/mapper.unit-test.o mapper.c

tests/unit-test-mapper: $(OBJDIR)/tests/test_mapper.o $(OBJDIR)/mapper.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/damage.unit-test.o: damage.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_DAMAGE -ffunction-sections -fdata-sections -o $(OBJDIR)/damage.unit-test.o damage.c

tests/unit-test-damage: $(OBJDIR)/tests/test_damage.o $(OBJDIR)/damage.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/buildare.unit-test.o: buildare.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_BUILDARE -ffunction-sections -fdata-sections -o $(OBJDIR)/buildare.unit-test.o buildare.c

tests/unit-test-buildare: $(OBJDIR)/tests/test_buildare.o $(OBJDIR)/buildare.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)


$(OBJDIR)/keep.unit-test.o: keep.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_KEEP -ffunction-sections -fdata-sections -o $(OBJDIR)/keep.unit-test.o keep.c

tests/unit-test-keep: $(OBJDIR)/tests/test_keep.o $(OBJDIR)/keep.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/act_obj.unit-test.o: act_obj.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_ACT_OBJ -ffunction-sections -fdata-sections -o $(OBJDIR)/act_obj.unit-test.o act_obj.c

tests/unit-test-act-obj: $(OBJDIR)/tests/test_act_obj.o $(OBJDIR)/act_obj.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/quests/%.unit-test.o: quests/%.c headers/ack.h
	@mkdir -p $(dir $@)
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_QUEST -ffunction-sections -fdata-sections -o $@ $<

QUEST_UNIT_TEST_OBJS = \
	$(OBJDIR)/quests/template.unit-test.o \
	$(OBJDIR)/quests/state.unit-test.o \
	$(OBJDIR)/quests/cartography.unit-test.o \
	$(OBJDIR)/quests/notify.unit-test.o \
	$(OBJDIR)/quests/commands.unit-test.o \
	$(OBJDIR)/quests/crusade.unit-test.o

tests/unit-test-quest: $(OBJDIR)/tests/test_quest.o $(QUEST_UNIT_TEST_OBJS) $(OBJDIR)/const.o $(OBJDIR)/const_exp.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/build.unit-test.o: build.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_BUILD -ffunction-sections -fdata-sections -o $(OBJDIR)/build.unit-test.o build.c

tests/unit-test-build: $(OBJDIR)/tests/test_build.o $(OBJDIR)/build.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)


$(OBJDIR)/invasion.unit-test.o: invasion.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_INVASION -ffunction-sections -fdata-sections -o $(OBJDIR)/invasion.unit-test.o invasion.c

tests/unit-test-invasion: $(OBJDIR)/tests/test_invasion.o $(OBJDIR)/invasion.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/special.unit-test.o: special.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_SPECIAL -ffunction-sections -fdata-sections -o $(OBJDIR)/special.unit-test.o special.c

tests/unit-test-special: $(OBJDIR)/tests/test_special.o $(OBJDIR)/special.unit-test.o $(AI_SUMMON_OBJS) $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/npc_dialogue.unit-test.o: npc_dialogue.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_NPC_DIALOGUE -ffunction-sections -fdata-sections -o $(OBJDIR)/npc_dialogue.unit-test.o npc_dialogue.c

$(OBJDIR)/tests/test_npc_dialogue_help.o: tests/test_npc_dialogue_help.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -I. -Itests/headers -o $(OBJDIR)/tests/test_npc_dialogue_help.o tests/test_npc_dialogue_help.c

tests/unit-test-npc-dialogue-help: $(OBJDIR)/tests/test_npc_dialogue_help.o $(OBJDIR)/npc_dialogue.unit-test.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)



$(OBJDIR)/item_generation.unit-test.o: item_generation.c headers/ack.h
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_ITEM_GENERATION -ffunction-sections -fdata-sections -o $(OBJDIR)/item_generation.unit-test.o item_generation.c

tests/unit-test-item-generation: $(OBJDIR)/tests/test_item_generation.o $(OBJDIR)/item_generation.unit-test.o $(OBJDIR)/item_generation_tables.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

tests/unit-test-act-flags: $(OBJDIR)/tests/test_act_flags.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)


$(OBJDIR)/interp.unit-test.o: interp.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/interp.unit-test.o interp.c

tests/unit-test-interp: $(OBJDIR)/tests/test_interp.o $(OBJDIR)/interp.unit-test.o
	rm -f $@
	$(CC) -Wl,--gc-sections -Wl,--unresolved-symbols=ignore-all -o $@ $^ $(L_FLAGS)

tests/unit-test-crusade: $(OBJDIR)/tests/test_crusade.o $(OBJDIR)/quests/crusade.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/death.unit-test.o: death.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/death.unit-test.o death.c

tests/unit-test-death: $(OBJDIR)/tests/test_death.o $(OBJDIR)/death.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/strfuns.unit-test.o: strfuns.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/strfuns.unit-test.o strfuns.c

tests/unit-test-strfuns: $(OBJDIR)/tests/test_strfuns.o $(OBJDIR)/strfuns.unit-test.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/prompt.unit-test.o: prompt.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/prompt.unit-test.o prompt.c

tests/unit-test-prompt: $(OBJDIR)/tests/test_prompt.o $(OBJDIR)/prompt.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/const.unit-test.o: const.c headers/ack.h spells/spell_table_data.c skills/skill_table_data.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/const.unit-test.o const.c

$(OBJDIR)/stance.unit-test.o: stance.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/stance.unit-test.o stance.c

tests/unit-test-skill-renames: $(OBJDIR)/tests/test_skill_renames.o $(OBJDIR)/const.unit-test.o $(OBJDIR)/stance.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -Wl,--unresolved-symbols=ignore-all -o $@ $^ $(L_FLAGS)

tests/unit-test-adept-skills: $(OBJDIR)/tests/test_adept_skills.o $(OBJDIR)/const.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -Wl,--unresolved-symbols=ignore-all -o $@ $^ $(L_FLAGS)

$(OBJDIR)/handler.unit-test.o: handler.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/handler.unit-test.o handler.c

tests/unit-test-handler: $(OBJDIR)/tests/test_handler.o $(OBJDIR)/handler.unit-test.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/skills.unit-test.o: skills.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/skills.unit-test.o skills.c

tests/unit-test-skills: $(OBJDIR)/tests/test_skills.o $(OBJDIR)/skills.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/revenant.unit-test.o: revenant.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/revenant.unit-test.o revenant.c

tests/unit-test-revenant: $(OBJDIR)/tests/test_revenant.o $(OBJDIR)/revenant.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

tests/unit-test-caravan-travel: $(OBJDIR)/tests/test_caravan_travel.o $(OBJDIR)/act_move.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

$(OBJDIR)/weapon_bond.unit-test.o: weapon_bond.c headers/ack.h
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $(OBJDIR)/weapon_bond.unit-test.o weapon_bond.c

tests/unit-test-weapon-bond: $(OBJDIR)/tests/test_weapon_bond.o $(OBJDIR)/weapon_bond.unit-test.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

tests/unit-test-overgrowth: $(OBJDIR)/tests/test_overgrowth.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

tests/unit-test-act-clan: $(OBJDIR)/tests/test_act_clan.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

tests/unit-test-clandata: $(OBJDIR)/tests/test_clandata.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

tests/unit-test-sentinel: $(OBJDIR)/tests/test_sentinel.o $(OBJDIR)/tests/test_is_fighting.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

tests/unit-test-inlines: $(OBJDIR)/tests/test_inlines.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

tests/unit-test-mssp: $(OBJDIR)/tests/test_mssp.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

tests/unit-test-gmcp: $(OBJDIR)/tests/test_gmcp.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS)

tests/unit-test-mccp: $(OBJDIR)/tests/test_mccp.o
	rm -f $@
	$(CC) -o $@ $^ $(L_FLAGS) -lz

$(OBJDIR)/lua/lua_engine.unit-test.o: lua/lua_engine.c headers/ack.h
	@mkdir -p $(OBJDIR)/lua
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o $@ lua/lua_engine.c

tests/unit-test-lua-engine: $(OBJDIR)/tests/test_lua_engine.o $(OBJDIR)/lua/lua_engine.unit-test.o
	rm -f $@
	$(CC) -Wl,--gc-sections -o $@ $^ $(L_FLAGS)

unit-tests: $(UNIT_TEST_TARGETS)
	@for t in $(UNIT_TEST_TARGETS); do ./$$t || exit 1; done
	$(MAKE) integration-tests

# Force test object recompilation when the module under test changes.
# Make recompiles the test .o even if no header changed (e.g. new/removed
# function body), ensuring the link step runs and the test is re-executed.
$(OBJDIR)/tests/test_sha256.o:           sha256.c
$(OBJDIR)/tests/test_update.o:           update.c
$(OBJDIR)/tests/test_comm.o:             comm.c
$(OBJDIR)/tests/test_login.o:            login.c
$(OBJDIR)/tests/test_fight.o:            fight.c
$(OBJDIR)/tests/test_act_info.o:         act_info.c
$(OBJDIR)/tests/test_chan_history.o:      act_comm.c
$(OBJDIR)/tests/test_act_move.o:         act_move.c
$(OBJDIR)/tests/test_cloak.o:            cloak.c
$(OBJDIR)/tests/test_spendqp.o:          spendqp.c
$(OBJDIR)/tests/test_skills_chi.o:       skills_chi.c
$(OBJDIR)/tests/test_spell_dam.o:        spell_dam.c
$(OBJDIR)/tests/test_pdelete.o:          pdelete.c
$(OBJDIR)/tests/test_email.o:            email.c
$(OBJDIR)/tests/test_rulers.o:           save/save_rulers.c
$(OBJDIR)/tests/test_save.o:             save/save.c save/save_players.c save/save_objects.c save/save_mobs.c save/save_areas.c
$(OBJDIR)/tests/test_skills_obj.o:       skills_obj.c
$(OBJDIR)/tests/test_skills_combo.o:     skills_combo.c
$(OBJDIR)/tests/test_reincarnate.o:      reincarnate.c
$(OBJDIR)/tests/test_ssm.o:              ssm.c
$(OBJDIR)/tests/test_db.o:               db.c strfuns.c
$(OBJDIR)/tests/test_magic.o:            magic.c heal.c
$(OBJDIR)/tests/test_mapper.o:           mapper.c
$(OBJDIR)/tests/test_damage.o:           damage.c
$(OBJDIR)/tests/test_buildare.o:         buildare.c
$(OBJDIR)/tests/test_keep.o:             keep.c
$(OBJDIR)/tests/test_act_obj.o:          act_obj.c
$(OBJDIR)/tests/test_quest.o:            quests/template.c quests/state.c quests/cartography.c quests/notify.c quests/commands.c quests/crusade.c
$(OBJDIR)/tests/test_build.o:            build.c
$(OBJDIR)/tests/test_invasion.o:         invasion.c
$(OBJDIR)/tests/test_special.o:          special.c
$(OBJDIR)/tests/test_npc_dialogue_help.o: npc_dialogue.c
$(OBJDIR)/tests/test_item_generation.o:  item_generation.c item_generation_tables.c
$(OBJDIR)/tests/test_interp.o:           interp.c
$(OBJDIR)/tests/test_crusade.o:          quests/crusade.c
$(OBJDIR)/tests/test_death.o:            death.c
$(OBJDIR)/tests/test_strfuns.o:          strfuns.c
$(OBJDIR)/tests/test_prompt.o:           prompt.c
$(OBJDIR)/tests/test_skill_renames.o:    const.c stance.c
$(OBJDIR)/tests/test_adept_skills.o:     const.c
$(OBJDIR)/tests/test_handler.o:          handler.c
$(OBJDIR)/tests/test_skills.o:           skills.c
$(OBJDIR)/tests/test_revenant.o:         revenant.c
$(OBJDIR)/tests/test_caravan_travel.o:   act_move.c
$(OBJDIR)/tests/test_weapon_bond.o:      weapon_bond.c
$(OBJDIR)/tests/test_lua_engine.o:       lua/lua_engine.c
