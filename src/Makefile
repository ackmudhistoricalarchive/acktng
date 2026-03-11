CC      = gcc
PROF    = 

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

C_FLAGS = -O -g2 -Wall -DACK_43 $(PROF) $(SOLARIS_FLAG)
L_FLAGS = -O -g2 -lcrypt $(PROF) $(SOLARIS_LINK)

O_FILES = act_clan.o act_comm.o act_info.o act_mob.o act_move.o act_obj.o \
	  act_wiz.o ai.o areachk.o areasave.o board.o build.o buildare.o \
	  buildtab.o cloak.o clutch.o comm.o const.o damage.o db.o death.o \
	  email.o enchant.o fight.o handler.o hash.o hunt.o interp.o invasion.o \
	  item_generation.o item_generation_tables.o keep.o lists.o macros.o magic.o magic2.o magic3.o \
	  magic4.o mapper.o mob_commands.o mob_prog.o obj_fun.o \
	  pdelete.o proposition.o quest.o reincarnate.o rulers.o save.o \
	  scheck.o skills.o skills_chi.o skills_combo.o skills_obj.o \
	  skills_poison.o social-edit.o special.o spell_dam.o spendqp.o ssm.o \
	  stance.o strfuns.o sysdata.o tables.o trigger.o update.o vampyre.o \
	  werewolf.o wizutil.o write.o
O_FILES = ai.o act_clan.o act_comm.o act_info.o act_mob.o act_move.o act_obj.o act_wiz.o \
	  areachk.o areasave.o board.o build.o buildare.o buildtab.o clutch.o comm.o \
	  const.o damage.o cloak.o death.o db.o email.o enchant.o fight.o handler.o hash.o hunt.o \
	  interp.o invasion.o item_generation.o item_generation_tables.o keep.o lists.o macros.o magic.o magic2.o magic3.o magic4.o mapper.o \
	  mob_commands.o mob_prog.o obj_fun.o pdelete.o proposition.o quest.o reincarnate.o rulers.o \
	  save.o scheck.o skills.o skills_chi.o skills_combo.o skills_obj.o skills_poison.o social-edit.o special.o spell_dam.o spendqp.o ssm.o \
	  stance.o strfuns.o sysdata.o tables.o trigger.o update.o vampyre.o werewolf.o wizutil.o \
	  write.o

.PHONY: all clean purge unit-tests integration-test web

all:
	$(MAKE) clean
	$(MAKE) web
	$(MAKE) ack
	$(MAKE) unit-tests

integration-test: ack
	cd .. && ./integration-test.sh

web:
	$(MAKE) -C ../web

ifdef CYGWIN
ack: $(O_FILES)
	rm -f ack.exe
	$(CC) -o ack.exe $(O_FILES) $(L_FLAGS)
	chmod g+w ack.exe
	chmod a+x ack.exe
	chmod g+w $(O_FILES)
else
ack: $(O_FILES)
	rm -f ack
	$(CC) -o ack $(O_FILES) $(L_FLAGS)
	chmod g+w ack
	chmod a+x ack
	chmod g+w $(O_FILES)
endif

clean:
	rm -f *.o ack ack.exe ../area/core *~
	$(MAKE) -C ../web clean

purge:
	rm -f *.o ack ack.exe ../area/core *~
	$(MAKE) -C ../web clean

%.o: %.c ack.h
	$(CC) -c $(C_FLAGS) $<

scan: scan.o
	rm -f scan
	$(CC) $(L_FLAGS) -o scan scan.o


unit-test-sha256: test_sha256.o sha256.o test_is_fighting.o
	rm -f unit-test-sha256
	$(CC) -o unit-test-sha256 test_sha256.o sha256.o test_is_fighting.o $(L_FLAGS)

update.unit-test.o: update.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_UPDATE -o update.unit-test.o update.c

unit-test-update: test_update.o update.unit-test.o test_is_fighting.o
	rm -f unit-test-update
	$(CC) -o unit-test-update test_update.o update.unit-test.o test_is_fighting.o $(L_FLAGS)


comm.unit-test.o: comm.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_COMM -o comm.unit-test.o comm.c

unit-test-comm: test_comm.o comm.unit-test.o test_is_fighting.o
	rm -f unit-test-comm
	$(CC) -o unit-test-comm test_comm.o comm.unit-test.o test_is_fighting.o $(L_FLAGS)

unit-test-websocket-validation: test_websocket_validation.o comm.unit-test.o test_is_fighting.o
	rm -f unit-test-websocket-validation
	$(CC) -o unit-test-websocket-validation test_websocket_validation.o comm.unit-test.o test_is_fighting.o $(L_FLAGS)

fight.unit-test.o: fight.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o fight.unit-test.o fight.c

unit-test-fight: test_fight.o fight.unit-test.o cloak.unit-test.o test_is_fighting.o
	rm -f unit-test-fight
	$(CC) -Wl,--gc-sections -o unit-test-fight test_fight.o fight.unit-test.o cloak.unit-test.o test_is_fighting.o $(L_FLAGS)

act_info.unit-test.o: act_info.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o act_info.unit-test.o act_info.c

unit-test-act-info: test_act_info.o act_info.unit-test.o test_is_fighting.o
	rm -f unit-test-act-info
	$(CC) -Wl,--gc-sections -o unit-test-act-info test_act_info.o act_info.unit-test.o test_is_fighting.o $(L_FLAGS)


act_move.unit-test.o: act_move.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o act_move.unit-test.o act_move.c

unit-test-act-move: test_act_move.o act_move.unit-test.o test_is_fighting.o
	rm -f unit-test-act-move
	$(CC) -Wl,--gc-sections -o unit-test-act-move test_act_move.o act_move.unit-test.o test_is_fighting.o $(L_FLAGS)


cloak.unit-test.o: cloak.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_CLOAK -o cloak.unit-test.o cloak.c

unit-test-cloak: test_cloak.o cloak.unit-test.o test_is_fighting.o
	rm -f unit-test-cloak
	$(CC) -o unit-test-cloak test_cloak.o cloak.unit-test.o test_is_fighting.o $(L_FLAGS)

spendqp.unit-test.o: spendqp.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_SPENDQP -o spendqp.unit-test.o spendqp.c

unit-test-spendqp: test_spendqp.o spendqp.unit-test.o test_is_fighting.o
	rm -f unit-test-spendqp
	$(CC) -o unit-test-spendqp test_spendqp.o spendqp.unit-test.o test_is_fighting.o $(L_FLAGS)

skills_chi.unit-test.o: skills_chi.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o skills_chi.unit-test.o skills_chi.c

unit-test-skills-chi: test_skills_chi.o skills_chi.unit-test.o test_is_fighting.o
	rm -f unit-test-skills-chi
	$(CC) -Wl,--gc-sections -o unit-test-skills-chi test_skills_chi.o skills_chi.unit-test.o test_is_fighting.o $(L_FLAGS)

spell_dam.unit-test.o: spell_dam.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o spell_dam.unit-test.o spell_dam.c

unit-test-spell-dam: test_spell_dam.o spell_dam.unit-test.o test_is_fighting.o
	rm -f unit-test-spell-dam
	$(CC) -Wl,--gc-sections -o unit-test-spell-dam test_spell_dam.o spell_dam.unit-test.o test_is_fighting.o $(L_FLAGS)


pdelete.unit-test.o: pdelete.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o pdelete.unit-test.o pdelete.c

unit-test-pdelete: test_pdelete.o pdelete.unit-test.o test_is_fighting.o
	rm -f unit-test-pdelete
	$(CC) -Wl,--gc-sections -o unit-test-pdelete test_pdelete.o pdelete.unit-test.o test_is_fighting.o $(L_FLAGS)

email.unit-test.o: email.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_EMAIL -o email.unit-test.o email.c

unit-test-email: test_email.o email.unit-test.o test_is_fighting.o
	rm -f unit-test-email
	$(CC) -o unit-test-email test_email.o email.unit-test.o test_is_fighting.o $(L_FLAGS)

rulers.unit-test.o: rulers.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_RULERS -o rulers.unit-test.o rulers.c

unit-test-rulers: test_rulers.o rulers.unit-test.o test_is_fighting.o
	rm -f unit-test-rulers
	$(CC) -o unit-test-rulers test_rulers.o rulers.unit-test.o test_is_fighting.o $(L_FLAGS)


save.unit-test.o: save.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_SAVE -ffunction-sections -fdata-sections -o save.unit-test.o save.c

unit-test-save: test_save.o save.unit-test.o test_is_fighting.o
	rm -f unit-test-save
	$(CC) -Wl,--gc-sections -o unit-test-save test_save.o save.unit-test.o test_is_fighting.o $(L_FLAGS)

skills_obj.unit-test.o: skills_obj.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o skills_obj.unit-test.o skills_obj.c

unit-test-skills-obj: test_skills_obj.o skills_obj.unit-test.o test_is_fighting.o
	rm -f unit-test-skills-obj
	$(CC) -Wl,--gc-sections -o unit-test-skills-obj test_skills_obj.o skills_obj.unit-test.o test_is_fighting.o $(L_FLAGS)

skills_combo.unit-test.o: skills_combo.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o skills_combo.unit-test.o skills_combo.c

unit-test-skills-combo: test_skills_combo.o skills_combo.unit-test.o test_is_fighting.o
	rm -f unit-test-skills-combo
	$(CC) -Wl,--gc-sections -o unit-test-skills-combo test_skills_combo.o skills_combo.unit-test.o test_is_fighting.o $(L_FLAGS)


reincarnate.unit-test.o: reincarnate.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o reincarnate.unit-test.o reincarnate.c

unit-test-reincarnate: test_reincarnate.o reincarnate.unit-test.o test_is_fighting.o
	rm -f unit-test-reincarnate
	$(CC) -Wl,--gc-sections -o unit-test-reincarnate test_reincarnate.o reincarnate.unit-test.o test_is_fighting.o $(L_FLAGS)

mob_commands.unit-test.o: mob_commands.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o mob_commands.unit-test.o mob_commands.c

unit-test-mob-commands: test_mob_commands.o mob_commands.unit-test.o test_is_fighting.o
	rm -f unit-test-mob-commands
	$(CC) -Wl,--gc-sections -o unit-test-mob-commands test_mob_commands.o mob_commands.unit-test.o test_is_fighting.o $(L_FLAGS)




ssm.unit-test.o: ssm.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_SSM -o ssm.unit-test.o ssm.c

unit-test-ssm: test_ssm.o ssm.unit-test.o test_is_fighting.o
	rm -f unit-test-ssm
	$(CC) -o unit-test-ssm test_ssm.o ssm.unit-test.o test_is_fighting.o $(L_FLAGS)

db.unit-test.o: db.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_DB -ffunction-sections -fdata-sections -o db.unit-test.o db.c

unit-test-db: test_db.o db.unit-test.o test_is_fighting.o
	rm -f unit-test-db
	$(CC) -Wl,--gc-sections -o unit-test-db test_db.o db.unit-test.o test_is_fighting.o $(L_FLAGS)

magic.unit-test.o: magic.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_MAGIC -ffunction-sections -fdata-sections -o magic.unit-test.o magic.c

unit-test-magic: test_magic.o magic.unit-test.o test_is_fighting.o
	rm -f unit-test-magic
	$(CC) -Wl,--gc-sections -o unit-test-magic test_magic.o magic.unit-test.o test_is_fighting.o $(L_FLAGS)

magic4.unit-test.o: magic4.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o magic4.unit-test.o magic4.c

unit-test-magic4: test_magic4.o magic4.unit-test.o test_is_fighting.o
	rm -f unit-test-magic4
	$(CC) -Wl,--gc-sections -o unit-test-magic4 test_magic4.o magic4.unit-test.o test_is_fighting.o $(L_FLAGS)

mapper.unit-test.o: mapper.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_MAPPER -ffunction-sections -fdata-sections -o mapper.unit-test.o mapper.c

unit-test-mapper: test_mapper.o mapper.unit-test.o test_is_fighting.o
	rm -f unit-test-mapper
	$(CC) -Wl,--gc-sections -o unit-test-mapper test_mapper.o mapper.unit-test.o test_is_fighting.o $(L_FLAGS)

damage.unit-test.o: damage.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_DAMAGE -ffunction-sections -fdata-sections -o damage.unit-test.o damage.c

unit-test-damage: test_damage.o damage.unit-test.o test_is_fighting.o
	rm -f unit-test-damage
	$(CC) -Wl,--gc-sections -o unit-test-damage test_damage.o damage.unit-test.o test_is_fighting.o $(L_FLAGS)

buildare.unit-test.o: buildare.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_BUILDARE -ffunction-sections -fdata-sections -o buildare.unit-test.o buildare.c

unit-test-buildare: test_buildare.o buildare.unit-test.o test_is_fighting.o
	rm -f unit-test-buildare
	$(CC) -Wl,--gc-sections -o unit-test-buildare test_buildare.o buildare.unit-test.o test_is_fighting.o $(L_FLAGS)


keep.unit-test.o: keep.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_KEEP -ffunction-sections -fdata-sections -o keep.unit-test.o keep.c

unit-test-keep: test_keep.o keep.unit-test.o test_is_fighting.o
	rm -f unit-test-keep
	$(CC) -Wl,--gc-sections -o unit-test-keep test_keep.o keep.unit-test.o test_is_fighting.o $(L_FLAGS)

act_obj.unit-test.o: act_obj.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_ACT_OBJ -ffunction-sections -fdata-sections -o act_obj.unit-test.o act_obj.c

unit-test-act-obj: test_act_obj.o act_obj.unit-test.o test_is_fighting.o
	rm -f unit-test-act-obj
	$(CC) -Wl,--gc-sections -o unit-test-act-obj test_act_obj.o act_obj.unit-test.o test_is_fighting.o $(L_FLAGS)

proposition.unit-test.o: proposition.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o proposition.unit-test.o proposition.c

unit-test-proposition: test_proposition.o proposition.unit-test.o test_is_fighting.o
	rm -f unit-test-proposition
	$(CC) -Wl,--gc-sections -o unit-test-proposition test_proposition.o proposition.unit-test.o test_is_fighting.o $(L_FLAGS)

build.unit-test.o: build.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_BUILD -ffunction-sections -fdata-sections -o build.unit-test.o build.c

unit-test-build: test_build.o build.unit-test.o test_is_fighting.o
	rm -f unit-test-build
	$(CC) -Wl,--gc-sections -o unit-test-build test_build.o build.unit-test.o test_is_fighting.o $(L_FLAGS)


invasion.unit-test.o: invasion.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_INVASION -ffunction-sections -fdata-sections -o invasion.unit-test.o invasion.c

unit-test-invasion: test_invasion.o invasion.unit-test.o test_is_fighting.o
	rm -f unit-test-invasion
	$(CC) -Wl,--gc-sections -o unit-test-invasion test_invasion.o invasion.unit-test.o test_is_fighting.o $(L_FLAGS)

special.unit-test.o: special.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_SPECIAL -ffunction-sections -fdata-sections -o special.unit-test.o special.c

unit-test-special: test_special.o special.unit-test.o test_is_fighting.o
	rm -f unit-test-special
	$(CC) -Wl,--gc-sections -o unit-test-special test_special.o special.unit-test.o test_is_fighting.o $(L_FLAGS)



item_generation.unit-test.o: item_generation.c
	$(CC) -c $(C_FLAGS) -DUNIT_TEST_ITEM_GENERATION -ffunction-sections -fdata-sections -o item_generation.unit-test.o item_generation.c

unit-test-item-generation: test_item_generation.o item_generation.unit-test.o item_generation_tables.o test_is_fighting.o
	rm -f unit-test-item-generation
	$(CC) -Wl,--gc-sections -o unit-test-item-generation test_item_generation.o item_generation.unit-test.o item_generation_tables.o test_is_fighting.o $(L_FLAGS)

unit-test-act-flags: test_act_flags.o
	rm -f unit-test-act-flags
	$(CC) -o unit-test-act-flags test_act_flags.o $(L_FLAGS)

unit-test-area-format: test_area_format.o
	rm -f unit-test-area-format
	$(CC) -o unit-test-area-format test_area_format.o $(L_FLAGS)

unit-test-help-format: test_help_format.o
	rm -f unit-test-help-format
	$(CC) -o unit-test-help-format test_help_format.o $(L_FLAGS)

unit-tests: unit-test-act-flags unit-test-area-format unit-test-help-format unit-test-sha256 unit-test-update unit-test-comm unit-test-websocket-validation unit-test-fight unit-test-act-info unit-test-act-move unit-test-cloak unit-test-spendqp unit-test-spell-dam unit-test-email unit-test-pdelete unit-test-rulers unit-test-save unit-test-skills-obj unit-test-skills-combo unit-test-reincarnate unit-test-mob-commands unit-test-db unit-test-magic unit-test-magic4 unit-test-mapper unit-test-damage unit-test-buildare unit-test-build unit-test-invasion unit-test-proposition unit-test-keep unit-test-act-obj unit-test-ssm unit-test-special unit-test-quest unit-test-death unit-test-item-generation
	./unit-test-act-flags
	./unit-test-area-format
	./unit-test-help-format
	./unit-test-sha256
	./unit-test-update
	./unit-test-comm
	./unit-test-websocket-validation
	./unit-test-fight
	./unit-test-act-info
	./unit-test-act-move
	./unit-test-cloak
	./unit-test-spendqp
	./unit-test-spell-dam
	./unit-test-email
	./unit-test-pdelete
	./unit-test-rulers
	./unit-test-save
	./unit-test-skills-obj
	./unit-test-skills-combo
	./unit-test-reincarnate
	./unit-test-mob-commands
	./unit-test-db
	./unit-test-magic
	./unit-test-magic4
	./unit-test-mapper
	./unit-test-damage
	./unit-test-buildare
	./unit-test-build
	./unit-test-invasion
	./unit-test-proposition
	./unit-test-keep
	./unit-test-act-obj
	./unit-test-ssm
	./unit-test-special
	./unit-test-quest
	./unit-test-death
	./unit-test-item-generation
	$(MAKE) integration-test


quest.unit-test.o: quest.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o quest.unit-test.o quest.c

unit-test-quest: test_quest.o quest.unit-test.o test_is_fighting.o
	rm -f unit-test-quest
	$(CC) -Wl,--gc-sections -o unit-test-quest test_quest.o quest.unit-test.o test_is_fighting.o $(L_FLAGS)

death.unit-test.o: death.c
	$(CC) -c $(C_FLAGS) -ffunction-sections -fdata-sections -o death.unit-test.o death.c

unit-test-death: test_death.o death.unit-test.o test_is_fighting.o
	rm -f unit-test-death
	$(CC) -Wl,--gc-sections -o unit-test-death test_death.o death.unit-test.o test_is_fighting.o $(L_FLAGS)
