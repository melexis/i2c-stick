.PHONY: clean all dist http serve


DIST_FILES=		   \
	assets/site/favicon.ico             \
	assets/site/chart.min.js            \
	assets/site/jquery-3.6.1.min.js     \
	assets/site/logo_white.png          \
	assets/site/lato_*.woff2            \
	assets/melexis-i2c-stick-overview.webp  \
	google*.html                        \
	index.html                          \
	sitemap.txt                         \
	robots.txt                          \
	melexis-bulma.min.css               \
	interface.min.js

PANDOC_FILES=               \
	products                \
	firmware                \
	i2c-stick

DIST_DIR=../dist


all: ${DIST_FILES} dist

clean:
	@rm -rfv ${DIST_DIR}
	@rm -fv $(patsubst %, %.html, ${PANDOC_FILES})
	@rm -fv index.html interface.min.js melexis-bulma.min.css melexis-bulma.css 

cleaner: clean
	@rm -rf theme/node_modules

dist:
	@mkdir -p ${DIST_DIR}/assets/site
	for file in $(DIST_FILES); do \
		cp -vrf $$file ${DIST_DIR}/$$file; \
	done

interface.min.js: interface.js
	@echo "minifying $< => $@"
	@terser $< --compress --mangle --toplevel --output $@

theme/node_modules: theme/package.json
	@echo "installing the bulma tools"
	cd theme && npm install

melexis-bulma.css: theme/sass/melexis-bulma.scss theme/node_modules
	@echo "generating $< => $@"
	cd theme && npm run css-build

melexis-bulma.min.css: melexis-bulma.css
	@echo "minifying $< => $@"
	@cssnano $< $@

products.html: products.md
	@echo "pandoc $< => $@"
	@pandoc $< -f markdown+lists_without_preceding_blankline+autolink_bare_uris+hard_line_breaks+smart --mathjax -o $@

i2c-stick.html: i2c-stick.md
	@echo "pandoc $< => $@"
	@pandoc $< -f markdown+lists_without_preceding_blankline+autolink_bare_uris+hard_line_breaks+smart --mathjax -o $@

firmware.html: firmware.md firmware_list.md
	@echo "pandoc $^ => $@"	
	@cat $^ | pandoc -f markdown+lists_without_preceding_blankline+autolink_bare_uris+hard_line_breaks+smart --mathjax -o $@

index.html: index.jinja2.html firmware.html i2c-stick.html products.html context.yaml
	@echo "mailmerge $^ => $@"	
	@python mailmerge.py

serve:
	python -m http.server

http:
	python -m http.server
