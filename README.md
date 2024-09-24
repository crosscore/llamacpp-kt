# llamacpp-kt

```CMakeLists.txt
cmake_minimum_required(VERSION 3.22.1)

project("llama-android")

# プロジェクトのルートディレクトリを設定
# 現在の CMakeLists.txt から PROJECT_ROOT までの相対パス
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../")

# FetchContent モジュールのインクルード
include(FetchContent)

# llama の FetchContent宣言
FetchContent_Declare(
        llama
        SOURCE_DIR "${PROJECT_ROOT}"
)

# FetchContent を利用可能にする
FetchContent_MakeAvailable(llama)

# ライブラリの作成 (相対パス)
add_library(${CMAKE_PROJECT_NAME} SHARED
        llama-android.cpp
)

# リンクするライブラリの指定
target_link_libraries(${CMAKE_PROJECT_NAME}
        llama
        common
        android
        log
)
```
