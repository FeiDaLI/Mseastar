# # 编译器
# CXX = g++
# # 编译选项
# CXXFLAGS = -w -Wall -std=c++20 -g -Iinclude -pthread -MMD
# LDFLAGS = -pthread -lboost_system -lboost_thread -lrt -lhwloc -lboost_program_options -laio  -lunwind

# SRCS = \
#     include/resource/resource.cc \
#     test/new_tests/smp_test.cc \
# 	include/app/app-template.cc \
# 	include/future/future_all10.cc \
	
# # 生成对象文件列表
# OBJS = $(SRCS:%.cc=build/%.o)
# DEPS = $(OBJS:.o=.d)

# # 可执行文件名称
# TEST_TARGET = build/smp_test

# # 默认目标
# all: $(TEST_TARGET)

# # 包含自动生成的依赖
# -include $(DEPS)

# # 创建构建目录结构
# $(shell mkdir -p $(dir $(OBJS)) 2>/dev/null)

# # 编译规则：将.cc文件编译为.o文件
# build/%.o: %.cc
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# # 链接可执行文件
# $(TEST_TARGET): $(OBJS)
# 	$(CXX) $^ -o $@ $(LDFLAGS)

# # 清理生成的文件
# clean:
# 	rm -rf build

# # 运行测试
# test: $(TEST_TARGET)
# 	./$(TEST_TARGET)

# .PHONY: all clean test







# -----------------------------------------udp_server-------------------------------------------------------------------------- 
# # 编译器
# CXX = g++
# # 编译选项
# CXXFLAGS = -w -Wall -std=c++20 -g -Iinclude -pthread -MMD
# LDFLAGS = -pthread -lboost_system -lboost_thread -lrt -lhwloc -lboost_program_options -laio  -lunwind

# SRCS = \
#     include/resource/resource.cc \
#     test/net_test/udp_server.cc \

# # 生成对象文件列表
# OBJS = $(SRCS:%.cc=build/%.o)
# DEPS = $(OBJS:.o=.d)

# # 可执行文件名称
# TEST_TARGET = build/udp_server_test

# # 默认目标
# all: $(TEST_TARGET)

# # 包含自动生成的依赖
# -include $(DEPS)

# # 创建构建目录结构
# $(shell mkdir -p $(dir $(OBJS)) 2>/dev/null)

# # 编译规则：将.cc文件编译为.o文件
# build/%.o: %.cc
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# # 链接可执行文件
# $(TEST_TARGET): $(OBJS)
# 	$(CXX) $^ -o $@ $(LDFLAGS)

# # 清理生成的文件
# clean:
# 	rm -rf build

# # 运行测试
# test: $(TEST_TARGET)
# 	./$(TEST_TARGET)

# .PHONY: all clean test



#编译器
CXX = g++
# 编译选项
CXXFLAGS = -w -Wall -std=c++20 -g -Iinclude -pthread -MMD
LDFLAGS = -pthread -lboost_system -lboost_thread -lrt -lhwloc -lboost_program_options -laio  -lunwind -lboost_json

SRCS = \
	include/resource/resource.cc \
    test/http_test/http_server.cc \

# 生成对象文件列表
OBJS = $(SRCS:%.cc=build/%.o)
DEPS = $(OBJS:.o=.d)

# 可执行文件名称
TEST_TARGET = build/http_server

# 默认目标
all: $(TEST_TARGET)

# 包含自动生成的依赖
-include $(DEPS)

# 创建构建目录结构
$(shell mkdir -p $(dir $(OBJS)) 2>/dev/null)

# 编译规则：将.cc文件编译为.o文件
build/%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 链接可执行文件
$(TEST_TARGET): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

# 清理生成的文件
clean:
	rm -rf build

# 运行测试
test: $(TEST_TARGET)
	./$(TEST_TARGET)

.PHONY: all clean test





# # 编译器
# CXX = g++
# # 编译选项
# CXXFLAGS = -w -Wall -std=c++20 -g -Iinclude -pthread -MMD
# LDFLAGS = -pthread -lboost_system -lboost_thread -lrt -lhwloc -lboost_program_options -laio  -lunwind -lboost_unit_test_framework

# SRCS = \
#     include/resource/resource.cc \
#     test/newer_tests/futures_test.cc \
# 	include/test/test-utils.cc \


# # 生成对象文件列表
# OBJS = $(SRCS:%.cc=build/%.o)
# DEPS = $(OBJS:.o=.d)

# # 可执行文件名称
# TEST_TARGET = build/future_test

# # 默认目标
# all: $(TEST_TARGET)

# # 包含自动生成的依赖
# -include $(DEPS)

# # 创建构建目录结构
# $(shell mkdir -p $(dir $(OBJS)) 2>/dev/null)

# # 编译规则：将.cc文件编译为.o文件
# build/%.o: %.cc
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# # 链接可执行文件
# $(TEST_TARGET): $(OBJS)
# 	$(CXX) $^ -o $@ $(LDFLAGS)

# # 清理生成的文件
# clean:
# 	rm -rf build

# # 运行测试
# test: $(TEST_TARGET)
# 	./$(TEST_TARGET)

# .PHONY: all clean test