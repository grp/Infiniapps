PROJECTS = infiniboard infinifolders infinidock

.PHONY: $(PROJECTS)

all: $(PROJECTS)

$(PROJECTS):
	$(MAKE) -C $@

