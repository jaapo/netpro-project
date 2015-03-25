define mdhtml
cat $1.md | mkd2html -css markdown.css >html/$2-$1.html
endef

htmlfiles=01-desc.html 02-arch.html
mdfiles=head.md desc.md arch.md protocols.md
extras=$(addprefix html/,arch.svg markdown.css)

all: $(addprefix html/,$(htmlfiles))

.PHONY: full
full: $(extras) $(mdfiles)
	cat $(mdfiles) | mkd2html -css markdown.css >html/doc.html

html/01-desc.html: desc.md | html
	$(call mdhtml,desc,01)
html/02-arch.html: arch.md | html
	$(call mdhtml,arch,02)

html/arch.svg: arch.svg | html
	cp arch.svg html/arch.svg

html/markdown.css: markdown.css | html
	cp markdown.css html/markdown.css

html:
	mkdir -p html

.PHONY: clean
clean:
	rm html/*
