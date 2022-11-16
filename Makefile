all:
	@cd src && $(MAKE)  all
	@cd example && $(MAKE)  all
	@cd uvcdemo && $(MAKE)  all

clean:
	@cd uvcdemo && $(MAKE) clean
	@cd example && $(MAKE) clean
	@cd src && $(MAKE) clean
