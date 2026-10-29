cd.PHONY: all clean dist style test gcov_report check valgrind cppcheck install uninstall dvi clear_settings

BUILD_DIR = ../build
PREFIX ?= ../build
INSTALL_DIR = $(PREFIX)/games

all: build

build:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake .. -DBUILD_TESTING=OFF -DBUILD_DVI=OFF -DRelease=ON && make

test:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake .. -DBUILD_TESTING=ON -DBUILD_DVI=OFF -DRelease=OFF && make && ./$(BUILD_DIR)/run_tests

gcov_report: clean
	mkdir -p $(BUILD_DIR) && mkdir -p $(BUILD_DIR)/report && cd $(BUILD_DIR) && cmake .. -DBUILD_TESTING=ON -DBUILD_DVI=OFF -DRelease=OFF -DCMAKE_CXX_FLAGS="--coverage" && make && ./$(BUILD_DIR)/run_tests
	gcovr -r . \
	--html --html-details -o ../build/report/index.html \
	--filter "Model/Model\.(cpp|h)" \
	--filter "Model/TypesModel\.h" \
	--filter "Model/ObjParser\.(cpp|h)" \
	--filter "Model/AnalyticMesh\.(cpp|h)" \
	--print-summary \
	--exclude-throw-branches --exclude-unreachable-branches \
	../build

clean:
	rm -rf $(BUILD_DIR)
	rm -rf ../dist

install: all
	@if [ ! -d $(INSTALL_DIR) ]; then mkdir -p $(INSTALL_DIR); fi
	install -d $(INSTALL_DIR)
	install -m 755 $(BUILD_DIR)/3DVisualizer $(INSTALL_DIR)
	@echo "Installed to $(INSTALL_DIR)"

uninstall:
	rm -f $(INSTALL_DIR)/3DVisualizer
	@echo "Uninstalled from $(INSTALL_DIR)"

dvi:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake .. -DBUILD_DVI=ON -DRelease=OFF -DBUILD_TESTING=OFF && make doxygen
	@echo "Документация готова в $(BUILD_DIR)/docs/html/index.html"

clear_settings:
	rm -rf ~/.config/s21

dist:
	mkdir -p ../dist
	cp -r ../src ../CMakeLists.txt ../dist/
	cd ../dist && tar -czf ../dist/3dViewer.tar.gz src CMakeLists.txt
	rm -rf ../dist/src ../dist/CMakeLists.txt

style:
	find ../src -name "*.cpp" -o -name "*.h" | xargs clang-format -n --style=Google

cppcheck:
	cppcheck --library=qt --enable=all  --suppress=missingIncludeSystem --suppress=unusedFunction ../src/Model ../src/View ../src/Controller ../src/main.cpp

valgrind: test
	valgrind --leak-check=full ./../build/run_tests

check: clean gcov_report style cppcheck valgrind
	@echo "All checks passed!"