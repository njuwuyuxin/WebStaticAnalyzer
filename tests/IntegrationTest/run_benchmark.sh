rm core
cd ../../cmake-build-debug
ninja
cd ../tests/IntegrationTest
../../cmake-build-debug/tools/Checker/Checker astList.txt config.txt

