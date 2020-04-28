TEST=$1
if [ -n "$TEST" ]; then
	ROOT=$PWD
	cd tests/$TEST
	clang++ -emit-ast -c example.cpp
	$ROOT/build/tools/$TEST/$TEST astList.txt config.txt
	cd $ROOT
fi
