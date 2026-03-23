CC      = gcc
PROF    =
.DEFAULT_GOAL := all

# Debugging flags possible:  DEBUG_MEM DEBUG_MEM_CORRUPT DEBUG_MEM_DUP_FREE
# DEBUG_MEM is most basic, just checks magic numbers when freeing
# DEBUG_MEM_CORRUPT checks every free block to see if magic numbers ok, every
#                   call to alloc_mem
#		    Also makes calls to void check_free_mem(void) check for
#		    corrupted free memory blocks.
# DEBUG_MEM_DUP_FREE checks to see if freed blocks are overlapping every call
# 		     to mem_alloc.
#

#-DDEBUG_MEM -DDEBUG_MEM_CORRUPT -DDEBUG_MEM_DUP_FREE
#-DDEBUG_MEM -DDEBUG_MEM_CORRUPT

#Uncomment to compile in Cygwin
#CYGWIN = -DCYGWIN

# Uncomment the two lines below if compiling on a Solaris box
#SOLARIS_FLAG = -Dsun -DSYSV -Wno-char-subscripts
#SOLARIS_LINK = -lnsl -lsocket -lresolv

OPENSSL_CFLAGS := $(shell pkg-config --cflags openssl 2>/dev/null)
OPENSSL_LIBS   := $(shell pkg-config --libs   openssl 2>/dev/null)

LIBPQ_CFLAGS   := $(shell pkg-config --cflags libpq 2>/dev/null)
LIBPQ_LIBS     := $(shell pkg-config --libs   libpq 2>/dev/null)
ifeq ($(LIBPQ_LIBS),)
    LIBPQ_HEADER := $(shell find /usr/include -name libpq-fe.h 2>/dev/null | head -1)
    ifneq ($(LIBPQ_HEADER),)
        LIBPQ_LIBS   := -lpq
        LIBPQ_CFLAGS := -I$(dir $(LIBPQ_HEADER))
    endif
endif

C_FLAGS = -O -g2 -Wall -DACK_43 -MMD -MP $(PROF) $(SOLARIS_FLAG) -Iheaders -Iai/headers -Itests/headers -Iquests
L_FLAGS = -O -g2 -lcrypt -lpthread $(PROF) $(SOLARIS_LINK) -lz

ifneq ($(OPENSSL_LIBS),)
    C_FLAGS += -DHAVE_OPENSSL $(OPENSSL_CFLAGS)
    L_FLAGS += $(OPENSSL_LIBS)
endif

ifneq ($(LIBPQ_LIBS),)
    C_FLAGS += -DHAVE_LIBPQ $(LIBPQ_CFLAGS)
    L_FLAGS += $(LIBPQ_LIBS)
endif

OBJDIR = build/obj

include ai/Rules.mk
include tests/Rules.mk

MAIN_OBJS = $(addprefix $(OBJDIR)/,$(O_FILES))

SPELL_OBJS = \
	spells/spell_acid_blast.o spells/spell_acid_breath.o spells/spell_animate.o \
	spells/spell_arcane_lock.o spells/spell_arcaneshield.o spells/spell_arc_lightning.o spells/spell_armor.o \
	spells/spell_astral_projection.o spells/spell_badbreath.o \
	spells/spell_bark_skin.o spells/spell_barrier.o spells/spell_beacon.o \
	spells/spell_black_curse.o spells/spell_black_hand.o spells/spell_bless.o \
	spells/spell_blindness.o spells/spell_bloody_tears.o spells/spell_burning_hands.o \
	spells/spell_cage.o spells/spell_call_lightning.o spells/spell_calm.o \
	spells/spell_cause_critical.o spells/spell_cause_light.o spells/spell_cause_serious.o \
	spells/spell_chain_lightning.o spells/spell_change_sex.o spells/spell_charm_person.o \
	spells/spell_chill_touch.o spells/spell_cloak_absorb.o spells/spell_cloak_adept.o \
	spells/spell_cloak_drain.o spells/spell_cloak_elements.o spells/spell_cloak_flaming.o \
	spells/spell_cloak_iron.o spells/spell_cloak_mental.o spells/spell_cloak_misery.o \
	spells/spell_cloak_oathbreaker.o spells/spell_cloak_precision.o spells/spell_cloak_reflect.o \
	spells/spell_cloak_transcendence.o spells/spell_cloak_valor.o spells/spell_color_spray.o \
	spells/spell_condense_soul.o spells/spell_cone_cold.o spells/spell_consecrate.o \
	spells/spell_continual_light.o spells/spell_control_weather.o \
	spells/spell_corrupt_bond.o spells/spell_creature_bond.o \
	spells/spell_cure_blindness.o spells/spell_cure_critical.o spells/spell_cure_light.o \
	spells/spell_cure_poison.o spells/spell_cure_serious.o spells/spell_curse.o \
	spells/spell_deflect_weapon.o spells/spell_detect_evil.o spells/spell_detect_hidden.o \
	spells/spell_detect_invis.o spells/spell_detect_magic.o spells/spell_detect_poison.o \
	spells/spell_detect_undead.o spells/spell_detection.o spells/spell_diamond_golem.o \
	spells/spell_dimension_blade.o spells/spell_dispel_evil.o spells/spell_dispel_magic.o \
	spells/spell_dispel_object.o spells/spell_divine_intervention.o \
	spells/spell_divine_strike.o \
	spells/spell_earth_shatter.o spells/spell_earthelem.o spells/spell_earthquake.o \
	spells/spell_ego_whip.o spells/spell_elemental_deluge.o spells/spell_elemental_inferno.o \
	spells/spell_exorcism.o \
	spells/spell_elemental_rupture.o spells/spell_elemental_shock.o \
	spells/spell_elemental_ward.o spells/spell_enchant_weapon.o spells/spell_encumber.o spells/spell_energy_drain.o \
	spells/spell_enhance_weapon.o spells/spell_faerie_fire.o spells/spell_faerie_fog.o \
	spells/spell_feeble_body.o spells/spell_feeble_mind.o spells/spell_fighting_trance.o \
	spells/spell_fire_breath.o spells/spell_fireball.o spells/spell_fireblast.o \
	spells/spell_fireelem.o spells/spell_fireshield.o spells/spell_flamestrike.o \
	spells/spell_flare.o spells/spell_fly.o spells/spell_frost_breath.o \
	spells/spell_gas_breath.o spells/spell_gate.o spells/spell_general_purpose.o \
	spells/spell_giant_strength.o spells/spell_group_heal.o spells/spell_harm.o \
	spells/spell_haste.o spells/spell_heal.o spells/spell_healing_light.o \
	spells/spell_heat_armor.o spells/spell_hellspawn.o spells/spell_high_explosive.o \
	spells/spell_holy_armor.o spells/spell_holy_avenger.o spells/spell_holy_light.o \
	spells/spell_holy_word.o spells/spell_holy_wrath.o spells/spell_holyshield.o spells/spell_hypnosis.o spells/spell_ice_bolt.o \
	spells/spell_iceshield.o spells/spell_identify.o spells/spell_influx.o \
	spells/spell_infravision.o spells/spell_infuse.o spells/spell_invis.o \
	spells/spell_iron_golem.o \
	spells/spell_know_alignment.o spells/spell_know_critical.o spells/spell_know_item.o \
	spells/spell_know_weakness.o spells/spell_laserbolt.o spells/spell_lava_burst.o \
	spells/spell_lightning_bolt.o spells/spell_lightning_breath.o \
	spells/spell_locate_object.o spells/spell_magic_missile.o \
	spells/spell_magical_supremacy.o spells/spell_mana_barrier.o \
	spells/spell_mana_drain.o spells/spell_mana_flare.o \
	spells/spell_mass_invis.o spells/spell_mind_bolt.o spells/spell_mind_flail.o \
	spells/spell_mindflame.o spells/spell_mindwall.o \
	spells/spell_mirror_image.o spells/spell_mystic_armor.o spells/spell_mystical_focus.o \
	spells/spell_nerve_fire.o spells/spell_night_vision.o spells/spell_null.o \
	spells/spell_overdrive.o spells/spell_pain.o spells/spell_pass_door.o spells/spell_phase.o \
	spells/spell_phobia.o spells/spell_phoenix_flare.o spells/spell_physic_crush.o \
	spells/spell_physic_thrust.o spells/spell_planergy.o spells/spell_poison.o \
	spells/spell_poison_arsenic.o spells/spell_poison_quinine.o \
	spells/spell_poison_weapon.o spells/spell_portal.o spells/spell_psishield.o \
	spells/spell_prayer.o spells/spell_precognition.o \
	spells/spell_protection.o spells/spell_psionic_recovery.o spells/spell_purify.o \
	spells/spell_refresh.o spells/spell_refresh_mana.o spells/spell_refuge.o \
	spells/spell_regen.o spells/spell_remove_curse.o spells/spell_restoration.o \
	spells/spell_retri_strike.o spells/spell_rictus_curse.o spells/spell_room_dispel.o \
	spells/spell_rune_fire.o spells/spell_rune_poison.o spells/spell_rune_shock.o \
	spells/spell_sanctuary.o spells/spell_seal_room.o \
	spells/spell_solar_flare.o spells/spell_see_magic.o \
	spells/spell_sense_evil.o spells/spell_shadowshield.o spells/spell_shield.o \
	spells/spell_shocking_grasp.o spells/spell_shockshield.o spells/spell_shockstorm.o \
	spells/spell_skeleton.o spells/spell_sleep.o spells/spell_slow.o \
	spells/spell_sonic_blast.o spells/spell_soul_net.o spells/spell_soul_thief.o \
	spells/spell_spirit_curse.o spells/spell_stalker.o spells/spell_static.o \
	spells/spell_stone_skin.o spells/spell_suffocate.o spells/spell_summon.o \
	spells/spell_summon_beast.o spells/spell_summon_devourer.o \
	spells/spell_summon_nightmare.o spells/spell_summon_pegasus.o \
	spells/spell_summon_shadow.o spells/spell_summon_thought_devourer.o \
	spells/spell_teleport.o spells/spell_terra_rend.o spells/spell_thought_vise.o \
	spells/spell_thoughtshield.o spells/spell_thoughtsteal.o spells/spell_throw_needle.o spells/spell_throw_star.o \
	spells/spell_tidal_lash.o spells/spell_travel.o spells/spell_ventriloquate.o \
	spells/spell_turn_undead.o \
	spells/spell_visit.o spells/spell_wall_of_fire.o spells/spell_waterelem.o \
	spells/spell_weaken.o \
	spells/spell_window.o spells/spell_wither_shadow.o spells/spell_word_of_recall.o \
	spells/spell_revenant.o spells/spell_wraith_touch.o \
	spells/spell_elemental_confluence.o spells/spell_convergence_shield.o \
	spells/spell_sacred_ward.o \
	spells/spell_momentum_drain.o spells/spell_entropic_shield.o \
	spells/spell_cognitive_disruption.o spells/spell_predictive_collapse.o \
	spells/spell_druid.o \
	spells/spell_ember_spark.o spells/spell_solar_brand.o \
	spells/spell_cognitive_pressure.o

SKILL_OBJS = \
	skills/do_anti_magic_shell.o skills/do_aurabolt.o skills/do_backstab.o \
	skills/do_bash.o skills/do_beserk.o skills/do_chakra.o skills/do_charge.o \
	skills/do_chiblock.o skills/do_circle.o skills/do_cripple.o skills/do_detox.o \
	skills/do_dirt.o skills/do_disarm.o skills/do_disguise.o skills/do_fleche.o \
	skills/do_flurry.o skills/do_frenzy.o skills/do_headbutt.o skills/do_holystrike.o \
	skills/do_kick.o skills/do_knee.o skills/do_leadership.o skills/do_mindoverbody.o \
	skills/do_morale.o skills/do_palmstrike.o skills/do_phantomfist.o skills/do_pick.o \
	skills/do_poison_arsenic.o skills/do_poison_nightshade.o skills/do_poison_quinine.o \
	skills/do_pummel.o skills/do_punch.o skills/do_rescue.o skills/do_riposte.o \
	skills/do_shieldblock.o skills/do_smash.o skills/do_spinfist.o skills/do_stun.o \
	skills/do_target.o skills/do_trip.o skills/do_warcry.o \
	skills/do_garrote.o skills/do_feign_death.o skills/do_shadow_step.o \
	skills/do_mark_target.o skills/do_conceal.o skills/do_set_trap.o \
	skills/do_read_intent.o \
	skills/do_cleave.o skills/do_fortify.o skills/do_taunt.o \
	skills/do_rend.o skills/do_field_patch.o skills/do_weapon_mastery.o \
	skills/do_ashfall_strike.o \
	skills/do_grapple.o skills/do_choke_hold.o skills/do_leg_sweep.o \
	skills/do_iron_skin.o skills/do_roll_with_blow.o skills/do_pressure_point.o \
	skills/do_feint.o skills/do_conditioning.o \
	skills/do_harbor_dust.o \
	skills/do_chi_surge.o skills/do_breath_of_endurance.o skills/do_fist_of_the_interior_form.o \
	skills/do_momentum_chain.o skills/do_veterans_cadence.o skills/do_overwhelming_assault.o \
	skills/do_oathshield.o skills/do_sanctified_strike.o \
	skills/do_shadow_reading.o skills/do_hex_ward.o skills/do_reflex_disruption.o \
	skills/do_patrol_sense.o skills/do_field_mend.o skills/do_rally_cry.o \
	skills/do_tactical_assess.o skills/do_command_presence.o \
	skills/do_dockside_audit.o skills/do_haggle.o skills/do_bonded_cargo.o \
	skills/do_anchor_slam.o \
	skills/do_forage.o skills/do_field_repair.o skills/do_ember_signal.o \
	skills/do_containment_ward.o skills/do_controlled_burn.o \
	skills/do_oath_mark.o skills/do_track_quarry.o skills/do_writ_of_hold.o \
	skills/do_claws_pursuit.o skills/do_claws_judgment.o \
	skills/do_dig_in.o skills/do_shelter.o skills/do_trailblaze.o \
	skills/do_signal_tower.o skills/do_rampart_slam.o \
	skills/do_wayfind.o skills/do_road_camp.o skills/do_forced_march.o \
	skills/do_dispatch_rider.o skills/do_waylay.o \
	skills/do_duty_strike.o skills/do_gap_probe.o \
	skills/do_primary_motion.o skills/do_vigil_strike.o

QUEST_OBJS = quests/template.o quests/state.o quests/cartography.o quests/notify.o quests/commands.o quests/crusade.o

O_FILES = affect.o ai.o act_clan.o act_comm.o act_info.o act_mob.o act_move.o act_obj.o act_pub_society.o act_wiz.o npc_dialogue.o db/db_conn.o db/db_worker.o db/db_load.o \
	  areachk.o board.o build.o buildare.o buildtab.o caravan.o clutch.o comm.o hotreboot.o login.o socket.o prompt.o \
	  const.o const_color.o const_class.o const_race.o const_clan.o const_exp.o const_liq.o const_pub_society.o \
	  damage.o cloak.o death.o db.o email.o enchant.o fight.o handler.o hash.o hunt.o \
	  heal.o interp.o invasion.o item_generation.o item_generation_tables.o keep.o lists.o lookup.o macros.o magic.o mapper.o \
	  obj_fun.o pdelete.o $(QUEST_OBJS) reincarnate.o revenant.o \
	  save/save.o save/save_players.o save/save_objects.o save/save_mobs.o save/save_areas.o \
	  save/save_area_files.o save/save_rulers.o save/save_socials.o save/save_sysdata.o save/save_notes.o \
	  scheck.o skills.o skills_chi.o skills_combo.o skills_obj.o skills_poison.o special.o spell_dam.o spendqp.o ssm.o \
	  skills_sentinel.o stance.o strfuns.o tables.o trigger.o update.o visibility.o weapon_bond.o wizutil.o \
	  write.o $(AI_SPEC_SRCS) $(SPELL_OBJS) $(SKILL_OBJS)

# Source files for linting (all .c and .h excluding third-party sha256 and build artifacts)
LINT_SOURCES = $(shell find . -name '*.c' -o -name '*.h' | grep -v sha256 | grep -v './build/' | sort)

.PHONY: all clean purge unit-tests integration-tests integration-test integration-test-telnet integration-test-telnet-tls integration-test-wss lint format

all:
	$(MAKE) clean
	$(MAKE) format
	$(MAKE) lint
	$(MAKE) ack
	$(MAKE) unit-tests

integration-tests: ack
	cd .. && ./integration-tests.sh

# WebSocket login flow (ws://, production port 18890)
integration-test: ack
	cd .. && ./integration-test.sh

# Plain telnet login flow (production port 8890 — always plain, never TLS)
integration-test-telnet: ack
	cd .. && ./integration-test-telnet.sh

# TLS telnet login flow (production port 9890 — Mudlet "connect securely")
integration-test-telnet-tls: ack
	cd .. && ./integration-test-telnet-tls.sh

# WSS login flow (production port 9891 — direct TLS WebSocket from browsers)
integration-test-wss: ack
	cd .. && ./integration-test-wss.sh


ifdef CYGWIN
ack: $(MAIN_OBJS)
	rm -f ack.exe
	$(CC) -o ack.exe $(MAIN_OBJS) $(L_FLAGS)
	chmod g+w ack.exe
	chmod a+x ack.exe
	chmod g+w $(MAIN_OBJS)
else
ack: $(MAIN_OBJS)
	rm -f ack
	$(CC) -o ack $(MAIN_OBJS) $(L_FLAGS)
	chmod g+w ack
	chmod a+x ack
	chmod g+w $(MAIN_OBJS)
endif

clean:
	rm -rf $(OBJDIR) tests/unit-test-* ack ack.exe ../area/core *~

purge:
	rm -rf $(OBJDIR) tests/unit-test-* ack ack.exe ../area/core *~

$(OBJDIR)/%.o: %.c headers/ack.h
	@mkdir -p $(dir $@)
	$(CC) -c $(C_FLAGS) -o $@ $<

scan: $(OBJDIR)/scan.o
	rm -f scan
	$(CC) $(L_FLAGS) -o scan $(OBJDIR)/scan.o

# Migration / DB tools (require HAVE_LIBPQ)
tools/import_to_db: tools/import_to_db.c
	$(CC) -O -g2 -Wall $(LIBPQ_CFLAGS) -o tools/import_to_db tools/import_to_db.c $(LIBPQ_LIBS)

tools/db_to_files: tools/db_to_files.c
	$(CC) -O -g2 -Wall $(LIBPQ_CFLAGS) -o tools/db_to_files tools/db_to_files.c $(LIBPQ_LIBS)

tools/migrate_chests: tools/migrate_chests.c
	$(CC) -O -g2 -Wall $(LIBPQ_CFLAGS) -o tools/migrate_chests tools/migrate_chests.c $(LIBPQ_LIBS)

# Check that all source files match the .clang-format style (fails if any differ)
lint:
	clang-format --dry-run --Werror $(LINT_SOURCES)

# Auto-apply the .clang-format style to all source files
format:
	clang-format -i $(LINT_SOURCES)

# Auto-generated header dependency files (produced by -MMD -MP during compilation)
-include $(shell find $(OBJDIR) -name '*.d' 2>/dev/null)
