all: 
	@$(MAKE) -C sector all --no-print-director
	@cc -std=c89 -Wpedantic -Wall -Wextra -Isector/include \
                 sector/src/sector.c src/sh1edit.c -o bin/sh1edit
clean:
	@$(MAKE) -C sector clean --no-print-director
	@rm -f bin/sh1edit

style:
	@clang-format-21 -i -style=file:clang_format src/sh1edit.c

lint:
	@echo Running make on dependancies...
	@$(MAKE) -C sector all --no-print-director
	@echo Testing...
	
	@echo " gcc in C mode: sector/src/sector.c src/sh1edit.c:"
	@gcc -std=c89 -Wpedantic -Wall -Wextra -Isector/include \
         sector/src/sector.c src/sh1edit.c -o bin/sh1edit
	@rm -f bin/sh1edit
	
	@echo " clang in C mode: sector/src/sector.c src/sh1edit.c:"
	@clang -std=c89 -Wpedantic -Wall -Wextra -Isector/include \
         sector/src/sector.c src/sh1edit.c -o bin/sh1edit
	@rm -f bin/sh1edit
	
	@echo " gcc in C++ mode: sector/src/sector.c src/sh1edit.c:"
	@g++ -Wpedantic -Wall -Wextra -Isector/include \
         sector/src/sector.c src/sh1edit.c -o bin/sh1edit
	@rm -f bin/sh1edit
	
	@echo " clang in C++ mode: src/sector.c src/bin2iso.c:"
	@clang++ -x c++ -Wpedantic -Wall -Wextra -Isector/include \
         sector/src/sector.c src/sh1edit.c -o bin/sh1edit
	@rm -f bin/sh1edit

	@echo " cppcheck: "
	@cppcheck --enable=all --suppress=missingIncludeSystem \
	          --inconclusive --check-config --std=c89 \
              src/sh1edit.c
	@$(MAKE) -C sector clean --no-print-director