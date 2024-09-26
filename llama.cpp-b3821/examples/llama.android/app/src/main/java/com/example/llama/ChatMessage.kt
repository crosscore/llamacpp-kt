// llama.cpp-b3821/examples/llama.android/app/src/main/java/com/example/llama/ChatMessage.kt

package com.example.llama

data class ChatMessage(
    val sender: Sender,
    val content: String
)

enum class Sender {
    User,
    LLM
}
