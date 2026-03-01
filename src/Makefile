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

O_FILES = ai.o act_clan.o act_comm.o act_info.o act_mob.o act_move.o act_obj.o act_wiz.o \
	  areachk.o areasave.o board.o build.o buildare.o buildtab.o clutch.o comm.o \
	  const.o damage.o death.o db.o email.o enchant.o fight.o handler.o hash.o hunt.o \
	  interp.o invasion.o item_generation.o keep.o lists.o macros.o magic.o magic2.o magic3.o magic4.o mapper.o \
	  mob_commands.o mob_prog.o mount.o obj_fun.o pdelete.o proposition.o quest.o reincarnate.o rulers.o \
	  save.o scheck.o skills.o skills_chi.o skills_combo.o skills_obj.o skills_poison.o social-edit.o special.o spell_dam.o spendqp.o ssm.o \
	  stance.o strfuns.o sysdata.o tables.o trigger.o update.o vampyre.o werewolf.o wizutil.o \
	  write.o

all:
	$(MAKE) ack

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

purge:
	rm -f *.o ack ack.exe ../area/core *~

%.o: %.c ack.h
	$(CC) -c $(C_FLAGS) $<

scan: scan.o
	rm -f scan
	$(CC) $(L_FLAGS) -o scan scan.o


unit-test-sha256: test_sha256.o sha256.o
	rm -f unit-test-sha256
	$(CC) -o unit-test-sha256 test_sha256.o sha256.o $(L_FLAGS)

unit-test-save: test_save.o
	rm -f unit-test-save
	$(CC) -o unit-test-save test_save.o $(L_FLAGS)

unit-tests: unit-test-sha256 unit-test-save
	./unit-test-sha256
	./unit-test-save

