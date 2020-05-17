BuildDir="cmake-build-debug"
TestFile1="CharArrayBound.cpp"
TestFile2="CompareChecker.cpp"
TestFile3="SwitchChecker.cpp"
TestFile4="ZeroChecker.cpp"
ConfigFile="config.txt"

cd ../../${BuildDir}
ninja
cd ../tests/IntegrationTest
clang++ -emit-ast -c ${TestFile1} ${TestFile2} ${TestFile3} ${TestFile4}
../../${BuildDir}/tools/Checker/Checker astList.txt ${ConfigFile}