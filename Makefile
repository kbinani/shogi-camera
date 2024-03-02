fmt:
	git diff HEAD --name-only --diff-filter=ACMR | grep -e '\.cpp$$' -e '\.hpp$$' -e '\.h$$' -e '\.mm$$' | xargs -P$$(nproc) -n1 clang-format -i
	swift-format -r . -i --configuration .swift-format
