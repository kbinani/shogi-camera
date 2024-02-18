fmt:
	git ls-files . | grep -v base64.hpp | grep -e '\.cpp$$' -e '\.hpp$$' -e '\.h$$' -e '\.mm$$' | xargs -P$$(nproc) -n1 clang-format -i
	swift-format -r . -i
