TestDir="tests/TemplateChecker"
TestFile="example.cpp"
ConfigFile="config.txt"

cd ${TestDir}
clang++ -emit-ast -c ${TestFile}
../../cmake-build-debug/tools/TemplateChecker/TemplateChecker astList.txt ${ConfigFile}
cd ../..